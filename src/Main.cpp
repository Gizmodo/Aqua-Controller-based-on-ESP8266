#include <Arduino.h>
#include <ArduinoJson.h>
#include <ctime>
#include <memory>
#include <vector>
#include "Mediator.h"
#include "Sensor.h"

#if defined(ARDUINO_ARCH_ESP8266)
#define HOSTNAME "ESP8266"
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#elif defined(ARDUINO_ARCH_ESP32)
#define HOSTNAME "ESP32"
#include <HTTPClient.h>
#include <WiFi.h>
#else
#endif
#include <OneWire.h>  // OneWire
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>
#include "DS3231.h"            // Чип RTC
#include "GBasic.h"            // DRV8825
#include "RtcDS3231.h"         // Время
#include "Shiftduino.h"        // Сдвиговый регистр
#include "TimeAlarms.h"        // Таймеры
#include "uEEPROMLib.h"        // EEPROM
#include "uptime_formatter.h"  // Время работы
//// EEPROM
uEEPROMLib eeprom(0x57);
//// Сдвиговый регистр
#define dataPin 10
#define clockPin 14
#define latchPin 16
#define countShiftRegister 4  // Кол-во сдвиговых регистров

Shiftduino shiftRegister(dataPin, clockPin, latchPin, countShiftRegister);

//// PROGMEM
const char contentType[] PROGMEM = {"Content-Type"};
const char applicationJson[] PROGMEM = {"application/json"};
const char androidTickerText[] PROGMEM = {"android-ticker-text"};
const char androidContentTitle[] PROGMEM = {"android-content-title"};
const char androidContentText[] PROGMEM = {"android-content-text"};

const char urlLights[] PROGMEM = {
    "https://api.backendless.com/2B9D61E8-C989-5520-FFEB-A720A49C0C00/078C7D14-D7FF-42E1-95FA-A012EB826621/data/"
    "Light?property=enabled&property=name&property=off&property=on&property=pin&property=state"};
const char urlDosers[] PROGMEM = {
    "https://api.backendless.com/2B9D61E8-C989-5520-FFEB-A720A49C0C00/078C7D14-D7FF-42E1-95FA-A012EB826621/data/"
    "Doser?property=dirPin&property=enablePin&property=index&property=mode0_pin&property=mode1_pin&property=mode2_pin&property="
    "name&property=sleepPin&property=stepPin&property=steps&property=time&property=volume"};
const char urlCompressor[] PROGMEM = {
    "https://api.backendless.com/2B9D61E8-C989-5520-FFEB-A720A49C0C00/078C7D14-D7FF-42E1-95FA-A012EB826621/data/"
    "Compressor?property=enabled&property=off&property=on&property=pin&property=state"};
const char urlFlow[] PROGMEM = {
    "https://api.backendless.com/2B9D61E8-C989-5520-FFEB-A720A49C0C00/078C7D14-D7FF-42E1-95FA-A012EB826621/data/"
    "Flow?property=enabled&property=off&property=on&property=pin&property=state"};
const char urlPutUptime[] PROGMEM = {
    "https://api.backendless.com/2B9D61E8-C989-5520-FFEB-A720A49C0C00/078C7D14-D7FF-42E1-95FA-A012EB826621/data/Uptime/"
    "66E232A0-EFF0-4A9C-9B6A-785C85B139B7"};
const char urlPutLastOnline[] PROGMEM = {
    "https://api.backendless.com/2B9D61E8-C989-5520-FFEB-A720A49C0C00/078C7D14-D7FF-42E1-95FA-A012EB826621/data/LastOnline/"
    "E190FD47-1611-41CF-A1A6-C852EC05BD4F"};
const char urlPostBoot[] PROGMEM = {
    "https://api.backendless.com/2B9D61E8-C989-5520-FFEB-A720A49C0C00/078C7D14-D7FF-42E1-95FA-A012EB826621/data/BootHistory"};
const char urlPushMessage[] PROGMEM = {
    "https://api.backendless.com/2B9D61E8-C989-5520-FFEB-A720A49C0C00/078C7D14-D7FF-42E1-95FA-A012EB826621/messaging/Default"};
const char urlPutLight[] PROGMEM = {
    "https://api.backendless.com/2B9D61E8-C989-5520-FFEB-A720A49C0C00/078C7D14-D7FF-42E1-95FA-A012EB826621/data/"
    "Light/"};
const char urlPutCompressor[] PROGMEM = {
    "https://api.backendless.com/2B9D61E8-C989-5520-FFEB-A720A49C0C00/078C7D14-D7FF-42E1-95FA-A012EB826621/data/Compressor/"
    "CB8B3EC7-2C05-4D0B-99BE-DE276D98DBDF"};
const char urlPutFlow[] PROGMEM = {
    "https://api.backendless.com/2B9D61E8-C989-5520-FFEB-A720A49C0C00/078C7D14-D7FF-42E1-95FA-A012EB826621/data/Flow/"
    "8E362A5B-6B3B-4A9F-909D-51BB3656E356"};
const char urlPostTemperature[] PROGMEM = {
    "https://api.backendless.com/2B9D61E8-C989-5520-FFEB-A720A49C0C00/078C7D14-D7FF-42E1-95FA-A012EB826621/data/TemperatureLog"};
const char urlPutTemperature[] PROGMEM = {
    "https://api.backendless.com/2B9D61E8-C989-5520-FFEB-A720A49C0C00/078C7D14-D7FF-42E1-95FA-A012EB826621/data/Temperature/"
    "6D267F27-84A2-4F80-816F-550790E07C34"};
const char urlPutUpdateSettings[] PROGMEM = {
    "https://api.backendless.com/2B9D61E8-C989-5520-FFEB-A720A49C0C00/078C7D14-D7FF-42E1-95FA-A012EB826621/data/UpdateSettings/"
    "60A7BF1D-355E-4796-9A17-EFCC8E12B41E"};
const char urlGetUpdateSettings[] PROGMEM = {
    "https://api.backendless.com/2B9D61E8-C989-5520-FFEB-A720A49C0C00/078C7D14-D7FF-42E1-95FA-A012EB826621/data/UpdateSettings"};

//// RTC
DS3231 clockRTC;
RtcDS3231 rtc;
IPAddress timeServerIP;

//// Time Synchronization
const char* ntpServerName = "pool.ntp.org";
const int NTP_PACKET_SIZE = 48;
byte packetBuffer[NTP_PACKET_SIZE];
byte count_sync = 0;
WiFiUDP udp;

//// Датчик температуры
#define ONE_WIRE_BUS 14  //Пин, к которому подключены датчики DS18B20 D5 GPIO14
uint8_t sensorTemperatureAddress1[8] = {0x28, 0xFF, 0x17, 0xF0, 0x8B, 0x16, 0x03, 0x13};
uint8_t sensorTemperatureAddress2[8] = {0x28, 0xFF, 0x5F, 0x1E, 0x8C, 0x16, 0x03, 0xE2};
OneWire onewire(ONE_WIRE_BUS);
uint8_t sensorData[12];
float sensorTemperatureValue1, sensorTemperatureValue2;

//// WIFI
#ifdef WORK_DEF
#define WIFI_SSID "Wi-Fi"
#define WIFI_PASSWORD "1357924680"
#else
#define WIFI_SSID "MikroTik"
#define WIFI_PASSWORD "11111111"
#endif
uint8_t wifiMaxTry = 10;  //Попытки подключения к сети
uint8_t wifiConnectCount = 0;
const char* WiFi_hostname = HOSTNAME;

//// HTTPS
#ifdef ARDUINO_ARCH_ESP8266
std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
#else
std::unique_ptr<WiFiClientSecure> client(new WiFiClientSecure);
#endif
HTTPClient https;
String responseString = "";

//// DEVICES

#define LIGHTS_COUNT (6)                 // Кол-во прожекторов
#define DEVICE_COUNT (LIGHTS_COUNT) + 2  // Кол-во всех устройств (нужно для Alarm'ов)
//// Дозаторы
doser_t dosersArray[3];
std::unique_ptr<GBasic> doserK{};
std::unique_ptr<GBasic> doserNP{};
std::unique_ptr<GBasic> doserFe{};

//----------------Медиаторы----------------
//Компрессор
Mediator<Sensor> medCompressor;
//Помпа течения
Mediator<Sensor> medFlow;
//Помпа подъемная
Mediator<Sensor> medPump;
//Прожекторы
Mediator<Sensor> medLight;
//Кормушка
// TODO сменить класс Sensor на определенный класс данного устройства
Mediator<Sensor> medFeeder;
// CO2
Mediator<Sensor> medCO2;
//Нагреватель
Mediator<Sensor> medHeater;
//Дозатор
// TODO сменить класс Sensor на определенный класс данного устройства
Mediator<Sensor> medDoser;

//-----------------------------------------
//Устройства
Sensor* compressor;
Sensor* flow;
Sensor* pump;
Sensor* feeder;
Sensor* co2;
Sensor* heater;
Sensor* doserNew;

std::array<Sensor, LIGHTS_COUNT> lights{Sensor(medLight, "", Sensor::light), Sensor(medLight, "", Sensor::light),
                                        Sensor(medLight, "", Sensor::light), Sensor(medLight, "", Sensor::light),
                                        Sensor(medLight, "", Sensor::light), Sensor(medLight, "", Sensor::light)};
//// Расписания всех устройств
std::array<Scheduler, DEVICE_COUNT> schedules;
//-----------------------------------------

//// METHODS

void getTime() {
    char* strDateTime = clockRTC.dateFormat("H:i:s", clockRTC.getDateTime());
    Serial.printf_P(PSTR("Время %s\n"), String(strDateTime).c_str());
    free(strDateTime);
}

[[maybe_unused]] void printSheduler() {
    auto startDayUTC = 1604966400;
    Serial.printf_P(PSTR("%s\n"), "Информация о расписаниях:");
    for (auto item : schedules) {
        Serial.printf_P(PSTR("%s %s\n"), "Устройство:", item.getDevice()->getName().c_str());
        auto alarmTimeOn = Alarm.read(item.getOn()) + startDayUTC;
        auto tmOn = localtime(&alarmTimeOn);
        char bufferOn[32];
        strftime(bufferOn, 32, "%H:%M:%S", tmOn);
        Serial.printf_P(PSTR("%s %s\n"), "Включение", bufferOn);

        auto alarmTimeOff = Alarm.read(item.getOff()) + startDayUTC;
        auto tmOff = localtime(&alarmTimeOff);
        char bufferOff[32];
        strftime(bufferOff, 32, "%H:%M:%S", tmOff);
        Serial.printf_P(PSTR("%s %s\n\n"), "Выключение", bufferOff);
    }
}

std::string string_format(const std::string& fmt_str, ...) {
    int final_n, n = ((int)fmt_str.size()) * 2;
    std::unique_ptr<char[]> formatted;
    va_list ap;
    while (true) {
        formatted.reset(new char[n]);
        strcpy(&formatted[0], fmt_str.c_str());
        va_start(ap, fmt_str);
        final_n = vsnprintf(&formatted[0], n, fmt_str.c_str(), ap);
        va_end(ap);
        if (final_n < 0 || final_n >= n)
            n += abs(final_n - n + 1);
        else
            break;
    }
    return std::string(formatted.get());
}

void delPtr(const char* p) {
    delete[] p;
}

char* newPtr(size_t len) {
    char* p = new char[len];
    memset(p, 0, len);
    return p;
}

char* getPGMString(PGM_P pgm) {
    size_t len = strlen_P(pgm) + 1;
    char* buf = newPtr(len);
    strcpy_P(buf, pgm);
    buf[len - 1] = 0;
    return buf;
}

void callbackCompressor(Sensor device) {
    Serial.printf_P(PSTR("%s %s\n"), "Вызван callback для устройства", device.getName().c_str());
    char* url = nullptr;
    char* ct = nullptr;
    char* aj = nullptr;
    String urlString;
    String payload;
    url = getPGMString(urlPutCompressor);
    urlString = String(url);

    delPtr(url);
    ct = getPGMString(contentType);
    aj = getPGMString(applicationJson);
    payload = String(device.serialize().c_str());
    if (https.begin(*client, urlString)) {
        https.addHeader(String(ct), String(aj));
        int httpCode = https.PUT(payload);
        if ((httpCode > 0) && (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)) {
            Serial.printf_P(PSTR("%s состояние %s\n"), device.getName().c_str(), (device.getState() ? "ON" : "OFF"));
        } else {
            Serial.printf_P(PSTR("%s: %s\n"), "Ошибка", HTTPClient::errorToString(httpCode).c_str());
        }
        https.end();
    } else {
        Serial.printf_P(PSTR("%s\n"), "Невозможно подключиться\n");
    }
    delPtr(ct);
    delPtr(aj);
}

void callbackFlow(Sensor device) {
    Serial.printf_P(PSTR("%s %s\n"), "Вызван callback для устройства", device.getName().c_str());
    char* url = nullptr;
    char* ct = nullptr;
    char* aj = nullptr;
    String urlString;
    String payload;
    url = getPGMString(urlPutFlow);
    urlString = String(url);

    delPtr(url);
    ct = getPGMString(contentType);
    aj = getPGMString(applicationJson);
    payload = String(device.serialize().c_str());
    if (https.begin(*client, urlString)) {
        https.addHeader(String(ct), String(aj));
        int httpCode = https.PUT(payload);
        if ((httpCode > 0) && (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)) {
            Serial.printf_P(PSTR("%s состояние %s\n"), device.getName().c_str(), (device.getState() ? "ON" : "OFF"));
        } else {
            Serial.printf_P(PSTR("%s: %s\n"), "Ошибка", HTTPClient::errorToString(httpCode).c_str());
        }
        https.end();
    } else {
        Serial.printf_P(PSTR("%s\n"), "Невозможно подключиться\n");
    }
    delPtr(ct);
    delPtr(aj);
}

void callbackPump(Sensor device) {
    Serial.printf_P(PSTR("%s %s\n"), "Вызван callback для устройства", device.getName().c_str());
}

void callbackLight(Sensor device) {
    Serial.printf_P(PSTR("%s %s\n"), "Вызван callback для устройства", device.getName().c_str());
    char* url = nullptr;
    char* ct = nullptr;
    char* aj = nullptr;
    String urlString;
    String payload;
    url = getPGMString(urlPutLight);
    urlString = String(url) + String(device.getObjectID().c_str());

    delPtr(url);
    ct = getPGMString(contentType);
    aj = getPGMString(applicationJson);
    payload = String(device.serialize().c_str());
    if (https.begin(*client, urlString)) {
        https.addHeader(String(ct), String(aj));
        int httpCode = https.PUT(payload);
        if ((httpCode > 0) && (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)) {
            Serial.printf_P(PSTR("%s состояние %s\n"), device.getName().c_str(), (device.getState() ? "ON" : "OFF"));
        } else {
            Serial.printf_P(PSTR("%s: %s\n"), "Ошибка", HTTPClient::errorToString(httpCode).c_str());
        }
        https.end();
    } else {
        Serial.printf_P(PSTR("%s\n"), "Невозможно подключиться\n");
    }
    delPtr(ct);
    delPtr(aj);
}

void callbackFeeder(Sensor device) {
    Serial.printf_P(PSTR("%s %s\n"), "Вызван callback для устройства", device.getName().c_str());
}

void callbackCO2(Sensor device) {
    Serial.printf_P(PSTR("%s %s\n"), "Вызван callback для устройства", device.getName().c_str());
}

void callbackHeater(Sensor device) {
    Serial.printf_P(PSTR("%s %s\n"), "Вызван callback для устройства", device.getName().c_str());
}

void callbackDoser(Sensor device) {
    Serial.printf_P(PSTR("%s %s\n"), "Вызван callback для устройства", device.getName().c_str());
}

void initMediators() {
    medCompressor.Register("1", callbackCompressor);
    medFlow.Register("1", callbackFlow);
    medPump.Register("1", callbackPump);
    medLight.Register("1", callbackLight);
    medFeeder.Register("1", callbackFeeder);
    medCO2.Register("1", callbackCO2);
    medHeater.Register("1", callbackHeater);
    medDoser.Register("1", callbackDoser);
}

void createDevicesAndScheduler() {
    compressor = new Sensor(medCompressor, "Компрессор", Sensor::compressor);
    flow = new Sensor(medFlow, "Помпа течения", Sensor::flow);
    pump = new Sensor(medPump, "Помпа подъёмная", Sensor::pump);
    feeder = new Sensor(medFeeder, "Кормушка", Sensor::feeder);
    co2 = new Sensor(medCO2, "CO2", Sensor::co2);
    heater = new Sensor(medHeater, "Нагреватель", Sensor::heater);
    doserNew = new Sensor(medDoser, "Дозатор", Sensor::doser);
    for (int i = 0; i < LIGHTS_COUNT; ++i) {
        std::string buffer = string_format("%s %d", "Прожектор", i + 1);
        auto item = lights.at(i);
        item.setName(buffer);
        item.setMediator(medLight);
        lights.at(i) = item;
        auto schedule = schedules.at(i);
        schedule.setDevice(&(lights.at(i)));
        schedules.at(i) = schedule;
    }
    auto schedule = schedules.at(6);
    schedule.setDevice(compressor);
    schedules.at(6) = schedule;

    schedule = schedules.at(7);
    schedule.setDevice(flow);
    schedules.at(7) = schedule;
}

void printDevice(Sensor* device) {
    Serial.printf_P(PSTR("%s\n"), device->sensorInfo().c_str());
}

void printAllDevices() {
    Serial.printf_P(PSTR("%s\n"), "Вывод информации об устройствах:");
    /*
      printDevice(pump);
      printDevice(feeder);
      printDevice(co2);
      printDevice(heater);
      printDevice(doserNew);
*/
    for (auto light : lights) {
        printDevice(&light);
    }
    printDevice(compressor);
    printDevice(flow);
}

float getSensorTemperature(const uint8_t* sensorAddress) {
    unsigned int rawData;
    onewire.reset();
    onewire.select(sensorAddress);
    onewire.write(0x44, 1);
    onewire.reset();
    onewire.select(sensorAddress);
    onewire.write(0xBE);
    for (uint8_t i = 0; i < 9; i++) {
        sensorData[i] = onewire.read();
    }
    rawData = (sensorData[1] << 8) | sensorData[0];
    float celsius = static_cast<float>(rawData) / static_cast<float>(16.0);
    return celsius;
}

void getSensorsTemperature() {
    sensorTemperatureValue1 = getSensorTemperature(sensorTemperatureAddress1);
    sensorTemperatureValue2 = getSensorTemperature(sensorTemperatureAddress2);
    Serial.printf_P(PSTR(" [T1: %s°]  [T2: %s°]\n"), String(sensorTemperatureValue1).c_str(),
                    String(sensorTemperatureValue2).c_str());
}

void sendMessage(const String& message) {
    char* url = getPGMString(urlPushMessage);
    char* aj = getPGMString(applicationJson);
    char* ct = getPGMString(contentType);
    char* att = getPGMString(androidTickerText);
    char* actitle = getPGMString(androidContentTitle);
    char* actext = getPGMString(androidContentText);
    if (https.begin(*client, String(url))) {
        String data;
        data = R"({"message":")" + message + "\"}";
        https.addHeader(String(ct), String(aj));
        https.addHeader(String(att), F("You just got a push notification!"));
        https.addHeader(String(actitle), F("This is a notification title"));
        https.addHeader(String(actext), F("Push Notifications are cool"));

        int httpCode = https.POST(data);
        if ((httpCode > 0) && (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)) {
            Serial.println(F("Сообщение отправлено"));
        } else {
            Serial.printf_P(PSTR("sendMessage() -> Ошибка: %s\n"), HTTPClient::errorToString(httpCode).c_str());
        }
        https.end();
    } else {
        Serial.printf_P(PSTR("%s\n"), "sendMessage() -> Невозможно подключиться\n");
    }
    delPtr(url);
    delPtr(ct);
    delPtr(att);
    delPtr(actitle);
    delPtr(actext);
    delPtr(aj);
}

void sendMessage(Sensor* device, bool state) {
    char dataMsg[100];
    String stateString = state ? "включение" : "выключение";
    char* p = clockRTC.dateFormat("H:i:s", clockRTC.getDateTime());
    sprintf(dataMsg, "%s: %s", device->getName().c_str(), String(p).c_str());
    delPtr(p);

    char* url = getPGMString(urlPushMessage);
    char* aj = getPGMString(applicationJson);
    char* ct = getPGMString(contentType);
    char* att = getPGMString(androidTickerText);
    char* actitle = getPGMString(androidContentTitle);
    char* actext = getPGMString(androidContentText);

    if (https.begin(*client, String(url))) {
        String data;
        data = R"({"message":")" + stateString + "\"}";
        https.addHeader(String(ct), String(aj));
        https.addHeader(String(att), F("You just got a push notification!"));
        https.addHeader(String(actitle), F("This is a notification title"));
        https.addHeader(String(actext), String(dataMsg));
        int httpCode = https.POST(data);
        if ((httpCode > 0) && (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)) {
            Serial.println(F("  Сообщение отправлено"));
        } else {
            Serial.printf_P(PSTR("sendMessage() -> Ошибка: %s\n"), HTTPClient::errorToString(httpCode).c_str());
        }
        https.end();
    } else {
        Serial.printf_P(PSTR("%s\n"), "sendMessage() -> Невозможно подключиться\n");
    }
    delPtr(url);
    delPtr(ct);
    delPtr(att);
    delPtr(actitle);
    delPtr(actext);
    delPtr(aj);
    stateString.clear();
}

AlarmID_t findAlarmByDevice(Sensor* device, bool isOn) {
    AlarmID_t result = -1;
    for (auto scheduleItem : schedules) {
        auto deviceToFind = scheduleItem.getDevice();
        if (device == deviceToFind) {
            result = isOn ? scheduleItem.getOn() : scheduleItem.getOff();
            break;
        }
    }
    return result;
}

void deviceOnHandler(Sensor* device) {
    getTime();
    if (device->getEnabled()) {
        Serial.printf_P(PSTR("%s: включение, pin %d\n"), device->getName().c_str(), device->getPin());
        shiftRegister.setPin(countShiftRegister, device->getPin(), HIGH);
        sendMessage(device, true);
        Alarm.enable(findAlarmByDevice(device, true));
        Alarm.enable(findAlarmByDevice(device, false));
        device->setStateNotify(true);
    } else {
        Serial.printf_P(PSTR("%s %s\n"), device->getName().c_str(), "нельзя изменять.");
        Alarm.disable(findAlarmByDevice(device, true));
        Alarm.disable(findAlarmByDevice(device, false));
    }
}

void deviceOffHandler(Sensor* device) {
    getTime();
    if (device->getEnabled()) {
        Serial.printf_P(PSTR("%s: выключение, pin %d\n"), device->getName().c_str(), device->getPin());
        shiftRegister.setPin(countShiftRegister, device->getPin(), LOW);
        sendMessage(device, false);
        Alarm.enable(findAlarmByDevice(device, true));
        Alarm.enable(findAlarmByDevice(device, false));
        device->setStateNotify(false);
    } else {
        Serial.printf_P(PSTR("%s %s\n"), device->getName().c_str(), "нельзя изменять.");
        Alarm.disable(findAlarmByDevice(device, true));
        Alarm.disable(findAlarmByDevice(device, false));
    }
}

void doserHandler(const doser& dos) {
    String message;
    message = "Включение дозатора " + dos.name;
    sendMessage(message);
    message.clear();

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

void splitTime(char* payload, uint8_t& hour, uint8_t& minute) {
    char* split = strtok(payload, ":");
    char* pEnd;
    hour = strtol(split, &pEnd, 10);
    split = strtok(nullptr, ":");
    minute = strtol(split, &pEnd, 10);
}

void parseJSONLights(const String& response) {
    DynamicJsonDocument doc(1300);
    DeserializationError err = deserializeJson(doc, response);
    if (err) {
        Serial.print(F(" Ошибка разбора: "));
        Serial.println(err.c_str());
        return;
    } else {
        JsonArray array = doc.as<JsonArray>();
        for (JsonObject obj : array) {
            boolean enabled = obj["enabled"];
            std::string name = obj["name"];
            std::string off = obj["off"];
            std::string on = obj["on"];
            uint8_t pin = obj["pin"];
            boolean state = obj["state"];
            std::string objectId = obj["objectId"];
            off = off.substr(0, 10);
            on = on.substr(0, 10);

            for (auto&& light : lights) {
                if (name == light.getName()) {
                    light.setState(state);
                    light.setEnabled(enabled);
                    light.setPin(pin);
                    light.setObjectID(objectId);
                    light.setOn(static_cast<time_t>(std::stoul(on)));
                    light.setOff(static_cast<time_t>(std::stoul(off)));
                }
            }
        }
        doc.shrinkToFit();
        doc.clear();
    }
}

void parseJSONDosers(const String& response) {
    std::unique_ptr<GBasic> emptyDoser{};
    uint8_t dosersCount = (sizeof(dosersArray) / sizeof(*dosersArray));
    uint8_t ind = 0;
    DynamicJsonDocument doc(1300);
    DeserializationError err = deserializeJson(doc, response);
    if (err) {
        Serial.print(F("parseJSONDosers -> Ошибка разбора: "));
        Serial.println(err.c_str());
        return;
    } else {
        JsonArray array = doc.as<JsonArray>();
        for (JsonObject obj : array) {
            // TODO добавить флаг доступности работы дозатора
            uint8_t dirPin = obj["dirPin"];
            uint8_t stepPin = obj["stepPin"];
            uint8_t enablePin = obj["enablePin"];
            uint8_t sleepPin = obj["sleepPin"];
            uint8_t mode0_pin = obj["mode0_pin"];
            uint8_t mode1_pin = obj["mode1_pin"];
            uint8_t mode2_pin = obj["mode2_pin"];

            uint8_t index = obj["index"];
            uint16_t steps = obj["steps"];
            uint16_t volume = obj["volume"];
            const char* name = obj["name"];
            const char* time = obj["time"];

            char* dupTime = strdup(time);
            for (auto& doserItem : dosersArray) {
                if (strcmp(name, doserItem.name.c_str()) == 0) {
                    for (size_t k = 0; k < dosersCount; k++) {
                        if (dosersArray[k].type == doserItem.type) {
                            ind = k;
                            k = dosersCount;
                        }
                    }
                    dosersArray[ind].dirPin = dirPin;
                    dosersArray[ind].stepPin = stepPin;
                    dosersArray[ind].enablePin = enablePin;
                    dosersArray[ind].sleepPin = sleepPin;
                    dosersArray[ind].mode0_pin = mode0_pin;
                    dosersArray[ind].mode1_pin = mode1_pin;
                    dosersArray[ind].mode2_pin = mode2_pin;

                    dosersArray[ind].index = index;
                    dosersArray[ind].steps = steps;
                    dosersArray[ind].volume = volume;

                    splitTime(dupTime, dosersArray[ind].hour, dosersArray[ind].minute);
#ifdef ARDUINO_ARCH_ESP32
                    emptyDoser = std_esp32::make_unique<GBasic>(dosersArray[ind].steps, dosersArray[ind].dirPin,
                                                                dosersArray[ind].stepPin, dosersArray[ind].enablePin,
                                                                dosersArray[ind].mode0_pin, dosersArray[ind].mode1_pin,
                                                                dosersArray[ind].mode2_pin, shiftRegister, dosersArray[ind].index);
#else
                    emptyDoser =
                        std::make_unique<GBasic>(dosersArray[ind].steps, dosersArray[ind].dirPin, dosersArray[ind].stepPin,
                                                 dosersArray[ind].enablePin, dosersArray[ind].mode0_pin, dosersArray[ind].mode1_pin,
                                                 dosersArray[ind].mode2_pin, shiftRegister, dosersArray[ind].index);
#endif
                    switch (doserItem.type) {
                        case K:
                            doserK = std::move(emptyDoser);
                            break;
                        case NP:
                            doserNP = std::move(emptyDoser);
                            break;
                        case Fe:
                            doserFe = std::move(emptyDoser);
                            break;
                        default:
                            Serial.printf_P(PSTR("%s %s"), "Неизвестный тип дозатора", ToString(doserItem.type));
                            break;
                    }
                    dosersArray[ind].alarm =
                        Alarm.alarmRepeat(dosersArray[ind].hour, dosersArray[ind].minute, 0, doserHandler, dosersArray[ind]);
                    // TODO Селать проверку на было ли уже событие в текущем дне
                    break;
                }
            }
            free(dupTime);
        }
        doc.shrinkToFit();
        doc.clear();
    }
}

void parseJSONCompressor(const String& response) {
    DynamicJsonDocument doc(250);
    DeserializationError err = deserializeJson(doc, response);
    if (err) {
        Serial.print(F(" Ошибка разбора: "));
        Serial.println(err.c_str());
        return;
    } else {
        JsonArray array = doc.as<JsonArray>();
        for (JsonObject obj : array) {
            boolean enabled = obj["enabled"];
            std::string off = obj["off"];
            std::string on = obj["on"];
            uint8_t pin = obj["pin"];
            boolean state = obj["state"];
            std::string objectId = obj["objectId"];
            off = off.substr(0, 10);
            on = on.substr(0, 10);

            compressor->setState(state);
            compressor->setEnabled(enabled);
            compressor->setPin(pin);
            compressor->setObjectID(objectId);
            compressor->setOn(static_cast<time_t>(std::stoul(on)));
            compressor->setOff(static_cast<time_t>(std::stoul(off)));
        }
        doc.shrinkToFit();
        doc.clear();
    }
}

void parseJSONFlow(const String& response) {
    DynamicJsonDocument doc(250);
    DeserializationError err = deserializeJson(doc, response);
    if (err) {
        Serial.print(F(" Ошибка разбора: "));
        Serial.println(err.c_str());
        return;
    } else {
        JsonArray array = doc.as<JsonArray>();
        for (JsonObject obj : array) {
            boolean enabled = obj["enabled"];
            std::string off = obj["off"];
            std::string on = obj["on"];
            uint8_t pin = obj["pin"];
            boolean state = obj["state"];
            std::string objectId = obj["objectId"];
            off = off.substr(0, 10);
            on = on.substr(0, 10);

            flow->setState(state);
            flow->setEnabled(enabled);
            flow->setPin(pin);
            flow->setObjectID(objectId);
            flow->setOn(static_cast<time_t>(std::stoul(on)));
            flow->setOff(static_cast<time_t>(std::stoul(off)));
        }
        doc.shrinkToFit();
        doc.clear();
    }
}

void setHostName() {
#ifdef ARDUINO_ARCH_ESP8266
    WiFi.hostname(WiFi_hostname);
#else
    WiFi.setHostname(WiFi_hostname);
#endif
}

boolean initWiFi() {
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
        return false;
    } else {
        Serial.printf_P(PSTR("\n%s: %s IP: %s\n"), "Успешное подключение к WiFi", WIFI_SSID, WiFi.localIP().toString().c_str());
        setHostName();
        return true;
    }
}

void attachLightScheduler() {
    for (size_t i = 0; i < DEVICE_COUNT; i++) {
        if (!schedules.at(i).getDevice()->isLight()) {
            continue;
        }
        auto schedule = schedules.at(i);
        auto sensor = schedule.getDevice();
        auto alarmOn = Alarm.alarmRepeat(sensor->getHourOn(), sensor->getMinuteOn(), 0, deviceOnHandler, sensor);
        auto alarmOff = Alarm.alarmRepeat(sensor->getHourOff(), sensor->getMinuteOff(), 0, deviceOffHandler, sensor);

        schedule.setOn(alarmOn);
        schedule.setOff(alarmOff);
        schedules.at(i) = schedule;
        (sensor->shouldRun(clockRTC.getDateTime().hour, clockRTC.getDateTime().minute)) ? deviceOnHandler(sensor)
                                                                                        : deviceOffHandler(sensor);
    }
}

void attachCompressorScheduler() {
    for (size_t i = 0; i < DEVICE_COUNT; i++) {
        if (!schedules.at(i).getDevice()->isCompressor()) {
            continue;
        }
        auto schedule = schedules.at(i);
        auto sensor = schedule.getDevice();
        auto alarmOn = Alarm.alarmRepeat(sensor->getHourOn(), sensor->getMinuteOn(), 0, deviceOnHandler, sensor);
        auto alarmOff = Alarm.alarmRepeat(sensor->getHourOff(), sensor->getMinuteOff(), 0, deviceOffHandler, sensor);

        schedule.setOn(alarmOn);
        schedule.setOff(alarmOff);
        schedules.at(i) = schedule;
        (sensor->shouldRun(clockRTC.getDateTime().hour, clockRTC.getDateTime().minute)) ? deviceOnHandler(sensor)
                                                                                        : deviceOffHandler(sensor);
    }
}

void attachFlowScheduler() {
    for (size_t i = 0; i < DEVICE_COUNT; i++) {
        if (!schedules.at(i).getDevice()->isFlow()) {
            continue;
        }
        auto schedule = schedules.at(i);
        auto sensor = schedule.getDevice();
        auto alarmOn = Alarm.alarmRepeat(sensor->getHourOn(), sensor->getMinuteOn(), 0, deviceOnHandler, sensor);
        auto alarmOff = Alarm.alarmRepeat(sensor->getHourOff(), sensor->getMinuteOff(), 0, deviceOffHandler, sensor);

        schedule.setOn(alarmOn);
        schedule.setOff(alarmOff);
        schedules.at(i) = schedule;
        (sensor->shouldRun(clockRTC.getDateTime().hour, clockRTC.getDateTime().minute)) ? deviceOnHandler(sensor)
                                                                                        : deviceOffHandler(sensor);
    }
}

void getParamLights() {
    char* url = getPGMString(urlLights);
    Serial.printf_P(PSTR(" %s\n"), "Прожекторы...");
    if (https.begin(*client, String(url))) {
        int httpCode = https.GET();
        if (httpCode > 0) {
            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
                responseString = https.getString();
            }
        } else {
            Serial.printf_P(PSTR(" %s %s\n"), "Ошибка:", HTTPClient::errorToString(httpCode).c_str());
        }
        https.end();
    } else {
        Serial.printf_P(PSTR(" %s\n"), "Невозможно подключиться\n");
    }
    delPtr(url);
    if (responseString.isEmpty()) {
        Serial.printf_P(PSTR(" %s\n"), "Ответ пустой");
    } else {
        parseJSONLights(responseString);
        responseString.clear();
        attachLightScheduler();
    }
}
/*
void getParamDosers() {
    char* url  = getPGMString(urlDosers);
    Serial.printf_P(PSTR(" %s\n"), "Дозаторы...");
    if (https.begin(*client, String(url))) {
        int httpCode = https.GET();
        if (httpCode > 0) {
            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
                responseString = https.getString();
            }
        } else {
            Serial.printf_P(PSTR("getParamDosers() -> Ошибка: %s\n"), HTTPClient::errorToString(httpCode).c_str());
        }
        https.end();
    } else {
        Serial.printf_P(PSTR("%s\n"), "getParamDosers() -> Невозможно подключиться\n");
    }
    delPtr(url);
    if (responseString.isEmpty()) {
        Serial.printf_P(PSTR(" %s\n"), "getParamDosers() -> Ответ пустой");
    } else {
        parseJSONDosers(responseString);
        responseString.clear();
    }
}
*/
void getParamCompressor() {
    char* url = getPGMString(urlCompressor);
    Serial.printf_P(PSTR(" %s\n"), "Компрессор...");
    if (https.begin(*client, String(url))) {
        int httpCode = https.GET();
        if (httpCode > 0) {
            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
                responseString = https.getString();
            }
        } else {
            Serial.printf_P(PSTR(" %s %s\n"), "Ошибка:", HTTPClient::errorToString(httpCode).c_str());
        }
        https.end();
    } else {
        Serial.printf_P(PSTR(" %s\n"), "Невозможно подключиться\n");
    }
    delPtr(url);
    if (responseString.isEmpty()) {
        Serial.printf_P(PSTR(" %s\n"), "Ответ пустой");
    } else {
        parseJSONCompressor(responseString);
        responseString.clear();
        attachCompressorScheduler();
    }
}

void getParamFlow() {
    char* url = getPGMString(urlFlow);
    Serial.printf_P(PSTR(" %s\n"), "Помпа течения...");
    if (https.begin(*client, String(url))) {
        int httpCode = https.GET();
        if (httpCode > 0) {
            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
                responseString = https.getString();
            }
        } else {
            Serial.printf_P(PSTR(" %s %s\n"), "Ошибка:", HTTPClient::errorToString(httpCode).c_str());
        }
        https.end();
    } else {
        Serial.printf_P(PSTR(" %s\n"), "Невозможно подключиться\n");
    }
    delPtr(url);
    if (responseString.isEmpty()) {
        Serial.printf_P(PSTR(" %s\n"), "Ответ пустой");
    } else {
        parseJSONFlow(responseString);
        responseString.clear();
        attachFlowScheduler();
    }
}

void getParamsEEPROM() {
    Serial.printf_P(PSTR("%s\n"), "Загрузка настроек из EEPROM");
    uint8_t address = 0;
    //    uint8_t szLed = sizeof(ledDescription_t);
    uint8_t szDoser = sizeof(doser_t);
    //   ledDescription_t ledFromEEPROM;
    /*
        for (auto& led : leds) {
            eeprom.eeprom_read<ledDescription_t>(address, &led);
            // led.led.off = Alarm.alarmRepeat(led.led.HOff, led.led.MOff, 0, ledOffHandler, led);
            // led.led.on = Alarm.alarmRepeat(led.led.HOn, led.led.MOn, 0, ledOnHandler, led);

            if (shouldRun(led)) {
                // ledOnHandler(led);
            } else {
                // ledOffHandler(led);
            }
            address += szLed + 1;
        }
    */
    //    address -= szLed;
    for (auto& doser : dosersArray) {
        eeprom.eeprom_read<doser_t>(address, &doser);
        doser.alarm = Alarm.alarmRepeat(doser.hour, doser.minute, 0, doserHandler, doser);
        // TODO проверить событие не выполнялось ли уже хотя это чтение настроек
        address += szDoser + 1;
    }

    address -= szDoser;
    // eeprom.eeprom_read<ledDescription_t>(address, &compressor);
    /* compressor.led.on = Alarm.alarmRepeat(compressor.led.HOn, compressor.led.MOn, 0, compressorOnHandler, compressor);
     compressor.led.off = Alarm.alarmRepeat(compressor.led.HOff, compressor.led.MOff, 0, compressorOffHandler, compressor);

     if (shouldRun(compressor)) {
         compressorOnHandler(compressor);
     } else {
         compressorOffHandler(compressor);
     }
     */
}

void setParamsEEPROM() {
    Serial.printf_P(PSTR("%s\n"), "Запись настроек в EEPROM");
    uint8_t address = 0;
    //    uint8_t szLed = sizeof(ledDescription_t);
    uint8_t szDoser = sizeof(doser_t);

    /* for (auto& led : leds) {
         eeprom.eeprom_write<ledDescription_t>(address, led);
         address += szLed + 1;
     }*/
    //  address -= szLed;
    for (auto& doser : dosersArray) {
        eeprom.eeprom_write<doser_t>(address, doser);
        address += szDoser + 1;
    }
    address -= szDoser;
    //    eeprom.eeprom_write<ledDescription_t>(address, compressor);
}

void getParamsBackEnd() {
    Serial.printf_P(PSTR("%s\n"), "Чтение параметров из облака");
    getParamLights();
    getParamCompressor();
    getParamFlow();
    // TODO отрефакторить блок ниже отсюда
    /* getParamDosers();

    setParamsEEPROM(); */
}

void initLocalClock() {
    configTime(0, 0, ntpServerName);
    setenv("TZ", "MSK-3MSD,M3.5.0/2,M10.5.0/3", 1);
    tzset();
}

void initDS3231() {
    clockRTC.begin();
    char* df = clockRTC.dateFormat("H:i:s d.m.Y", clockRTC.getDateTime());
    Serial.printf_P(PSTR("Время DS3231: %s\n"), String(df).c_str());
    free(df);
}

void sendNTPpacket(const IPAddress& address) {
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

void syncTime() {
    Serial.printf_P(PSTR("Синхронизация DS3231/NTP %s\n"), ntpServerName);
    udp.begin(2390);
    WiFi.hostByName(ntpServerName, timeServerIP);
    sendNTPpacket(timeServerIP);
    delay(100);
    int cb = udp.parsePacket();
    if (!cb) {
        Serial.printf_P(PSTR(" Нет ответа от сервера времени %s\n"), ntpServerName);
        count_sync++;
    } else {
        count_sync = 0;
        Serial.printf_P(PSTR(" Получен ответ от сервера времени %s\n"), ntpServerName);
        udp.read(packetBuffer, NTP_PACKET_SIZE);
        unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
        unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
        unsigned long secsSince1900 = highWord << 16 | lowWord;
        const unsigned long seventyYears = 2208988800UL;
        unsigned long epoch = secsSince1900 - seventyYears;
        // 2 секунды разница с большим братом
        epoch = epoch + 2 + 10800;

        char str[20];
        rtc.dateTimeToStr(str);
        Serial.printf_P(PSTR(" %s\n"), str);
        uint32_t rtcEpoch = rtc.getEpoch();
        Serial.printf_P(PSTR(" DS3231: %lu\n"), rtcEpoch);
        Serial.printf_P(PSTR("    NTP: %lu\n"), epoch);

        if ((rtcEpoch - epoch) > 2) {
            Serial.printf_P(PSTR(" %s"), "Обновляем DS3231 (разница между эпохами = ");
            if ((rtcEpoch - epoch) > 10000) {
                Serial.printf_P(PSTR(" %s\n"), (epoch - rtcEpoch));
            } else {
                Serial.printf_P(PSTR(" %s\n"), (rtcEpoch - epoch));
            }
            rtc.setEpoch(epoch);
        } else {
            Serial.printf_P(PSTR(" %s\n"), "Дата и время DS3231 не требуют синхронизации");
        }
    }
}

void initHTTPClient() {
#ifdef ARDUINO_ARCH_ESP8266
    client->setInsecure();
    bool mfl = client->probeMaxFragmentLength("api.backendless.com", 443, 1024);
    if (mfl) {
        client->setBufferSizes(1024, 1024);
    }
#endif
}

void initDosersArray() {
    dosersArray[0].name = "K";
    dosersArray[0].type = K;
    dosersArray[1].name = "NP";
    dosersArray[1].type = NP;
    dosersArray[2].name = "Fe";
    dosersArray[2].type = Fe;
}

void putUptime(const String& uptime) {
    char* url = getPGMString(urlPutUptime);
    char* ct = getPGMString(contentType);
    char* aj = getPGMString(applicationJson);
    if (https.begin(*client, String(url))) {
        String payload;
        payload = R"({"uptime":")" + uptime + "\"}";
        https.addHeader(String(ct), String(aj));
        int httpCode = https.PUT(payload);
        if ((httpCode > 0) && (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)) {
        } else {
            Serial.printf_P(PSTR("putUptime() -> Ошибка: %s\n"), HTTPClient::errorToString(httpCode).c_str());
        }
        https.end();
    } else {
        Serial.printf_P(PSTR("%s\n"), "putUptime() -> Невозможно подключиться\n");
    }
    delPtr(url);
    delPtr(ct);
    delPtr(aj);
}

void uptime() {
    String uptime = uptime_formatter::getUptime();
    Serial.printf_P(PSTR("Время работы устройства: %s\n"), uptime.c_str());
    putUptime(uptime);
}

void putLastOnline(const String& currentTime) {
    char* ct = getPGMString(contentType);
    char* aj = getPGMString(applicationJson);
    char* url = getPGMString(urlPutLastOnline);
    if (https.begin(*client, String(url))) {
        String payload;
        payload = R"({"lastonline":")" + currentTime + "\"}";
        https.addHeader(String(ct), String(aj));
        int httpCode = https.PUT(payload);
        if ((httpCode > 0) && (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)) {
        } else {
            Serial.printf_P(PSTR("postLastOnline() -> Ошибка: %s\n"), HTTPClient::errorToString(httpCode).c_str());
        }
        https.end();
    } else {
        Serial.printf_P(PSTR("%s\n"), "postLastOnline() -> Невозможно подключиться\n");
    }
    delPtr(url);
    delPtr(ct);
    delPtr(aj);
}

void lastOnline() {
    char* currentTime = clockRTC.dateFormat("H:i:s d.m.Y", clockRTC.getDateTime());
    String payload = String(currentTime);
    putLastOnline(payload);
    free(currentTime);
}

void postBoot() {
    char* currentTime = clockRTC.dateFormat("H:i:s d.m.Y", clockRTC.getDateTime());
    char* url = getPGMString(urlPostBoot);
    char* ct = getPGMString(contentType);
    char* aj = getPGMString(applicationJson);
    if (https.begin(*client, String(url))) {
        String payload;
        payload = R"({"time":")" + String(currentTime) + "\"}";
        https.addHeader(String(ct), String(aj));
        int httpCode = https.POST(payload);
        if ((httpCode > 0) && (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)) {
        } else {
            Serial.printf_P(PSTR("postBoot() -> Ошибка: %s\n"), HTTPClient::errorToString(httpCode).c_str());
        }
        https.end();
    } else {
        Serial.printf_P(PSTR("%s\n"), "postBoot() -> Невозможно подключиться\n");
    }
    delPtr(url);
    delPtr(ct);
    delPtr(aj);
    String message;
    message = "Перезагрузка " + String(currentTime);
    sendMessage(message);
    message.clear();
}

void parseJSONUpdateSettings(const String& response, bool& result) {
    const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(6) + 110;
    DynamicJsonDocument doc(capacity);
    DeserializationError err = deserializeJson(doc, response);
    if (err) {
        Serial.print(F("Ошибка разбора: "));
        Serial.println(err.c_str());
        result = false;
    } else {
        JsonArray array = doc.as<JsonArray>();
        for (JsonObject obj : array) {
            result = obj["flag"];
        }
        doc.shrinkToFit();
        doc.clear();
    }
}

void putUpdateSettings() {
    char* url = getPGMString(urlPutUpdateSettings);
    char* ct = getPGMString(contentType);
    char* aj = getPGMString(applicationJson);
    if (https.begin(*client, String(url))) {
        String payload = "{\"flag\": false }";
        https.addHeader(String(ct), String(aj));
        int httpCode = https.PUT(payload);
        (httpCode > 0) && (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
            ? Serial.printf_P(PSTR("%s\n"), "putUpdateSettings() -> Флаг сброшен")
            : Serial.printf_P(PSTR("%s %s\n"), "putUpdateSettings() -> Ошибка:", HTTPClient::errorToString(httpCode).c_str());
        https.end();
    } else {
        Serial.printf_P(PSTR("%s\n"), "putUpdateSettings() -> Невозможно подключиться\n");
    }
    delPtr(url);
    delPtr(ct);
    delPtr(aj);
}

bool getUpdateSettings() {
    char* url = getPGMString(urlGetUpdateSettings);
    bool flag = false;
    if (https.begin(*client, String(url))) {
        int httpCode = https.GET();
        if (httpCode > 0) {
            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
                responseString = https.getString();
            }
        } else {
            Serial.printf_P(PSTR("getUpdateSettings() -> Ошибка: %s\n"), HTTPClient::errorToString(httpCode).c_str());
        }
        https.end();
    } else {
        Serial.printf_P(PSTR("%s\n"), "getUpdateSettings() -> Невозможно подключиться\n");
    }
    delPtr(url);
    if (responseString.isEmpty()) {
        Serial.printf_P(PSTR(" %s\n"), "getUpdateSettings() -> Ответ пустой");
    } else {
        parseJSONUpdateSettings(responseString, flag);
        responseString.clear();
    }
    return flag;
}

void printDosersParam() {
    Serial.printf_P(PSTR("%s\n"), "Настройки дозаторов");
    for (auto& doser : dosersArray) {
        Serial.printf_P(
            PSTR("%d => %s: Вкл %02d:%02d. Pins: dir=%d step=%d enable=%d sleep=%d.\nОбъём: %d. Modes:%d.%d.%d Steps:%d\n"),
            doser.index, doser.name.c_str(), doser.hour, doser.minute, doser.dirPin, doser.stepPin, doser.enablePin, doser.sleepPin,
            doser.volume, doser.mode0_pin, doser.mode1_pin, doser.mode2_pin, doser.steps);
    }
}

void timer1() {
    getTime();
    getSensorsTemperature();

    if (getUpdateSettings()) {
        Serial.printf_P(PSTR("%s\n"), "Будет выполнено обновление всех параметров");
        getParamsBackEnd();
        printAllDevices();
        putUpdateSettings();
    }
}

String serializeTemperature() {
    String output;
    const int capacity = JSON_OBJECT_SIZE(5);
    DynamicJsonDocument doc(capacity);
    char* currentTime = clockRTC.dateFormat("d.m.Y H:i:s", clockRTC.getDateTime());
    String time = String(currentTime);

    doc["sensor1"] = static_cast<double>(sensorTemperatureValue1);
    doc["Sensor"] = static_cast<double>(sensorTemperatureValue2);
    doc["time"] = time;
    serializeJson(doc, output);
    delPtr(currentTime);
    return output;
}

void sendTemperature() {
    char* urlPost = getPGMString(urlPostTemperature);
    char* urlPut = getPGMString(urlPutTemperature);
    char* ct = getPGMString(contentType);
    char* aj = getPGMString(applicationJson);
    String payload = serializeTemperature();
    String urlPostString = String(urlPost);
    String urlPutString = String(urlPut);
    String ctString = String(ct);
    String ajString = String(aj);

    if (https.begin(*client, urlPostString)) {
        https.addHeader(ctString, ajString);
        int httpCode = https.POST(payload);
        if ((httpCode > 0) && (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)) {
            Serial.println(F("Значения датчиков температуры отправлены"));
        } else {
            Serial.printf_P(PSTR("postTemperature() -> Ошибка: %s\n"), HTTPClient::errorToString(httpCode).c_str());
        }
    } else {
        Serial.printf_P(PSTR("%s\n"), "postTemperature() -> Невозможно подключиться\n");
    }
    https.end();
    urlPostString.clear();

    if (https.begin(*client, urlPutString)) {
        https.addHeader(ctString, ajString);
        int httpCode = https.PUT(payload);
        if ((httpCode > 0) && (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)) {
        } else {
            Serial.printf_P(PSTR("putTemperature() -> Ошибка: %s\n"), HTTPClient::errorToString(httpCode).c_str());
        }
    } else {
        Serial.printf_P(PSTR("%s\n"), "putTemperature() -> Невозможно подключиться\n");
    }
    https.end();
    payload.clear();
    urlPutString.clear();
    ctString.clear();
    ajString.clear();

    delPtr(ct);
    delPtr(aj);
    delPtr(urlPost);
    delPtr(urlPut);
}

void timer5() {
    uptime();
    lastOnline();
    getSensorsTemperature();
    sendTemperature();
}

void startTimers() {
    Alarm.timerRepeat(5 * 60, timer5);
    Alarm.timerRepeat(60, timer1);
    Serial.printf_P(PSTR("%s %d\n"), "Всего таймеров", Alarm.count());
}

void setup() {
    Serial.begin(115200);
    Serial.println();
    initDS3231();
    initMediators();
    createDevicesAndScheduler();
    initDosersArray();
    if (!initWiFi()) {
        getParamsEEPROM();
    } else {
        initHTTPClient();
        initLocalClock();
        // syncTime();
        postBoot();
        getParamsBackEnd();
        printAllDevices();
    }

    startTimers();
    timer5();
}

void loop() {
    Alarm.delay(10);
}