#include <SoftwareSerial.h>
#include <FirebaseESP8266.h>	// Install Firebase ESP8266 library
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

SoftwareSerial NodeMCUSerial(D2,D3);
String serialResponse; // complete message from arduino, which consists of snesors data
char sample_packet[] = "0;0";

bool received_state = true;

FirebaseData fbdo;

FirebaseJson cmdData;
FirebaseJson stateData;

FirebaseJson data;
FirebaseJsonData jsonData;

void setup() {

    Serial.begin(9600);
    while (!Serial) {
        ; // wait for serial port to connect. Needed for Native USB only
    }
    NodeMCUSerial.begin(9600);

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
    Serial.println("Connected to firebase.");
    Firebase.reconnectWiFi(true);
    fbdo.setBSSLBufferSize(1024, 1024); //minimum size is 512 bytes, maximum size is 16384 bytes
    fbdo.setResponseSize(1024);    
}

void loop()
{
    if(received_state) {
        Serial.println("Reading command...");
        
        if(!Firebase.getJSON(fbdo, "/cmd")) {
            Serial.print("getting /command failed:");
            Serial.println("REASON: " + fbdo.errorReason());
        } else {
            received_state = false;
            Serial.println("Command read complete, parsing...");

            data = fbdo.jsonObject();

            data.get(jsonData, "command");
            int command = jsonData.intValue;
            data.get(jsonData, "arg1");
            int arg1 = jsonData.intValue;
            data.get(jsonData, "arg2");
            int arg2 = jsonData.intValue;
            Serial.printf("%d%d%d\r\n", command, arg1, arg2);
            NodeMCUSerial.printf("%d%d%d\n", command, arg1, arg2);
        }
        Serial.println("Finished");
        
    }
    else {  
        if(NodeMCUSerial.available() > 0) {
            int state_num = NodeMCUSerial.parseInt();
            if(NodeMCUSerial.read() == '\n') {
                NodeMCUSerial.flush();
                Serial.println("ACK! save state, can read next command...");
                int game_state = state_num / 10;
                int num_players = state_num % 10;
                
                stateData.clear();
                stateData.set("game_state", game_state);
                stateData.set("player_num", num_players);
            
                if(!Firebase.updateNode(fbdo, "/state", stateData)) {
                    Serial.print("set /state failed:");
                    Serial.println("REASON: " + fbdo.errorReason());
                }

                cmdData.clear();
                cmdData.set("command", 0);
                cmdData.set("arg1", 0);
                cmdData.set("arg2", 0);

                if(!Firebase.updateNode(fbdo, "/cmd", cmdData)) {
                    Serial.print("set /cmd failed:");
                    Serial.println("REASON: " + fbdo.errorReason());
                }
            }
            received_state = true;

        }
    }
    
    fbdo.clear();
    delay(1000);
}
