#include "Arduino.h"
#include <Servo.h>
#include <Stepper.h>
#include <Wire.h>
#include "VL53L1X.h"
#include <SoftwareSerial.h>
#include "./protocol.h"

// "http://librarymanager/All#SparkFun_VL53L1X


//----------------------SIGNALS START---------------------------


#define CARD_ACCELERATOR_1 0
#define CARD_ACCELERATOR_2 1

//START COLOR SENSOR :
#define s0 2       //Module pins wiring
#define s1 3
#define s2 4
#define s3 5
#define out 6
//END COLOR SENSOR !


#define PLATFORM_OUT_1 8
#define PLATFORM_OUT_2 9
#define PLATFORM_OUT_3 10
#define PLATFORM_OUT_4 11

#define CARD_PUSHER_OUT 12
#define CARD_FLIPPING_OUT 13


//----------------------SIGNALS END---------------------------


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
    SCANNING_PLAYERS = 2,
    SCANNED_PLAYERS = 3,
    ROUND_STARTED = 4,
    DEALT_CARD_1 = 5,
    DEALT_CARD_2 = 6,
    DEALT_CARD_3 = 7,
    DEALT_CARD_4 = 8,
    ROUND_FINISHED = 9,
    ERROR = 100
};


String serialResponse; // complete message from arduino, which consists of snesors data
char sample_packet[] = "0;0;0";

/// GAME STATE
int game_state = NOT_STARTED;
int player_angles[4] = {-1, -1, -1, -1};
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
    // pinMode(enA, OUTPUT);
    pinMode(CARD_ACCELERATOR_1, OUTPUT);
    pinMode(CARD_ACCELERATOR_2, OUTPUT);
    // pinMode(in3, OUTPUT);
    // pinMode(in4, OUTPUT);
}

void setup_stepper() {
      baseStepper.setSpeed(30);
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
 * State methods
 *********************************/

void sync_game_state() {
    delay(100);
    Serial.println("sending state...");

    int packet = game_state * 10 + player_num;

    if(!send_packet(packet)) {
        Serial.println("Couldn't send packet to wifi over serial!");
    }
}



/*********************************
 * Hardware methods
 *********************************/

void spin_cards_first(int time=500) {
  card_pusher_servo.write(70);
  delay(time);
  card_pusher_servo.write(90);
}

void spin_cards_exit(int time=400) {
    Serial.print("SPIN MOTOR!");
    digitalWrite(CARD_ACCELERATOR_1, HIGH);
    digitalWrite(CARD_ACCELERATOR_2, LOW);
    delay(time);
    digitalWrite(CARD_ACCELERATOR_1, LOW);
    digitalWrite(CARD_ACCELERATOR_2, LOW);
}

void push_card_out(){
    spin_cards_first();
    delay(100);
    spin_cards_exit();
}

void set_card_flipper_regular() {
    card_flipping_servo.write(CARD_FLIPPER_REGULAR);
}

void set_card_flipper_flipped() {
    card_flipping_servo.write(CARD_FLIPPER_INSERT);
}

void rotate_platform(int target_degree) {
    int stepsToRotate = target_degree - current_degree;
    Serial.print(stepsToRotate);
    int step_size = 1;
    if(stepsToRotate < 0) step_size = -1;
    stepsToRotate = abs(stepsToRotate);
    int steps_after_scale = round(stepsToRotate/2); //degree scale
    for(int i=0; i<steps_after_scale; i++) {
        baseStepper.step(step_size);
        delay(30);
    }
    digitalWrite(PLATFORM_OUT_1, LOW);
    digitalWrite(PLATFORM_OUT_2, LOW);
    digitalWrite(PLATFORM_OUT_3, LOW);
    digitalWrite(PLATFORM_OUT_4, LOW);
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

void deal_regular(int target_angle) {

    rotate_platform(target_angle);
    delay(200);

    set_card_flipper_regular();
    delay(200);
    
    push_card_out();
    delay(200);
}

void deal_flipped(int target_angle) {

    rotate_platform(180 - target_angle);
    delay(200);

    set_card_flipper_flipped();
    delay(200);

    push_card_out();
    delay(200);
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


/*********************************
 * Game Functions
 *********************************/
void start_game() {
    game_state = STARTED;
    sync_game_state();
    rotate_platform(180);
    set_card_flipper_regular();
}

void detect_players() {
    game_state = SCANNING_PLAYERS;
    sync_game_state();

    for (int pos = 180; pos >= 0; pos -= 20) { // goes from 180 degrees to 0 degrees
        rotate_platform(pos);
        if(is_player_detected()) {
            player_angles[player_num] = pos;
            player_num++;
        }
        if (player_num == 4){
            break;
        }
        delay(30);
    }

    rotate_platform(90);
    game_state = SCANNED_PLAYERS;
    sync_game_state();
    Serial.println("Scanned All players");
}

void start_round() {
    for(int i = 0; i < 4; i++) {
        if(player_angles[i] >= 0) {
            deal_card(false, player_angles[i]);
            deal_card(false, player_angles[i]);
        }
    }
    rotate_platform(90);
    game_state = ROUND_STARTED;
    sync_game_state();
}

void show_card() {
    deal_card(true, 90);
    game_state = game_state + 1;
    sync_game_state();
}

void player_quit(int player_idx) {
    player_angles[player_idx] = -1;
    player_num--;
}

void reset() {
    game_state = NOT_STARTED;
    for(int i = 0; i < 4; i++) {
        player_angles[i] = -1;
    }
    player_num = 0;
    sync_game_state();
}


/*********************************
 * Setup
 *********************************/
void setup() {
    Serial.begin(9600);
    
    Wire.begin();
    Wire.setClock(400000); // use 400 kHz I2C

    //setup_serial(3, 2); //WIFI

    setup_servos();
    setup_dc_motors();
    // setup_stepper();
    // setup_proximity_sensor();
    // setup_color_sensor();
    //sync_game_state();

}

/*********************************
 * Loop
 *********************************/
void loop() {

    push_card_out();
    delay (2000);

    // set_card_flipper_flipped();

    // delay(5000);
    
    // set_card_flipper_regular();

    // delay(5000);
    

    /*
    int command_packet = read_packet();
    if(command_packet != -1) {
        Serial.println("Got command!");
    
        int command = command_packet / 100;
        int arg1 = command_packet /10 % 10;
        int arg2 = command_packet % 10;

        Serial.println(command);
        Serial.println(arg1);
        Serial.println(arg2);

        switch (command) {
            case START_GAME:
                start_game();
                break;
            case SCAN_PLAYERS:
                detect_players();
                break;
            case START_ROUND:
                start_round();
                break;
            case SHOW_CARD:
                show_card();
                break;
            case PLAYER_QUIT:
                int player_idx = arg1;
                player_quit(player_idx);
                break;
            case RESET:
                reset();
                break;
        }

        
    } 
    
    sync_game_state();
        

    delay(100);
    */

}

