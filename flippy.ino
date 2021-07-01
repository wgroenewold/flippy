#include <TimeLib.h>
#include <Time.h>                 //https://github.com/PaulStoffregen/Time
//#include <Timezone.h>             //https://github.com/JChristensen/Timezone
#include <NTPClient.h>            //https://github.com/arduino-libraries/NTPClient
#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library
#include <WiFiUdp.h>              
//#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic

//deze input vervangen we straks door WifiManager
const char ssid     = "<SSID>";
const char password = "<PASSWORD>";
const byte  hw_hour = 23; //Hours as shown on the clock
const byte  hw_minute = 59; //Minutes as shown on the clock

time_t hardware;

WiFiUDP ntpUDP;

NTPClient timeClient(ntpUDP);

//Pin configuration for L298N
int ENA = 4;
int IN1 = 0;
int IN2 = 2;

int pulsecount = 0;

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

  unsigned long ntp_epoch;
  ntp_epoch = timeClient.getEpochTime();

  unsigned long hw_epoch;
  hw_epoch = TimeToEpoch(year(ntp_epoch), month(ntp_epoch), timeClient.getDay(), hw_hour, hw_minute, 0);
  
  if(hw_epoch >= ntp_epoch){
    //eerst pulsen naar 0, dan naar tijd
    int pulsecount = MinutesToMidnight(hw_hour, hw_minute);
  }
  
  int pulsecount += (timeClient.getHours() * 60);
  int pulsecount += (timeClient.getMinutes()); 
 
  pulse(pulsecount);

  //indien pulsecount >= 375 dan moet je het nog een keer syncen, misschien zelfs in een while loopje tot pulsecount < 375 is.
}

void loop() {
    //if timeclient gets updated, sent pulse (odd or even somehow)
    timeClient.update();    
    Serial.println(timeClient.getFormattedTime());
    delay(1000);

    flip();
}

byte MinutesToMidnight(byte hh, byte mm){
    return (59 - mm) + ((23 - hh)*60) + 1;
}

unsigned long TimeToEpoch(int YYYY, byte MM, byte DD, byte hh, byte mm, byte ss){
  tmElements_t tm;
  
  tm.Year = YYYY - 1970; // years since 1970, so deduct 1970
  tm.Month = MM  - 1;      // months start from 0, so deduct 1
  tm.Day = DD;
  tm.Hour = hh;
  tm.Minute = mm;
  tm.Second = ss;
  
  return (unsigned long) makeTime(tm);
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
