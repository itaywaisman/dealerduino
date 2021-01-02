#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>
#include "FirebaseESP8266.h"	// Install Firebase ESP8266 library
#include <ESP8266WiFi.h>


#define WIFI_SSID "waisman"
#define WIFI_PASS "akuarium12"

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

SoftwareSerial NodeMCUSerial(0,1);
String serialResponse; // complete message from arduino, which consists of snesors data
char sample_packet[] = "0;0";

FirebaseData commandData;
FirebaseData arg1Data;
FirebaseData arg2Data;
FirebaseData arg3Data;
FirebaseData arg4Data;

FirebaseData gameStateData;
FirebaseData playerNumData;

void setup() {

    Serial.begin(9600);
    while (!Serial) {
        ; // wait for serial port to connect. Needed for Native USB only
    }
    NodeMCUSerial.begin(4800);

    pinMode(D2,INPUT);
	pinMode(D3,OUTPUT);

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
    Serial.print("Connected to firebase.");
    Firebase.reconnectWiFi(true);

    
}

void loop()
{
  
    // Sync State
    if (NodeMCUSerial.available() > 0 ) 
    {
        serialResponse = NodeMCUSerial.readStringUntil('\r\n');


        char buf[sizeof(sample_packet)];
        serialResponse.toCharArray(buf, sizeof(buf));

        char *p = buf;
        char *str;
        int parts [2];
        int current_part = 0;
        while ((str = strtok_r(p, ";", &p)) != NULL) { // delimiter is the semicolon
            parts[current_part] = String(str).toInt();
            current_part++;
        }

        int game_state = parts[0];
        int player_num = parts[1];

        if(!Firebase.setInt(gameStateData, "/game_state", game_state)) {
            Serial.print("getting /game_state failed. ");
            Serial.println("REASON: " + commandData.errorReason());
        }
        if(!Firebase.setInt(playerNumData, "/player_num", player_num)) {
            Serial.print("getting /game_state failed. ");
            Serial.println("REASON: " + commandData.errorReason());
        }
    }

    bool hasError = false;

    if(!Firebase.getInt(commandData, "/command")) {
        Serial.print("getting /command failed:");
        Serial.println("REASON: " + commandData.errorReason());
        hasError = true;
    }
    if(!Firebase.getInt(arg1Data, "/arg1")) {
        Serial.print("getting /arg1 failed:");
        Serial.println("REASON: " + arg1Data.errorReason());
        hasError = true;
    }
    if(!Firebase.getInt(arg2Data, "/arg2")) {
        Serial.print("getting /arg2 failed:");
        Serial.println("REASON: " + arg2Data.errorReason());
        hasError = true;
    }

    if(!hasError) {
        
        int command = commandData.intData();
        int arg1 = arg1Data.intData();
        int arg2 = arg2Data.intData();
        Serial.printf("%d;%d;%d\r\n", command, arg1, arg2);
        NodeMCUSerial.printf("%d;%d;%d\r\n", command, arg1, arg2);
    }

    if(!Firebase.setInt(commandData, "command", DO_NOTHING)) {
        Serial.print("setting /command failed:");
        Serial.println("REASON: " + arg4Data.errorReason());
    }
    
}
