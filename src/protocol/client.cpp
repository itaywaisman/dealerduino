#include "protocol/client.h"
#include "protocol/utils.h"

SerialClient::SerialClient(int rx, int tx) 
: serial(rx, tx) {
    pinMode(tx, OUTPUT);
    pinMode(rx, INPUT);
    this->flush_log();
    this->serial.begin(115200);
    
}

const unsigned long timeout = 1000;

void SerialClient::sync() {
    // if(!this->last_transmit_ok) {
    //     if(millis() - this->last_transmit_time > timeout) {
    //         if(retransmit_count < TRANSMIT_RETRIES)
    //             this->send_bytes(0, this->last_transmit_packet.packet_num, this->last_transmit_packet.command, this->last_transmit_packet.arg1, this->last_transmit_packet.arg2);
    //         else {
    //             Serial.printf("ERROR TRANSMIT packet_num: %d command: %d\n", last_transmit_packet.packet_num, last_transmit_packet.command);
    //             last_transmit_ok = true; // DUMP the packet and move on
    //         }
    //     }
    // } else {
        this->retransmit_count = 0;
        this->send_bytes(0, rand(), this->stored_command, this->stored_command_arg1, this->stored_command_arg2);
    // }
    this->receive_bytes();
    this->parse_bytes();
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

int SerialClient::get_machine_state() {
    return this->current_machine_state;
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

int SerialClient::get_player1_angle() {
    return this->current_player1_angle;
}

int SerialClient::get_player2_angle() {
    return this->current_player2_angle;
}

int SerialClient::get_player3_angle() {
    return this->current_player3_angle;
}

int SerialClient::get_player4_angle() {
    return this->current_player4_angle;
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

void SerialClient::flush_state() {
    this->current_game_state = -1;
    this->current_machine_state = -1;
    this->current_num_of_players = 0;
    this->current_is_working = false;
}

void SerialClient::receive_bytes() {
    static boolean recvInProgress = false;
    static byte ndx = 0;

    // Start and End markers for reliability
    byte startMarker = 0x3C;
    byte endMarker = 0x3E;

    // Read buffer
    byte rb;

    int start_marker_count = 0;
    int end_marker_count = 0;

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
                end_marker_count+=1;
                if(end_marker_count == END_SEQ_LENGTH) {
                    recvInProgress = false;
                    this->num_bytes_received = ndx;  // save the number for use when printing
                    ndx = 0;
                    this->has_new_data = true;
                }
            }
        }

        else if (rb == startMarker) {
            start_marker_count+=1;
            if(start_marker_count == START_SEQ_LENGTH) {
                recvInProgress = true;
            }
        }
    }
}

bool SerialClient::validate_packet() {
        server_packet_t* packet = (server_packet_t*)this->received_bytes;
        
        const byte* data = (byte*)&packet + sizeof(uint8_t); //skip packet_num and checksum values
        uint8_t checksum = calc_checksum(data, sizeof(server_packet_t) - sizeof(uint8_t));

        // return checksum == packet->checksum;
        return true;
        
}

void SerialClient::parse_bytes(){
    if (this->has_new_data == true) {
        if(validate_packet()) {
            server_packet_t* packet = (server_packet_t*)this->received_bytes;
        
            // if(packet->ack == 1) {
            //     this->last_transmit_ok = packet->packet_num == last_transmit_packet.packet_num;
            //     Serial.printf("GOT ACK packet_num: %d command: %d\n", packet->packet_num, last_transmit_packet.command);
            // } else {
                this->current_machine_state = packet->machine_state;
                this->current_game_state = packet->game_state;
                this->current_is_working = packet->is_working == 1;
                this->current_num_of_players = packet->num_of_players;
                this->current_player1_angle = packet->player1_angle;
                this->current_player2_angle = packet->player2_angle;
                this->current_player3_angle = packet->player3_angle;
                this->current_player4_angle = packet->player4_angle;
                for(int i = 0; i < LOG_LENGTH ; i++) this->current_log[i] = packet->log[i];
                this->is_state_available = true;

                // // Acknowledge packet
                // this->send_bytes(1, packet->packet_num, 0, 0, 0);
            // }            
        }   

        this->has_new_data = false;
        
    }
}

void SerialClient::send_bytes(uint8_t ack, uint8_t packet_num, uint8_t command, uint8_t arg1, uint8_t arg2) {
    client_packet_t packet = {};
    packet.ack = ack;
    packet.packet_num = packet_num;
    packet.command = command;
    packet.arg1 = arg1;
    packet.arg2 = arg2;
    
    const byte* data = (byte*)&packet + sizeof(uint8_t); //skip packet_num and checksum values
    packet.checksum = calc_checksum(data, sizeof(client_packet_t) - sizeof(uint8_t));
    
    
    byte startMarker = 0x3C; // >
    byte endMarker = 0x3E; // <

    for(int i=0; i<START_SEQ_LENGTH; i++) this->serial.write(startMarker);
    int n_transmitted = this->serial.write((byte*)&packet, sizeof(packet));
    for(int i=0; i<END_SEQ_LENGTH; i++) this->serial.write(endMarker);

    // // if(ack == 0) {
    //     this->retransmit_count += 1;
    //     this->last_transmit_packet = packet;
    //     this->last_transmit_time = millis();
    //     this->last_transmit_ok = false;
    //     Serial.printf("REGULAR ack: %d packet_num: %d command: %d\n", ack, packet_num, command);
    // // } else {
    //     this->last_transmit_ok = true;
    //     Serial.printf("ACK ack: %d packet_num: %d command: %d\n", ack, packet_num, command);
    // }
    
}
