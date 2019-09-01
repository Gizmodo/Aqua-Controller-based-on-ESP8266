#include <DS3231.h>  //Время
#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <OneWire.h>
#include <Wire.h>

#include <RtcDS3231.h>
#include <Ticker.h>
#include <WiFiUdp.h>
#include <uEEPROMLib.h>
#include "EEPROMAnything.h"
#include "TimeAlarms.h"

#define FIREBASE_HOST "aqua-3006a.firebaseio.com"
#define FIREBASE_AUTH "eRxKqsNsandXnfrDtd3wjGMHMc05nUeo5yeKmuni"

#define WIFI_SSID "MikroTik"
#define WIFI_PASSWORD "11111111"

#define GMT 3

#define ONE_WIRE_BUS 14  //Пин, к которому подключены датчики DS18B20 D5 GPIO15

#define ALARMS_COUNT 6  //Количество таймеров, которые нужно удалять. Помимо их есть еще два основных - каждую минуту и каждые пять.
//Указанное кол-во надо увеличить в случае появления нового расписания для нового устройства, например, дозаторы, CO2, нагреватель и
//прочие устройства которые будут запланированы на включение или выключение
//----------------------------------------------------------------------------------
uint8_t wifiMaxTry = 5;  //Попытки подключения к сети
uint8_t wifiConnectCount = 0;

// uEEPROMLib eeprom;
uEEPROMLib eeprom(0x57);
unsigned int pos;

uint32_t count = 0;
unsigned long lastmillis;
byte upM = 0, upH = 0;
int upD = 0;

DS3231 clockRTC;
RTCDateTime dt;
RtcDS3231 rtc;

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

unsigned long sendDataPrevMillis = 0;

String pathLight = "Light/";
String pathTemperatureOnline = "Temperature/Online/";
String pathTemperatureHistory = "Temperature/History/";

String pathUpdateSettings = "UpdateSettings";

OneWire ds(ONE_WIRE_BUS);
byte data[12];
float temp1, temp2;

byte addr1[8] = {0x28, 0xFF, 0x17, 0xF0, 0x8B, 0x16, 0x03, 0x13};  //адрес датчика DS18B20
byte addr2[8] = {0x28, 0xFF, 0x5F, 0x1E, 0x8C, 0x16, 0x03, 0xE2};  //адрес датчика DS18B20

enum ledPosition { LEFT, CENTER, RIGHT };

struct ledStruct {
    uint8_t HOn, HOff;
    uint8_t MOn, MOff;
    bool enabled;
    bool currentState;
    // ledPosition position;
    AlarmId on, off;
    uint8_t pin;
    String russianName;
};
typedef struct {
    ledPosition position;
    String name;
    ledStruct led;
} ledDescription;
ledDescription leds[3];
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
    uint8_t countAlarms = Alarm.count();
    for (size_t i = 0; i < countAlarms; i++) {
        Alarm.disable(i);
        Alarm.free(i);
    }
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
        Serial.printf_P(PSTR("Uptime %d дн. %02d:%02d\n"), upD, upH, upM);
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
        Serial.println("Not found led!!!");
    } else {
        Serial.printf_P(PSTR("Прожектор: %s. Вкл-%02d:%02d. Выкл-%02d:%02d. Состояние: %s. Доступен к использованию: %s\n"),
                        String(leds[index].led.russianName).c_str(), leds[index].led.HOn, leds[index].led.MOn, leds[index].led.HOff,
                        leds[index].led.MOff, ((leds[index].led.currentState == true) ? "включен" : "выключен"),
                        ((leds[index].led.enabled == true) ? "да" : "нет")

        );
    }
}
void printAllLedsTime() {
    Serial.printf_P(PSTR("Отображение текущих настроек всех прожекторов\n"));
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
                Serial.println(jsonData);
                json.clear();
                json.setJsonData(jsonData);
                json.parse();
                size_t count = json.getJsonObjectIteratorCount();
                Serial.printf_P(PSTR("Количество объектов: %d\n"), count);
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
                    if (key == "position") {
                        leds[index].led.russianName = jsonParseResult.stringValue;
                    }
                    if (key == "state") {
                        leds[index].led.currentState = jsonParseResult.boolValue;
                    }
                    Serial.printf_P(PSTR("Key: %s, value:%s, type:%s\n"), key.c_str(), value.c_str(), jsonParseResult.type.c_str());
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
}
void readOptionsFirebase() {
    Serial.printf_P(PSTR("Загрузка настроек из Firebase\n"));
    setLEDTime(LEFT);
    setLEDTime(CENTER);
    setLEDTime(RIGHT);
};
//Сохранение показаний датчиков температуры
void writeOnlineTemperature() {
    if (WiFi.isConnected()) {
        FirebaseData firebaseData;
        FirebaseJson json;
        Serial.printf_P(PSTR("Сохраняем в Firebase текущее показание температурных датчиков: "));
        json.addDouble("temp1", temp1)
            .addDouble("temp2", temp2)
            .addString("DateTime", String(clockRTC.dateFormat("H:i:s d.m.Y", dt)));

        if (Firebase.setJSON(firebaseData, pathTemperatureOnline, json)) {
            Serial.printf_P(PSTR("Успешно\n"));
        } else {
            Serial.printf_P(PSTR("\nОшибка: %s\n"), firebaseData.errorReason().c_str());
        }

        Serial.printf_P(PSTR("Сохраняем в журнал Firebase текущее показание температурных датчиков: "));
        String deviceDateKey = clockRTC.dateFormat("Y-m-d", clockRTC.getDateTime());
        json.clear();
        json.addDouble("temp1", temp1)
            .addDouble("temp2", temp2)
            .addString("DateTime", String(clockRTC.dateFormat("H:i:s", clockRTC.getDateTime())));

        if (Firebase.pushJSON(firebaseData, pathTemperatureHistory + deviceDateKey, json)) {
            Serial.printf_P(PSTR("Успешно\n"));
        } else {
            Serial.printf_P(PSTR("\nОшибка: %s\n"), firebaseData.errorReason().c_str());
        }
    } else {
        Serial.printf_P(PSTR("Сохраняем в EEPROM\n"));
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

void fiveMinuteTimer() {
    getTemperature();
    writeOnlineTemperature();
}
void oneMinuteTimer() {
    Serial.println(String(clockRTC.dateFormat("H:i:s d.m.Y", clockRTC.getDateTime())));
    uptime();
    getTemperature();
    checkUpdateSettings();
}
void startMainTimers() {
    Serial.printf_P(PSTR("Количество таймеров до: %d\n"), Alarm.count());
    Alarm.timerRepeat(5 * 60, fiveMinuteTimer);  // сохраняем температуру в Firebase/EEPROM
    Alarm.timerRepeat(60, oneMinuteTimer);       // вывод uptime и тмемпературу каждую минуту
    Serial.printf_P(PSTR("Количество таймеров после: %d\n"), Alarm.count());
}
void checkUpdateSettings() {
    FirebaseData data;
    if (Firebase.getBool(data, pathUpdateSettings)) {
        if (data.dataType() == "boolean") {
            if (data.boolData()) {
                Serial.printf_P(PSTR("Запрос на обновление всех настроек!!!\n"));
                clearAlarms();
                readOptionsFirebase();
                //  startMainTimers();
                if (!Firebase.setBool(data, pathUpdateSettings, false)) {
                    Serial.printf_P(PSTR("Не удалось вернуть флаг UpdateSettings: %s\n"), data.errorReason().c_str());
                }
            }
        } else {
            Serial.printf_P(PSTR("UpdateSettings не boolean\n"));
        }
    } else {
        Serial.printf_P(PSTR("Ошибка: %s\n"), data.errorReason().c_str());
    }
}
void setup() {
    Serial.begin(115200);
    Serial.println();

    //Запуск часов реального времени
    initRTC();
    initLeds();
    getTemperature();
    lastmillis = millis();

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
        WiFi.hostname(WiFi_hostname);
        setClock();
        Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
        Firebase.reconnectWiFi(true);
        syncTime();  // синхронизируем время
        readOptionsFirebase();
        printAllLedsTime();
    }
    startMainTimers();
    fiveMinuteTimer();
}

void loop() {
    Alarm.delay(10);
}
