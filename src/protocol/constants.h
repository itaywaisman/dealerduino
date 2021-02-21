#ifndef constants_h
#define constants_h

#define TRANSMIT_RETRIES 3

#define MAX_NUM_OF_PLAYERS 4

#define GAME_STATE_NOT_STARTED             0x000
#define GAME_STATE_STARTED                 0x001
#define GAME_STATE_SCANNING_PLAYERS        0x002
#define GAME_STATE_SCANNED_PLAYERS         0x003
#define GAME_STATE_ROUND_STARTING          0x004
#define GAME_STATE_ROUND_STARTED           0x010
#define GAME_STATE_DEALING_CARD_1          0x011
#define GAME_STATE_DEALT_CARD_1            0x012
#define GAME_STATE_DEALING_CARD_2          0x021
#define GAME_STATE_DEALT_CARD_2            0x022
#define GAME_STATE_DEALING_CARD_3          0x031
#define GAME_STATE_DEALT_CARD_3            0x032
#define GAME_STATE_ROUND_FINISHED          0x019
#define GAME_STATE_GAME_ENDED              0x100

#define COMMAND_DO_NOTHING 0
#define COMMAND_START_GAME 1
#define COMMAND_SCAN_PLAYERS 2
#define COMMAND_START_ROUND 3
#define COMMAND_SHOW_CARD_1 4
#define COMMAND_SHOW_CARD_2 5
#define COMMAND_SHOW_CARD_3 6
#define COMMAND_PLAYER_QUIT 998
#define COMMAND_RESET 999

#define LOG_LENGTH 32

typedef struct __attribute__((__packed__)) client_packet_t {
    uint8_t command_num;
    uint8_t command;
    uint8_t arg1;
    uint8_t arg2;
}* ClientPacket;

typedef struct __attribute__((__packed__)) server_packet_t {
    uint8_t packet_num;
    uint8_t reset_flag;
    uint8_t game_state;
    uint8_t is_working;
    uint8_t num_of_players;
    char log[LOG_LENGTH];
}* ServerPacket;


#endif