#include <ArduinoJson.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <OneWire.h>
#include <WiFiUdp.h>
#include <memory>
#include "DS3231.h"  //Время
#include "GBasic.h"
#include "RtcDS3231.h"
#include "Shiftduino.h"
#include "TimeAlarms.h"
#include "uEEPROMLib.h"
#include "uptime_formatter.h"

// Shift register pinout
#define dataPin 10
#define clockPin 14
#define latchPin 16
#define numOfRegisters 3
Shiftduino shiftRegister(dataPin, clockPin, latchPin, numOfRegisters);

std::unique_ptr<GBasic> doserK{};
std::unique_ptr<GBasic> doserNP{};
std::unique_ptr<GBasic> doserFe{};

#define FIREBASE_HOST "aqua-3006a.firebaseio.com"
#define FIREBASE_AUTH "eRxKqsNsandXnfrDtd3wjGMHMc05nUeo5yeKmuni"
#define FIREBASE_FCM_SERVER_KEY                                                                             \
    "AAAAXMB0htY:APA91bHWX0TsxqBxol0i-1eSJKn0y6e9TpxAfwmyVO6x7BWfwrU0Bmpp_BNun1x6RWLo8lDa98A0WXfKoxzCBhoC_" \
    "SzDdzk1ZiizAWQceyClzHC79_Rw9u_9t85p6jHTRGc5qD4WpMwd"
#define FIREBASE_FCM_DEVICE_TOKEN                                                                                                  \
    "f9LZ68h4adI:"                                                                                                                 \
    "APA91bHDpiNHPCytTH6sQBLV7s97VwWTVKPrZN8P7nsLn73MbNIZxPnZRxn5ot6UlywQl7uNi5NhNtSdnrficBXabuPSdbgAIcEJH6oFRElckf9h3kE9p6H0OVR7" \
    "IJBLcPAlZFPEO2bz"

#define WORK_DEF

#ifdef WORK_DEF
    #define WIFI_SSID "Wi-Fi"
    #define WIFI_PASSWORD "1234567890"
#define WIFI_RETRY 50
#else
    #define WIFI_SSID "MikroTik"
    #define WIFI_PASSWORD "11111111"
    #define WIFI_RETRY 10
#endif

#define ONE_WIRE_BUS 14  //Пин, к которому подключены датчики DS18B20 D5 GPIO14
//#define ALARMS_COUNT 6  //Количество таймеров, которые нужно удалять. Помимо их есть еще два основных - каждую минуту и каждые
//пять. Указанное кол-во надо увеличить в случае появления нового расписания для нового устройства, например, дозаторы, CO2,
//нагреватель и прочие устройства которые будут запланированы на включение или выключение
//----------------------------------------------------------------------------------
uint8_t wifiMaxTry = WIFI_RETRY;  //Попытки подключения к сети
uint8_t wifiConnectCount = 0;

// uEEPROMLib eeprom;
uEEPROMLib eeprom(0x57);
unsigned int pos;
const uint16_t StartAddress = 16;

DS3231 clockRTC;
RtcDS3231 rtc;
bool shouldUpdateFlag = false;
std::vector<ledState_t> vectorState;
const char* WiFi_hostname = "ESP8266";
IPAddress timeServerIP;
const char* ntpServerName = "pool.ntp.org";
const int NTP_PACKET_SIZE = 48;

const long timeZoneOffset = 10800;
byte packetBuffer[NTP_PACKET_SIZE];
boolean update_status = false;
byte count_sync = 0;
byte hour1 = 0;
unsigned long hour_sync = 0;
byte minute1 = 0;
unsigned long minute_sync = 0;
byte second1 = 0;

WiFiUDP udp;

FirebaseData data;
unsigned long currentMillis = 1UL;
unsigned long previousMillis = 1UL;

String pathLight = "Light/";
String pathTemperatureOnline = "Temperature/Online/";
String pathTemperatureHistory = "Temperature/History/";
String pathDoser = "Doser/";
String pathUpdateSettings = "UpdateSettings";
String pathToLastOnline = "LastOnline";
String pathToUptime = "Uptime";
String pathToBootHistory = "BootHistory";

OneWire ds(ONE_WIRE_BUS);
byte sensorData[12];
float temp1, temp2;

byte addr1[8] = {0x28, 0xFF, 0x17, 0xF0, 0x8B, 0x16, 0x03, 0x13};  //адрес датчика DS18B20
byte addr2[8] = {0x28, 0xFF, 0x5F, 0x1E, 0x8C, 0x16, 0x03, 0xE2};  //адрес датчика DS18B20

typedef Iterator<ledPosition, ledPosition::ONE, ledPosition::SIX> ledPositionIterator;
typedef Iterator<doserType, doserType::K, doserType::Fe> doserTypeIterator;

ledDescription_t leds[6];
doser_t dosers[3];
std::vector<String> splitVector(const String& msg, const char delim) {
    std::vector<String> subStrings;
    uint32_t j = 0;
    for (uint32_t i = 0; i < msg.length(); i++) {
        if (msg.charAt(i) == delim) {
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
        sensorData[i] = ds.read();
    }
    raw = (sensorData[1] << 8) | sensorData[0];
    float celsius = (float)raw / 16.0;
    return celsius;
}
void initDosers() {
    dosers[0].name = "K";
    dosers[0].type = K;
    dosers[1].name = "NP";
    dosers[1].type = NP;
    dosers[2].name = "Fe";
    dosers[2].type = Fe;
}
void initLeds() {
    leds[0].name = "One";
    leds[0].position = ONE;
    leds[1].name = "Two";
    leds[1].position = TWO;
    leds[2].name = "Three";
    leds[2].position = THREE;
    leds[3].name = "Four";
    leds[3].position = FOUR;
    leds[4].name = "Five";
    leds[4].position = FIVE;
    leds[5].name = "Six";
    leds[5].position = SIX;
}
void getTemperature() {
    temp1 = DS18B20(addr1);
    temp2 = DS18B20(addr2);
    Serial.printf_P(PSTR(" [T1: %s°]  [T2: %s°]\n"), String(temp1).c_str(), String(temp2).c_str());
}
void sendMessage() {
    char* p = clockRTC.dateFormat("H:i:s d.m.Y", clockRTC.getDateTime());
    data.fcm.setNotifyMessage("Перезагрузка", String(p));
    free(p);
    if (Firebase.broadcastMessage(data)) {
        Serial.printf_P(PSTR("Сообщение отправлено\n"));
    } else {
        Serial.printf_P(PSTR("Ошибка отправки сообщения: %s\n"), data.errorReason().c_str());
    }
}
void sendMessage(ledDescription& led, bool state) {
    char dataMsg[100];
    String stateString = state ? "Включение" : "Выключение";
    char* p = clockRTC.dateFormat("H:i:s", clockRTC.getDateTime());
    sprintf(dataMsg, "Прожектор %s в %s", led.name.c_str(), String(p).c_str());
    free(p);
    data.fcm.setNotifyMessage(stateString, dataMsg);

    if (Firebase.broadcastMessage(data)) {
        Serial.printf_P(PSTR("Сообщение отправлено\n"));
    } else {
        Serial.printf_P(PSTR("Ошибка отправки сообщения: %s\n"), data.errorReason().c_str());
    }
}
void clearAlarms() {
    uint8_t ledsCount(sizeof(leds) / sizeof(*leds));
    for (size_t i = 0; i < ledsCount; i++) {
        Alarm.disable(leds[i].led.off);
        Alarm.disable(leds[i].led.on);
        Alarm.free(leds[i].led.off);
        Alarm.free(leds[i].led.on);
    }
    uint8_t dosersCount(sizeof(dosers) / sizeof(*dosers));
    for (size_t i = 0; i < dosersCount; i++) {
        Alarm.disable(dosers[i].alarm);
        Alarm.free(dosers[i].alarm);
    }
}

void uptime() {
    Serial.printf_P(PSTR("Время работы устройства: %s\n"), uptime_formatter::getUptime().c_str());
    if (!Firebase.setString(data, pathToUptime, uptime_formatter::getUptime())) {
        Serial.printf_P(PSTR("\nОшибка uptime: %s\n"), data.errorReason().c_str());
    };
}
void initRTC() {
    clockRTC.begin();
    char* p = clockRTC.dateFormat("H:i:s Y-m-d", clockRTC.getDateTime());
    Serial.printf_P(PSTR("Время: %s\n"), String(p).c_str());
    free(p);
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
        epoch = epoch + 2 + 10800;
        hour1 = (epoch % 86400L) / 3600;
        minute1 = (epoch % 3600) / 60;
        second1 = epoch % 60;

        char str[20];
        rtc.dateTimeToStr(str);
        Serial.printf_P(PSTR("%s\n"), str);
        uint32_t rtcEpoch = rtc.getEpoch();
        Serial.printf_P(PSTR("RTC: %d\n"), rtcEpoch);
        Serial.printf_P(PSTR("NTP: %d\n"), epoch);

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

void setCurrentState(boolean state, const String& name) {
    String stateString = state ? "ON" : "OFF";
    if (!Firebase.setBool(data, pathLight + name + "/state", state)) {
        Serial.printf_P(PSTR("Не удалось установить состояние прожектора %s в %s\n"), name.c_str(), stateString.c_str());
    } else {
        Serial.printf_P(PSTR("Состояние прожектора %s установлено в %s\n"), name.c_str(), stateString.c_str());
    }
}
void doserHandler(doser& dos) {
    switch (dos.type) {
        case K:
            doserK->begin();
            doserK->setEnableActiveState(LOW);
            doserK->move(dos.steps);
            doserK->setEnableActiveState(HIGH);
            break;
        case NP:
            doserNP->begin();
            doserNP->setEnableActiveState(LOW);
            doserNP->move(dos.steps);
            doserNP->setEnableActiveState(HIGH);
            break;
        case Fe:
            doserFe->begin();
            doserFe->setEnableActiveState(LOW);
            doserFe->move(dos.steps);
            doserFe->setEnableActiveState(HIGH);
            break;
        default:
            break;
    }
}
void ledOnHandler(ledDescription& led) {
    if (led.led.enabled) {
        Serial.printf_P(PSTR("Включение прожектора PIN %d\n"), led.led.pin);
        sendMessage(led, true);
        Alarm.enable(led.led.off);
        Alarm.enable(led.led.on);

        led.led.currentState = true;

        ledState_t state;
        state.name = led.name;
        state.state = true;
        vectorState.push_back(state);
    } else {
        Serial.printf_P(PSTR("%s\n"), "Прожектор не доступен для изменения состояния");
        Alarm.disable(led.led.off);
        Alarm.disable(led.led.on);
    }
}

void ledOffHandler(ledDescription& led) {
    if (led.led.enabled) {
        Serial.printf_P(PSTR("Выключение прожектора PIN %d\n"), led.led.pin);
        sendMessage(led, false);
        Alarm.enable(led.led.off);
        Alarm.enable(led.led.on);

        led.led.currentState = false;

        ledState_t state;
        state.name = led.name;
        state.state = false;
        vectorState.push_back(state);
    } else {
        Serial.printf_P(PSTR("%s\n"), "Прожектор не доступен для изменения состояния");
        Alarm.disable(led.led.off);
        Alarm.disable(led.led.on);
    }
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
        Serial.printf_P(PSTR("%s\n"), "Прожектор не найден!!!");
    } else {
        Serial.printf_P(PSTR(" Вкл-%02d:%02d. Выкл-%02d:%02d. Состояние: %s. Разрешен: %s. PIN: %d\n"), leds[index].led.HOn,
                        leds[index].led.MOn, leds[index].led.HOff, leds[index].led.MOff,
                        ((leds[index].led.currentState == true) ? "включен" : "выключен"),
                        ((leds[index].led.enabled == true) ? "да" : "нет"), leds[index].led.pin

        );
    }
}

void printAllLedsTime() {
    Serial.printf_P(PSTR("%s\n"), "Отображение текущих настроек всех прожекторов");
    printLEDTime(ONE);
    printLEDTime(TWO);
    printLEDTime(THREE);
    printLEDTime(FOUR);
    printLEDTime(FIVE);
    printLEDTime(SIX);
}
void printDoser(doserType dosertype) {
    uint8_t dosersCount(sizeof(dosers) / sizeof(*dosers));
    uint8_t index;
    bool found = false;
    for (size_t i = 0; i < dosersCount; i++) {
        if (dosers[i].type == dosertype) {
            index = i;
            i = dosersCount;
            found = true;
        }
    }
    if (!found) {
        Serial.printf_P(PSTR("%s\n"), "Дозатор не найден!!!");
    } else {
        Serial.printf_P(
            PSTR("%d => %s: Вкл %02d:%02d. Pins: dir=%d step=%d enable=%d sleep=%d.\nОбъём: %d. Modes:%d.%d.%d Steps:%d\n"),
            dosers[index].index, dosers[index].name.c_str(), dosers[index].hour, dosers[index].minute, dosers[index].dirPin,
            dosers[index].stepPin, dosers[index].enablePin, dosers[index].sleepPin, dosers[index].volume, dosers[index].mode0_pin,
            dosers[index].mode1_pin, dosers[index].mode2_pin, dosers[index].steps);
    }
}

void printAllDosers() {
    Serial.printf_P(PSTR("%s\n"), "Отображение текущих настроек всех прожекторов");
    printDoser(K);
    printDoser(NP);
    printDoser(Fe);
}
void setDoser(doserType dosertype) {
    uint8_t dosersCount = (sizeof(dosers) / sizeof(*dosers));
    String name;
    uint8_t i = 0;
    bool found = false;
    for (size_t k = 0; k < dosersCount; k++) {
        if (dosers[k].type == dosertype) {
            name = dosers[k].name;
            i = k;
            found = true;
            k = dosersCount;
        }
    }
    if (!found) {
        Serial.printf_P(PSTR("%s %s %s\n"), "Дозатор", name.c_str(), "не найден");
    } else {
        if (!Firebase.getString(data, pathDoser + name)) {
            Serial.printf_P(PSTR("%s %s: %s\n"), "Ошибка загрузки параметров дозатора", name.c_str(), data.errorReason().c_str());
        } else {
            if ((data.dataType() == "string")) {
                String payload = data.stringData();
                //"d=1#s=1#e=2#sl=2#ss=200#0=1#1=2#2=4#i=3#t=23:59"
                std::vector<String> vectorPayload = splitVector(payload, '#');

                for (String pair : vectorPayload) {
                    std::vector<String> pairItem = splitVector(pair, '=');
                    if (pairItem[0] == "d") {
                        dosers[i].dirPin = pairItem[1].toInt();
                    }
                    if (pairItem[0] == "s") {
                        dosers[i].stepPin = pairItem[1].toInt();
                    }
                    if (pairItem[0] == "e") {
                        dosers[i].enablePin = pairItem[1].toInt();
                    }
                    if (pairItem[0] == "sl") {
                        dosers[i].sleepPin = pairItem[1].toInt();
                    }
                    if (pairItem[0] == "ss") {
                        dosers[i].steps = pairItem[1].toInt();
                    }
                    if (pairItem[0] == "0") {
                        dosers[i].mode0_pin = pairItem[1].toInt();
                    }
                    if (pairItem[0] == "1") {
                        dosers[i].mode1_pin = pairItem[1].toInt();
                    }
                    if (pairItem[0] == "2") {
                        dosers[i].mode2_pin = pairItem[1].toInt();
                    }
                    if (pairItem[0] == "i") {
                        dosers[i].index = pairItem[1].toInt();
                    }
                    if (pairItem[0] == "t") {
                        std::vector<String> timeVector = splitVector(pairItem[1], ':');
                        dosers[i].hour = timeVector[0].toInt();
                        dosers[i].minute = timeVector[1].toInt();
                        timeVector.clear();
                    }
                    pairItem.clear();
                }
                vectorPayload.clear();

                switch (dosertype) {
                    case K:
                        doserK = std::make_unique<GBasic>(dosers[i].steps, dosers[i].dirPin, dosers[i].stepPin, dosers[i].enablePin,
                                                          dosers[i].mode0_pin, dosers[i].mode1_pin, dosers[i].mode2_pin,
                                                          shiftRegister, dosers[i].index);
                        break;
                    case NP:
                        doserNP = std::make_unique<GBasic>(dosers[i].steps, dosers[i].dirPin, dosers[i].stepPin,
                                                           dosers[i].enablePin, dosers[i].mode0_pin, dosers[i].mode1_pin,
                                                           dosers[i].mode2_pin, shiftRegister, dosers[i].index);
                        break;
                    case Fe:
                        doserFe = std::make_unique<GBasic>(dosers[i].steps, dosers[i].dirPin, dosers[i].stepPin,
                                                           dosers[i].enablePin, dosers[i].mode0_pin, dosers[i].mode1_pin,
                                                           dosers[i].mode2_pin, shiftRegister, dosers[i].index);
                        break;
                    default:
                        Serial.printf_P(PSTR("%s %s"), "Неизвестный тип дозатора", ToString(dosertype));
                        break;
                }
                dosers[i].alarm = Alarm.alarmRepeat(dosers[i].hour, dosers[i].minute, 0, doserHandler, dosers[i]);
            } else {
                Serial.printf_P(PSTR("%s\n"), "Ответ не String");
            }
        }
    }
}
//Загрузка времени включения/выключения прожектора
void setLEDTime(ledPosition position) {
    uint8_t ledsCount = (sizeof(leds) / sizeof(*leds));
    String ledPath;
    uint8_t i;
    bool found = false;
    for (size_t k = 0; k < ledsCount; k++) {
        if (leds[k].position == position) {
            ledPath = leds[k].name;
            i = k;
            found = true;
            k = ledsCount;
        }
    }
    if (found) {
        if (Firebase.getJSON(data, pathLight + ledPath)) {
            if ((data.dataType() == "json")) {
                String json = data.jsonString();
                StaticJsonDocument<180> doc;
                std::vector<String> vectorString;
                DeserializationError err = deserializeJson(doc, json);
                if (err) {
                    Serial.printf_P(PSTR("Ошибка десериализации: %s\n"), err.c_str());
                } else {
                    leds[i].led.enabled = doc["enabled"];
                    leds[i].led.pin = doc["pin"];
                    leds[i].led.currentState = doc["state"];

                    vectorString.clear();
                    vectorString = splitVector(doc["off"], ':');
                    leds[i].led.HOff = vectorString[0].toInt();
                    leds[i].led.MOff = vectorString[1].toInt();

                    vectorString.clear();
                    vectorString = splitVector(doc["on"], ':');
                    leds[i].led.HOn = vectorString[0].toInt();
                    leds[i].led.MOn = vectorString[1].toInt();

                    leds[i].led.off = Alarm.alarmRepeat(leds[i].led.HOff, leds[i].led.MOff, 0, ledOffHandler, leds[i]);
                    leds[i].led.on = Alarm.alarmRepeat(leds[i].led.HOn, leds[i].led.MOn, 0, ledOnHandler, leds[i]);

                    uint8_t minutes = clockRTC.getDateTime().hour * 60 + clockRTC.getDateTime().minute;
                    uint8_t minutesOn = leds[i].led.HOn * 60 + leds[i].led.MOn;
                    uint8_t minutesOff = leds[i].led.HOff * 60 + leds[i].led.MOff;
                    if ((minutes > minutesOn) && (minutes < minutesOff)) {
                        ledOnHandler(leds[i]);
                    } else {
                        ledOffHandler(leds[i]);
                    }
                    doc.clear();
                }
            } else {
                Serial.printf_P(PSTR("%s\n"), "Ответ не является JSON объектом");
            }
        } else {
            Serial.printf_P(PSTR("%s: %s\n"), "Ошибка загрузки параметров прожектора", data.errorReason().c_str());
        }
    } else {
        Serial.printf_P(PSTR("%s\n"), "Прожектор не найден");
    }
}
uint16_t doserAddress(const uint8_t num) {
    uint8_t ledsCount(sizeof(leds) / sizeof(*leds));
    return StartAddress + num * sizeof(doser) + ledsCount * sizeof(ledDescription) + 1;
}
void writeEEPROMDoser() {
    uint8_t dosersCount(sizeof(dosers) / sizeof(*dosers));
    for (size_t i = 0; i < dosersCount; i++) {
        if (eeprom.eeprom_write(doserAddress(i), dosers[i])) {
            Serial.printf_P(PSTR(" Параметры по дозатору %s сохранены в EEPROM\n"), dosers[i].name.c_str());
        } else {
            Serial.printf_P(PSTR(" Ошибка сохранения параметров дозатора %s в EEPROM\n"), dosers[i].name.c_str());
        }
    }
}
uint16_t ledAddress(const uint8_t num) {
    return StartAddress + num * sizeof(ledDescription);
}
void writeEEPROMLed() {
    uint8_t ledsCount(sizeof(leds) / sizeof(*leds));
    for (size_t i = 0; i < ledsCount; i++) {
        if (eeprom.eeprom_write(ledAddress(i), leds[i])) {
            Serial.printf_P(PSTR(" Параметры по прожектору %s сохранены в EEPROM\n"), leds[i].name.c_str());
        } else {
            Serial.printf_P(PSTR(" Ошибка сохранения параметров прожектора %s в EEPROM\n"), leds[i].name.c_str());
        }
    }
}
void readOptionsEEPROM() {
    ledDescription_t ledFromEEPROM;
    doser doserFromEEPROM;
    Serial.printf_P(PSTR("%s\n"), "Загрузка настроек из EEPROM");
    uint8_t ledsCount(sizeof(leds) / sizeof(*leds));
    uint8_t dosersCount(sizeof(dosers) / sizeof(*dosers));
    for (size_t i = 0; i < ledsCount; i++) {
        eeprom.eeprom_read(ledAddress(i), &ledFromEEPROM);
        leds[i] = ledFromEEPROM;
        leds[i].led.off = Alarm.alarmRepeat(leds[i].led.HOff, leds[i].led.MOff, 0, ledOffHandler, leds[i]);
        leds[i].led.on = Alarm.alarmRepeat(leds[i].led.HOn, leds[i].led.MOn, 0, ledOnHandler, leds[i]);

        uint8_t minutes = clockRTC.getDateTime().hour * 60 + clockRTC.getDateTime().minute;
        uint8_t minutesOn = leds[i].led.HOn * 60 + leds[i].led.MOn;
        uint8_t minutesOff = leds[i].led.HOff * 60 + leds[i].led.MOff;
        if ((minutes > minutesOn) && (minutes < minutesOff)) {
            ledOnHandler(leds[i]);
        } else {
            ledOffHandler(leds[i]);
        }
    }
    for (size_t i = 0; i < dosersCount; i++) {
        eeprom.eeprom_read(doserAddress(i), &doserFromEEPROM);
        dosers[i] = doserFromEEPROM;
        dosers[i].alarm = Alarm.alarmRepeat(dosers[i].hour, dosers[i].minute, 0, doserHandler, dosers[i]);
    }
    printAllLedsTime();
    printAllDosers();
}
void readOptionsFirebase() {
    Serial.printf_P(PSTR("%s\n"), "Загрузка из Firebase");

    Serial.printf_P(PSTR("%s\n"), "Прожекторы...");
    for (ledPosition i : ledPositionIterator()) {  // notice the parentheses!
        setLEDTime(i);
    }

    Serial.printf_P(PSTR("%s\n"), "Дозаторы...");
    for (doserType i : doserTypeIterator()) {  // notice the parentheses!
        setDoser(i);
    }

    Serial.printf_P(PSTR("%s\n"), "Запись в EEPROM");
    writeEEPROMLed();
    writeEEPROMDoser();
}
double floatToDouble(float x) {
    return static_cast<double>(x);
}
//Сохранение показаний датчиков температуры
void writeTemperatureFirebase() {
    if (!WiFi.isConnected())
        return;

    char* p1 = clockRTC.dateFormat("Y-m-d", clockRTC.getDateTime());
    String deviceDateKey = p1;
    free(p1);

    FirebaseJson jsonOnline, jsonHistory;
    char* p2 = clockRTC.dateFormat("H:i:s d.m.Y", clockRTC.getDateTime());
    jsonOnline.add("DateTime", String(p2));
    jsonOnline.add("temp1", floatToDouble(temp1));
    jsonOnline.add("temp2", floatToDouble(temp2));
    free(p2);
    char* p3 = clockRTC.dateFormat("H:i:s", clockRTC.getDateTime());
    jsonHistory.add("DateTime", String(p3));
    jsonHistory.add("temp1", floatToDouble(temp1));
    jsonHistory.add("temp2", floatToDouble(temp2));
    free(p3);
    Serial.printf_P(PSTR("%s"), "Отправляем температуру в ветку Online -> ");
    if (Firebase.setJSON(data, pathTemperatureOnline, jsonOnline)) {
        Serial.printf_P(PSTR("%s\n"), "OK");
    } else {
        Serial.printf_P(PSTR("Ошибка: %s\n"), data.errorReason().c_str());
    }
    jsonOnline.clear();

    Serial.printf_P(PSTR("%s"), "Отправляем температуру в ветку History -> ");
    if (Firebase.pushJSON(data, pathTemperatureHistory + deviceDateKey, jsonHistory)) {
        Serial.printf_P(PSTR("%s\n"), "OK");
    } else {
        Serial.printf_P(PSTR("Ошибка: %s\n"), data.errorReason().c_str());
    }
    jsonHistory.clear();
}

void writeBootHistory() {
    char* p = clockRTC.dateFormat("H:i:s d.m.Y", clockRTC.getDateTime());
    if (Firebase.pushString(data, pathToBootHistory, String(p))) {
        Serial.printf_P(PSTR("%s\n"), "Запись времени старта");
    } else {
        Serial.printf_P(PSTR("\n%s %s\n"), "Ошибка записи времени старта:", data.errorReason().c_str());
    }
    free(p);
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
    char* p = clockRTC.dateFormat("H:i:s d.m.Y", clockRTC.getDateTime());
    Firebase.setString(data, pathToLastOnline, String(p));
    free(p);
}
void Timer5Min() {
    uptime();
    lastOnline();
    getTemperature();
    writeTemperatureFirebase();
}

void Timer1Min() {
    ledState_t currentLed;
    char* p = clockRTC.dateFormat("H:i:s", clockRTC.getDateTime());
    Serial.printf_P(PSTR("%s "), String(p).c_str());
    free(p);
    getTemperature();
    checkUpdateSettings();
    if (shouldUpdateFlag) {
        readOptionsFirebase();
        printAllLedsTime();
        printAllDosers();
        shouldUpdateFlag = false;
    }
    while (!vectorState.empty()) {
        currentLed = vectorState.back();
        setCurrentState(currentLed.state, currentLed.name);
        vectorState.pop_back();
    }
    vectorState.clear();
}
void startMainTimers() {
    Serial.printf_P(PSTR("%s -> %d\n"), "Таймеры: основные", Alarm.count());
    Alarm.timerRepeat(5 * 60, Timer5Min);  // сохраняем температуру в Firebase/EEPROM
    Alarm.timerRepeat(20, Timer1Min);      // вывод uptime и тмемпературу каждую минуту
    Serial.printf_P(PSTR("%s -> %d\n"), "Таймеры: основные и прожекторные", Alarm.count());
}
void checkUpdateSettings() {
    if (Firebase.getBool(data, pathUpdateSettings)) {
        if (data.dataType() == "boolean") {
            if (data.boolData()) {
                Serial.printf_P(PSTR("%s\n"), "Запрос на обновление всех настроек");
                shouldUpdateFlag = true;
                clearAlarms();
                if (!Firebase.setBool(data, pathUpdateSettings, false)) {
                    Serial.printf_P(PSTR("%s: %s\n"), "Не удалось отключить флаг сброса настроек", data.errorReason().c_str());
                }
            }
        } else {
            Serial.printf_P(PSTR("%s\n"), "Флаг сброса настроек UpdateSettings должен быть типа boolean");
        }
    } else {
        Serial.printf_P(PSTR("Ошибка чтения флага сброса настроек UpdateSettings: %s\n"), data.errorReason().c_str());
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println();

    //Запуск часов реального времени
    initRTC();
    initLeds();
    initDosers();
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
        Serial.printf_P(PSTR("\n%s: %s IP: %s\n"), "Успешное подключение к WiFi", WIFI_SSID, WiFi.localIP().toString().c_str());
        WiFi.hostname(WiFi_hostname);
        setClock();
        Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
        Firebase.reconnectWiFi(true);

        data.fcm.begin(FIREBASE_FCM_SERVER_KEY);
        data.fcm.addDeviceToken(FIREBASE_FCM_DEVICE_TOKEN);
        data.fcm.setPriority("high");
        data.fcm.setTimeToLive(1000);

        sendMessage();
        writeBootHistory();
        syncTime();  // синхронизируем время
        readOptionsFirebase();
        printAllLedsTime();
        printAllDosers();
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
        // prWiFiStatus(WiFi.status());
    }
}
