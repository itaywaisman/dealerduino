//
// Created by LIORD on 20/11/2020.
//

#include "Arduino.h"
#include <Servo.h>
#include <Wire.h>
#include "VL53L1X.h"
#include <SoftwareSerial.h>
#include "ESP8266.h"
#include "FirebaseArduino.h"

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

#define SSID "BezeqWiFi51"
#define WIFI_PASS "q1w2e3r4"

#define FIREBASE_HOST "https://dealerduino-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "AIzaSyB3Ls1UACtiIUWYpg56qwFP_mt96uR8bIg"

enum DEALER_COMMANDS {
    DO_NOTHING = 0,
    SCAN_PLAYERS = 1,
    DEAL_CARD = 2,
};


SoftwareSerial ser(RX,TX);
ESP8266 wifi(ser)

VL53L1X distance_sensor;

Servo card_flipping_servo;
Servo platform_rotation_servo;

void setup_wifi() {
    ser.begin(9600);

    ser.begin(9600);

    Serial.print("FW Version:");
    Serial.println(wifi.getVersion().c_str());

    if (wifi.setOprToStationSoftAP()) {
        Serial.print(F("to station + softap ok\r\n"));
    } else {
        Serial.print(F("to station + softap err\r\n"));
    }
    if (wifi.joinAP(SSID, WIFI_PASS))
    {
        Serial.print(F("Join AP success\r\n"));
        Serial.print(F("IP: "));
        Serial.println(wifi.getLocalIP().c_str());
    }
    else
    {
        Serial.print(F("Join AP failure\r\n"));
        while(1);
    }

    if (wifi.disableMUX())
    {
        Serial.print(F("single ok\r\n"));
    }
    else
    {
        Serial.print(F("single err\r\n"));
    }
    Serial.print(F("setup end\r\n"));
    delay(2000);

}

void setup_firebase() {
    Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
}

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

    setup_wifi();
//    setup_dc_motors();
//    setup_servos();
    setup_proximity_sensor();
//    setup_color_sensor();
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

void rotate_platform(int deg) {
    platform_rotation_servo.write(deg);
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

void detect_cards() {
    for (int pos = 180; pos >= 0; pos -= 1) { // goes from 180 degrees to 0 degrees
        rotate_platform(pos);
        if(is_player_detected()) {
            Serial.write("DETECTED PLAYER");
            // save degree to FIREBASE
        }
    }
}

void deal_regular() {
    spin_cards_wheel(2000);
    delay(200);
}

void deal_flipped() {
    set_card_flipper_insert();
    delay(1000);
    spin_cards_wheel(2000);
    delay(500);
    set_card_flipper_flip();
    delay(500);
    set_card_flipper_regular();
    delay(500);
}

void deal_card(boolean is_open) {
    boolean card_facing_up = is_card_facing_up();
    if(is_open && card_facing_up) {
        deal_regular();
    }
    if(is_open && !card_facing_up) {
        deal_flipped();
    }
    if(!is_open && card_facing_up) {
        deal_flipped();
    }
    if(!is_open && !card_facing_up) {
        deal_regular();
    }
}

/*********************************
 * Loop
 *********************************/

void loop() {


    int command = Firebase.getInt("command");
    int deal_open = Firebase.getInt("deal_open");
    Serial.print("Got command:")
    Serial.print(command);
    Serial.println();

    switch (command) {
        case DEAL_CARD:
            deal_card(deal_open == 0);
        default:
    }

    delay(500);
}