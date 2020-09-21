#include "Device.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <string>

Device::Device(){};

Device::Device(const std::string& name,
               uint8_t pin,
               uint8_t hourOn,
               uint8_t minuteOn,
               uint8_t hourOff,
               uint8_t minuteOff,
               bool state,
               bool enabled,
               const std::string& objectId)
    : _name(name),
      _pin(pin),
      _hourOn(hourOn),
      _minuteOn(minuteOn),
      _hourOff(hourOff),
      _minuteOff(minuteOff),
      _state(state),
      _enabled(enabled),
      _objectId(objectId) {
}

void Device::setPin(uint8_t pin) {
    this->_pin = pin;
}

void Device::setName(const std::string& name) {
    this->_name = name;
}

void Device::setState(bool state) {
    this->_state = state;
}

void Device::setEnabled(bool enabled) {
    this->_enabled = enabled;
}

void Device::setObjectId(const std::string& objectId) {
    this->_objectId = objectId;
}

uint8_t Device::getPin() {
    return this->_pin;
}

std::string& Device::getName() {
    return this->_name;
}

uint8_t Device::getHourOn() const {
    return this->_hourOn;
}

uint8_t Device::getMinuteOn() const {
    return this->_minuteOn;
}

uint8_t Device::getHourOff() const {
    return this->_hourOff;
}

uint8_t Device::getMinuteOff() const {
    return this->_minuteOff;
}

bool Device::getState() {
    return this->_state;
}

bool Device::getEnabled() {
    return this->_enabled;
}

std::string& Device::getObjectId() {
    return this->_objectId;
}

std::string Device::serialize() {
    std::string output;
    const int capacity = JSON_OBJECT_SIZE(11);
    StaticJsonDocument<capacity> doc;
    doc["enabled"] = this->_enabled;
    doc["on"] = std::to_string(this->_hourOn) + ":" + std::to_string(this->_minuteOn);
    doc["off"] = std::to_string(this->_hourOff) + ":" + std::to_string(this->_minuteOff);
    doc["pin"] = this->_pin;
    doc["state"] = this->_state;
    doc["name"] = this->_name;
    serializeJson(doc, output);
    return output;
}

Device::~Device() {
}

void Device::setTimeOn(char* time) {
    splitTime(time, _hourOn, _minuteOn);
}

void Device::setTimeOff(char* time) {
    splitTime(time, _hourOff, _minuteOff);
}

void Device::splitTime(char* payload, uint8_t& hour, uint8_t& minute) {
    char* split = strtok(payload, ":");
    char* pEnd;
    hour = strtol(split, &pEnd, 10);
    split = strtok(nullptr, ":");
    minute = strtol(split, &pEnd, 10);
}
