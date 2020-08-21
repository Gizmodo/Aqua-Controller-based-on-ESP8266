#include <ArduinoJson.h>
#include <EEPROM.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>
#include "DS3231.h"      // Чип RTC
#include "GBasic.h"      // DRV8825
#include "RtcDS3231.h"   // Время
#include "Shiftduino.h"  // Сдвиговый регистр
#include "TimeAlarms.h"  // Таймеры

//// Сдвиговый регистр
#define dataPin 10
#define clockPin 14
#define latchPin 16
#define countShiftRegister 4  // Кол-во сдвиговых регистров
Shiftduino shiftRegister(dataPin, clockPin, latchPin, countShiftRegister);

//// PROGMEM
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

//// WIFI
#define WORK_DEF1
#ifdef WORK_DEF
#define WIFI_SSID "Wi-Fi"
#define WIFI_PASSWORD "1357924680"
#define WIFI_RETRY 50
#else
#define WIFI_SSID "MikroTik"
#define WIFI_PASSWORD "11111111"
#define WIFI_RETRY 10
#endif
uint8_t wifiMaxTry = WIFI_RETRY;  //Попытки подключения к сети
uint8_t wifiConnectCount = 0;
const char* WiFi_hostname = "ESP8266";

//// HTTPS
std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
HTTPClient https;
String responseString = "";

//// DEVICES
//Аэратор
ledDescription_t compressor;

//Дозаторы
doser_t dosersArray[3];
std::unique_ptr<GBasic> doserK{};
std::unique_ptr<GBasic> doserNP{};
std::unique_ptr<GBasic> doserFe{};

// Прожекторы
ledDescription_t leds[6];
std::vector<ledState_t> vectorState;
typedef Iterator<ledPosition, ledPosition::ONE, ledPosition::SIX> ledPositionIterator;
typedef Iterator<doserType, doserType::K, doserType::Fe> doserTypeIterator;

//// METHODS
void _delPtr(const char* p) {
    delete[] p;
}

char* _newPtr(size_t len) {
    char* p = new char[len];
    memset(p, 0, len);
    return p;
}

char* _getPGMString(PGM_P pgm) {
    size_t len = strlen_P(pgm) + 1;
    char* buf = _newPtr(len);
    strcpy_P(buf, pgm);
    buf[len - 1] = 0;
    return buf;
}

void ledOnHandler(ledDescription& led) {
    if (led.led.enabled) {
        Serial.printf_P(PSTR("Включение прожектора PIN %d\n"), led.led.pin);
        shiftRegister.setPin(countShiftRegister, led.led.pin, HIGH);
        // TODO sendMessage(led, true); // отправка события о включении прожектора
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
        shiftRegister.setPin(countShiftRegister, led.led.pin, LOW);
        // TODO sendMessage(led, false); // отправка события о выключении прожектора
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

void compressorOnHandler(ledDescription& led) {
    if (led.led.enabled) {
        Serial.printf_P(PSTR("Включение компрессора PIN %d\n"), led.led.pin);
        shiftRegister.setPin(countShiftRegister, led.led.pin, HIGH);
        // TODO sendMessage(led, true); // отправка события о включении прожектора
        Alarm.enable(led.led.off);
        Alarm.enable(led.led.on);

        led.led.currentState = true;

        ledState_t state;
        state.name = led.name;
        state.state = true;
        vectorState.push_back(state);
    } else {
        Serial.printf_P(PSTR("%s\n"), "Компрессор не доступен для изменения состояния");
        Alarm.disable(led.led.off);
        Alarm.disable(led.led.on);
    }
}

void compressorOffHandler(ledDescription& led) {
    if (led.led.enabled) {
        Serial.printf_P(PSTR("Выключение компрессора PIN %d\n"), led.led.pin);
        shiftRegister.setPin(countShiftRegister, led.led.pin, LOW);
        // TODO sendMessage(led, false); // отправка события о выключении прожектора
        Alarm.enable(led.led.off);
        Alarm.enable(led.led.on);

        led.led.currentState = false;

        ledState_t state;
        state.name = led.name;
        state.state = false;
        vectorState.push_back(state);
    } else {
        Serial.printf_P(PSTR("%s\n"), "Компрессор не доступен для изменения состояния");
        Alarm.disable(led.led.off);
        Alarm.disable(led.led.on);
    }
}

void doserHandler(const doser& dos) {
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
    DynamicJsonDocument doc(2000);
    DeserializationError err = deserializeJson(doc, response);
    if (err) {
        Serial.print(F("parseJSONLights -> Ошибка разбора: "));
        Serial.println(err.c_str());
        return;
    } else {
        JsonArray array = doc.as<JsonArray>();
        for (JsonObject obj : array) {
            boolean enabled = obj["enabled"];
            const char* name = obj["name"];
            const char* off = obj["off"];
            const char* on = obj["on"];
            uint8_t pin = obj["pin"];
            boolean state = obj["state"];

            char* dupOn = strdup(on);
            char* dupOff = strdup(off);
            for (auto& ledItem : leds) {
                if (strcmp(name, ledItem.name.c_str()) == 0) {
                    ledItem.led.currentState = state;
                    ledItem.led.enabled = enabled;
                    ledItem.led.pin = pin;
                    splitTime(dupOn, ledItem.led.HOn, ledItem.led.MOn);
                    splitTime(dupOff, ledItem.led.HOff, ledItem.led.MOff);

                    ledItem.led.off = Alarm.alarmRepeat(ledItem.led.HOff, ledItem.led.MOff, 0, ledOffHandler, ledItem);
                    ledItem.led.on = Alarm.alarmRepeat(ledItem.led.HOn, ledItem.led.MOn, 0, ledOnHandler, ledItem);

                    uint16_t minutes = clockRTC.getDateTime().hour * 60 + clockRTC.getDateTime().minute;
                    uint16_t minutesOn = ledItem.led.HOn * 60 + ledItem.led.MOn;
                    uint16_t minutesOff = ledItem.led.HOff * 60 + ledItem.led.MOff;
                    if ((minutes > minutesOn) && (minutes < minutesOff)) {
                        ledOnHandler(ledItem);
                    } else {
                        ledOffHandler(ledItem);
                    }
                    break;
                }
            }
            free(dupOn);
            free(dupOff);
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
                    emptyDoser =
                        std::make_unique<GBasic>(dosersArray[ind].steps, dosersArray[ind].dirPin, dosersArray[ind].stepPin,
                                                 dosersArray[ind].enablePin, dosersArray[ind].mode0_pin, dosersArray[ind].mode1_pin,
                                                 dosersArray[ind].mode2_pin, shiftRegister, dosersArray[ind].index);
                    switch (doserItem.type) {
                        case K:
                            // TODO Проверить в Clion многократную передачу doser
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
                    /*
                    uint16_t minutes = clockRTC.getDateTime().hour * 60 + clockRTC.getDateTime().minute;
                    uint16_t minutesOn = led.led.HOn * 60 + led.led.MOn;
                    uint16_t minutesOff = led.led.HOff * 60 + led.led.MOff;
                    if ((minutes > minutesOn) && (minutes < minutesOff)) {
                        ledOnHandler(led);
                    } else {
                        ledOffHandler(led);
                    }
                    */
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
    DynamicJsonDocument doc(300);
    DeserializationError err = deserializeJson(doc, response);
    if (err) {
        Serial.print(F("parseJSONCompressor -> Ошибка разбора: "));
        Serial.println(err.c_str());
        return;
    } else {
        JsonArray array = doc.as<JsonArray>();
        for (JsonObject obj : array) {
            boolean enabled = obj["enabled"];
            const char* off = obj["off"];
            const char* on = obj["on"];
            uint8_t pin = obj["pin"];
            boolean state = obj["state"];

            char* dupOn = strdup(on);
            char* dupOff = strdup(off);

            compressor.led.currentState = state;
            compressor.led.enabled = enabled;
            compressor.led.pin = pin;
            splitTime(dupOn, compressor.led.HOn, compressor.led.MOn);
            splitTime(dupOff, compressor.led.HOff, compressor.led.MOff);

            compressor.led.off = Alarm.alarmRepeat(compressor.led.HOff, compressor.led.MOff, 0, compressorOffHandler, compressor);
            compressor.led.on = Alarm.alarmRepeat(compressor.led.HOn, compressor.led.MOn, 0, compressorOnHandler, compressor);

            uint16_t minutes = clockRTC.getDateTime().hour * 60 + clockRTC.getDateTime().minute;
            uint16_t minutesOn = compressor.led.HOn * 60 + compressor.led.MOn;
            uint16_t minutesOff = compressor.led.HOff * 60 + compressor.led.MOff;

            if ((minutes > minutesOn) && (minutes < minutesOff)) {
                compressorOnHandler(compressor);
            } else {
                compressorOffHandler(compressor);
            }

            free(dupOn);
            free(dupOff);
        }
        doc.shrinkToFit();
        doc.clear();
    }
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
        WiFi.hostname(WiFi_hostname);
        return true;
    }
}

void getParamLights() {
    char* url = nullptr;
    url = _getPGMString(urlLights);
    Serial.printf_P(PSTR(" %s\n"), "Прожекторы...");
    if (https.begin(*client, String(url))) {
        int httpCode = https.GET();
        if (httpCode > 0) {
            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
                responseString = https.getString();
            }
        } else {
            Serial.printf_P(PSTR("getParamLights() -> Ошибка: %s\n"), HTTPClient::errorToString(httpCode).c_str());
        }
        https.end();
    } else {
        Serial.printf_P(PSTR("%s\n"), "getParamLights() -> Невозможно подключиться\n");
    }
    _delPtr(url);
    if (responseString.isEmpty()) {
        Serial.printf_P(PSTR(" %s\n"), "getParamLights() -> Ответ пустой");
    } else {
        parseJSONLights(responseString);
        responseString.clear();
    }
}

void getParamDosers() {
    char* url = nullptr;
    url = _getPGMString(urlDosers);
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
    _delPtr(url);
    if (responseString.isEmpty()) {
        Serial.printf_P(PSTR(" %s\n"), "getParamDosers() -> Ответ пустой");
    } else {
        parseJSONDosers(responseString);
        responseString.clear();
    }
    /*
     for (doserType i : doserTypeIterator()) {
         setDoser(i);
     }
     */
}

void getParamCompressor() {
    char* url = nullptr;
    url = _getPGMString(urlCompressor);
    Serial.printf_P(PSTR(" %s\n"), "Компрессор...");
    if (https.begin(*client, String(url))) {
        int httpCode = https.GET();
        if (httpCode > 0) {
            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
                responseString = https.getString();
            }
        } else {
            Serial.printf_P(PSTR("getParamCompressor() -> Ошибка: %s\n"), HTTPClient::errorToString(httpCode).c_str());
        }
        https.end();
    } else {
        Serial.printf_P(PSTR("%s\n"), "getParamCompressor() -> Невозможно подключиться\n");
    }
    _delPtr(url);
    if (responseString.isEmpty()) {
        Serial.printf_P(PSTR(" %s\n"), "getParamCompressor() -> Ответ пустой");
    } else {
        parseJSONCompressor(responseString);
        responseString.clear();
    }
}

void getParamsBackEnd() {
    Serial.printf_P(PSTR("%s\n"), "Чтение параметров из облака");
    getParamLights();
    getParamDosers();
    getParamCompressor();
    /*
        Serial.printf_P(PSTR("%s\n"), "Запись в EEPROM");
        writeEEPROMLed();
        writeEEPROMDoser();
        writeEEPROMAir();
        */
}

void getParamsEEPROM() {
}

void setInternalClock() {
    configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
    time_t now = time(nullptr);
    uint8_t tryCount = 0;
    while (now < 8 * 3600 * 2) {
        delay(100);
        now = time(nullptr);
        tryCount++;
        if (tryCount > 50) {
            break;
        }
    }
    struct tm timeinfo {};
    gmtime_r(&now, &timeinfo);
}

void initRealTimeClock() {
    clockRTC.begin();
    char* df = clockRTC.dateFormat("H:i:s Y-m-d", clockRTC.getDateTime());
    Serial.printf_P(PSTR("Время: %s\n"), String(df).c_str());
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
    Serial.printf_P(PSTR("Синхронизация времени\n"), ntpServerName);
    udp.begin(2390);
    WiFi.hostByName(ntpServerName, timeServerIP);
    sendNTPpacket(timeServerIP);
    delay(1000);
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
        Serial.printf_P(PSTR(" RTC: %d\n"), rtcEpoch);
        Serial.printf_P(PSTR(" NTP: %d\n"), epoch);

        if (abs(rtcEpoch - epoch) > 2) {
            Serial.printf_P(PSTR(" %s"), "Обновляем RTC (разница между эпохами = ");
            if (abs(rtcEpoch - epoch) > 10000) {
                Serial.printf_P(PSTR(" %s\n"), abs(epoch - rtcEpoch));
            } else {
                Serial.printf_P(PSTR(" %s\n"), abs(rtcEpoch - epoch));
            }
            rtc.setEpoch(epoch);
        } else {
            Serial.printf_P(PSTR(" %s\n"), "Дата и время RTC не требуют синхронизации");
        }
    }
}

void initHTTPClient() {
    client->setInsecure();
    bool mfl = client->probeMaxFragmentLength("api.backendless.com", 443, 1024);
    if (mfl) {
        client->setBufferSizes(1024, 1024);
    }
}

void initCompressor() {
    compressor.name = "Компрессор";
}

void initLedsArray() {
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

void initDosersArray() {
    dosersArray[0].name = "K";
    dosersArray[0].type = K;
    dosersArray[1].name = "NP";
    dosersArray[1].type = NP;
    dosersArray[2].name = "Fe";
    dosersArray[2].type = Fe;
}

void setup() {
    Serial.begin(115200);
    Serial.println();

    //  initArrays();
    //  getTemperature();
    initRealTimeClock();
    initLedsArray();
    initDosersArray();
    initCompressor();

    if (!initWiFi()) {
        getParamsEEPROM();
    } else {
        initHTTPClient();
        getParamsBackEnd();
        syncTime();
        setInternalClock();

        // sendMessage();
        // writeBootHistory();
        // syncTime();
        // readOptionsFirebase();
        // readOptionsBEL();
        // printAll();
    }

    // startMainTimers();
    // Timer5Min();
}
// TODO Добавить вывод информации по устройствам
// TODO Добавить Uptime информацию
// TODO Отправка сообщений о событиях на backend
// TODO Чтение температуры
// TODO Таймеры
// TODO EEPROM чтение и запись

void loop() {
}