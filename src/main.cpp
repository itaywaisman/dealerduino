#include "Arduino.h"
#include <Servo.h>
#include <Stepper.h>
#include <Wire.h>
#include "VL53L1X.h"
#include <SoftwareSerial.h>

#include "protocol/constants.h"
#include "protocol/server.h"

//----------------------SIGNALS START---------------------------
#define RX                          0   // Serial RX
#define TX                          1   // Serial TX
#define s0                          2   // ColorSensor - 1    
#define s1                          3   // ColorSensor - 2
#define s2                          4   // ColorSensor - 3
#define s3                          5   // ColorSensor - 4
#define out                         6   // ColorSensor - out
#define CARD_PLATFORM_FLIPPING_OUT  7   // Digital servo out
#define CARD_ACCELERATOR_1          8   // Digital servo out (PWM)
#define CARD_ACCELERATOR_2          9   // Digital DC out (PWM)
#define STEPPER_STEP                10  // Platform rotation stepper out
#define STEPPER_DIRECTION           11  // Platform rotation stepper out
#define CARD_PUSHER_OUT             12  // Card pusher dc (PWM) out
//----------------------SIGNALS END---------------------------

#define CARD_FLIPPER_REGULAR 0
#define CARD_FLIPPER_INSERT 180

String serialResponse; // complete message from arduino, which consists of snesors data
VL53L1X distance_sensor;
Servo card_flipping_servo;
Servo card_pusher_servo;

SerialServer* server;

/// GAME STATE
int game_state = GAME_STATE_NOT_STARTED;
bool is_working = false;
int player_angles[4] = {-1, -1, -1, -1};
int num_of_players = 0;
int current_degree = 0;


void setup_server() {
    Serial.begin(9600);
    server = new SerialServer(RX, TX);
}

void setup_dc_motors() {
    pinMode(CARD_ACCELERATOR_1, OUTPUT);
    pinMode(CARD_ACCELERATOR_2, OUTPUT);
}

void setup_stepper() {
    pinMode(STEPPER_DIRECTION,OUTPUT); 
    pinMode(STEPPER_STEP,OUTPUT);
}

void setup_proximity_sensor_and_color_sensor() { //there is no setup for color
    Wire.begin();
    Wire.setClock(400000); // use 400 kHz I2C
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

long previousMillis = 0;

long sync_interval = 100;
long last_sync_time = 0;

void sync_if_needed() {
    unsigned long currentMillis = millis();
    
    // check if we need to sync the state
    if(currentMillis - last_sync_time > sync_interval) {
        server->set_game_state(game_state);
        server->set_num_of_players(num_of_players);
        server->sync();
        last_sync_time =  currentMillis;
    }
}

void delay_busy(unsigned long ms) {
    unsigned long start_millis = millis();
    unsigned long current_millis = start_millis;

    while(current_millis - start_millis < ms) {
        // The delay hasn't finished
        
        sync_if_needed();

        current_millis = millis();
    }
}

void set_is_working(bool value) {
    is_working = true;
    sync_if_needed();
}


/*********************************
 * Hardware methods
 *********************************/

void push_card_out(){
    card_pusher_servo.write(60);
    delay_busy(385);
    digitalWrite(CARD_ACCELERATOR_1, LOW);
    digitalWrite(CARD_ACCELERATOR_2, HIGH);
    delay_busy(200);
    card_pusher_servo.write(90);
    delay_busy(1250);
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
            delay_busy(80); 
            digitalWrite(STEPPER_STEP,LOW); 
        }
   }
   else{
        digitalWrite(STEPPER_DIRECTION,LOW); //Changes the rotations direction
        for(int x = 0; x < how_much_to_move; x++) {
            digitalWrite(STEPPER_STEP,HIGH);
            delay_busy(80);
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
    delay_busy(20);

    digitalWrite(s2,LOW);
    digitalWrite(s3,HIGH);
    //Serial.print("Blue value= ");
    int blue_value = read_color_sensor();
    delay_busy(20);

    digitalWrite(s2,HIGH);
    digitalWrite(s3,HIGH);
    int green_value = read_color_sensor();
    delay_busy(20);
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
    delay_busy(300);

    set_card_flipper_regular();
    delay_busy(900);
    
    push_card_out();
    delay_busy(300);
}

void deal_flipped(int target_angle) {

    rotate_platform((180 + target_angle) %360);
    delay_busy(300);

    set_card_flipper_flipped();
    delay_busy(900);

    push_card_out();
    delay_busy(300);
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
    game_state = GAME_STATE_STARTED;
    set_is_working(true);
    
    digitalWrite(STEPPER_STEP,HIGH); 
    rotate_platform(0);
    set_card_flipper_regular();
    set_is_working(false);
}

void detect_players() {
    game_state = GAME_STATE_SCANNING_PLAYERS;
    set_is_working(true);
    
    for (int pos = 0; pos <= 180; pos += 18) { // goes from 180 degrees to 0 degrees - 18 degrees jump.
        rotate_platform(pos);
        delay_busy(500);
        if(is_player_detected_and_range() != -1) {
            if (num_of_players >0 ){
                if (player_angles[num_of_players-1] + 18 != pos){ //doesnt allow 2 players to be too close - at least 36 degrees between them.
                    player_angles[num_of_players] = pos;
                    Serial.println(pos);
                    num_of_players++;
                }
            }
            else{
                player_angles[num_of_players] = pos;
                Serial.println(pos);
                num_of_players++;
            }
        }
        if (num_of_players == 4){
            break;
        }
    }
    rotate_platform(90);
    game_state = GAME_STATE_SCANNED_PLAYERS;
    set_is_working(false);
}

void start_round() {
    game_state = GAME_STATE_ROUND_STARTING;
    set_is_working(true);
    for(int i = 0; i < 4; i++) {
        if(player_angles[i] >= 0) {
            deal_card(false, player_angles[i]);
            delay_busy(500);
            deal_card(false, player_angles[i]);
        }
        delay_busy(200);
    }
    rotate_platform(90);
    set_card_flipper_regular();
    game_state = GAME_STATE_ROUND_STARTED;
    //sync_game_state();
    set_is_working(false);
}

void show_cards_round_1() {
    game_state = GAME_STATE_DEALING_CARD_1;
    set_is_working(true);
    deal_card(false, 115);
    deal_card(true, 105);
    deal_card(true, 95);
    deal_card(true, 85);
    game_state = GAME_STATE_DEALT_CARD_1;
    set_is_working(false);
}

void show_cards_round_2() {
    game_state = GAME_STATE_DEALING_CARD_2;
    set_is_working(true);
    deal_card(false, 115);
    deal_card(true, 75);
    game_state = GAME_STATE_DEALT_CARD_2;
    set_is_working(false);
}

void show_cards_round_3() {
    game_state = GAME_STATE_DEALING_CARD_3;
    set_is_working(true);
    deal_card(false, 115);
    deal_card(true, 65);
    game_state = GAME_STATE_DEALT_CARD_3;
    set_is_working(false);
}

void player_quit(int player_idx) {
    player_angles[player_idx] = -1;
    num_of_players--;
}

void reset() {
    set_is_working(true);
    game_state = GAME_STATE_NOT_STARTED;
    for(int i = 0; i < 4; i++) {
        player_angles[i] = -1;
    }
    num_of_players = 0;
    rotate_platform(0);
    current_degree = 0;
    set_is_working(false);
}

/*********************************
 * Setup
 *********************************/
void setup() {
    setup_server();
    setup_servos();
    setup_dc_motors();
    setup_stepper();
    setup_proximity_sensor_and_color_sensor();
}

/*********************************
 * Loop
 *********************************/
void loop() {
    // deal_card(false, 0);
    if(server->command_available()) {
        int command = server->get_command();
        int arg1 = server->get_arg1();
        int arg2 = server->get_arg2();

        switch (command) {
            case COMMAND_START_GAME:
                start_game();
                break;
            case COMMAND_SCAN_PLAYERS:
                detect_players();
                break;
            case COMMAND_START_ROUND:
                start_round();
                break;
            case COMMAND_SHOW_CARD_1:
                show_cards_round_1();
                break;
            case COMMAND_SHOW_CARD_2:
                show_cards_round_2();
                break;
            case COMMAND_SHOW_CARD_3:
                show_cards_round_1();
                break;
            case COMMAND_PLAYER_QUIT:
                int player_idx = arg1;
                player_quit(player_idx);
                break;
            case COMMAND_RESET:
                reset();
                break;
        }
    }
    sync_if_needed();
}