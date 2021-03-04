#ifndef constants_h
#define constants_h

#define TRANSMIT_RETRIES 3

#define START_SEQ_LENGTH 2
#define END_SEQ_LENGTH 2

#define MAX_NUM_OF_PLAYERS 4


#define GAME_STATE_NOT_STARTED             0
#define GAME_STATE_STARTED                 1
#define GAME_STATE_SCANNED_PLAYERS         3
#define GAME_STATE_ROUND_STARTED           10
#define GAME_STATE_DEALT_CARD_1            12
#define GAME_STATE_DEALT_CARD_2            22
#define GAME_STATE_DEALT_CARD_3            32
#define GAME_STATE_ROUND_FINISHED          40
#define GAME_STATE_GAME_ENDED              100

#define MACHINE_STATE_IDLE                    0
#define MACHINE_STATE_RESETING                1
#define MACHINE_STATE_SCANNING_PLAYERS        2
#define MACHINE_STATE_ROUND_STARTING          4
#define MACHINE_STATE_DEALING_CARD_1          11
#define MACHINE_STATE_DEALING_CARD_2          21
#define MACHINE_STATE_DEALING_CARD_3          31
#define MACHINE_STATE_CELEBRATING             64

#define COMMAND_DO_NOTHING                  0
#define COMMAND_START_GAME                  1
#define COMMAND_SCAN_PLAYERS                2
#define COMMAND_START_ROUND                 3
#define COMMAND_SHOW_CARD_1                 4
#define COMMAND_SHOW_CARD_2                 5
#define COMMAND_SHOW_CARD_3                 6
#define COMMAND_ROUND_ENDED                 10
#define COMMAND_CELEBRATE                   64
#define COMMAND_DEAL_CARD                   66
#define COMMAND_PLAYER_QUIT                 100
#define COMMAND_RESET                       101

#define LOG_LENGTH 32


typedef struct __attribute__((__packed__)) client_packet_t {
    uint8_t checksum;
    uint8_t ack;
    uint8_t packet_num;
    uint8_t command;
    uint8_t arg1;
    uint8_t arg2;
}* ClientPacket;

typedef struct __attribute__((__packed__)) server_packet_t {
    uint8_t checksum;
    uint8_t ack;
    uint8_t packet_num;
    uint8_t machine_state;
    uint8_t game_state;
    uint8_t is_working;
    uint8_t num_of_players;
    uint8_t player1_angle;
    uint8_t player2_angle;
    uint8_t player3_angle;
    uint8_t player4_angle;
    char log[LOG_LENGTH];
}* ServerPacket;


#endif