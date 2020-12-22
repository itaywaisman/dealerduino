#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>
#include <FirebaseArduino.h>
#include <ESP8266HTTPClient.h>


#define WIFI_SSID "BezeqWiFi51"
#define WIFI_PASS "q1w2e3r4"

#define FIREBASE_HOST "https://dealerduino-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "AIzaSyB3Ls1UACtiIUWYpg56qwFP_mt96uR8bIg"


String myString; // complete message from arduino, which consists of snesors data
char rdata; // received characters

int firstVal;


void setup() {

    Serial.begin(9600);

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
}


String getValue(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;
 
    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void loop()
{
  
  if (Serial.available() > 0 ) 
  {
    rdata = Serial.read(); 
    myString = myString+ rdata; 
   // Serial.print(rdata);
        if( rdata == '\n')
        {
            
            String l = getValue(myString, ',', 0);
            
            firstVal = l.toInt(); 
          Serial.println(firstVal); 
              myString = "";

            if ( firstVal == 1001) 
            {
             
            //  Firebase.setString("1001","Present"); 
             Firebase.setString("1001/s_name","fawad");
              Firebase.setString("1001/f_name","Jamshaid khan");
              Firebase.setString("1001/class","4th");
              Firebase.setString("1001/Attendance","present");
              
            }

             if ( firstVal == 1002) 
            {
              
            // Firebase.setString("1002","Present"); 
             Firebase.setString("1002/s_name","mazhar");
              Firebase.setString("1002/f_name","Javeid");
              Firebase.setString("1002/class","4th");
              Firebase.setString("1002/Attendance","present");
            }

         
    
        }
  }

}
