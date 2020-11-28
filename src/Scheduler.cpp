
#include "Scheduler.h"
Scheduler::~Scheduler() = default;

Scheduler::Scheduler() = default;

Sensor* Scheduler::getDevice() {
    return this->_device;
}

void Scheduler::setDevice(Sensor* device) {
    this->_device = device;
}

AlarmID_t Scheduler::getOn() {
    return this->_on;
}

void Scheduler::setOn(AlarmID_t on) {
    this->_on = on;
}

AlarmID_t Scheduler::getOff() {
    return this->_off;
}

void Scheduler::setOff(AlarmID_t off) {
    this->_off = off;
}

AlarmID_t Scheduler::getAlarm() {
    return this->_alarm;
}

void Scheduler::setAlarm(AlarmID_t alarm) {
    this->_alarm = alarm;
}