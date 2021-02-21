
#include <FirebaseESP8266.h>	// Install Firebase ESP8266 library

#include "protocol/constants.h"
#include "protocol/client.h"


#define WIFI_SSID "BezeqWiFi51"
#define WIFI_PASS "q1w2e3r4"

#define FIREBASE_HOST "dealerduino-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "75X8PNv8e92mpLXpLO0HwYwyB8qfAcfZ0jjJRfkW"


enum DEALER_COMMANDS {
    DO_NOTHING = 0,
    START_GAME = 1,
    SCAN_PLAYERS = 2,
    START_ROUND = 3,
    SHOW_CARD = 4,
    PLAYER_QUIT = 5,
    RESET = 9
};

enum GAME_STATE {
    NOT_STARTED = 0,
    STARTED = 1,
    ROUND_STARTED = 2,
    DEALT_CARD_1 = 12,
    DEALT_CARD_2 = 13,
    DEALT_CARD_3 = 14,
    DEALT_CARD_4 = 15,
    ROUND_FINISHED = 16,
    ERROR = 100
};

SerialClient* client;

FirebaseData fbdo;

FirebaseJson cmdData;
FirebaseJson stateData;

FirebaseJson data;
FirebaseJsonData jsonData;

/*********************************
 * State methods
 *********************************/

// long previousMillis = 0;

// long sync_interval = 100;
// long last_sync_time = 0;

// void sync_if_needed() {
//     unsigned long currentMillis = millis();
    
//     // check if we need to sync the state
//     if(currentMillis - last_sync_time > sync_interval) {
//         client->set(game_state);
//         client->set_num_of_players(num_of_players);
//         client->sync();
//         last_sync_time =  currentMillis;
//     }
// }

void delay_busy(unsigned long ms) {
    unsigned long start_millis = millis();
    unsigned long current_millis = start_millis;

    while(current_millis - start_millis < ms) {
        // The delay hasn't finished
        
        // sync_if_needed();

        current_millis = millis();
    }
}


void setup() {

    Serial.begin(9600);
    while (!Serial) {
        ; // wait for serial port to connect. Needed for Native USB only
    }

    client = new SerialClient(D3, D2);

    // connect to wifi.
    // pinMode(D0,OUTPUT);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.print("connecting");
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(500);
    }
    Serial.println();
    Serial.print("connected: ");
    Serial.println(WiFi.localIP());

    Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
    Serial.println("Connected to firebase.");
    Firebase.reconnectWiFi(true);
    fbdo.setBSSLBufferSize(1024, 1024); //minimum size is 512 bytes, maximum size is 16384 bytes
    fbdo.setResponseSize(1024);    
}

void print_log() {
    if(client->has_log()) {
        Serial.print("[LOG]");
        Serial.printf("[%d] ", millis());
        Serial.print(client->get_log());
        Serial.println();
        client->flush_log();
    }
}

void save_state() {
    int game_state = client->get_game_state();
    int num_players = client->get_num_of_players();
    
    stateData.clear();
    stateData.set("game_state", game_state);
    stateData.set("player_num", num_players);

    if(!Firebase.updateNode(fbdo, "/state", stateData)) {
        Serial.print("set /state failed:");
        Serial.println("REASON: " + fbdo.errorReason());
    }

}

void send_command() {
    if(client->is_working()) return;

    if(!Firebase.getJSON(fbdo, "/cmd")) {
        Serial.print("getting /command failed:");
        Serial.println("REASON: " + fbdo.errorReason());
    } else {
        data = fbdo.jsonObject();

        data.get(jsonData, "command");
        int command = jsonData.intValue;
        data.get(jsonData, "arg1");
        int arg1 = jsonData.intValue;
        data.get(jsonData, "arg2");
        int arg2 = jsonData.intValue;
        
        client->set_command(command);
        client->set_arg1(arg1);
        client->set_arg2(arg2);
        
        cmdData.clear();
        cmdData.set("command", COMMAND_DO_NOTHING);
        cmdData.set("arg1", 0);
        cmdData.set("arg2", 0);

        if(!Firebase.updateNode(fbdo, "/cmd", cmdData)) {
            Serial.print("set /cmd failed:");
            Serial.println("REASON: " + fbdo.errorReason());
        }

    }
}

void loop()
{
    
    if(client->state_available()) {
        print_log();
        save_state();
        send_command();
    }

    client->sync();

}