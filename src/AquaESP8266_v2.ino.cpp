#include <FirebaseESP8266.h>
#include <ESP8266WiFi.h>
#include <OneWire.h>
#include <DS3231.h> //Время
#include <Wire.h>
#include <ArduinoJson.h>
//#include "TimeAlarms.h"
#include <WiFiUdp.h>
#include <Ticker.h>
#include <RtcDS3231.h>

//Defines
#define DEBUG   //If you comment this line, the DPRINT & DPRINTLN lines are defined as blank.
#ifdef DEBUG    //Macros are usually in all capital letters.
#define DPRINT(...)    Serial.print(__VA_ARGS__)     //DPRINT is a macro, debug print
#define DPRINTLN(...)  Serial.println(__VA_ARGS__)   //DPRINTLN is a macro, debug print with new line
#else
#define DPRINT(...)     //now defines a blank line
#define DPRINTLN(...)   //now defines a blank line
#endif

#define FIREBASE_HOST "aqua-3006a.firebaseio.com"
#define FIREBASE_AUTH "eRxKqsNsandXnfrDtd3wjGMHMc05nUeo5yeKmuni"

#define WIFI_SSID "MikroTik"
#define WIFI_PASSWORD "11111111"
//----------------------------------------------------------------------------------


unsigned long lastTime = 0;

unsigned long lastTimeRTC = 0;
uint32_t count = 0;
unsigned long lastmillis ;
byte upM = 0, upH = 0;
int upD = 0;

//Время

DS3231 clockRTC;
RTCDateTime dt;

//NTP
//NTP
const char* WiFi_hostname = "ESP8266-0";
Ticker flipper;
boolean point;
unsigned int localPort = 2390; // local port to listen for UDP packets
// Don't hardwire the IP address or we won't get the benefits of the pool.
// Lookup the IP address for the host name instead
IPAddress timeServerIP; // time.nist.gov NTP server address
const char* ntpServerName = "pool.ntp.org";
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
#define GMT 3                       // часовой пояс, GMT+3 for Moscow Time
const long timeZoneOffset = GMT * 3600;
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
// статус синхронизации
boolean update_status = false;
byte count_sync = 0;
byte hour1 = 0;
unsigned long hour_sync = 0;
byte minute1 = 0;
unsigned long minute_sync = 0;
byte second1 = 0;

WiFiUDP udp;
RtcDS3231 rtc;

//Define FirebaseESP8266 data object
FirebaseData firebaseData;

unsigned long sendDataPrevMillis = 0;

String path = "/ESP8266_Test/Stream";

uint16_t count11111 = 0;

void uptime() {
  if ( lastmillis + 60000 < millis())
  {
    lastmillis = millis();
    upM++;
    if (upM == 60)
    {
      upM = 0;
      upH++;
    }
    if (upH == 24)
    {
      upH = 0;
      upD++;
    }
    char buf[21];
    sprintf(buf, "Uptime %d day(s) %d:%d", upD, upH, upM);
    Serial.println(buf);
  }
}
void initRTC() {
  clockRTC.begin();
  dt = clockRTC.getDateTime();
  //setTime(dt.hour, dt.minute, dt.second, dt.day, dt.month, dt.year);
  // clockRTC.setDateTime(dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second);
  Serial.println("Часы запущены. Время " + String(clockRTC.dateFormat("H:i:s Y-m-d", dt)));
}

void setup() {
  lastmillis = millis();
  lastTimeRTC = millis();
  //Запуск часов реального времени
  initRTC();

  Serial.begin(115200);
  Serial.println();
  Serial.print("connecting to WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
    Serial.println();
    uptime();
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();

Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);

  if (!Firebase.beginStream(firebaseData, path))
  {
    Serial.println("------------------------------------");
    Serial.println("Can't begin stream connection...");
    Serial.println("REASON: " + firebaseData.errorReason());
    Serial.println("------------------------------------");
    Serial.println();
  }
}

void loop() {
  //udp.begin(localPort);
  //syncTime();                  // синхронизируем время
  uptime();
  if (millis() - lastTimeRTC > 1000) {
    lastTimeRTC = millis();
    dt = clockRTC.getDateTime();
    Serial.println("RTC1 is " + String(clockRTC.dateFormat("H:i:s Y-m-d", dt)));
  }

  if (millis() - lastTime > 5000) {
    lastTime = millis();
    if ( WiFi.status() != WL_CONNECTED) {
      Serial.println("WIFi connection lost");
      Serial.println();
      return;
    }
    if (millis() - sendDataPrevMillis > 15000)
  {
    sendDataPrevMillis = millis();
    count++;

    Serial.println("------------------------------------");
    Serial.println("Set string...");
    if (Firebase.setString(firebaseData, path + "/String", "Hello World! " + String(count)))
    {
      Serial.println("PASSED");
      Serial.println("PATH: " + firebaseData.dataPath());
      Serial.println("TYPE: " + firebaseData.dataType());
      Serial.print("VALUE: ");
      if (firebaseData.dataType() == "int")
        Serial.println(firebaseData.intData());
      else if (firebaseData.dataType() == "float")
        Serial.println(firebaseData.floatData(), 5);
      else if (firebaseData.dataType() == "double")
        printf("%.9lf\n", firebaseData.doubleData());
      else if (firebaseData.dataType() == "boolean")
        Serial.println(firebaseData.boolData() == 1 ? "true" : "false");
      else if (firebaseData.dataType() == "string")
        Serial.println(firebaseData.stringData());
      else if (firebaseData.dataType() == "json")
        Serial.println(firebaseData.jsonData());
      Serial.println("------------------------------------");
      Serial.println();
    }
    else
    {
      Serial.println("FAILED");
      Serial.println("REASON: " + firebaseData.errorReason());
      Serial.println("------------------------------------");
      Serial.println();
    }

    //Pause WiFi client from all Firebase calls and use shared SSL WiFi client
    if (firebaseData.pauseFirebase(true))
    {

      WiFiClientSecure client = firebaseData.getWiFiClient();
      //Use the client to make your own http connection...
    }
    else
    {
      Serial.println("------------------------------------");
      Serial.println("Can't pause the WiFi client...");
      Serial.println("------------------------------------");
      Serial.println();
    }
    //Unpause WiFi client from Firebase task
    firebaseData.pauseFirebase(false);
  }

  if (!Firebase.readStream(firebaseData))
  {
    Serial.println("------------------------------------");
    Serial.println("Can't read stream data...");
    Serial.println("REASON: " + firebaseData.errorReason());
    Serial.println("------------------------------------");
    Serial.println();
  }

  if (firebaseData.streamTimeout())
  {
    Serial.println("Stream timeout, resume streaming...");
    Serial.println();
  }

  if (firebaseData.streamAvailable())
  {
    Serial.println("------------------------------------");
    Serial.println("Stream Data available...");
    Serial.println("STREAM PATH: " + firebaseData.streamPath());
    Serial.println("EVENT PATH: " + firebaseData.dataPath());
    Serial.println("DATA TYPE: " + firebaseData.dataType());
    Serial.println("EVENT TYPE: " + firebaseData.eventType());
    Serial.print("VALUE: ");
    if (firebaseData.dataType() == "int")
      Serial.println(firebaseData.intData());
    else if (firebaseData.dataType() == "float")
      Serial.println(firebaseData.floatData());
    else if (firebaseData.dataType() == "boolean")
      Serial.println(firebaseData.boolData() == 1 ? "true" : "false");
    else if (firebaseData.dataType() == "string")
      Serial.println(firebaseData.stringData());
    else if (firebaseData.dataType() == "json")
      Serial.println(firebaseData.jsonData());
    Serial.println("------------------------------------");
    Serial.println();
  }
    /*
    if (client.connect(FIREBASE_HOST, 443)) {
      Serial.println();
      Serial.println(">> " + String(count));
      client.print("PUT /----test----.json?auth=");
      client.print(FIREBASE_AUTH);
      client.print(" HTTP/1.1\r\n");
      client.print("Host: ");
      client.print(FIREBASE_HOST);
      client.print("\r\n");
      client.print("Connection: keep-alive\r\n");
      client.print("Keep-Alive: timeout=30, max=100\r\n");
      client.print("Content-Length: 1\r\n\r\n");
      client.print(0);
      while (client.connected() && !client.available())
        delay(1);
      if (client.connected() && client.available())
        while (client.available())
          Serial.print((char)client.read());
      Serial.println();
      Serial.println("<<");
      client.print("GET /----test----.json?auth=");
      client.print(FIREBASE_AUTH);
      client.print(" HTTP/1.1\r\n");
      client.print("Host: ");
      client.print(FIREBASE_HOST);
      client.print("\r\n");
      client.print("Connection: close\r\n\r\n");
      while (client.connected() && !client.available())
        delay(1);
      if (client.connected() && client.available())
        while (client.available())
          Serial.print((char)client.read());
      Serial.println();
    } else {
      Serial.println("Connect to Firebase failed!");
    }
    */
  }

}
//-----------------------------------------------------------------------------------------------
// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress& address)
{
  //  Serial.println("sending NTP packet...");
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
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

// Синхронизация времени
void syncTime()
{
  //get a random server from the pool
  WiFi.hostByName(ntpServerName, timeServerIP);
  sendNTPpacket(timeServerIP); // send an NTP packet to a time server
  // wait to see if a reply is available - вот она 1 секунда потеряшка
  delay(1000);
  int cb = udp.parsePacket();
  if (!cb) {
    // ответ не получен или ещё что
    DPRINTLN("Нет ответа от сервера времени");
    update_status = false;
    count_sync++;
    minute_sync = rtc.getEpoch();

  }
  else {
    // разбираем полученное время и пишем в DS3231
    update_status = true;
    count_sync = 0;
    hour_sync = rtc.getEpoch();


    DPRINTLN("Получен ответ от сервера");
    DPRINTLN(String(cb));

    // We've received a packet, read the data from it
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:
    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    // now convert NTP time into everyday time:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears;
    // корректировка часового пояса и синхронизация, epoch - местное время
    // 2 секунды разница с большим братом
    epoch = epoch + 2 + GMT * 3600;
    hour1 = (epoch  % 86400L) / 3600;
    minute1 = (epoch  % 3600) / 60;
    second1 = epoch % 60;

    // print the hour, minute and second:
    Serial.print("2-The UTC time is = ");       // UTC is the time at Greenwich Meridian (GMT)
    // hour
    Serial.print(hour1); // print the hour (86400 equals secs per day)
    Serial.print(':');
    // minutes
    if ( minute1 < 10 ) {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.print(minute1); // print the minute (3600 equals secs per minute)
    Serial.print(':');
    // second
    if ( second1 < 10 ) {
      // In the first 10 seconds of each minute, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.println(second1); // print the second

    // RTS date and time
    char str[20];
    rtc.dateTimeToStr(str);

    DPRINTLN("2-The RTC date and time is ");
    DPRINTLN(str);

    // RTS
    uint32_t rtcEpoch = rtc.getEpoch();

    Serial.print("2-The RTC epoch is = ");
    Serial.println(rtcEpoch);
    // NTP
    Serial.print("2-The NTP epoch is = ");
    Serial.println(epoch);

    if (abs(rtcEpoch - epoch) > 2) {

      Serial.print("2-Updating RTC (epoch difference is = ");
      if ((rtcEpoch - epoch) > 10000) {
        Serial.print(abs(epoch - rtcEpoch));
      } else {
        Serial.print(abs(rtcEpoch - epoch));
      }
      Serial.println(')');

      rtc.setEpoch(epoch);
    } else {

      Serial.println("1-RTC date and time are synchronized.");

    }
  }
}


