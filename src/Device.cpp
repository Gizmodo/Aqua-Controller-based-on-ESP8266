//
// Created by IT on 28.08.2020.
//
#include <Arduino.h>
#include "Device.h"
#include <ArduinoJson.h>
#include <string>
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
Device::Device(const std::string& name,
               uint8_t pin,
               uint8_t hourOn,
               uint8_t minuteOn,
               uint8_t hourOff,
               uint8_t minuteOff,
               bool state,
               bool enabled,
               const std::string& objectId,
               AlarmId alarmOn,
               AlarmId alarmOff)
    : _name(name),
      _pin(pin),
      _hourOn(hourOn),
      _minuteOn(minuteOn),
      _hourOff(hourOff),
      _minuteOff(minuteOff),
      _state(state),
      _enabled(enabled),
      _objectId(objectId),
      _alarmOn(alarmOn),
      _alarmOff(alarmOff) {
}

void Device::setPin(uint8_t pin) {
    this->_pin = pin;
}
void Device::setName(const std::string& name) {
    this->_name = name;
}
void Device::setHourOn(uint8_t hourOn) {
    this->_hourOn = hourOn;
}
void Device::setMinuteOn(uint8_t minuteOn) {
    this->_minuteOn = minuteOn;
}
void Device::setHourOff(uint8_t hourOff) {
    this->_hourOff = hourOff;
}
void Device::setMinuteOff(uint8_t minuteOff) {
    this->_minuteOff = minuteOff;
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
void Device::setAlarmOn(AlarmId alarmOn) {
    this->_alarmOn = alarmOn;
}
void Device::setAlarmOff(AlarmId alarmOff) {
    this->_alarmOff = alarmOff;
}
uint8_t Device::getPin() const {
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
bool Device::getState() const {
    return this->_state;
}
bool Device::getEnabled() const {
    return this->_enabled;
}
std::string& Device::getObjectId() {
    return this->_objectId;
}
AlarmId Device::getAlarmOn() const {
    return this->_alarmOn;
}
AlarmId Device::getAlarmOff() const {
    return this->_alarmOff;
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
    Alarm.free(_alarmOn);
    Alarm.free(_alarmOff);
}
#pragma clang diagnostic pop