#ifndef serial_client_h
#define serial_client_h

#include "Arduino.h"
#include <SoftwareSerial.h>


#include "protocol/constants.h"

class SerialClient {
    public:
        /**
         * <desc>
         * Initializes and setup the serial port for communication
         * 
         * <param> rx - the rx port for serial communication
         * <param> tx - the tx port for serial communication
         * <param> baud - the baud rate for serial communication
         */
        SerialClient(int rx, int tx);

        /**
         * <desc>
         * Tries to read the next command, and sends the current state back to the client.
         * This sends the current state to the client even if it didn't change.
         */
        void sync();

        /***
         * <desc>
         * Returns true if a new state was recieved
         */
        boolean state_available();

        /**
         * <desc>
         * Get the current command stored.
         * 
         * <return> The current command.
         */
        void set_command(int command);

        /**
         * <desc>
         * set the current command arg1.
         * 
         * <return> The current command arg1.
         */
        void set_arg1(int arg1);

        /**
         * <desc>
         * set the current command arg2.
         * 
         * <return> The current command arg2.
         */
        void set_arg2(int arg2);

        /**
         * <desc>
         * get the current game state.
         */
        int get_game_state();

        /**
         * <desc>
         * Returns true if the is_working flag equals 1
         */
        bool is_working();

        /**
         * <desc>
         * get the detected number of players
         */
        int get_num_of_players();

        bool has_log();

        char* get_log();
        
        void flush_log();

    private:
        SoftwareSerial serial;

        int stored_command_num = -1;
        int stored_command = COMMAND_DO_NOTHING;
        int stored_command_arg1 = 0;
        int stored_command_arg2 = 0;

        boolean is_state_available = false;
        int current_packet_num = 0;
        int current_game_state = GAME_STATE_NOT_STARTED;
        bool current_is_working = false;
        int current_num_of_players = 0;
        char current_log[LOG_LENGTH];

        const byte numBytes = sizeof(server_packet_t);
        byte received_bytes[sizeof(server_packet_t)];
        byte num_bytes_received = 0;

        boolean has_new_data = false;

        void receive_bytes();
        void parse_bytes();
        void send_bytes();
};


#endif