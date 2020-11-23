#include <Arduino.h>
#include <ArduinoJson.h>
#include <ctime>
#include <memory>
#include <vector>
#include "Doser.h"
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
#include "DS3231.h"      // Чип RTC
#include "GBasic.h"      // DRV8825
#include "RtcDS3231.h"   // Время
#include "Shiftduino.h"  // Сдвиговый регистр
#include "TimeAlarms.h"  // Таймеры
//#include "uEEPROMLib.h"        // EEPROM
#include "uptime_formatter.h"  // Время работы
//// EEPROM
// uEEPROMLib eeprom(0x57);
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
    "Dosers?property=dir&property=enable&property=enabled&property=index&property=m0&property=m1&property=m2&property=name&"
    "property=objectId&property=on&property=sleep&property=state&property=step&property=steps&property=volume"};
const char urlCompressor[] PROGMEM = {
    "https://api.backendless.com/2B9D61E8-C989-5520-FFEB-A720A49C0C00/078C7D14-D7FF-42E1-95FA-A012EB826621/data/"
    "Compressor?property=enabled&property=off&property=on&property=pin&property=state"};
const char urlFlow[] PROGMEM = {
    "https://api.backendless.com/2B9D61E8-C989-5520-FFEB-A720A49C0C00/078C7D14-D7FF-42E1-95FA-A012EB826621/data/"
    "Flow?property=enabled&property=off&property=on&property=pin&property=state"};
const char urlCO2[] PROGMEM = {
    "https://api.backendless.com/2B9D61E8-C989-5520-FFEB-A720A49C0C00/078C7D14-D7FF-42E1-95FA-A012EB826621/data/"
    "CO2?property=enabled&property=off&property=on&property=pin&property=state"};
const char urlHeater[] PROGMEM = {
    "https://api.backendless.com/2B9D61E8-C989-5520-FFEB-A720A49C0C00/078C7D14-D7FF-42E1-95FA-A012EB826621/data/"
    "Heater?property=enabled&property=off&property=on&property=pin&property=state"};
const char urlPump[] PROGMEM = {
    "https://api.backendless.com/2B9D61E8-C989-5520-FFEB-A720A49C0C00/078C7D14-D7FF-42E1-95FA-A012EB826621/data/"
    "Pump?property=enabled&property=off&property=on&property=pin&property=state"};
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
const char urlPutCO2[] PROGMEM = {
    "https://api.backendless.com/2B9D61E8-C989-5520-FFEB-A720A49C0C00/078C7D14-D7FF-42E1-95FA-A012EB826621/data/CO2/"
    "7F3F2627-D359-4D8E-B18E-5D6527146DEA"};
const char urlPutHeater[] PROGMEM = {
    "https://api.backendless.com/2B9D61E8-C989-5520-FFEB-A720A49C0C00/078C7D14-D7FF-42E1-95FA-A012EB826621/data/Heater/"
    "61D70CF3-7187-4B50-BBF5-DE6FD3B2DC47"};
const char urlPutPump[] PROGMEM = {
    "https://api.backendless.com/2B9D61E8-C989-5520-FFEB-A720A49C0C00/078C7D14-D7FF-42E1-95FA-A012EB826621/data/Pump/"
    "A81224A3-2A4E-4881-9E8D-18BDF3DF1476"};
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

#define LIGHTS_COUNT (6)                             // Кол-во прожекторов
#define DEVICE_COUNT (LIGHTS_COUNT) + 3 + 3 + 1 + 1  // Кол-во всех устройств (нужно для Alarm'ов)
//// Дозаторы
#define DOSERS_COUNT (3)  // Кол-во дозаторов

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
// CO2
Mediator<Sensor> medCO2;
//Нагреватель
Mediator<Sensor> medHeater;
//-----------------------------------------
//Устройства
Sensor* compressor;
Sensor* flow;
Sensor* pump;
Sensor* co2;
Sensor* heater;

std::array<Doser, DOSERS_COUNT> dosers{Doser("Fe", Doser::Fe), Doser("K", Doser::K), Doser("NP", Doser::NP)};

std::array<Sensor, LIGHTS_COUNT> lights{Sensor(medLight, "", Sensor::light), Sensor(medLight, "", Sensor::light),
                                        Sensor(medLight, "", Sensor::light), Sensor(medLight, "", Sensor::light),
                                        Sensor(medLight, "", Sensor::light), Sensor(medLight, "", Sensor::light)};
//// Расписания всех устройств
std::array<Scheduler, DEVICE_COUNT> schedules;
//-----------------------------------------
#ifdef ARDUINO_ARCH_ESP32
namespace std_esp32 {
template <class T>
struct _Unique_if {
    typedef std::unique_ptr<T> _Single_object;
};

template <class T>
struct _Unique_if<T[]> {
    typedef std::unique_ptr<T[]> _Unknown_bound;
};

template <class T, size_t N>
struct _Unique_if<T[N]> {
    typedef void _Known_bound;
};

template <class T, class... Args>
typename _Unique_if<T>::_Single_object make_unique(Args&&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template <class T>
typename _Unique_if<T>::_Unknown_bound make_unique(size_t n) {
    typedef typename std::remove_extent<T>::type U;
    return std::unique_ptr<T>(new U[n]());
}

template <class T, class... Args>
typename _Unique_if<T>::_Known_bound make_unique(Args&&...) = delete;
}  // namespace std_esp32
#endif
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
    Serial.printf_P(PSTR("%s %s\n"), "Callback", device.getName().c_str());
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
    Serial.printf_P(PSTR("%s %s\n"), "Callback", device.getName().c_str());
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
    Serial.printf_P(PSTR("%s %s\n"), "Callback", device.getName().c_str());
    char* url = nullptr;
    char* ct = nullptr;
    char* aj = nullptr;
    String urlString;
    String payload;
    url = getPGMString(urlPutPump);
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

void callbackLight(Sensor device) {
    Serial.printf_P(PSTR("%s %s\n"), "Callback", device.getName().c_str());
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

void callbackCO2(Sensor device) {
    Serial.printf_P(PSTR("%s %s\n"), "Callback", device.getName().c_str());
    char* url = nullptr;
    char* ct = nullptr;
    char* aj = nullptr;
    String urlString;
    String payload;
    url = getPGMString(urlPutCO2);
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

void callbackHeater(Sensor device) {
    Serial.printf_P(PSTR("%s %s\n"), "Callback", device.getName().c_str());
    char* url = nullptr;
    char* ct = nullptr;
    char* aj = nullptr;
    String urlString;
    String payload;
    url = getPGMString(urlPutHeater);
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

void initMediators() {
    medCompressor.Register("1", callbackCompressor);
    medFlow.Register("1", callbackFlow);
    medPump.Register("1", callbackPump);
    medLight.Register("1", callbackLight);
    medCO2.Register("1", callbackCO2);
    medHeater.Register("1", callbackHeater);
}

void createDevicesAndScheduler() {
    compressor = new Sensor(medCompressor, "Компрессор", Sensor::compressor);
    flow = new Sensor(medFlow, "Помпа течения", Sensor::flow);
    pump = new Sensor(medPump, "Помпа подъёмная", Sensor::pump);
    co2 = new Sensor(medCO2, "CO2", Sensor::co2);
    heater = new Sensor(medHeater, "Нагреватель", Sensor::heater);

    for (int i = 0; i < LIGHTS_COUNT; ++i) {
        std::string buffer = "Прожектор " + std::to_string(i + 1);
        auto item = lights.at(i);
        item.setName(buffer);
        item.setMediator(medLight);
        lights.at(i) = item;
        auto schedule = schedules.at(i);
        schedule.setDevice(&(lights.at(i)));
        schedules.at(i) = schedule;
    }

    // Compressor
    auto schedule = schedules.at(6);
    schedule.setDevice(compressor);
    schedules.at(6) = schedule;

    // Flow
    schedule = schedules.at(7);
    schedule.setDevice(flow);
    schedules.at(7) = schedule;

    // CO2
    schedule = schedules.at(8);
    schedule.setDevice(co2);
    schedules.at(8) = schedule;

    // Dosers
    for (int i = 0; i < DOSERS_COUNT; ++i) {
        auto schedule_ = schedules.at(i + 9);
        schedule_.setDevice(&(dosers.at(i)));
        schedules.at(i + 9) = schedule_;
    }

    // Heater
    schedule = schedules.at(12);
    schedule.setDevice(heater);
    schedules.at(12) = schedule;

    // Pump
    schedule = schedules.at(13);
    schedule.setDevice(pump);
    schedules.at(13) = schedule;
}

void printDevice(Sensor* device) {
    if (device->isDoser()) {
        auto doser = static_cast<Doser*>(device);
        Serial.printf_P(PSTR("%s\n"), doser->DoserInfo().c_str());
    } else {
        Serial.printf_P(PSTR("%s\n"), device->sensorInfo().c_str());
    }
}

void printAllDevices() {
    Serial.printf_P(PSTR("\n%s\n"), "Вывод информации об устройствах:");
    for (auto light : lights) {
        printDevice(&light);
    }
    printDevice(compressor);
    printDevice(flow);
    printDevice(co2);

    for (auto doser : dosers) {
        printDevice(&doser);
    }

    printDevice(heater);
    printDevice(pump);
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

void doserOnHandler(Sensor* device) {
    auto sensor = static_cast<Doser*>(device);
    if (device->getEnabled()) {
        Serial.printf_P(PSTR("%s: включение\n"), sensor->getName().c_str());

        String message;
        message = "Включение дозатора " + String(sensor->getName().c_str());
        sendMessage(message);
        message.clear();
        uint16_t steps = sensor->getSteps();

        switch (sensor->getDoserType()) {
            case Doser::K:
                doserK->begin();
                doserK->setEnableActiveState(LOW);
                doserK->move(steps);
                doserK->setEnableActiveState(HIGH);
                break;
            case Doser::NP:
                doserNP->begin();
                doserNP->setEnableActiveState(LOW);
                doserNP->move(steps);
                doserNP->setEnableActiveState(HIGH);
                break;
            case Doser::Fe:
                doserFe->begin();
                doserFe->setEnableActiveState(LOW);
                doserFe->move(steps);
                doserFe->setEnableActiveState(HIGH);
                break;
            default:
                break;
        }

        Alarm.enable(findAlarmByDevice(device, true));
    } else {
        Serial.printf_P(PSTR("%s %s\n"), sensor->getName().c_str(), "нельзя изменять.");
        Alarm.disable(findAlarmByDevice(device, true));
    }
}

void deviceOnHandler(Sensor* device) {
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
    DynamicJsonDocument document(1300);
    DeserializationError error = deserializeJson(document, response);
    if (error) {
        Serial.printf_P(PSTR("%s: %s"), "Невозможно выполнить парсинг", error.c_str());
        return;
    } else {
        JsonArray array = document.as<JsonArray>();
        for (JsonObject object : array) {
            uint8_t m0 = object["m0"];
            uint8_t m1 = object["m1"];
            uint8_t m2 = object["m2"];
            uint8_t index = object["index"];
            uint8_t dir = object["dir"];
            uint16_t steps = object["steps"];
            bool enabled = object["enabled"];
            uint8_t sleep = object["sleep"];
            uint16_t volume = object["volume"];
            uint8_t enable = object["enable"];
            std::string name = object["name"];
            uint8_t step = object["step"];
            bool state = object["state"];
            std::string objectId = object["objectId"];
            std::string on = object["on"];

            on = on.substr(0, 10);
            for (auto&& doserItem : dosers) {
                if (name == doserItem.getName()) {
                    doserItem.setState(state);
                    doserItem.setEnabled(enabled);
                    doserItem.setObjectID(objectId);
                    doserItem.setOn(static_cast<time_t>(std::stoul(on)));

                    doserItem.setMode0Pin(m0);
                    doserItem.setMode1Pin(m1);
                    doserItem.setMode2Pin(m2);
                    doserItem.setIndex(index);
                    doserItem.setDirPin(dir);
                    doserItem.setSteps(steps);
                    doserItem.setSleepPin(sleep);
                    doserItem.setVolume(volume);
                    doserItem.setEnablePin(enable);
                    doserItem.setStepPin(step);
                    /*
                    #ifdef ARDUINO_ARCH_ESP32
                                        emptyDoser = std_esp32::make_unique<GBasic>(dosersArray[ind].steps, dosersArray[ind].dirPin,
                                                                                    dosersArray[ind].stepPin,
                    dosersArray[ind].enablePin, dosersArray[ind].mode0_pin, dosersArray[ind].mode1_pin, dosersArray[ind].mode2_pin,
                    shiftRegister, dosersArray[ind].index); #else emptyDoser = std::make_unique<GBasic>(dosersArray[ind].steps,
                    dosersArray[ind].dirPin, dosersArray[ind].stepPin, dosersArray[ind].enablePin, dosersArray[ind].mode0_pin,
                    dosersArray[ind].mode1_pin, dosersArray[ind].mode2_pin, shiftRegister, dosersArray[ind].index); #endif switch
                    (doserItem.type) { case K: doserK = std::move(emptyDoser); break; case NP: doserNP = std::move(emptyDoser);
                                                break;
                                            case Fe:
                                                doserFe = std::move(emptyDoser);
                                                break;
                                            default:
                                                Serial.printf_P(PSTR("%s %s"), "Неизвестный тип дозатора",
                    ToString(doserItem.type)); break;
                                        }
                                        dosersArray[ind].alarm =
                                            Alarm.alarmRepeat(dosersArray[ind].hour, dosersArray[ind].minute, 0, doserHandler,
                    dosersArray[ind]);
                                        // TODO Селать проверку на было ли уже событие в текущем дне
                                      */
                }
            }
        }
        document.shrinkToFit();
        document.clear();
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

void parseJSONCO2(const String& response) {
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

            co2->setState(state);
            co2->setEnabled(enabled);
            co2->setPin(pin);
            co2->setObjectID(objectId);
            co2->setOn(static_cast<time_t>(std::stoul(on)));
            co2->setOff(static_cast<time_t>(std::stoul(off)));
        }
        doc.shrinkToFit();
        doc.clear();
    }
}

void parseJSONHeater(const String& response) {
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

            heater->setState(state);
            heater->setEnabled(enabled);
            heater->setPin(pin);
            heater->setObjectID(objectId);
            heater->setOn(static_cast<time_t>(std::stoul(on)));
            heater->setOff(static_cast<time_t>(std::stoul(off)));
        }
        doc.shrinkToFit();
        doc.clear();
    }
}

void parseJSONPump(const String& response) {
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

            pump->setState(state);
            pump->setEnabled(enabled);
            pump->setPin(pin);
            pump->setObjectID(objectId);
            pump->setOn(static_cast<time_t>(std::stoul(on)));
            pump->setOff(static_cast<time_t>(std::stoul(off)));
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

void attachCO2Scheduler() {
    for (size_t i = 0; i < DEVICE_COUNT; i++) {
        if (!schedules.at(i).getDevice()->isCO2()) {
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

void attachDosersScheduler() {
    std::unique_ptr<GBasic> emptyDoser{};
    for (size_t i = 0; i < DEVICE_COUNT; i++) {
        if (!schedules.at(i).getDevice()->isDoser()) {
            continue;
        }
        auto schedule = schedules.at(i);
        auto sensor = static_cast<Doser*>(schedule.getDevice());

        emptyDoser = std::make_unique<GBasic>(sensor->getSteps(), sensor->getDirPin(), sensor->getStepPin(), sensor->getEnablePin(),
                                              sensor->getMode0Pin(), sensor->getMode1Pin(), sensor->getMode2Pin(), shiftRegister,
                                              sensor->getIndex());
        switch (sensor->getDoserType()) {
            case Doser::K:
                doserK = std::move(emptyDoser);
                break;
            case Doser::NP:
                doserNP = std::move(emptyDoser);
                break;
            case Doser::Fe:
                doserFe = std::move(emptyDoser);
                break;
            default:
                Serial.printf_P(PSTR("%s"), "Неизвестный тип дозатора");
                break;
        }

        auto alarmOn = Alarm.alarmRepeat(sensor->getHourOn(), sensor->getMinuteOn(), 0, doserOnHandler, sensor);

        schedule.setOn(alarmOn);
        schedules.at(i) = schedule;
        // TODO Селать проверку на было ли уже событие в текущем дне
        // (sensor->shouldRun(clockRTC.getDateTime().hour, clockRTC.getDateTime().minute)) ? deviceOnHandler(sensor)
        //                                                                                 : deviceOffHandler(sensor);
    }
}

void attachHeaterScheduler() {
    for (size_t i = 0; i < DEVICE_COUNT; i++) {
        if (!schedules.at(i).getDevice()->isHeater()) {
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

void attachPumpScheduler() {
    for (size_t i = 0; i < DEVICE_COUNT; i++) {
        if (!schedules.at(i).getDevice()->isPump()) {
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

void getParamDosers() {
    char* url = getPGMString(urlDosers);
    Serial.printf_P(PSTR(" %s\n"), "Дозаторы...");
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
        parseJSONDosers(responseString);
        responseString.clear();
        attachDosersScheduler();
    }
}

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

void getParamCO2() {
    char* url = getPGMString(urlCO2);
    Serial.printf_P(PSTR(" %s\n"), "CO2...");
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
        parseJSONCO2(responseString);
        responseString.clear();
        attachCO2Scheduler();
    }
}

void getParamHeater() {
    char* url = getPGMString(urlHeater);
    Serial.printf_P(PSTR(" %s\n"), "Нагреватель...");
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
        parseJSONHeater(responseString);
        responseString.clear();
        attachHeaterScheduler();
    }
}

void getParamPump() {
    char* url = getPGMString(urlPump);
    Serial.printf_P(PSTR(" %s\n"), "Помпа...");
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
        parseJSONPump(responseString);
        responseString.clear();
        attachPumpScheduler();
    }
}

/*
void getParamsEEPROM() {
   Serial.printf_P(PSTR("%s\n"), "Загрузка настроек из EEPROM");
   uint8_t address = 0;
   //    uint8_t szLed = sizeof(ledDescription_t);
   // uint8_t szDoser = sizeof(doser_t);
   //   ledDescription_t ledFromEEPROM;

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

    //    address -= szLed;
    for (auto& doser : dosersArray) {
         eeprom.eeprom_read<doser_t>(address, &doser);
         doser.alarm = Alarm.alarmRepeat(doser.hour, doser.minute, 0, doserOnHandler, doser);
         // TODO проверить событие не выполнялось ли уже хотя это чтение настроек
         address += szDoser + 1;
     }

    // address -= szDoser;
    // eeprom.eeprom_read<ledDescription_t>(address, &compressor);
     compressor.led.on = Alarm.alarmRepeat(compressor.led.HOn, compressor.led.MOn, 0, compressorOnHandler, compressor);
     compressor.led.off = Alarm.alarmRepeat(compressor.led.HOff, compressor.led.MOff, 0, compressorOffHandler, compressor);

     if (shouldRun(compressor)) {
         compressorOnHandler(compressor);
     } else {
         compressorOffHandler(compressor);
     }
}
void setParamsEEPROM() {
    Serial.printf_P(PSTR("%s\n"), "Запись настроек в EEPROM");
    uint8_t address = 0;
    //    uint8_t szLed = sizeof(ledDescription_t);
    // uint8_t szDoser = sizeof(doser_t);

    for (auto& led : leds) {
         eeprom.eeprom_write<ledDescription_t>(address, led);
         address += szLed + 1;
     }
    //  address -= szLed;
    for (auto& doser : dosersArray) {
        eeprom.eeprom_write<doser_t>(address, doser);
        address += szDoser + 1;
    }
    // address -= szDoser;
    //    eeprom.eeprom_write<ledDescription_t>(address, compressor);
}
*/
void getParamsBackEnd() {
    Serial.printf_P(PSTR("%s\n"), "Чтение параметров из облака");
    getParamLights();
    getParamCompressor();
    getParamFlow();
    getParamCO2();
    getParamDosers();
    getParamHeater();
    getParamPump();
    /*
    // TODO сделать сохранение в EEPROM
    setParamsEEPROM();
    */
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

void timer1() {
    getTime();
    getSensorsTemperature();
    Serial.printf_P(PSTR("%s %d\n"), "AlarmClass size ", sizeof(AlarmClass));
    Serial.printf_P(PSTR("%s %d\n"), "FreeHeap size ", ESP.getFreeHeap());
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
    if (!initWiFi()) {
        // getParamsEEPROM();
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

void callbackTest(Sensor sensor) {
}

Sensor* sensorTest;
Mediator<Sensor> medTest;
void testAlarm() {
    sensorTest = new Sensor(medTest, "test", Sensor::compressor);
    medTest.Register("1", callbackTest);
    sensorTest->setState(true);
    sensorTest->setEnabled(true);
    sensorTest->setPin(99);
    sensorTest->setObjectID("asdasdrevrev");
    sensorTest->setOn(static_cast<time_t>(std::stoul("1606162500")));
    sensorTest->setOff(static_cast<time_t>(std::stoul("1606163400")));
}