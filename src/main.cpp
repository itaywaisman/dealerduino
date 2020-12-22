//
// Created by LIORD on 20/11/2020.
//

#include "Arduino.h"
#include <Servo.h>
#include <Wire.h>
#include "VL53L1X.h"
#include <SoftwareSerial.h>

// "http://librarymanager/All#SparkFun_VL53L1X

#define s0 4       //Module pins wiring
#define s1 5
#define s2 6
#define s3 7
#define out 8
//
#define enA 12
#define in1 12
#define in2 13
#define in3 6
#define in4 7

#define CARD_FLIPPING_OUT 3
#define PLATFORM_ROTATION_OUT 4

#define CARD_FLIPPER_REGULAR 100
#define CARD_FLIPPER_INSERT 110
#define CARD_FLIPPER_FLIP 0

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
};

String serialResponse; // complete message from arduino, which consists of snesors data
char sample_packet[] = "0;0;0;0;0";

/// GAME STATE
int game_state = NOT_STARTED;
int player_angles[8] = {-1, -1, -1, -1, -1, -1, -1, -1};

VL53L1X distance_sensor;

Servo card_flipping_servo;
Servo platform_rotation_servo;



void setup_dc_motors() {
    pinMode(enA, OUTPUT);
    pinMode(in1, OUTPUT);
    pinMode(in2, OUTPUT);
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
    platform_rotation_servo.attach(PLATFORM_ROTATION_OUT);

}

void setup() {

    Serial.begin(9600);
    Wire.begin();
    Wire.setClock(400000); // use 400 kHz I2C

    setup_dc_motors();
    setup_servos();
    setup_proximity_sensor();
    setup_color_sensor();
}


/*********************************
 * Hardware methods
 *********************************/


void spin_cards_wheel(int ms=1000) {
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
    delay(ms);

    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
    delay(ms);
}

void set_card_flipper_regular() {
    card_flipping_servo.write(CARD_FLIPPER_REGULAR);
}

void set_card_flipper_insert() {
    card_flipping_servo.write(CARD_FLIPPER_INSERT);
}

void set_card_flipper_flip() {
    card_flipping_servo.write(CARD_FLIPPER_FLIP);
}

void rotate_platform(int target_degree) {
    platform_rotation_servo.write(target_degree);
    delay(15);                       // waits 15ms for the servo to reach the position
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

    return blue_value > 22 and green_value > 40 and red_value > 50;
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

    rotate_platform(90);
}

void deal_regular(int target_angle) {
    spin_cards_wheel(2000);
    delay(200);
}

void deal_flipped(int target_angle) {

    rotate_platform(360 - target_angle);

    set_card_flipper_insert();
    delay(1000);
    spin_cards_wheel(2000);
    delay(500);
    set_card_flipper_flip();
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
    Serial.print("STATE " + game_state);
}


/*********************************
 * Loop
 *********************************/

void loop() {

    if (Serial.available() > 0 ) 
    {
        serialResponse = Serial.readStringUntil('\r\n');

        char buf[sizeof(sample_packet)];
        serialResponse.toCharArray(buf, sizeof(buf));

        char *p = buf;
        char *str;
        int parts [5];
        int current_part = 0;
        while ((str = strtok_r(p, ";", &p)) != NULL) { // delimiter is the semicolon
            parts[current_part] = String(str).toInt();
            current_part++;
        }

        int command = parts[0];
        int arg1 = parts[1];
        // int arg2 = parts[2];
        // int arg3 = parts[3];
        // int arg4 = parts[4];

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

        sync_game_state();
        

    }
}