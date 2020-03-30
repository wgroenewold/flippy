// Flippy - ESP8266 based control for flipclocks

#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library (you most likely already have this in your sketch)

#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <Time.h>                 //https://github.com/PaulStoffregen/Time
#include <Timezone.h>             //https://github.com/JChristensen/Timezone

//Reset Pin
int rstPin = 5;
char flippy_hour[2] = "23";
char flippy_sec[2] = "59";

//TimeZone Settings Details https://github.com/JChristensen/Timezone
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};     //Central European Time (Frankfurt, Paris)
TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};       //Central European Time (Frankfurt, Paris)
Timezone CE(CEST, CET);

//Pointer To The Time Change Rule, Use to Get The TZ Abbrev
TimeChangeRule *tcr;
time_t utc;

//Time server
static const char ntpServerName[] = "time.google.com";

WiFiUDP Udp;
// Local Port to Listen For UDP Packets
uint16_t localPort;

void setup() { 
  pinMode(rstPin, INPUT_PULLUP);
  
  WiFiManager wifiManager;
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setBreakAfterConfig(true);
  if (digitalRead(inPin) == LOW) {   
    delay(5000);
    wifiManager.resetSettings();
    ESP.restart();
  }

  if (!wifiManager.autoConnect("Flippy")) {
    delay(3000);
    ESP.reset();
    delay(5000);
  }
  
  {
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
    }
    // Seed Random With vValues Unique To This Device
    uint8_t macAddr[6];
    WiFi.macAddress(macAddr);
    uint32_t seed1 =
      (macAddr[5] << 24) | (macAddr[4] << 16) |
      (macAddr[3] << 8)  | macAddr[2];
    randomSeed(WiFi.localIP() + seed1 + micros());
    localPort = random(1024, 65535);
    Udp.begin(localPort);
    setSyncProvider(getNtpTime);
    //Set Sync Intervals
    setSyncInterval(5 * 60);
  }
}

void loop() {
  // put your main code here, to run repeatedly:
}

void configModeCallback (WiFiManager *myWiFiManager) {
  
}

/*-------- NTP code ----------*/
const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets
time_t getNtpTime()
{
  IPAddress timeServerIP; // time.nist.gov NTP server address
  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.print(F("Transmit NTP Request "));
  //get a random server from the pool
  WiFi.hostByName(ntpServerName, timeServerIP);
  Serial.println(timeServerIP);
  sendNTPpacket(timeServerIP);
  uint32_t beginWait = millis();
  while ((millis() - beginWait) < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL;
    }
  }
  return 0; // return 0 if unable to get the time
}
// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
