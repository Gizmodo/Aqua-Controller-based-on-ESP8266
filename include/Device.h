//
// Created by IT on 28.08.2020.
//

#ifndef AQUACONTROLLER_DEVICE_H
#define AQUACONTROLLER_DEVICE_H
#include <Arduino.h>
#include <TimeAlarms.h>
#include <string>
class Device {
   private:
    std::string _name;
    uint8_t _pin;
    uint8_t _hourOn;
    uint8_t _minuteOn;
    uint8_t _hourOff;
    uint8_t _minuteOff;
    bool _state;
    bool _enabled;
    std::string _objectId;
    AlarmId _alarmOn;
    AlarmId _alarmOff;

   public:
    ~Device();
    Device(const std::string& name,
           uint8_t pin,
           uint8_t hourOn,
           uint8_t minuteOn,
           uint8_t hourOff,
           uint8_t minuteOff,
           bool state,
           bool enabled,
           const std::string& objectId,
           AlarmId alarmOn,
           AlarmId alarmOff);

    void setPin(uint8_t pin);
    void setName(const std::string& name);
    void setHourOn(uint8_t hourOn);
    void setMinuteOn(uint8_t minuteOn);
    void setHourOff(uint8_t hourOff);
    void setMinuteOff(uint8_t minuteOff);
    void setState(bool state);
    void setEnabled(bool enabled);
    void setObjectId(const std::string& objectId);
    void setAlarmOn(AlarmId alarmOn);
    void setAlarmOff(AlarmId alarmOff);
    uint8_t getPin() const;
    std::string& getName();
    uint8_t getHourOn() const;
    uint8_t getMinuteOn() const;
    uint8_t getHourOff() const;
    uint8_t getMinuteOff() const;
    bool getState() const;
    bool getEnabled() const;
    std::string& getObjectId();
    AlarmId getAlarmOn() const;
    AlarmId getAlarmOff() const;
    std::string serialize();
};

#endif  // AQUACONTROLLER_ESP8266_DEVICE_H
