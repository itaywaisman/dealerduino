#ifndef serial_server_h
#define serial_server_h

#include "Arduino.h"
#include <SoftwareSerial.h>

#include "./protocol/constants.h"

class SerialServer {
    public:
        /**
         * <desc>
         * Initializes and setup the serial port for communication
         * 
         * <param> rx - the rx port for serial communication
         * <param> tx - the tx port for serial communication
         * <param> baud - the baud rate for serial communication
         */
        SerialServer();

        /**
         * <desc>
         * Tries to read the next command, and sends the current state back to the client.
         * This sends the current state to the client even if it didn't change.
         */
        void sync();
        void send();
        void flush();

        /**
         * <desc>
         * Returns whether a new command is available.
         * A command is new if the <command_num> argument is strictly larger than the current saved number.
         * 
         * <returns> Returns whether a new command is available.
         */
        boolean command_available();

        /**
         * <desc>
         * Get the current command stored.
         * 
         * <return> The current command.
         */
        int get_command();

        /**
         * <desc>
         * Get the current command arg1.
         * 
         * <return> The current command arg1.
         */
        int get_arg1();

        /**
         * <desc>
         * Get the current command arg2.
         * 
         * <return> The current command arg2.
         */
        int get_arg2();

        /**
         * <desc>
         * Set the current game state.
         */
        void set_machine_state(int machine_state);
        
        /**
         * <desc>
         * Set the current game state.
         */
        void set_game_state(int game_state);

        /**
         * <desc>
         * Set the detected number of players
         */
        void set_num_of_players(int num_of_players);


        void set_angles(int player1_angle, int player2_angle, int player3_angle, int player4_angle);
        /**
         * <desc>
         * Sets the log to send to the client
         */
        void set_log(char log[LOG_LENGTH]);

    private:
        HardwareSerial* serial;

        boolean is_command_available = false;

        int current_command = COMMAND_DO_NOTHING;
        int current_command_arg1 = 0;
        int current_command_arg2 = 0;

        int stored_machine_state = MACHINE_STATE_IDLE;
        int stored_game_state = GAME_STATE_NOT_STARTED;
        bool stored_is_working = false;
        int stored_num_of_players = 0;
        int stored_player1_angle;
        int stored_player2_angle;
        int stored_player3_angle;
        int stored_player4_angle;
        char stored_log[LOG_LENGTH];

        const byte numBytes = sizeof(client_packet_t);
        byte received_bytes[sizeof(client_packet_t)];
        byte num_bytes_received = 0;

        boolean has_new_data = false;

        bool last_transmit_ok = true;
        unsigned long last_transmit_time = 0;
        server_packet_t last_transmit_packet;
        int retransmit_count = 0;

        void receive_bytes();
        bool validate_packet();
        void parse_bytes();
        void send_bytes(uint8_t ack,
         uint8_t packet_num, 
         uint8_t macine_state, 
         uint8_t game_state, 
         uint8_t is_working, 
         uint8_t num_of_players,
         uint8_t player1_angle,
         uint8_t player2_angle,
         uint8_t player3_angle,
         uint8_t player4_angle,
         char* log);
};

#endif