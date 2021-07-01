#include <TimeLib.h>
#include <Time.h>                 //https://github.com/PaulStoffregen/Time
//#include <Timezone.h>             //https://github.com/JChristensen/Timezone
#include <NTPClient.h>            //https://github.com/arduino-libraries/NTPClient
#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library
#include <WiFiUdp.h>              
//#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic

bool debug = true;
#define SerialDebug(text)   Serial.print(text);
#define SerialDebugln(text) Serial.println(text);

//deze input vervangen we straks door WifiManager
const char ssid     = "<SSID>";
const char password = "<PASSWORD>";
const byte  hours = 23;
const byte  minutes = 59;

time_t hardware;

WiFiUDP ntpUDP;

NTPClient timeClient(ntpUDP);

//Pin configuration for L298N
int ENA = 4;
int IN1 = 0;
int IN2 = 2;

bool polarityState = false;


void setup() {
//  WiFiManager wifiManager;
//  wifiManager.autoConnect("Flippy");

  Serial.begin(115200);
  WiFi.begin(ssid, password);
  
  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 500 );
    Serial.print ( "." );
  }

  timeClient.begin();

  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  //haal uren en minuten uit NTP
  //indien hwtijd later is dan NTP pulse naar 0 en dan naar tijd
  //indien hwtijd eerder is dan NTP pulse naar tijd
 
  //set hours en minutes in een time_t object en compare dan 2 objecten 
}

void loop() {
    //if timeclient gets updated, sent pulse (odd or even somehow)
    timeClient.update();    
    Serial.println(timeClient.getFormattedTime());
    delay(1000);

    flip();
}


//polaritystate hierin verwerken
void pulse(int count){
  for(int i = 0; i < count; i++){
    if((i % 2) == 0){
      digitalWrite(IN1, HIGH); //Polarity state #1
      digitalWrite(IN2, LOW);
    }else{   
      digitalWrite(IN1, LOW); //Polarity state #2
      digitalWrite(IN2, HIGH);
    } 

    delay(160); //160ms pulse

    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
  }
}

void flip(){
  pulse(1);
  delay(60000);
}
