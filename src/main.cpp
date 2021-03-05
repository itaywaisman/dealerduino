#include <stdio.h>
#include <stdarg.h>
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



// #define DEBUG_DEALER




String serialResponse; // complete message from arduino, which consists of snesors data
VL53L1X distance_sensor;
Servo card_flipping_servo;
Servo card_pusher_servo;

SerialServer* server;

/// GAME STATE
int machine_state = MACHINE_STATE_IDLE;
int game_state = GAME_STATE_NOT_STARTED;
bool is_working = false;
int player_angles[4] = {-1, -1, -1, -1};
int num_of_players = 0;
int current_degree = 0;


void set_card_flipper_regular();

void setup_server() {
    server = new SerialServer();
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
        while (1);
    }

    distance_sensor.setDistanceMode(VL53L1X::Long);
    distance_sensor.setMeasurementTimingBudget(50000);
    distance_sensor.startContinuous(50);
}

void setup_servos() {
    card_flipping_servo.attach(CARD_PLATFORM_FLIPPING_OUT);
    card_pusher_servo.attach(CARD_PUSHER_OUT);
    set_card_flipper_regular();
}



/*********************************
 * State methods
 *********************************/

unsigned long previousMillis = 0;

unsigned long sync_interval = 50;
unsigned long last_sync_time = 0;

void sync_if_needed() {
    unsigned long currentMillis = millis();
    
    // check if we need to sync the state
    if(currentMillis - last_sync_time > sync_interval) {
        server->sync();
        last_sync_time =  currentMillis;
    }
}

void send() {
    server->set_machine_state(machine_state);
    server->set_game_state(game_state);
    server->set_num_of_players(num_of_players);
    server->send();
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
}

/*********************************
 * logging methods
 *********************************/

void level_logf(char* format, char* level, ...) {
    char buffer[LOG_LENGTH];
    memset(&buffer[0], 0, sizeof(buffer));
    va_list args;
    va_start (args, format);
    vsprintf (buffer,format, args);
    va_end (args);

    char final_buffer[LOG_LENGTH];
    sprintf(final_buffer, "%s|%s", level, buffer);

    server->set_log(final_buffer);
}

void debug_logf(char* format, ...) {
    va_list args;
    va_start (args, format);
    level_logf(format, "DBG", args);
    va_end (args);
}

void info_logf(char* format, ...) {
    va_list args;
    va_start (args, format);
    level_logf(format, "INF", args);
    va_end (args);
}

void error_logf(char* format, ...) {
    va_list args;
    va_start (args, format);
    level_logf(format, "ERR", args);
    va_end (args);
}

/*********************************
 * Hardware methods
 *********************************/

void push_card_out_regular(){
    digitalWrite(CARD_ACCELERATOR_1, LOW);
    digitalWrite(CARD_ACCELERATOR_2, HIGH);
    delay_busy(300);
    card_pusher_servo.write(60);
    delay_busy(450);
    card_pusher_servo.write(90);
    delay_busy(1450);
    digitalWrite(CARD_ACCELERATOR_1, LOW);
    digitalWrite(CARD_ACCELERATOR_2, LOW);
}

void push_card_out_flipped(){
    digitalWrite(CARD_ACCELERATOR_1, LOW);
    digitalWrite(CARD_ACCELERATOR_2, HIGH);
    delay_busy(300);
    card_pusher_servo.write(60);
    delay_busy(380);
    card_pusher_servo.write(90);
    delay_busy(1450);
    digitalWrite(CARD_ACCELERATOR_1, LOW);
    digitalWrite(CARD_ACCELERATOR_2, LOW);
}

void set_card_flipper_regular() {
    card_flipping_servo.write(CARD_FLIPPER_REGULAR);
}

void set_card_flipper_flipped() {
    card_flipping_servo.write(CARD_FLIPPER_INSERT);
}

void set_card_flipper_up() {
    card_flipping_servo.write(90);
}

void rotate_platform(int target_degree) {
   int degree_to_move = abs(target_degree - current_degree);
   int how_much_to_move = round(100*degree_to_move/180);
   if (target_degree - current_degree >= 0){
        digitalWrite(STEPPER_DIRECTION,HIGH); // Enables the motor to move in a particular direction
        // Makes 200 pulses for making one full cycle rotation
        for(int x = 0; x < how_much_to_move; x++) {
            digitalWrite(STEPPER_STEP,HIGH); 
            delay_busy(110); 
            digitalWrite(STEPPER_STEP,LOW); 
        }
   }
   else{
        digitalWrite(STEPPER_DIRECTION,LOW); //Changes the rotations direction
        for(int x = 0; x < how_much_to_move; x++) {
            digitalWrite(STEPPER_STEP,HIGH);
            delay_busy(110);
            digitalWrite(STEPPER_STEP,LOW);
        }
   }
   digitalWrite(STEPPER_STEP,HIGH); 
   current_degree = target_degree;
}

int read_proximity_sensor() {
    distance_sensor.read();
    int range_in_mm = distance_sensor.ranging_data.range_mm;
    // Serial.print("The range in mm is: ");
    // Serial.println(range_in_mm);
    return range_in_mm;
}

int read_color_sensor(){
    int data = pulseIn(out,LOW);       //here we wait until "out" go LOW, we start measuring the duration and stops when "out" is HIGH again

    return data;
}

int is_player_detected_and_range() {
    int range_to_player = read_proximity_sensor();
    if (range_to_player <= 1200 && range_to_player >= 400 ){ // 1.0 meters from players
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
    int blue_value = read_color_sensor();
    delay_busy(20);

    digitalWrite(s2,HIGH);
    digitalWrite(s3,HIGH);
    int green_value = read_color_sensor();
    // Serial.print("green: ");
    // Serial.println(green_value);
    // Serial.print("blue: ");
    // Serial.println(red_value);
    // Serial.print("red: ");
    // Serial.println(blue_value);
    delay_busy(20);

    if (blue_value < 20 and green_value < 20 and red_value < 20){
      //Serial.println("CARD FACING DOWN! ");
      return false;
    }
    //Serial.println("CARD FACING UP! ");
    return true;
}

void deal_regular(int target_angle) {

    rotate_platform(target_angle);
    delay_busy(100);

    set_card_flipper_regular();
    delay_busy(900);
    
    push_card_out_regular();
    delay_busy(100);
}

void deal_flipped(int target_angle) {

    rotate_platform((180 + target_angle) %360);
    delay_busy(100);

    set_card_flipper_flipped();
    delay_busy(900);

    push_card_out_flipped();
    delay_busy(100);
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
    machine_state = MACHINE_STATE_RESETING;
    info_logf("STARTING GAME");
    set_is_working(true);
    send();
    
    #ifndef DEBUG_DEALER
    digitalWrite(STEPPER_STEP,HIGH); 
    rotate_platform(0); 
    set_card_flipper_regular();
    #endif

    #ifdef DEBUG_DEALER
    delay_busy(1000);
    #endif

    machine_state = MACHINE_STATE_IDLE;
    game_state = GAME_STATE_STARTED;
    set_is_working(false);
    info_logf("GAME STARTED");
    send();
}

void detect_players() {
    info_logf("DET START");
    machine_state = MACHINE_STATE_SCANNING_PLAYERS;
    set_is_working(true);
    send();

    num_of_players = 0;
    for(int i = 0; i < MAX_NUM_OF_PLAYERS; i++) player_angles[i] = -1;

    #ifndef DEBUG_DEALER
    int indicator = 0;
    for (int pos = 0; pos <= 180; pos += 9) { // goes from 180 degrees to 0 degrees - 9 degrees jump.
        rotate_platform(pos);
        if(is_player_detected_and_range() != -1 && indicator == 0) {
            player_angles[num_of_players] = pos;
            info_logf("DET [%d]", pos);
            num_of_players++;
            indicator = 1;
        }
        else if (is_player_detected_and_range() == -1){
            indicator = 0;
        }
        if (num_of_players == 4){
            break;
        }
    }
    rotate_platform(90);
    #endif

    #ifdef DEBUG_DEALER
    delay_busy(1000);
    #endif

    machine_state = MACHINE_STATE_IDLE;
    game_state = GAME_STATE_SCANNED_PLAYERS;
    set_is_working(false);
    info_logf("DET END");
    send();
}

void start_round() {
    info_logf("NEWROUND START");
    machine_state = MACHINE_STATE_ROUND_STARTING;
    set_is_working(true);
    send();

    #ifndef DEBUG_DEALER
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
    #endif

    #ifdef DEBUG_DEALER
    delay_busy(1000);
    #endif

    machine_state = MACHINE_STATE_IDLE;
    game_state = GAME_STATE_ROUND_STARTED;
    set_is_working(false);
    info_logf("NEWROUND END");
    send();
}

void show_cards_round_1() {
    info_logf("SHOW1 START");
    machine_state = MACHINE_STATE_DEALING_CARD_1;
    set_is_working(true);
    send();

    #ifndef DEBUG_DEALER

    deal_card(false, 115);
    deal_card(true, 105);
    deal_card(true, 95);
    deal_card(true, 85);

    #endif

    #ifdef DEBUG_DEALER
    delay_busy(1000);
    #endif

    machine_state = MACHINE_STATE_IDLE;
    game_state = GAME_STATE_DEALT_CARD_1;
    set_is_working(false);
    info_logf("SHOW1 END");
    send();
}

void show_cards_round_2() {
    info_logf("SHOW2 START");
    machine_state = MACHINE_STATE_DEALING_CARD_2;
    set_is_working(true);
    send();

    #ifndef DEBUG_DEALER

    deal_card(false, 115);
    deal_card(true, 75);

    #endif

    #ifdef DEBUG_DEALER
    delay_busy(1000);
    #endif

    machine_state = MACHINE_STATE_IDLE;
    game_state = GAME_STATE_DEALT_CARD_2;
    set_is_working(false);
    info_logf("SHOW2 END");
    send();
}

void show_cards_round_3() {
    info_logf("SHOW3 START");
    machine_state = MACHINE_STATE_DEALING_CARD_3;
    set_is_working(true);
    send();

    #ifndef DEBUG_DEALER

    deal_card(false, 115);
    deal_card(true, 65);

    #endif

    #ifdef DEBUG_DEALER
    delay_busy(1000);
    #endif

    machine_state = MACHINE_STATE_IDLE;
    game_state = GAME_STATE_DEALT_CARD_3;
    set_is_working(false);
    info_logf("SHOW3 END");
    send();
}

void round_ended() {
    set_card_flipper_regular();
}

void celebrate() {

    info_logf("CELEB START");
    machine_state = MACHINE_STATE_CELEBRATING;
    set_is_working(true);
    send();

    set_card_flipper_up();
    
    digitalWrite(CARD_ACCELERATOR_1, LOW);
    digitalWrite(CARD_ACCELERATOR_2, HIGH);
    card_pusher_servo.write(60);
    delay_busy(5000);
    card_pusher_servo.write(90);
    digitalWrite(CARD_ACCELERATOR_1, LOW);
    digitalWrite(CARD_ACCELERATOR_2, LOW);

    
    machine_state = MACHINE_STATE_IDLE;
    set_is_working(false);
    info_logf("CELEB END");
    send();
}

void player_quit(int player_idx) {
    player_angles[player_idx] = -1;
    num_of_players--;
}

void reset() {
    machine_state = MACHINE_STATE_RESETING;
    info_logf("RESETING...");
    set_is_working(true);
    send();

    #ifndef DEBUG_DEALER

    for(int i = 0; i < 4; i++) {
        player_angles[i] = -1;
    }
    num_of_players = 0;
    rotate_platform(0);
    current_degree = 0;
    machine_state = MACHINE_STATE_IDLE;
    game_state = GAME_STATE_NOT_STARTED;
    #endif

    #ifdef DEBUG_DEALER
    delay_busy(1000);
    #endif

    machine_state = MACHINE_STATE_IDLE;
    set_is_working(false);
    info_logf("RESET");
    send();
}

/*********************************
 * Setup
 *********************************/
void setup() {
    setup_server();
    #ifndef DEBUG_DEALER
    setup_servos();
    setup_dc_motors();
    setup_stepper();
    setup_proximity_sensor_and_color_sensor();
    #endif
}

/*********************************
 * Loop
 *********************************/
void loop() {

    if(server->command_available()) {
        // Serial.println(".");    
        int command = server->get_command();
        int arg1 = server->get_arg1();
        int arg2 = server->get_arg2();

        #ifdef DEBUG_DEALER
        if(command != COMMAND_DO_NOTHING) {
            pinMode(LED_BUILTIN, OUTPUT);
            for(int i = 0; i< 10; i++) {
                digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
                delay_busy(100);                       // wait for a second
                digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
                delay_busy(100); 
            }

            server->flush();
        }
        #endif

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
                show_cards_round_3();
                break;
            case COMMAND_ROUND_ENDED:
                round_ended();
                break;
            case COMMAND_CELEBRATE:
                celebrate();
                break;
            case COMMAND_DEAL_CARD:
                deal_card(arg1 == 1, player_angles[arg2]);
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
    delay_busy(10);
    
}