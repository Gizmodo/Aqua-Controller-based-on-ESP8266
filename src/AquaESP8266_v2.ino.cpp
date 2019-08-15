#include <ArduinoJson.h>
#include <DS3231.h>  //Время
#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <OneWire.h>
#include <Wire.h>

//#include "TimeAlarms.h"
#include <RtcDS3231.h>
#include <Ticker.h>
#include <WiFiUdp.h>
#include <uEEPROMLib.h>

#define FIREBASE_HOST "aqua-3006a.firebaseio.com"
#define FIREBASE_AUTH "eRxKqsNsandXnfrDtd3wjGMHMc05nUeo5yeKmuni"

#define WIFI_SSID "MikroTik"
#define WIFI_PASSWORD "11111111"

#define GMT 3

#define ONE_WIRE_BUS 14  //Пин, к которому подключены датчики DS18B20 D5 GPIO15
//----------------------------------------------------------------------------------
uint8_t wifiMaxTry = 5;  //Попытки подключения к сети
uint8_t wifiConnectCount = 0;

// uEEPROMLib eeprom;
uEEPROMLib eeprom(0x57);
unsigned int pos;

unsigned long lastTime = 0;

unsigned long lastTimeRTC = 0;
uint32_t count = 0;
unsigned long lastmillis;
byte upM = 0, upH = 0;
int upD = 0;

DS3231 clockRTC;
RTCDateTime dt;
RtcDS3231 rtc;

const char* WiFi_hostname = "ESP8266-0";
IPAddress timeServerIP;
const char* ntpServerName = "pool.ntp.org";
const int NTP_PACKET_SIZE = 48;

const long timeZoneOffset = GMT * 3600;
byte packetBuffer[NTP_PACKET_SIZE];
boolean update_status = false;
byte count_sync = 0;
byte hour1 = 0;
unsigned long hour_sync = 0;
byte minute1 = 0;
unsigned long minute_sync = 0;
byte second1 = 0;

WiFiUDP udp;

FirebaseData firebaseData;

unsigned long sendDataPrevMillis = 0;

String path = "/ESP8266_Test/Stream";

OneWire ds(ONE_WIRE_BUS);
byte data[12];
float temp1, temp2;
//Адреса датчиков
byte addr1[8] = {0x28, 0xFF, 0x17, 0xF0, 0x8B, 0x16, 0x03, 0x13};  //адрес датчика DS18B20
byte addr2[8] = {0x28, 0xFF, 0x5F, 0x1E, 0x8C, 0x16, 0x03, 0xE2};  //адрес датчика DS18B20

float DS18B20(byte* adres) {
    unsigned int raw;
    ds.reset();
    ds.select(adres);
    ds.write(0x44, 1);
    ds.reset();
    ds.select(adres);
    ds.write(0xBE);
    for (byte i = 0; i < 9; i++) {
        data[i] = ds.read();
    }
    raw = (data[1] << 8) | data[0];
    float celsius = (float)raw / 16.0;
    return celsius;
}

void getTemperature() {
    temp1 = DS18B20(addr1);
    temp2 = DS18B20(addr2);
    Serial.println(temp1);
    Serial.println(temp2);
}

void uptime() {
    if (lastmillis + 60000 < millis()) {
        lastmillis = millis();
        upM++;
        if (upM == 60) {
            upM = 0;
            upH++;
        }
        if (upH == 24) {
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
    Serial.println("Часы запущены. Время " + String(clockRTC.dateFormat("H:i:s Y-m-d", dt)));
}
void eeprom_test() {
    delay(2000);

    Serial.println("Serial OK");

    delay(2500);
    Serial.println("Delay OK");

#ifdef ARDUINO_ARCH_ESP8266
    Wire.begin();  // D3 and D4 on ESP8266
#else
    Wire.begin();
#endif

#ifdef ARDUINO_ARCH_AVR
    int inttmp = 32123;
#else
    // too logng for AVR 16 bits!
    int inttmp = 24543557;
#endif
    float floattmp = 3.1416;
    char chartmp = 'A';

    char string[17] = "ForoElectro.Net\0";

    // Testing template
    if (!eeprom.eeprom_write(0, inttmp)) {
        Serial.println("Failed to store INT");
    } else {
        Serial.println("INT correctly stored");
    }
    if (!eeprom.eeprom_write(4, floattmp)) {
        Serial.println("Failed to store FLOAT");
    } else {
        Serial.println("FLOAT correctly stored");
    }
    if (!eeprom.eeprom_write(8, chartmp)) {
        Serial.println("Failed to store CHAR");
    } else {
        Serial.println("CHAR correctly stored");
    }

    if (!eeprom.eeprom_write(9, (byte*)&string[0], 16)) {
        Serial.println("Failed to store STRING");
    } else {
        Serial.println("STRING correctly stored");
    }

    inttmp = 0;
    floattmp = 0;
    chartmp = 0;
    string[0] = string[1] = string[2] = string[3] = string[4] = 0;

    Serial.print("int: ");
    eeprom.eeprom_read(0, &inttmp);
    Serial.println(inttmp);

    Serial.print("float: ");
    eeprom.eeprom_read(4, &floattmp);
    Serial.println((float)floattmp);

    Serial.print("char: ");
    eeprom.eeprom_read(8, &chartmp);
    Serial.println(chartmp);

    Serial.print("chararray: ");
    eeprom.eeprom_read(9, (byte*)&string[0], 16);
    Serial.println(string);

    Serial.println();

    for (pos = 26; pos < 1000; pos++) {
        eeprom.eeprom_write(pos, (unsigned char)(pos % 256));
    }

    pos = 0;
}

unsigned long sendNTPpacket(IPAddress& address) {
    memset(packetBuffer, 0, NTP_PACKET_SIZE);
    packetBuffer[0] = 0b11100011;
    packetBuffer[1] = 0;
    packetBuffer[2] = 6;
    packetBuffer[3] = 0xEC;
    packetBuffer[12] = 49;
    packetBuffer[13] = 0x4E;
    packetBuffer[14] = 49;
    packetBuffer[15] = 52;

    udp.beginPacket(address, 123);
    udp.write(packetBuffer, NTP_PACKET_SIZE);
    udp.endPacket();
}

// Синхронизация времени
void syncTime() {
    udp.begin(2390);
    WiFi.hostByName(ntpServerName, timeServerIP);
    sendNTPpacket(timeServerIP);
    delay(1000);
    int cb = udp.parsePacket();
    if (!cb) {
        Serial.printf_P(PSTR("Нет ответа от сервера времени %s\n"), ntpServerName);
        update_status = false;
        count_sync++;
        minute_sync = rtc.getEpoch();
    } else {
        update_status = true;
        count_sync = 0;
        hour_sync = rtc.getEpoch();
        Serial.printf_P(PSTR("Получен ответ от сервера времени %s\n"), ntpServerName);
        udp.read(packetBuffer, NTP_PACKET_SIZE);
        unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
        unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
        unsigned long secsSince1900 = highWord << 16 | lowWord;
        const unsigned long seventyYears = 2208988800UL;
        unsigned long epoch = secsSince1900 - seventyYears;
        // 2 секунды разница с большим братом
        epoch = epoch + 2 + GMT * 3600;
        hour1 = (epoch % 86400L) / 3600;
        minute1 = (epoch % 3600) / 60;
        second1 = epoch % 60;

        char str[20];
        rtc.dateTimeToStr(str);
        Serial.printf_P(PSTR("%s\n"), str);
        uint32_t rtcEpoch = rtc.getEpoch();
        Serial.printf_P(PSTR("RTC эпоха: %d\n"), rtcEpoch);
        Serial.printf_P(PSTR("NTP эпоха: %d\n"), epoch);

        if (abs(rtcEpoch - epoch) > 2) {
            Serial.printf_P(PSTR("Обновляем RTC (разница между эпохами = "));
            if ((rtcEpoch - epoch) > 10000) {
                Serial.printf_P(PSTR("%s\n"), abs(epoch - rtcEpoch));
            } else {
                Serial.printf_P(PSTR("%s\n"), abs(rtcEpoch - epoch));
            }
            rtc.setEpoch(epoch);
        } else {
            Serial.printf_P(PSTR("Дата и время RTC не требуют синхронизации\n"));
        }
    }
}

void readOptionsEEPROM() {
    Serial.printf_P(PSTR("Загрузка настроек из внутренней памяти\n"));
    // todo
};

void readOptionsFirebase() {
    Serial.printf_P(PSTR("Загрузка настроек из Firebase\n"));
    // todo
};

void DS18B201() {
    Serial.printf_P(PSTR("Считывание показаний с датчиков температуры\n"));
    // todo
}

void setup() {
    Serial.begin(115200);
    Serial.println();

    //Запуск часов реального времени
    initRTC();

    DS18B201();
    lastmillis = millis();
    lastTimeRTC = millis();

    // eeprom_test();
    Serial.printf_P(PSTR("Подключение к WiFi: %s\n"), WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while ((WiFi.status() != WL_CONNECTED) && (wifiConnectCount != wifiMaxTry)) {
        Serial.printf_P(PSTR("."));
        delay(1000);
        Serial.printf_P(PSTR("\n"));
        wifiConnectCount++;
    }

    if (WiFi.status() != WL_CONNECTED) {
        wifiConnectCount = 5;
        Serial.printf_P(PSTR("Не удалось подключиться к WiFi: %s"), WIFI_SSID);
        readOptionsEEPROM();
    } else {
        Serial.printf_P(PSTR("Успешное подключение к WiFi: %s\n"), WIFI_SSID);
        Serial.println("IP адрес: " + WiFi.localIP().toString() + "\n");

        Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
        Firebase.reconnectWiFi(true);

        syncTime();  // синхронизируем время

        readOptionsFirebase();
        /*
                if (!Firebase.beginStream(firebaseData, path)) {
                    Serial.println("------------------------------------");
                    Serial.println("Can't begin stream connection...");
                    Serial.println("REASON: " + firebaseData.errorReason());
                    Serial.println("------------------------------------");
                    Serial.println();
                }
          */
    }
}

void writeTemperature() {
    if (WiFi.isConnected()) {
    } else {
    }
}
void loop() {
    uptime();
    if (millis() - lastTimeRTC > 1000) {
        lastTimeRTC = millis();
        dt = clockRTC.getDateTime();
        Serial.println(String(clockRTC.dateFormat("H:i:s Y-m-d", dt)));
        getTemperature();
        writeTemperature();
    }

    if (millis() - lastTime > 5000) {
        lastTime = millis();
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("WIFi connection lost");
            Serial.println();
            return;
        }
        if (millis() - sendDataPrevMillis > 15000) {
            sendDataPrevMillis = millis();
            count++;

            Serial.println("------------------------------------");
            Serial.println("Set string...");
            if (Firebase.setString(firebaseData, path + "/String", "Hello World! " + String(count))) {
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
            } else {
                Serial.println("FAILED");
                Serial.println("REASON: " + firebaseData.errorReason());
                Serial.println("------------------------------------");
                Serial.println();
            }

            // Pause WiFi client from all Firebase calls and use shared SSL WiFi
            // client
            if (firebaseData.pauseFirebase(true)) {
                WiFiClientSecure client = firebaseData.getWiFiClient();
                // Use the client to make your own http connection...
            } else {
                Serial.println("------------------------------------");
                Serial.println("Can't pause the WiFi client...");
                Serial.println("------------------------------------");
                Serial.println();
            }
            // Unpause WiFi client from Firebase task
            firebaseData.pauseFirebase(false);
        }

        if (!Firebase.readStream(firebaseData)) {
            Serial.println("------------------------------------");
            Serial.println("Can't read stream data...");
            Serial.println("REASON: " + firebaseData.errorReason());
            Serial.println("------------------------------------");
            Serial.println();
        }

        if (firebaseData.streamTimeout()) {
            Serial.println("Stream timeout, resume streaming...");
            Serial.println();
        }

        if (firebaseData.streamAvailable()) {
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
