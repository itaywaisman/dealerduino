//
// Created by LIORD on 20/11/2020.
//

#include "Arduino.h"
#include <Servo.h>
#include <Stepper.h>
#include <Wire.h>
#include "VL53L1X.h"
#include <SoftwareSerial.h>


// "http://librarymanager/All#SparkFun_VL53L1X

//START COLOR SENSOR :
#define s0 2       //Module pins wiring
#define s1 3
#define s2 4
#define s3 5
#define out 6
//END COLOR SENSOR !

//
#define enA 12
#define in1 1
#define in2 2
#define in3 3
#define in4 4
#define in5 5
#define in6 6
#define in7 7
#define in7 8
#define in7 9
#define in7 10
#define in7 11
#define in12 12
#define in13 13

#define CARD_FLIPPING_OUT 7
#define PLATFORM_OUT_1 8
#define PLATFORM_OUT_2 9
#define PLATFORM_OUT_3 10
#define PLATFORM_OUT_4 11
#define CARD_PUSHER_OUT 12
#define PLATFORM_ROTATION_OUT 4

#define CARD_FLIPPER_REGULAR 0
#define CARD_FLIPPER_INSERT 180

#define PROXIMITY_DISTANCE 1000
#define PROXIMITY_DELTA 200

enum DEALER_COMMANDS {
    DO_NOTHING = 0,
    START_GAME = 1,
    SCAN_PLAYERS = 2,
    START_ROUND = 3,
    SHOW_CARD = 4,
    PLAYER_QUIT = 5,
    RESET = 9
};

enum GAME_STATE {
    NOT_STARTED = 0,
    STARTED = 1,
    ROUND_STARTED = 2,
    DEALT_CARD_1 = 12,
    DEALT_CARD_2 = 13,
    DEALT_CARD_3 = 14,
    DEALT_CARD_4 = 15,
    ROUND_FINISHED = 16,
    ERROR = 100
};

SoftwareSerial ArduinoUnoSerial(0,1);
String serialResponse; // complete message from arduino, which consists of snesors data
char sample_packet[] = "0;0;0";

/// GAME STATE
int game_state = NOT_STARTED;
int player_angles[8] = {-1, -1, -1, -1, -1, -1, -1, -1};
int player_num = 0;

VL53L1X distance_sensor;

Servo card_flipping_servo;
Servo card_pusher_servo;

// Number of steps per output rotation
const int stepsPerRevolution = 200;

// Create Instance of Stepper library
Stepper baseStepper(stepsPerRevolution, PLATFORM_OUT_1, PLATFORM_OUT_2, PLATFORM_OUT_3, PLATFORM_OUT_4);
int current_degree = 0;


void setup_dc_motors() {
    pinMode(enA, OUTPUT);
    pinMode(0, OUTPUT);
    pinMode(1, OUTPUT);
    pinMode(in3, OUTPUT);
    pinMode(in4, OUTPUT);
}

void setup_proximity_sensor() {

    if (!distance_sensor.init())
    {
        Serial.println("Failed to detect and initialize sensor!");
        while (1);
    }

    distance_sensor.setDistanceMode(VL53L1X::Long);
    distance_sensor.setMeasurementTimingBudget(50000);
    distance_sensor.startContinuous(50);
}

void setup_color_sensor() {

}

void setup_servos() {
    card_flipping_servo.attach(CARD_FLIPPING_OUT);
    card_pusher_servo.attach(CARD_PUSHER_OUT);
}


/*********************************
 * Hardware methods
 *********************************/


void spin_cards_wheel(int ms=1000) {
  //not working yet !!!
    Serial.print("SPIN MOTOR!");
    digitalWrite(0, HIGH);
    digitalWrite(1, LOW);
    delay(ms);

    digitalWrite(0, LOW);
    digitalWrite(1, LOW);
    delay(ms);
}

void set_card_flipper_regular() {
    card_flipping_servo.write(CARD_FLIPPER_REGULAR);
}

void set_card_flipper_flipped() {
    card_flipping_servo.write(CARD_FLIPPER_INSERT);
}

void rotate_platform(int target_degree) {
  int stepsToRotate = target_degree - current_degree;
  
  int step_size = 1;
  if(stepsToRotate < 0) step_size = -1;
  stepsToRotate = abs(stepsToRotate);
  for(int i=0; i<stepsToRotate; i++) {
    baseStepper.step(step_size);
    delay(30);
  }

  current_degree = target_degree;
    
}

int read_proximity_sensor() {
    distance_sensor.read();

    int range_in_mm = distance_sensor.ranging_data.range_mm;
    return range_in_mm;
}

int read_color_sensor(){
    int data = pulseIn(out,LOW);       //here we wait until "out" go LOW, we start measuring the duration and stops when "out" is HIGH again

    return data;
}


/*********************************
 * Game logic methods
 *********************************/

void start_game() {
    game_state = STARTED;
}

boolean is_player_detected() {
    return abs(read_proximity_sensor() - PROXIMITY_DISTANCE) < PROXIMITY_DELTA;
}

boolean is_card_facing_up() {
    //S2/S3 levels define which set of photodiodes we are using LOW/LOW is for RED LOW/HIGH is for Blue and HIGH/HIGH is for green
    digitalWrite(s2,LOW);
    digitalWrite(s3,LOW);
    int red_value = read_color_sensor();                   //Executing GetData function to get the value
    delay(20);

    digitalWrite(s2,LOW);
    digitalWrite(s3,HIGH);
    //Serial.print("Blue value= ");
    int blue_value = read_color_sensor();
    delay(20);

    digitalWrite(s2,HIGH);
    digitalWrite(s3,HIGH);
    int green_value = read_color_sensor();
    delay(20);
    Serial.println("COLORS:");
    Serial.print(" RED: ");
    Serial.print(red_value);
    Serial.println();
    Serial.print(" BLUE: ");
    Serial.print(blue_value);
    Serial.println();
    Serial.print(" GREEN: ");
    Serial.print(green_value);
    Serial.println();
    if (blue_value <= 21 and green_value <= 21 and red_value <= 21){
      if ((blue_value >= 16 and green_value >= 16 ) or (blue_value >= 16 and red_value >= 16) or (green_value >= 16 and red_value >= 16) ){
        Serial.println("CARD FACING UP! ");
       return true;
      }
    Serial.println("CARD FACING DOWN! ");
    return false;
    }
}

void detect_players() {
    int player_idx = 0;
    for (int pos = 180; pos >= 0; pos -= 1) { // goes from 180 degrees to 0 degrees
        rotate_platform(pos);
        if(is_player_detected()) {
            player_angles[player_idx] = pos;
            player_idx++;
        }
    }

    player_num = player_idx;

    rotate_platform(90);
}

void deal_regular(int target_angle) {
    rotate_platform(target_angle);
    delay(500);
    set_card_flipper_regular();
    delay(500);
    card_pusher_servo.write(70);
    delay(500);
    card_pusher_servo.write(90);
    delay(500);
    spin_cards_wheel(500);
    delay(500);
}

void deal_flipped(int target_angle) {

    rotate_platform(360 - target_angle);

    // set_card_flipper_insert();
    delay(1000);
    spin_cards_wheel(2000);
    delay(500);
    //set_card_flipper_flip();
    delay(500);
    set_card_flipper_regular();
    delay(500);
}

void deal_card(boolean is_open, int target_angle) {

    boolean card_facing_up = is_card_facing_up();

    if(is_open && card_facing_up) {
        deal_regular(target_angle);
    }
    if(is_open && !card_facing_up) {
        deal_flipped(target_angle);
    }
    if(!is_open && card_facing_up) {
        deal_flipped(target_angle);
    }
    if(!is_open && !card_facing_up) {
        deal_regular(target_angle);
    }
}

void show_card() {
    deal_card(true, 90);
    game_state = game_state + 1;

}

void start_round() {
    for(int i = 0; i < 8; i++) {
        if(player_angles[i] >= 0) {

            deal_card(false, player_angles[i]);
            deal_card(false, player_angles[i]);

        }
    }
    game_state = ROUND_STARTED;
}

void player_quit(int player_idx) {
    player_angles[player_idx] = -1;
}

void reset() {
    game_state = NOT_STARTED;
    for(int i = 0; i < 8; i++) {
        player_angles[i] = -1;
    }
}

void sync_game_state() {
    Serial.print(game_state);
    Serial.print(player_num);
    Serial.println();
}


/*********************************
 * Setup
 *********************************/
void setup() {
  Serial.begin(9600);
  Wire.begin();
  Wire.setClock(400000); // use 400 kHz I2C
  setup_servos();
  setup_dc_motors();
//    setup_proximity_sensor();
  setup_color_sensor();
}

/*********************************
 * Loop
 *********************************/
void loop() {
    //deal_card(false, 10);

    //is_card_facing_up();
    spin_cards_wheel(500);
    delay(5000);
    //rotate_platform(30);

    // delay(100);

    // set_card_flipper_flipped();

    // delay(5000);

    // set_card_flipper_regular();

    // delay(5000);
    

    // card_pusher_servo.write(70);

    // delay(5000);

    // card_pusher_servo.write(90);

    // if(ArduinoUnoSerial.available() <= 0) return;


    // int command_packet = ArduinoUnoSerial.parseInt();
    // if(ArduinoUnoSerial.read() == '\n') {
    //     ArduinoUnoSerial.flush();
    //     Serial.println("Got command!");
    //     int command = command_packet / 100;
    //     int arg1 = command_packet /10 % 10;
    //     int arg2 = command_packet % 10;


    //     switch (command) {
    //         case START_GAME:
    //             start_game();
    //             break;
    //         case SCAN_PLAYERS:
    //             detect_players();
    //             break;
    //         case START_ROUND:
    //             start_round();
    //             break;
    //         case SHOW_CARD:
    //             show_card();
    //             break;
    //         case PLAYER_QUIT:
    //             int player_idx = arg1;
    //             player_quit(player_idx);
    //             break;
    //         case RESET:
    //             reset();
    //             break;
    //     }
    // }

    // sync_game_state();
}

// #include "Arduino.h"
// #include <Stepper.h>

// const int stepsPerRevolution = 200;  // change this to fit the number of steps per revolution
// // for your motor

// // initialize the stepper library on pins 8 through 11:
// Stepper myStepper(stepsPerRevolution, 8,9,10,11);

// int stepCount = 0;         // number of steps the motor has taken

// void setup() {
//   // initialize the serial port:
//   Serial.begin(9600);
// }

// void loop() {
//   // step one step:
//     for(int i=0; i<200; i++) {
//         myStepper.step(1);
//         Serial.print("steps:");
//         Serial.println(stepCount);
//         stepCount++;
//         delay(30);
//     }

//     delay(5000);

//     for(int i=200; i>0; i--) {
//         myStepper.step(-1);
//         Serial.print("steps:");
//         Serial.println(stepCount);
//         stepCount++;
//         delay(30);
//     }

//     delay(5000);

// }

