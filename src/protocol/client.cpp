#include "protocol/client.h"

SerialClient::SerialClient(int rx, int tx) 
: serial(rx, tx) {
    pinMode(tx, OUTPUT);
    pinMode(rx, INPUT);
    this->flush_log();
    this->serial.begin(9600);
    
}

void SerialClient::sync() {
    this->receive_bytes();
    this->parse_bytes();
    this->send_bytes();
}

boolean SerialClient::state_available() {
    return this->is_state_available;
}

void SerialClient::set_command(int command) {
    this->stored_command = command;
}


void SerialClient::set_arg1(int arg1) {
    this->stored_command_arg1 = arg1;
}


void SerialClient::set_arg2(int arg2) {
    this->stored_command_arg2 = arg2;
}


int SerialClient::get_game_state() {
    return this->current_game_state;
}

bool SerialClient::is_working() {
    return this->current_is_working;
}

int SerialClient::get_num_of_players() {
    return this->current_num_of_players;
}

bool SerialClient::has_log() {
    char* log = this->current_log;
    bool has_chars = false;
    for(int i = 0 ; i < LOG_LENGTH; i++) has_chars = has_chars || log[i] != 0;
    return has_chars;
}

char* SerialClient::get_log() {
    return this->current_log;
}

void SerialClient::flush_log() {
    for(int i = 0 ; i < LOG_LENGTH; i++) this->current_log[i] = 0;
}

void SerialClient::receive_bytes() {
    static boolean recvInProgress = false;
    static byte ndx = 0;

    // Start and End markers for reliability
    byte startMarker = 0x3C;
    byte endMarker = 0x3E;

    // Read buffer
    byte rb;

    while (this->serial.available() > 0 && has_new_data == false) {
        rb = this->serial.read();

        if (recvInProgress == true) {
            if (rb != endMarker) {
                this->received_bytes[ndx] = rb;
                ndx++;
                if (ndx >= this->numBytes) {
                    ndx = this->numBytes - 1;
                }
            }
            else {
                recvInProgress = false;
                this->num_bytes_received = ndx;  // save the number for use when printing
                ndx = 0;
                this->has_new_data = true;
            }
        }

        else if (rb == startMarker) {
            recvInProgress = true;
        }
    }
}

void SerialClient::parse_bytes(){
    if (this->has_new_data == true) {
        server_packet_t* packet = (server_packet_t*)this->received_bytes;
        
        if(packet->packet_num > this->current_packet_num || packet->reset_flag == 1) {

            this->current_game_state = packet->game_state;
            this->current_is_working = packet->is_working == 1;
            this->current_num_of_players = packet->num_of_players;
            this->current_packet_num = packet->packet_num;
            for(int i = 0; i < LOG_LENGTH ; i++) this->current_log[i] = packet->log[i];
            this->is_state_available = true;
        }

        this->has_new_data = false;
        
    }
}

void SerialClient::send_bytes() {
    client_packet_t packet = {};
    packet.command_num = this->stored_command_num + 1;
    packet.command = this->stored_command;
    packet.arg1 = this->stored_command_arg1;
    packet.arg2 = this->stored_command_arg2;

    for(int r = TRANSMIT_RETRIES; r > 0; r--) {
        int n_transmitted = this->serial.write((byte*)&packet, sizeof(packet));
        if(n_transmitted == sizeof(packet)) break;
    }
    
    this->stored_command_num = packet.command_num;
}