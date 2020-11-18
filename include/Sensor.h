#pragma clang diagnostic push
#pragma ide diagnostic ignored "modernize-use-nodiscard"
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#ifndef FACTORYMETHOD_SENSOR2_H
#define FACTORYMETHOD_SENSOR2_H

#include <ArduinoJson.h>
#include <string>
#include <utility>
#include "Mediator.h"
#define UNDEFINED (-1)

class Sensor {
   public:
    enum SensorType { unknown, light, compressor, co2, doser, feeder, flow, pump, heater };

    ~Sensor() = default;

    Sensor(Mediator<Sensor> mediator, std::string name, SensorType type) {
        mMediator = mediator;
        _type = type;
        _name = std::move(name);
    }

    Sensor(std::string name, SensorType type) {
        _name = std::move(name);
        _type = type;
    }

    void setMediator(const Mediator<Sensor>& mediator) {
        mMediator = mediator;
    }

    bool isLight() {
        return (this->_type == SensorType::light);
    }

    bool isCompressor() {
        return (this->_type == SensorType::compressor);
    }

    bool isFlow() {
        return (this->_type == SensorType::flow);
    }

    bool isCO2() {
        return (this->_type == SensorType::co2);
    }

    bool isDoser() {
        return (this->_type == SensorType::doser);
    }

    void setStateNotify(bool state) {
        this->_state = state;
        mMediator.Send("1", *this);
    }

    void setEnabledNotify(bool enabled) {
        this->_enabled = enabled;
        mMediator.Send("1", *this);
    }

    void callMediator() {
        mMediator.Send("1", *this);
    }

    void setPin(uint8_t pin) {
        _pin = pin;
    }
    void setState(bool b) {
        _state = b;
    }

    void setEnabled(bool b) {
        _enabled = b;
    }

    void setObjectID(std::string objectID) {
        this->_objectID = std::move(objectID);
    }

    void setName(std::string name) {
        this->_name = std::move(name);
    }

    std::string getName() {
        return this->_name;
    }

    uint8_t getHourOn() const {
        return gmOn()->tm_hour;
    }

    uint8_t getHourOff() const {
        return gmOff()->tm_hour;
    }

    uint8_t getMinuteOn() const {
        return gmOn()->tm_min;
    }

    uint8_t getMinuteOff() const {
        return gmOff()->tm_min;
    }

    uint8_t getHourOnLocal() const {
        return localOn()->tm_hour;
    }

    uint8_t getHourOffLocal() const {
        return localOff()->tm_hour;
    }

    uint8_t getMinuteOnLocal() const {
        return localOn()->tm_min;
    }

    uint8_t getMinuteOffLocal() const {
        return localOff()->tm_min;
    }

    uint8_t getPin() const {
        return this->_pin;
    }

    bool getState() const {
        return this->_state;
    }

    bool getEnabled() const {
        return this->_enabled;
    }

    std::string getObjectID() {
        return this->_objectID;
    }

    std::string sensorInfo() {
        char bufferOn[32];
        char bufferOff[32];
        const tm* tmOn = localtime(&_on);
        strftime(bufferOn, 32, "%H:%M:%S", tmOn);
        const tm* tmOff = localtime(&_off);
        strftime(bufferOff, 32, "%H:%M:%S", tmOff);

        std::string typeName;
        typeName = sensorTypeToString(this->_type);

        std::string buffer;
        buffer = "[Название] " + this->_name + " [Тип] " + typeName + " [ON] " + bufferOn + " [OFF] " + bufferOff + " [STATE] " +
                 std::to_string(this->_state) + " [ENABLED] " + std::to_string(this->_enabled) + " [PIN] " +
                 std::to_string(this->_pin) + " [OBJECTID] ..." + tail(this->_objectID, 5);
        return buffer;
    }

    std::string serialize() {
        std::string output;

        if (_type == light) {
            const int capacity = JSON_OBJECT_SIZE(7);
            StaticJsonDocument<capacity> doc;
            doc["enabled"] = this->_enabled;
            doc["on"] = std::stoull(std::to_string(_on) + "000");
            doc["off"] = std::stoull(std::to_string(_off) + "000");
            doc["pin"] = this->_pin;
            doc["state"] = this->_state;
            doc["name"] = this->_name.c_str();
            serializeJson(doc, output);
        }

        if ((_type == compressor) || (_type == flow) || (_type == co2)) {
            const int capacity = JSON_OBJECT_SIZE(6);
            StaticJsonDocument<capacity> doc;
            doc["enabled"] = this->_enabled;
            doc["on"] = std::stoull(std::to_string(_on) + "000");
            doc["off"] = std::stoull(std::to_string(_off) + "000");
            doc["pin"] = this->_pin;
            doc["state"] = this->_state;
            serializeJson(doc, output);
        }
        return output;
    }

    bool shouldRun(uint8_t hour, uint8_t minute) const {
        uint16_t minutes = hour * 60 + minute;
        uint16_t minutesOn = getHourOnLocal() * 60 + getMinuteOnLocal();
        uint16_t minutesOff = getHourOffLocal() * 60 + getMinuteOffLocal();
        bool result;
        if ((minutes > minutesOn) && (minutes < minutesOff)) {
            result = true;
        } else {
            result = false;
        }
        return result;
    }

    time_t getOn() const {
        return this->_on;
    }

    void setOn(time_t on) {
        this->_on = on;
    }

    time_t getOff() const {
        return this->_off;
    }

    void setOff(time_t off) {
        this->_off = off;
    }

   private:
    std::string _name;
    std::string _objectID;
    SensorType _type = unknown;
    uint8_t _pin = UNDEFINED;
    //Время в UTC на включение сенсора
    time_t _on = UNDEFINED;
    //Время в UTC на выключение сенсора
    time_t _off = UNDEFINED;
    bool _state = false;
    bool _enabled = false;
    Mediator<Sensor> mMediator;

    tm* gmOn() const {
        return gmtime(&_on);
    }

    tm* gmOff() const {
        return gmtime(&_off);
    }

    tm* localOn() const {
        return localtime(&_on);
    }

    tm* localOff() const {
        return localtime(&_off);
    }

    static std::string tail(std::string const& source, size_t const length) {
        if (length >= source.size()) {
            return source;
        }
        return source.substr(source.size() - length);
    }

    static std::string sensorTypeToString(SensorType type) {
        switch (type) {
            case light:
                return "Прожектор";
            case compressor:
                return "Компрессор";
            case co2:
                return "CO2";
            case doser:
                return "Дозатор";
            case feeder:
                return "Кормушка";
            case flow:
                return "Помпа течения";
            case pump:
                return "Помпа подъёмная";
            case heater:
                return "Нагреватель";
            default:
                return "Unknown";
        }
    }
};

#endif  // FACTORYMETHOD_SENSOR2_H
#pragma clang diagnostic pop