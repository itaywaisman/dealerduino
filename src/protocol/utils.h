#ifndef serial_utils_h
#define serial_utils_h

#include <Arduino.h>

uint8_t calc_checksum(const byte* frame, unsigned long width) {
    uint8_t sum = 0;
    for(unsigned long i = 0; i < width; i++) {
        sum+=frame[i];
    }
    return sum;
}

#endif