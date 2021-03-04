
#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>	// Install Firebase ESP8266 library
#include <esp8266httpclient.h>
#include "protocol/constants.h"
#include "protocol/client.h"


#define WIFI_SSID "DESKTOP-P01MN4L 6138"
#define WIFI_PASS "80C179>t"

#define FIREBASE_HOST "dealerduino-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "dtPlzFgFKK3fErpaby8LKk00QvMwrh0JY1H9QHuz"

SerialClient* client;

FirebaseData fbdo;

FirebaseJson cmdData;
FirebaseJson stateData;

FirebaseJson data;
FirebaseJsonData jsonData;

/*********************************
 * State methods
 *********************************/

unsigned long previousMillis = 0;

unsigned long sync_interval = 10;
unsigned long last_sync_time = 0;

void sync_if_needed() {
    unsigned long currentMillis = millis();
    
    // check if we need to sync the state
    if(currentMillis - last_sync_time > sync_interval) {
        
        last_sync_time =  currentMillis;
    }
}

void delay_busy(unsigned long ms) {
    unsigned long start_millis = millis();
    unsigned long current_millis = start_millis;

    while(current_millis - start_millis < ms) {
        // The delay hasn't finished
        
        // sync_if_needed();

        current_millis = millis();
    }
}

bool check_connection() {
    bool successful = false;
    if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status
 
        HTTPClient http;  //Declare an object of class HTTPClient
    
        http.begin("http://jsonplaceholder.typicode.com/users/1");  //Specify request destination
        int httpCode = http.GET();                                  //Send the request
    
        if (httpCode > 0) { //Check the returning code
            String payload = http.getString();   //Get the request response payload
            successful = true;
        }
    
        http.end();   //Close connection
    }
    return successful;
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
    if(!check_connection()) {
        Serial.println("Couldn't connect to the internet!");
    }

    Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
    
    Serial.println("Connected to firebase.");
    // Firebase.reconnectWiFi(true);
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

    int machine_state = client->get_machine_state();
    int game_state = client->get_game_state();
    int num_players = client->get_num_of_players();
    int player1_angle = client->get_player1_angle();
    int player2_angle = client->get_player1_angle();
    int player3_angle = client->get_player1_angle();
    int player4_angle = client->get_player1_angle();
    
    if(machine_state == -1 || game_state == -1) return;

    Serial.printf("Current state: %d %d %d\n", machine_state, game_state, num_players);

    stateData.clear();
    stateData.set("machine_state", machine_state);
    if(game_state != GAME_STATE_NOT_STARTED) stateData.set("game_state", game_state);
    stateData.set("player_num", num_players);

    stateData.set("player1_angle", player1_angle);
    stateData.set("player2_angle", player2_angle);
    stateData.set("player3_angle", player3_angle);
    stateData.set("player4_angle", player4_angle);

    if(!Firebase.updateNode(fbdo, "/state", stateData)) {
        Serial.print("set /state failed:");
        Serial.println("REASON: " + fbdo.errorReason());
    }
    client->flush_state();
}

void send_command() {
    if(client->is_working()) return;
    fbdo.clear();
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
        
        Serial.printf("sending command %d\n", command);
        
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
    }
    send_command();
    client->sync();
    delay(30);
}