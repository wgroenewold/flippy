#include <TimeLib.h>
#include <Time.h>                 //https://github.com/PaulStoffregen/Time
#include <Timezone.h>             //https://github.com/JChristensen/Timezone
#include <NTPClient.h>            //https://github.com/arduino-libraries/NTPClient
#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library
#include <WiFiUdp.h>              
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic

//const char ssid     = "<SSID>";
//const char password = "<PASSWORD>";
//const byte  hw_hour = 23; //Hours as shown on the clock
//const byte  hw_minute = 59; //Minutes as shown on the clock

int pulsecount = 0;
bool polarityState = false;

#define RST_PIN 0;

//Pin configuration for L298N
int ENA = 4;
int IN1 = 0;
int IN2 = 2;

WiFiManager wm;
WiFiManagerParameter hours_field;
WiFiManagerParameter minutes_field;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

void setup() {
  WiFi.mode(WIFI_STA);

  Serial.begin(115200);
  //WiFi.begin(ssid, password);
  Serial.setDebugOutput(true);  
  delay(3000);
  pinMode(RST_PIN, INPUT);

  new (&hours_field) WiFiManagerParameter("hours_id", "Hours", "", 2,"placeholder=\"23\"");
  new (&minutes_field) WiFiManagerParameter("minutes_id", "Minutes", "", 2,"placeholder=\"59\"");

  wm.addParameter(&custom_field1);
  wm.addParameter(&custom_field2);
  wm.setSaveParamsCallback(saveParamCallback);

  
//  while ( WiFi.status() != WL_CONNECTED ) {
//    delay ( 500 );
//    Serial.print ( "." );
//  }

  timeClient.begin();

  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  bool res;
  res = wm.autoConnect("fl1ppy");

  if(getParam("hours_id") && getParam("minutes_id")){
    byte hw_hour = getParam("hours_id").toInt();
    byte hw_minute = getParam("minutes_id").toInt();
  
    int pulsecount;

    pulsecount = SyncHardware();
    if(pulsecount >= 375){
      SyncHardware();  
    }     
  }  
}

int SyncHardware(){
  unsigned long ntp_epoch;
  ntp_epoch = timeClient.getEpochTime();

  unsigned long hw_epoch;
  hw_epoch = TimeToEpoch(year(ntp_epoch), month(ntp_epoch), timeClient.getDay(), hw_hour, hw_minute, 0);
  
  if(hw_epoch >= ntp_epoch){
    //eerst pulsen naar 0, dan naar tijd
    int pulsecount = MinutesToMidnight(hw_hour, hw_minute);
  }

  time_t corrected_ntp_time;
  TimeChangeRule *tcr;
  
  TimeChangeRule nlDST = {"DST", Last, Sun, Mar, 1, +120};  //UTC + 2 hours
  TimeChangeRule nlSTD = {"STD", Last, Sun, Oct, 1, +60};   //UTC + 1 hour
  Timezone nlDST(nlDST, nlSTD);
  
  corrected_ntp_time = nlDST.toLocal(ntp_epoch, &tcr); //corrected_ntp_time is NTP time with TZ + DST correction
  
  int pulsecount += (hour(corrected_ntp_time) * 60);
  int pulsecount += (minute(corrected_ntp_time); 
 
  pulse(pulsecount);

  return pulsecount
}

void loop() {
    //als de timeclient achterloopt dan moet je een tijdje stilstaan. Ook bij DST correctie.
    //timechangerules moeten hier ook nog wat in    
    //if timeclient gets updated, sent pulse (odd or even somehow)

    checkButton();

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

void pulse(int count){
  for(int i = 0; i < count; i++){
    if(polarityState == false){
      digitalWrite(IN1, HIGH); //Polarity state #1
      digitalWrite(IN2, LOW);
    }else{   
      digitalWrite(IN1, LOW); //Polarity state #2
      digitalWrite(IN2, HIGH);
    } 

    delay(160); //160ms pulse

    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);

    polarityState = !polarityState;
  }
}

void flip(){
  pulse(1);
  delay(60000);
}

//Short press = start portal (50ms)
//Long press = reset config (3s)

//deze functie nog even herschrijven want die blocked
void checkButton(){
  // check for button press
  if ( digitalRead(TRIGGER_PIN) == LOW ) {
    // poor mans debounce/press-hold, code not ideal for production
    delay(50);
    if( digitalRead(TRIGGER_PIN) == LOW ){
      Serial.println("Button Pressed");
      // still holding button for 3000 ms, reset settings, code not ideaa for production
      delay(3000); // reset delay hold
      if( digitalRead(TRIGGER_PIN) == LOW ){
        Serial.println("Button Held");
        Serial.println("Erasing Config, restarting");
        wm.resetSettings();
        ESP.restart();
      }
      
      // start portal w delay
      Serial.println("Starting config portal");
      wm.setConfigPortalTimeout(120);
      
      if (!wm.startConfigPortal("fl1ppy")) {
        Serial.println("failed to connect or hit timeout");
        delay(3000);
        // ESP.restart();
      } else {
        //if you get here you have connected to the WiFi
        Serial.println("connected...yeey :)");
      }
    }
  }
}

String getParam(String name){  
  String value;
  if(wm.server->hasArg(name)) {
    value = wm.server->arg(name);
  }
  return value;
}

void saveParamCallback(){ 
  Serial.println("PARAM hours = " + getParam("hours_id"));
  Serial.println("PARAM minutes = " + getParam("minutes_id"));
}
