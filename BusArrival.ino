/*
  Author : Srijith
  Last Modified : 16-Aug-2016
  More Info : http://www.srisoftstudios.com/arduino/catchingthebusfromthecouch

  LTA Bus Arrival
  This sketch will connect to LTA website and invoke REST API to fetch bus arrival every 60 seconds
  Bus Arrival information fetched for the following bus stops
  
  Bus Stop 46831 : Bus No 913
  Bus Stop 46419 : Bus No 858
  Bus Stop 46419 : Bus No 911
  
  All the time calculation is done in UTC time zone.

  Credits:
  Most of the code is put together by refering various sketches that was in public domain
  LCD Progress Bar - http://forum.arduino.cc/index.php?topic=309129.0
  JSON Parsing - https://github.com/bblanchon/ArduinoJson/tree/master/examples/JsonHttpClient
  NTP Time - https://github.com/PaulStoffregen/Time/tree/master/examples/TimeNTP
 
 */
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <TimeLib.h>
#include <WiFiUdp.h>
#include <Wire.h>

// Set the LCD address to 0x27 for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x3F, 20, 4);

const unsigned long BAUD_RATE = 115200;                 // serial connection speed
const char* ssid     = "<Your SSID>";                   // Wifi SSID
const char* password = "<Your WiFi Password>";          // Wifi Password
const char* server = "datamall2.mytransport.sg";        // server's address
const char* contentType = "application/json";
const char* accountKey   = "<Your account key>";
const char* UUID = "<Your UUID for the account key>";
const unsigned long HTTP_TIMEOUT = 5000;                // max respone time from server
const size_t MAX_CONTENT_SIZE = 2048;                   // max size of the HTTP response
const long timer = 3000;							    // timer to update the LCD progress bar
long interval = 0;						                // interval for 1 minute delay
long lcdInterval = 600;
unsigned long previousMillis = 0;                       // will store last time
unsigned long previousMillisLCD = 0;                    // will store last time
int lcdindex = 0;
//base URL
String resource = "/ltaodataservice/BusArrival?BusStopID=";

WiFiClient client;										// Use WiFiClient class to create TCP connections

WiFiUDP Udp;
unsigned int localPort = 8888;                          // local port to listen for UDP packets
// NTP Servers:
static const char ntpServerName[] = "us.pool.ntp.org";
//static const char ntpServerName[] = "time.nist.gov";
//static const char ntpServerName[] = "time-a.timefreq.bldrdoc.gov";
//static const char ntpServerName[] = "time-b.timefreq.bldrdoc.gov";
//static const char ntpServerName[] = "time-c.timefreq.bldrdoc.gov";

//const int timeZone = 1;   // Central European Time
//const int timeZone = -5;  // Eastern Standard Time (USA)
//const int timeZone = -4;  // Eastern Daylight Time (USA)
//const int timeZone = -8;  // Pacific Standard Time (USA)
//const int timeZone = -7;  // Pacific Daylight Time (USA)
//const int timeZone = 8;   // Singapore Standard Time
const int timeZone = 0;     // UTC

byte percentage_1[8] = { B10000, B10000, B10000, B10000, B10000, B10000, B10000, B10000 };
byte percentage_2[8] = { B11000, B11000, B11000, B11000, B11000, B11000, B11000, B11000 };
byte percentage_3[8] = { B11100, B11100, B11100, B11100, B11100, B11100, B11100, B11100 };
byte percentage_4[8] = { B11110, B11110, B11110, B11110, B11110, B11110, B11110, B11110 };
byte percentage_5[8] = { B11111, B11111, B11111, B11111, B11111, B11111, B11111, B11111 };

// The type of data that we want to extract from LTA JSON Response
struct BusTimingInfo {
  String busNo;
  String busStopNo;
  String nextBus;
  String subsequentBus;
  String subsequentBus3;
};
//BusTimingInfo timingInfo[3]; // Array to hold 3 bus information
BusTimingInfo busTimingInfo[3] = {
  {"913", "46831", "NA", "NA", "NA"},
  {"858", "46419", "NA", "NA", "NA"},
  {"911", "46419", "NA", "NA", "NA"}
};

// Structure to store final arrival informaiton
struct BusArrivalTime {
  String busNo;
  String busStopNo;
  String nextBus;
  String subsequentBus;
  String subsequentBus3;
};
//BusArrivalTime BusArrivalTime[3]; //Array to hold 3 bus information
BusArrivalTime busArrivalTime[3] = {
  {"913", "46831", "NA", "NA", "NA"},
  {"858", "46419", "NA", "NA", "NA"},
  {"911", "46419", "NA", "NA", "NA"}
};

// Initialize Serial port
void initSerial() {
  Serial.begin(BAUD_RATE);
}
// Connect to WiFi
void connectWiFi() {
  // We start by connecting to a WiFi network
  //LCD
  lcd.setCursor(0, 0);
  lcd.print("Connecting to ");
  lcd.setCursor(0, 1);
  lcd.print(ssid);
  lcd.setCursor(0, 2);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    lcd.print(".");
  }
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi Connected");
}

// Open connection to the HTTP server
bool connect(const char* hostName) {
  bool ok = client.connect(hostName, 80);
  return ok;
}


// Send the HTTP GET request to the server
bool sendRequest(String res, String bsno, String srvNo) {
  res += bsno;
  res += "&ServiceNo=";
  res += srvNo;
  //res += "&SST=True"; -- SGT not required at the moment

  client.print("GET ");
  client.print(res);
  client.println(" HTTP/1.1");
  client.print("Host: ");
  client.println(server);
  client.print("AccountKey: ");
  client.println(accountKey);
  client.print("UniqueUserID: ");
  client.println(UUID);
  client.print("accept: ");
  client.println(contentType);
  client.println("Connection: close");
  client.println();

  return true;
}

// Skip HTTP headers so that we are at the beginning of the response's body
bool skipResponseHeaders() {
  // HTTP headers end with an empty line
  char endOfHeaders[] = "\r\n\r\n";

  //client.setTimeout(HTTP_TIMEOUT);
  bool ok = client.find(endOfHeaders);

  if (!ok) {
  }
  return ok;
}

// Read the body of the response from the HTTP server
void readReponseContent(char* content, size_t maxSize) {
  size_t length = client.readBytes(content, maxSize);
  content[length] = 0;
  client.stop();
}

//Parse the JSON from the input string and extract the bus arrival timings
bool parseUserData(char* content, int busIndex) {

  // Compute optimal size of the JSON buffer according to what we need to parse.
  // Allocate a temporary memory pool on the stack
  // If the memory pool is too big for the stack, DynamicBuffer
  DynamicJsonBuffer jsonBuffer;

  //To handle transfer-encoding: chunked
  String contentstr = String(content);
  String finalstr;
  if (content[0] == '{') {
    finalstr = contentstr;
  }
  else
  {
    finalstr = contentstr.substring(4, contentstr.length() - 5);
  }
  //Parse JSON
  JsonObject& root = jsonBuffer.parseObject(finalstr);

  if (!root.success()) {
    return false;
  }
  String BusStop = root["BusStopID"];
  String BusNumber = root["Services"][0]["ServiceNo"];
  String NextBusTime = root["Services"][0]["NextBus"]["EstimatedArrival"];
  String SubsequentBusTime = root["Services"][0]["SubsequentBus"]["EstimatedArrival"];
  String SubsequentBus3Time = root["Services"][0]["SubsequentBus3"]["EstimatedArrival"];

  // Here were copy the strings we're interested in
  busTimingInfo[busIndex].nextBus = NextBusTime.c_str();
  busTimingInfo[busIndex].subsequentBus = SubsequentBusTime.c_str();
  busTimingInfo[busIndex].subsequentBus3 = SubsequentBus3Time.c_str();

  return true;
}

// Print the data extracted from the JSON
void printUserData() {
  //NTP Time Print
  //displayCurrentTime();
}

//Get the time from NTP and sync to local clock
void networkTimeSync() {
  Udp.begin(localPort);
  // wait until the time is set
  while (!(timeStatus() == timeSet))
  {
    lcd.setCursor(0, 0);
    lcd.print("Synchronizing Time..");
    delay(10);
    setSyncProvider(getNtpTime);
  }
  setSyncInterval(3600);     // Sync the system clock from NTP every 24 hours
  lcd.setCursor(0, 0);
  lcd.print("Time Synchronized   ");
  delay(2000);
  lcd.setCursor(0, 0);
  lcd.print("                    ");
}


/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  IPAddress ntpServerIP; // NTP server's ip address

  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
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
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

//Display NTP Time
void displayCurrentTime()
{
  // Display Current Local time
  Serial.print(year());
  Serial.print("-");
  Serial.print(month());
  Serial.print("-");
  Serial.print(day());
  Serial.print("-T");
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.println("+00:00");
}

void printDigits(int digits)
{
  // utility for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

//convert time object to unix time
uint32_t getUnixTime(String ltaDateTime)
{
  //ltaDateTime Format : 2016-07-27T13:27:39+00:00
  tmElements_t tm;
  tm.Second = atoi(ltaDateTime.substring(17, 19).c_str());
  tm.Hour = atoi(ltaDateTime.substring(11, 13).c_str());
  tm.Minute = atoi(ltaDateTime.substring(14, 16).c_str());
  tm.Day = atoi(ltaDateTime.substring(8, 10).c_str());
  tm.Month = atoi(ltaDateTime.substring(5, 7).c_str());
  tm.Year = atoi(ltaDateTime.substring(0, 4).c_str()) - 1970;
  time_t t = makeTime(tm);
  return t;
}

//Convert LTA Timestamp to UTC and compute difference to derive Arrival Time
void calculateArrivalTime(int busIndex)
{
  time_t nextbus_t;
  time_t subsequentBus_t;
  time_t subsequentBus3_t;
  time_t currentTime_t = now(); // Get the current time from system clock which is synced to NTP
  uint32_t diffSeconds; // difference in seconds
  int min;


  if (busTimingInfo[busIndex].nextBus != "") {
    nextbus_t = getUnixTime(busTimingInfo[busIndex].nextBus);
    // Next bus has passed thru but its not yet refreshed in LTA
    // This happens sometimes
    if (currentTime_t > nextbus_t) {
      busArrivalTime[busIndex].nextBus = "0";
    }
    else {
      //Calculate diffSeconds
      diffSeconds = nextbus_t - currentTime_t;
      min = (diffSeconds - (diffSeconds % 60)) / 60;
      busArrivalTime[busIndex].nextBus = min;
    }
  }
  else {
    busArrivalTime[busIndex].nextBus = "NA  ";
  }

  if (busTimingInfo[busIndex].subsequentBus != "") {
    subsequentBus_t = getUnixTime(busTimingInfo[busIndex].subsequentBus);
    // Next bus has passed thru but its not yet refreshed in LTA
    // This happens sometimes
    if (currentTime_t > subsequentBus_t) {
      busArrivalTime[busIndex].subsequentBus = "0";
    }
    else {
      //Calculate diffSeconds
      diffSeconds =  subsequentBus_t - currentTime_t ;
      min = (diffSeconds - (diffSeconds % 60)) / 60;
      busArrivalTime[busIndex].subsequentBus = min;
    }
  }
  else
  {
    busArrivalTime[busIndex].subsequentBus = "NA  ";
  }
  if (busTimingInfo[busIndex].subsequentBus3 != "") {
    subsequentBus3_t = getUnixTime(busTimingInfo[busIndex].subsequentBus3);
    // Next bus has passed thru but its not yet refreshed in LTA
    // This happens sometimes
    if (currentTime_t > subsequentBus3_t) {
      busArrivalTime[busIndex].subsequentBus3 = "0";
    }
    else {
      //Calculate diffSeconds
      diffSeconds = subsequentBus3_t - currentTime_t;
      min = (diffSeconds - (diffSeconds % 60)) / 60;
      busArrivalTime[busIndex].subsequentBus3 = min;
    }
  }
  else
  {
    busArrivalTime[busIndex].subsequentBus3 = "NA  ";
  }
}

//Dispaly ArrivalTimings
void displayArrivalTime(int busIndex)
{
  Serial.print("nextBus in Minutes =");
  Serial.println(busArrivalTime[busIndex].nextBus);
  Serial.print("subsequentBus in Minutes =");
  Serial.println(busArrivalTime[busIndex].subsequentBus);
  Serial.print("subsequentBus3 in Minutes =");
  Serial.println(busArrivalTime[busIndex].subsequentBus3);

}

//Dispaly ArrivalTimings in LCD
void displayLCDArrivalTime(int busIndex)
{
  int pos = busIndex + 1;
  lcd.setCursor(0, 1);
  lcd.print("913:");
  lcd.setCursor(0, 2);
  lcd.print("858:");
  lcd.setCursor(0, 3);
  lcd.print("911:");

  if (busIndex == 0) {
    lcd.setCursor(6, 1);
    lcd.print("    ");
    lcd.setCursor(11, 1);
    lcd.print("    ");
    lcd.setCursor(16, 1);
    lcd.print("    ");
    lcd.setCursor(6, 1);
    lcd.print(busArrivalTime[busIndex].nextBus);
    lcd.setCursor(11, 1);
    lcd.print(busArrivalTime[busIndex].subsequentBus);
    lcd.setCursor(16, 1);
    lcd.print(busArrivalTime[busIndex].subsequentBus3);
  }
  if (busIndex == 1) {
    lcd.setCursor(6, 2);
    lcd.print("    ");
    lcd.setCursor(11, 2);
    lcd.print("    ");
    lcd.setCursor(16, 2);
    lcd.print("    ");
    lcd.setCursor(6, 2);
    lcd.print(busArrivalTime[busIndex].nextBus);
    lcd.setCursor(11, 2);
    lcd.print(busArrivalTime[busIndex].subsequentBus);
    lcd.setCursor(16, 2);
    lcd.print(busArrivalTime[busIndex].subsequentBus3);
  }
  if (busIndex == 2) {
    lcd.setCursor(6, 3);
    lcd.print("    ");
    lcd.setCursor(11, 3);
    lcd.print("    ");
    lcd.setCursor(16, 3);
    lcd.print("    ");
    lcd.setCursor(6, 3);
    lcd.print(busArrivalTime[busIndex].nextBus);
    lcd.setCursor(11, 3);
    lcd.print(busArrivalTime[busIndex].subsequentBus);
    lcd.setCursor(16, 3);
    lcd.print(busArrivalTime[busIndex].subsequentBus3);
  }

}

//Set Err in case of Parsing or Connection Failure
void setArrivalTimeError(int busIndex)
{
  busArrivalTime[busIndex].nextBus = "Err ";
  busArrivalTime[busIndex].subsequentBus = "Err ";
  busArrivalTime[busIndex].subsequentBus3 = "Err ";
}

// display animation
void lcd_percentage(int percentage, int cursor_x, int cursor_x_end, int cursor_y) {

  int calc = (percentage * cursor_x_end * 5 / 100) - (percentage * cursor_x * 5 / 100);
  while (calc >= 5) {
    lcd.setCursor(cursor_x, cursor_y);
    lcd.write((byte)4);
    calc -= 5;
    cursor_x++;
  }
  while (calc >= 4 && calc < 5) {
    lcd.setCursor(cursor_x, cursor_y);
    lcd.write((byte)3);
    calc -= 4;

  }
  while (calc >= 3 && calc < 4) {
    lcd.setCursor(cursor_x, cursor_y);
    lcd.write((byte)2);
    calc -= 3;
  }
  while (calc >= 2 && calc < 3) {
    lcd.setCursor(cursor_x, cursor_y);
    lcd.write((byte)1);
    calc -= 2;
  }
  while (calc >= 1 && calc < 2) {
    lcd.setCursor(cursor_x, cursor_y);
    lcd.write((byte)0);
    calc -= 1;
  }

}

void initLCD()
{
  lcd.begin();

  //Create character for progress bar
  lcd.createChar(0, percentage_1);
  lcd.createChar(1, percentage_2);
  lcd.createChar(2, percentage_3);
  lcd.createChar(3, percentage_4);
  lcd.createChar(4, percentage_5);
}

// calculate sleep time
boolean isTimeToSleep()
{
  int currHour = hour();
  int currMinute = minute();
  int currLocalHour = currHour + 8;
  int onTime = 700;
  int offTime = 2200;
  if (currLocalHour >= 24) {
    currLocalHour = currLocalHour - 24;
  }
  int currTime = (currLocalHour * 100) + currMinute;

  if (currTime >= onTime && currTime <= offTime) {
    //Serial.println("Time To on");
    return false;
  } else {
    //Serial.println("Time To sleep");
    return true;
  }
}

// Pause for a 1 minute
void wait() {
  delay(60000);
}

void setup() {
  initLCD();         // Initiliase lCD
  initSerial();
  connectWiFi();
  networkTimeSync();  // Sync Clock with NTP

}
// ARDUINO entry point #1: runs once when you press reset or power the board
void loop() {

  // LCD and program will be ON from 0700hrs to 2200hrs
  if (isTimeToSleep())
  {
    lcd.noDisplay();
    lcd.noBacklight();
    return;
  }
  else
  {
    lcd.display();
    lcd.backlight();

    unsigned long currentMillis = millis();

    //LCD progress bar delay loop
    if (currentMillis - previousMillisLCD >= lcdInterval) {
      previousMillisLCD = currentMillis;
      lcd_percentage(lcdindex, 0, 20, 0);
      lcdindex = lcdindex + 1;
      if (lcdindex == 100) {
        lcdindex = 0;
        lcd.setCursor(0, 0);
        lcd.print("                    ");
      }
    }

    // main program logic delay loop
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      interval = 60000; // 60 seconds delay for next loop;
      for (int i = 0; i < 3; i++) {
        if (connect(server)) {
          if (sendRequest(resource, busArrivalTime[i].busStopNo, busArrivalTime[i].busNo) && skipResponseHeaders()) {
            char response[MAX_CONTENT_SIZE];
            readReponseContent(response, sizeof(response));
            if (parseUserData(response, i)) {
              printUserData();
              calculateArrivalTime(i);
              displayLCDArrivalTime(i);
            }
            else { //Json Parsing Failed
              setArrivalTimeError(i);
              //displayArrivalTime(i);
              displayLCDArrivalTime(i);
            }
          }
        }
        // Re-set progres bar
        if (i == 2)
        {
          lcdindex = 0;
          previousMillisLCD = 0;
          lcd.setCursor(0, 0);
          lcd.print("                    ");
        }
      }
    }
  }
}
