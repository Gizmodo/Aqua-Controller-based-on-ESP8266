#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <OneWire.h>
#include <WiFiUdp.h>
#include "DS3231.h"  //Время
#include "RtcDS3231.h"
#include "TimeAlarms.h"
#include "uEEPROMLib.h"

#define FIREBASE_HOST "aqua-3006a.firebaseio.com"
#define FIREBASE_AUTH "eRxKqsNsandXnfrDtd3wjGMHMc05nUeo5yeKmuni"

#define WIFI_SSID "MikroTik"
#define WIFI_PASSWORD1 "123"
#define WIFI_PASSWORD "11111111"

#define GMT 3

#define ONE_WIRE_BUS 14  //Пин, к которому подключены датчики DS18B20 D5 GPIO14

#define ALARMS_COUNT 6  //Количество таймеров, которые нужно удалять. Помимо их есть еще два основных - каждую минуту и каждые пять.
//Указанное кол-во надо увеличить в случае появления нового расписания для нового устройства, например, дозаторы, CO2, нагреватель и
//прочие устройства которые будут запланированы на включение или выключение
//----------------------------------------------------------------------------------
uint8_t wifiMaxTry = 10;  //Попытки подключения к сети
uint8_t wifiConnectCount = 0;

// uEEPROMLib eeprom;
uEEPROMLib eeprom(0x57);
unsigned int pos;
const uint16_t StartAddress = 16;

uint32_t count = 0;

byte upM = 0, upH = 0;
int upD = 0;

DS3231 clockRTC;
RTCDateTime dt;
RtcDS3231 rtc;
bool shouldUpdateFlag = false;
const char* WiFi_hostname = "ESP8266";
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

unsigned long currentMillis = 1UL;
unsigned long previousMillis = 1UL;

String pathLight = "Light/";
String pathTemperatureOnline = "Temperature/Online/";
String pathTemperatureHistory = "Temperature/History/";

String pathUpdateSettings = "UpdateSettings";
String pathToLastOnline = "LastOnline";
String pathToUptime = "Uptime";

OneWire ds(ONE_WIRE_BUS);
byte data[12];
float temp1, temp2;

byte addr1[8] = {0x28, 0xFF, 0x17, 0xF0, 0x8B, 0x16, 0x03, 0x13};  //адрес датчика DS18B20
byte addr2[8] = {0x28, 0xFF, 0x5F, 0x1E, 0x8C, 0x16, 0x03, 0xE2};  //адрес датчика DS18B20

enum ledPosition { LEFT, CENTER, RIGHT };

struct ledStruct_t {
    byte HOn, HOff;
    byte MOn, MOff;
    byte enabled;
    byte currentState;
    AlarmId on, off;
    byte pin;
} ledStruct;

struct ledDescription_t {
    ledPosition position;
    String name;
    ledStruct_t led;
} ledDescription;

ledDescription_t leds[3];
ledDescription_t led_test;

std::vector<String> splitStringToVector(const String& msg) {
    std::vector<String> subStrings;
    uint32_t j = 0;
    for (uint32_t i = 0; i < msg.length(); i++) {
        if (msg.charAt(i) == ':') {
            subStrings.push_back(msg.substring(j, i));
            j = i + 1;
        }
    }
    subStrings.push_back(msg.substring(j, msg.length()));  // to grab the last value of the string
    return subStrings;
}

void checkUpdateSettings();
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
void initLeds() {
    leds[0].name = "Left";
    leds[0].position = LEFT;
    leds[1].name = "Center";
    leds[1].position = CENTER;
    leds[2].name = "Right";
    leds[2].position = RIGHT;
}
void getTemperature() {
    temp1 = DS18B20(addr1);
    temp2 = DS18B20(addr2);
    Serial.printf_P(PSTR("Датчик температуры 1: %s\n"), String(temp1).c_str());
    Serial.printf_P(PSTR("Датчик температуры 2: %s\n"), String(temp2).c_str());
}
void clearAlarms() {
    uint8_t ledsCount(sizeof(leds) / sizeof(*leds));
    for (size_t i = 0; i < ledsCount; i++) {
        Alarm.disable(leds[i].led.off);
        Alarm.disable(leds[i].led.on);
        Alarm.free(leds[i].led.off);
        Alarm.free(leds[i].led.on);
    }
}

void uptime() {
    FirebaseData data;
    char buffer[32];
    upM++;
    if (upM == 60) {
        upM = 0;
        upH++;
    }
    if (upH == 24) {
        upH = 0;
        upD++;
    }
    sprintf_P(buffer, PSTR("Uptime %d дн. %02d:%02d"), upD, upH, upM);
    Serial.println(buffer);
    if (!Firebase.setString(data, pathToUptime, buffer)) {
        Serial.printf_P(PSTR("\nОшибка uptime: %s\n"), data.errorReason().c_str());
    };
}
void initRTC() {
    clockRTC.begin();
    dt = clockRTC.getDateTime();
    Serial.println("Часы запущены. Время " + String(clockRTC.dateFormat("H:i:s Y-m-d", dt)));
}
/*
void eeprom_test() {
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
    float floattmp = 2.4687;
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
*/
void sendNTPpacket(IPAddress& address) {
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
    Serial.printf_P(PSTR("Синхронизация времени\n"), ntpServerName);
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
            Serial.printf_P(PSTR("%s"), "Обновляем RTC (разница между эпохами = ");
            if ((rtcEpoch - epoch) > 10000) {
                Serial.printf_P(PSTR("%s\n"), abs(epoch - rtcEpoch));
            } else {
                Serial.printf_P(PSTR("%s\n"), abs(rtcEpoch - epoch));
            }
            rtc.setEpoch(epoch);
        } else {
            Serial.printf_P(PSTR("%s\n"), "Дата и время RTC не требуют синхронизации");
        }
    }
}

void ledOnHandler(byte pin) {
}
void ledOffHandler(byte pin) {
}

void printLEDTime(ledPosition position) {
    uint8_t ledsCount(sizeof(leds) / sizeof(*leds));
    uint8_t index;
    bool found = false;
    for (size_t i = 0; i < ledsCount; i++) {
        if (leds[i].position == position) {
            index = i;
            i = ledsCount;
            found = true;
        }
    }
    if (!found) {
        Serial.println("Прожектор не найден!!!");
    } else {
        Serial.printf_P(PSTR("Вкл-%02d:%02d. Выкл-%02d:%02d. Состояние: %s. Разрешен: %s. PIN: %d\n"),
                        leds[index].led.HOn, leds[index].led.MOn, leds[index].led.HOff, leds[index].led.MOff,
                        ((leds[index].led.currentState == true) ? "включен" : "выключен"),
                        ((leds[index].led.enabled == true) ? "да" : "нет"), leds[index].led.pin

        );
    }
}

void printAllLedsTime() {
    Serial.printf_P(PSTR("%s\n"), "Отображение текущих настроек всех прожекторов");
    printLEDTime(LEFT);
    printLEDTime(CENTER);
    printLEDTime(RIGHT);
}
//Загрузка времени включения/выключения прожектора
void setLEDTime(ledPosition position) {
    FirebaseData firebaseData;
    FirebaseJson json;

    uint8_t ledsCount = (sizeof(leds) / sizeof(*leds));
    String ledPath;
    uint8_t index;
    bool found = false;
    for (size_t i = 0; i < ledsCount; i++) {
        if (leds[i].position == position) {
            ledPath = leds[i].name;
            index = i;
            found = true;
            i = ledsCount;
        }
    }
    if (found) {
        if (Firebase.getJSON(firebaseData, pathLight + ledPath)) {
            if (firebaseData.dataType() == "json") {
                String jsonData = "";
                FirebaseJsonObject jsonParseResult;
                jsonData = firebaseData.jsonData();
                json.clear();
                json.setJsonData(jsonData);
                json.parse();
                size_t count = json.getJsonObjectIteratorCount();
                String key, value;
                std::vector<String> vectorString;
                for (size_t i = 0; i < count; i++) {
                    json.jsonObjectiterator(i, key, value);
                    jsonParseResult = json.parseResult();

                    if (key == "enabled") {
                        leds[index].led.enabled = jsonParseResult.boolValue;
                    }
                    if (key == "off") {
                        vectorString.clear();
                        vectorString = splitStringToVector(value);
                        leds[index].led.HOff = vectorString[0].toInt();
                        leds[index].led.MOff = vectorString[1].toInt();
                    }
                    if (key == "on") {
                        vectorString.clear();
                        vectorString = splitStringToVector(value);
                        leds[index].led.HOn = vectorString[0].toInt();
                        leds[index].led.MOn = vectorString[1].toInt();
                    }
                    if (key == "pin") {
                        leds[index].led.pin = jsonParseResult.intValue;
                    }
                    if (key == "state") {
                        leds[index].led.currentState = jsonParseResult.boolValue;
                    }
                }
                leds[index].led.off =
                    Alarm.alarmRepeat(leds[index].led.HOff, leds[index].led.MOff, 0, ledOffHandler, leds[index].led.pin);
                leds[index].led.on =
                    Alarm.alarmRepeat(leds[index].led.HOn, leds[index].led.MOn, 0, ledOnHandler, leds[index].led.pin);
            } else {
                Serial.println("Reply isn't JSON object!");
            }
        } else {
            Serial.println("Firebase failed to get JSON");
        }
    } else {
        Serial.println("Not found led!!!");
    }
    json.clear();
}
uint16_t ledAddress(const uint8_t num) {
    return StartAddress + num * sizeof(ledDescription);
}
void writeEEPROMLed() {
    uint8_t ledsCount(sizeof(leds) / sizeof(*leds));
    for (size_t i = 0; i < ledsCount; i++) {
        if (eeprom.eeprom_write(ledAddress(i), leds[i])) {
            Serial.println("Параметры по прожектору сохранены в EEPROM");
        } else {
            Serial.println("Ошибка сохранения параметров прожектора в EEPROM");
        }
    }
}
void readOptionsEEPROM() {
    Serial.printf_P(PSTR("%s\n"), "Загрузка настроек из EEPROM");
    uint8_t ledsCount(sizeof(leds) / sizeof(*leds));
    for (size_t i = 0; i < ledsCount; i++) {
        eeprom.eeprom_read(ledAddress(i), &led_test);
        leds[i] = led_test;
    }
    printAllLedsTime();
};
void readOptionsFirebase() {
    Serial.printf_P(PSTR("%s\n"), "Загрузка из Firebase");
    setLEDTime(LEFT);
    setLEDTime(CENTER);
    setLEDTime(RIGHT);
    Serial.printf_P(PSTR("%s\n"), "Запись в EEPROM");
    writeEEPROMLed();
};
//Сохранение показаний датчиков температуры
void writeTemperatureFirebase() {
    if (WiFi.isConnected()) {
        FirebaseData firebaseData;
        FirebaseJson json;
        Serial.printf_P(PSTR("%s"), "Сохраняем в Firebase текущее показание температурных датчиков: ");
        json.addDouble("temp1", temp1)
            .addDouble("temp2", temp2)
            .addString("DateTime", String(clockRTC.dateFormat("H:i:s d.m.Y", dt)));

        if (Firebase.setJSON(firebaseData, pathTemperatureOnline, json)) {
            Serial.printf_P(PSTR("%s\n"), "Успешно");
        } else {
            Serial.printf_P(PSTR("\nОшибка writeTemperatureFirebase: %s\n"), firebaseData.errorReason().c_str());
        }

        Serial.printf_P(PSTR("%s"), "Сохраняем в журнал Firebase текущее показание температурных датчиков: ");
        String deviceDateKey = clockRTC.dateFormat("Y-m-d", clockRTC.getDateTime());
        json.clear();
        json.addDouble("temp1", temp1)
            .addDouble("temp2", temp2)
            .addString("DateTime", String(clockRTC.dateFormat("H:i:s", clockRTC.getDateTime())));

        if (Firebase.pushJSON(firebaseData, pathTemperatureHistory + deviceDateKey, json)) {
            Serial.printf_P(PSTR("%s\n"), "Успешно");
        } else {
            Serial.printf_P(PSTR("\n%s %s\n"), "Ошибка:", firebaseData.errorReason().c_str());
        }
    }
}

void setClock() {
    configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
    time_t now = time(nullptr);
    uint8_t tryCount = 0;
    while (now < 8 * 3600 * 2) {
        delay(100);
        now = time(nullptr);
        tryCount++;
        if (tryCount > 50)
            break;
    }
    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);
}
void lastOnline() {
    FirebaseData data;
    Firebase.setString(data, pathToLastOnline, String(clockRTC.dateFormat("H:i:s d.m.Y", clockRTC.getDateTime())));
}
void Timer5Min() {
    uptime();
    lastOnline();
    getTemperature();
    writeTemperatureFirebase();
}

void Timer1Min() {
    Serial.println(String(clockRTC.dateFormat("H:i:s d.m.Y", clockRTC.getDateTime())));
    getTemperature();
    checkUpdateSettings();
    if (shouldUpdateFlag) {
        readOptionsFirebase();
        printAllLedsTime();
        shouldUpdateFlag = false;
    }
}
void startMainTimers() {
    Serial.printf_P(PSTR("%s: %d\n"), "Количество таймеров до", Alarm.count());
    Alarm.timerRepeat(5 * 60, Timer5Min);  // сохраняем температуру в Firebase/EEPROM
    Alarm.timerRepeat(20, Timer1Min);      // вывод uptime и тмемпературу каждую минуту
    Serial.printf_P(PSTR("%s: %d\n"), "Количество таймеров после", Alarm.count());
}
void checkUpdateSettings() {
    FirebaseData data;
    if (Firebase.getBool(data, pathUpdateSettings)) {
        if (data.dataType() == "boolean") {
            if (data.boolData()) {
                Serial.printf_P(PSTR("%s\n"), "Запрос на обновление всех настроек!!!");
                shouldUpdateFlag = true;
                clearAlarms();
                if (!Firebase.setBool(data, pathUpdateSettings, false)) {
                    Serial.printf_P(PSTR("%s: %s\n"), "Не удалось вернуть флаг UpdateSettings", data.errorReason().c_str());
                }
            }
        } else {
            Serial.printf_P(PSTR("%s\n"), "UpdateSettings не boolean");
        }
    } else {
        Serial.printf_P(PSTR("Ошибка checkUpdateSettings: %s\n"), data.errorReason().c_str());
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println();

    //Запуск часов реального времени
    initRTC();
    initLeds();
    getTemperature();

    Serial.printf_P(PSTR("%s: %s\n"), "Подключение к WiFi", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    WiFi.setAutoReconnect(true);
    while ((WiFi.status() != WL_CONNECTED) && (wifiConnectCount != wifiMaxTry)) {
        Serial.printf_P(PSTR("%s"), ".");
        delay(500);
        wifiConnectCount++;
    }

    if (WiFi.status() != WL_CONNECTED) {
        wifiConnectCount = 0;
        Serial.printf_P(PSTR("%s: %s\n"), "Не удалось подключиться к WiFi", WIFI_SSID);
        readOptionsEEPROM();
    } else {
        Serial.printf_P(PSTR("%s: %s IP:%s\n"), "Успешное подключение к WiFi", WIFI_SSID, WiFi.localIP().toString().c_str());
        WiFi.hostname(WiFi_hostname);
        setClock();
        Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
        Firebase.reconnectWiFi(true);
        syncTime();  // синхронизируем время
        readOptionsFirebase();
        printAllLedsTime();
    }
    startMainTimers();
    Timer5Min();
}
void prWiFiStatus(int s) {
#define VALCASE(x)                    \
    case x:                           \
        Serial.print("WiFi.status "); \
        Serial.println(#x);           \
        break;

    switch (s) {
        VALCASE(WL_NO_SHIELD);
        VALCASE(WL_IDLE_STATUS);
        VALCASE(WL_NO_SSID_AVAIL);
        VALCASE(WL_SCAN_COMPLETED);
        VALCASE(WL_CONNECTED);
        VALCASE(WL_CONNECT_FAILED);
        VALCASE(WL_CONNECTION_LOST);
        VALCASE(WL_DISCONNECTED);
        default:
            Serial.println(s);
            break;
    }
}
void loop() {
    Alarm.delay(10);
    currentMillis = millis();

    if (currentMillis - previousMillis > 60000UL) {
        previousMillis = currentMillis;
        prWiFiStatus(WiFi.status());
    }
}
