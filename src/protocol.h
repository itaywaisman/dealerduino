#ifndef protocol_h
#define protocol_h




#include "Arduino.h"
#include <SoftwareSerial.h>

SoftwareSerial* ArduinoUnoSerial;

void setup_serial(int rx, int tx) {
    ArduinoUnoSerial = new SoftwareSerial(rx, tx);
    ArduinoUnoSerial->begin(4800);
    pinMode(tx, OUTPUT);
    pinMode(rx, INPUT);
}

bool send_packet(int packet) {
    bool ack = false;
    int retries = 10;
    while(!ack && retries > 0) {
        ArduinoUnoSerial->print(packet);
        ArduinoUnoSerial->println();
        retries = retries - 1;
        while(ArduinoUnoSerial->available() > 0) {
            char ack_packet = ArduinoUnoSerial->read();
            if(ArduinoUnoSerial->read() == '\n') {
               if(ack_packet == 'A')  {
                   ack = true;
               }
            }
        }
        delay(30);
    }

    return ack;
}

int read_packet() {
    int packet = -1;
    while(packet == -1) {
        while(ArduinoUnoSerial->available() > 0) {
            int packet_data = ArduinoUnoSerial->parseInt();
            if(ArduinoUnoSerial->read() == '\n') {
                packet = packet_data;

                Serial.println('A');
            }
        }
        delay(30);
    }

    return packet;
}

#endif