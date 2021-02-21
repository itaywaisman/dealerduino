#include "Arduino.h"
#include <Servo.h>
#include <Stepper.h>
#include <Wire.h>
#include "VL53L1X.h"
#include <SoftwareSerial.h>
#include "./protocol.h"
// "http://librarymanager/All#SparkFun_VL53L1X

//----------------------SIGNALS START---------------------------

#define CARD_ACCELERATOR_1 8
#define CARD_ACCELERATOR_2 9
//START COLOR SENSOR :
#define s0 2       //Module pins wiring
#define s1 3
#define s2 4
#define s3 5
#define out 6
//END COLOR SENSOR !
#define CARD_PLATFORM_FLIPPING_OUT 7
#define STEPPER_STEP 10
#define STEPPER_DIRECTION 11
#define CARD_PUSHER_OUT 12

//----------------------SIGNALS END---------------------------

#define CARD_FLIPPER_REGULAR 0
#define CARD_FLIPPER_INSERT 180

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
VL53L1X distance_sensor;
Servo card_flipping_servo;
Servo card_pusher_servo;

char sample_packet[] = "0;0;0";

/// GAME STATE
int game_state = NOT_STARTED;
int player_angles[4] = {-1, -1, -1, -1};
int player_num = 0;
int current_degree = 0;

void setup_dc_motors() {
    // pinMode(enA, OUTPUT);
    pinMode(CARD_ACCELERATOR_1, OUTPUT);
    pinMode(CARD_ACCELERATOR_2, OUTPUT);
    // pinMode(in3, OUTPUT);
    // pinMode(in4, OUTPUT);
}

void setup_stepper() {
    pinMode(STEPPER_DIRECTION,OUTPUT); 
    pinMode(STEPPER_STEP,OUTPUT);
}

void setup_proximity_sensor_and_color_sensor() { //there is no setup for color

    if (!distance_sensor.init())
    {
        Serial.println("Failed to detect and initialize sensor!");
        while (1);
    }

    distance_sensor.setDistanceMode(VL53L1X::Long);
    distance_sensor.setMeasurementTimingBudget(50000);
    distance_sensor.startContinuous(50);
}

void setup_servos() {
    card_flipping_servo.attach(CARD_PLATFORM_FLIPPING_OUT);
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

void push_card_out(){
    card_pusher_servo.write(60);
    digitalWrite(CARD_ACCELERATOR_1, LOW);
    digitalWrite(CARD_ACCELERATOR_2, HIGH);
    delay(400);
    card_pusher_servo.write(90);
    delay(1300);
    digitalWrite(CARD_ACCELERATOR_1, LOW);
    digitalWrite(CARD_ACCELERATOR_2, LOW);
}

void set_card_flipper_regular() {
    card_flipping_servo.write(CARD_FLIPPER_REGULAR);
}

void set_card_flipper_flipped() {
    card_flipping_servo.write(CARD_FLIPPER_INSERT);
}

void rotate_platform(int target_degree) {
   int degree_to_move = abs(target_degree - current_degree);
   int how_much_to_move = round(100*degree_to_move/180);
   if (target_degree - current_degree >= 0){
        digitalWrite(STEPPER_DIRECTION,HIGH); // Enables the motor to move in a particular direction
        // Makes 200 pulses for making one full cycle rotation
        for(int x = 0; x < how_much_to_move; x++) {
            digitalWrite(STEPPER_STEP,HIGH); 
            delay(80); 
            digitalWrite(STEPPER_STEP,LOW); 
        }
   }
   else{
        digitalWrite(STEPPER_DIRECTION,LOW); //Changes the rotations direction
        for(int x = 0; x < how_much_to_move; x++) {
            digitalWrite(STEPPER_STEP,HIGH);
            delay(80);
            digitalWrite(STEPPER_STEP,LOW);
        }
   }
   digitalWrite(STEPPER_STEP,HIGH); 
   current_degree = target_degree;
}

int read_proximity_sensor() {
    distance_sensor.read();
    int range_in_mm = distance_sensor.ranging_data.range_mm;
    Serial.print("The range in mm is: ");
    Serial.println(range_in_mm);
    return range_in_mm;
}

int read_color_sensor(){
    int data = pulseIn(out,LOW);       //here we wait until "out" go LOW, we start measuring the duration and stops when "out" is HIGH again

    return data;
}

int is_player_detected_and_range() {
    int range_to_player = read_proximity_sensor();
    if (range_to_player <= 1000){ // 1.0 meters from players
        return range_to_player;
    }
    return -1;
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
      if ((blue_value >= 17 and green_value >= 17 ) or (blue_value >= 17 and red_value >= 17) or (green_value >= 17 and red_value >= 17) ){
        Serial.println("CARD FACING UP! ");
       return true;
      }
    Serial.println("CARD FACING DOWN! ");
    return false;
    }
    return false;
}

void deal_regular(int target_angle) {

    rotate_platform(target_angle);
    delay(300);

    set_card_flipper_regular();
    delay(900);
    
    push_card_out();
    delay(300);
}

void deal_flipped(int target_angle) {

    rotate_platform((180 + target_angle) %360);
    delay(300);

    set_card_flipper_flipped();
    delay(900);

    push_card_out();
    delay(300);
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
    //sync_game_state();
    digitalWrite(STEPPER_STEP,HIGH); 
    rotate_platform(0);
    set_card_flipper_regular();
}

void detect_players() {
    game_state = SCANNING_PLAYERS;
    //sync_game_state();
    for (int pos = 0; pos <= 180; pos += 18) { // goes from 180 degrees to 0 degrees - 18 degrees jump.
        rotate_platform(pos);
        delay(500);
        if(is_player_detected_and_range() != -1) {
            if (player_num >0 ){
                if (player_angles[player_num-1] + 18 != pos){ //doesnt allow 2 players to be too close - at least 36 degrees between them.
                    player_angles[player_num] = pos;
                    Serial.println(pos);
                    player_num++;
                }
            }
            else{
                player_angles[player_num] = pos;
                Serial.println(pos);
                player_num++;
            }
        }
        if (player_num == 4){
            break;
        }
    }
    rotate_platform(90);
    game_state = SCANNED_PLAYERS;
    //sync_game_state();
    Serial.println("Scanned All players");
}

void start_round() {
    // player_num =3;
    // player_angles[0] = 0;
    // player_angles[1] = 36;
    // player_angles[2] = 90;
    for(int i = 0; i < 4; i++) {
        if(player_angles[i] >= 0) {
            deal_card(false, player_angles[i]);
            delay(500);
            deal_card(false, player_angles[i]);
        }
        delay(200);
    }
    rotate_platform(90);
    set_card_flipper_regular();
    game_state = ROUND_STARTED;
    //sync_game_state();
}

void show_cards_round_1() {
    deal_card(false, 115);
    deal_card(true, 105);
    deal_card(true, 95);
    deal_card(true, 85);

    // game_state = game_state + 1;
    // sync_game_state();
}

void show_cards_round_2() {
    deal_card(false, 115);
    deal_card(true, 75);
    
    // game_state = game_state + 1;
    // sync_game_state();
}

void show_cards_round_3() {
    deal_card(false, 115);
    deal_card(true, 65);
    
    // game_state = game_state + 1;
    // sync_game_state();
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
    rotate_platform(0);
    current_degree = 0;
    // sync_game_state();
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
    setup_stepper();
    setup_proximity_sensor_and_color_sensor();
    //sync_game_state();

}

/*********************************
 * Loop
 *********************************/
void loop() {
    deal_card(true,0);
    delay(5000);
    // detect_players();
    // delay(4000);
    // show_cards_round_1();
    // delay(5000);
    // rotate_platform(0);
    // show_cards_round_2();
    // delay(5000);
    // rotate_platform(0);
    // show_cards_round_3();
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