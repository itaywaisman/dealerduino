#include "./protocol/server.h"
#include "protocol/utils.h"

SerialServer::SerialServer() {
    this->serial = &Serial;
    for(int i = 0 ; i < LOG_LENGTH; i++) this->stored_log[i] = 0;
    this->serial->begin(115200);
    
}

const unsigned long timeout = 1000;

void SerialServer::sync() {
    this->receive_bytes();
    this->parse_bytes();
    // if(!this->last_transmit_ok) {
    //     if(millis() - this->last_transmit_time > timeout) {
    //         if(retransmit_count < TRANSMIT_RETRIES)
    //             this->send_bytes(0, this->last_transmit_packet.packet_num, this->last_transmit_packet.game_state, this->last_transmit_packet.is_working, this->last_transmit_packet.num_of_players, this->last_transmit_packet.log);
    //         else
    //             last_transmit_ok = true; // DUMP the packet and move on
    //     }
    // } else {
    //     this->retransmit_count = 0;
        
    // }
}

void SerialServer::send() {
    this->send_bytes(0, rand(), this->stored_machine_state, 
    this->stored_game_state,
    this->stored_is_working, 
    this->stored_num_of_players, 
    this->stored_player1_angle,
    this->stored_player2_angle,
    this->stored_player3_angle,
    this->stored_player4_angle,
    this->stored_log);
    this->stored_game_state = GAME_STATE_NOT_STARTED;
}

void SerialServer::flush() {
    this->is_command_available = false;
    this->current_command = COMMAND_DO_NOTHING;
    this->current_command_arg1 = 0;
    this->current_command_arg2 = 0;
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

void SerialServer::set_machine_state(int machine_state) {
    this->stored_machine_state = machine_state;
}

void SerialServer::set_game_state(int game_state) {
    this->stored_game_state = game_state;
}


void SerialServer::set_num_of_players(int num_of_players) {
    this->stored_num_of_players = num_of_players;
}

void SerialServer::set_angles(int player1_angle, int player2_angle, int player3_angle, int player4_angle) {
    this->stored_player1_angle = player1_angle;
    this->stored_player2_angle = player2_angle;
    this->stored_player3_angle = player3_angle;
    this->stored_player4_angle = player4_angle;
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

    int start_marker_count = 0;
    int end_marker_count = 0;

    while (this->serial->available() > 0 && has_new_data == false) {
        rb = this->serial->read();
        
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

bool SerialServer::validate_packet() {
    client_packet_t* packet = (client_packet_t*)this->received_bytes;
        
    const byte* data = (byte*)&packet + sizeof(uint8_t); //skip packet_num and checksum values
    uint8_t checksum = calc_checksum(data, sizeof(client_packet_t) - sizeof(uint8_t));

    // return checksum == packet->checksum;
    return true;
}

void SerialServer::parse_bytes(){
    if (this->has_new_data == true) {
        if(validate_packet()) {
            client_packet_t* packet = (client_packet_t*)this->received_bytes;
            
            // Serial.println();
            // Serial.print("GOT PACKET packet_num: ");
            // Serial.print(packet->packet_num);
            // for(int i = 0 ; i < sizeof(client_packet_t); i++) Serial.print(received_bytes[i]);
            // Serial.println();

            // if(packet->ack == 1) {
            //     this->last_transmit_ok = packet->packet_num == last_transmit_packet.packet_num;

            //     this->stored_game_state = 0;
            //     this->stored_is_working = 0;
            //     this->stored_num_of_players = 0;
            //     for(int i = 0 ; i < LOG_LENGTH; i++) this->stored_log[i] = 0;
            // } else {
                this->current_command = packet->command;
                this->current_command_arg1 = packet->arg1;
                this->current_command_arg2 = packet->arg2;
                this->is_command_available = true;

                

            //     // Acknowledge packet
            //     this->send_bytes(1, packet->packet_num, 0, 0, 0, "");
            // }       

            
        }

        this->has_new_data = false;
    }
}

void SerialServer::send_bytes(uint8_t ack, uint8_t packet_num, 
    uint8_t machine_state,
    uint8_t game_state,
    uint8_t is_working,
    uint8_t num_of_players,
    uint8_t player1_angle,
    uint8_t player2_angle,
    uint8_t player3_angle,
    uint8_t player4_angle,
    char* log) {
    server_packet_t packet = {};
    packet.ack = ack;
    packet.packet_num = packet_num;
    packet.machine_state = machine_state;
    packet.game_state = game_state;
    packet.is_working = is_working;
    packet.num_of_players = num_of_players;
    packet.player1_angle = player1_angle;
    packet.player2_angle = player2_angle;
    packet.player3_angle = player3_angle;
    packet.player4_angle = player4_angle;
    for(int i = 0 ; i < LOG_LENGTH; i++) packet.log[i] = log[i];
    
    const byte* data = (byte*)&packet + sizeof(uint8_t); //skip packet_num and checksum values
    packet.checksum = calc_checksum(data, sizeof(server_packet_t) - sizeof(uint8_t));

    byte startMarker = 0x3C; // >
    byte endMarker = 0x3E; // <

    for(int i=0; i<START_SEQ_LENGTH; i++) this->serial->write(startMarker);
    int n_transmitted = this->serial->write((byte*)&packet, sizeof(packet));
    for(int i=0; i<END_SEQ_LENGTH; i++) this->serial->write(endMarker);

    for(int i = 0 ; i < sizeof(client_packet_t); i++) Serial.print(((byte*)&packet)[i]);

    
    for(int i = 0 ; i < LOG_LENGTH; i++) this->stored_log[i] = 0;

    // if(ack == 0) {
    //     this->retransmit_count += 1;
    //     this->last_transmit_packet = packet;
    //     this->last_transmit_time = millis();
    //     this->last_transmit_ok = false;
    // } else {
    //     this->last_transmit_ok = true;
    //     Serial.println();
    //     Serial.print("ACK ack: %d ");
    //     Serial.print(ack);
    //     Serial.print("packet_num: "); 
    //     Serial.print(packet_num);
    //     Serial.println();

    // }
}