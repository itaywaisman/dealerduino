#include "./protocol/server.h"

SerialServer::SerialServer(int rx, int tx, int baud = 9600) 
: serial(rx, tx) {
    pinMode(tx, OUTPUT);
    pinMode(rx, INPUT);
    for(int i = 0 ; i < LOG_LENGTH; i++) this->stored_log[i] = 0;
    this->serial.begin(baud);
    
}

void SerialServer::sync() {
    this->receive_bytes();
    this->parse_bytes();
    this->send_bytes();
}

boolean SerialServer::command_available() {
    return this->is_command_available;
}


int SerialServer::get_command() {
    return this->current_command;
}


int SerialServer::get_arg1() {
    return this->current_command_arg1;
}


int SerialServer::get_arg2() {
    return this->current_command_arg2;
}


void SerialServer::set_game_state(int game_state) {
    this->stored_game_state = game_state;
}


void SerialServer::set_num_of_players(int num_of_players) {
    this->stored_num_of_players = num_of_players;
}

void SerialServer::set_log(char log[LOG_LENGTH]) {
    for(int i = 0 ; i < LOG_LENGTH; i++) this->stored_log[i] = log[i];
}

void SerialServer::receive_bytes() {
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

void SerialServer::parse_bytes(){
    if (this->has_new_data == true) {

        client_packet_t* data = (client_packet_t*)this->received_bytes;
        
        if(data->command_num > this->current_command_num) {
            this->current_command = data->command_num;
            this->current_command_arg1 = data->arg1;
            this->current_command_arg2 = data->arg2;
            this->current_command_num = data->command_num;
            this->is_command_available = true;
        }

        this->has_new_data = false;
    }
}

void SerialServer::send_bytes() {
    server_packet_t state = {};
    state.packet_num = this->stored_packet_num + 1;
    state.game_state = this->stored_game_state;
    state.is_working = this->stored_is_working ? 1 : 0;
    state.num_of_players = this->stored_num_of_players;
    for(int i = 0 ; i < LOG_LENGTH; i++) state.log[i] = this->stored_log[i];
    
    byte startMarker = 0x3C; // >
    byte endMarker = 0x3E; // <

    for(int r = TRANSMIT_RETRIES; r > 0; r--) {
        Serial.write(startMarker);
        int n_transmitted = Serial.write((byte*)&state, sizeof(state));
        Serial.write(endMarker);
        if(n_transmitted == sizeof(state)) break;
    }
    
    this->stored_packet_num = state.packet_num;
    this->stored_reset_flag = 0;
    for(int i = 0 ; i < LOG_LENGTH; i++) this->stored_log[i] = 0;
}