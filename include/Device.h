#ifndef AQUACONTROLLER_DEVICE_H
#define AQUACONTROLLER_DEVICE_H
#include <Arduino.h>
#include <string>
class Device {
   public:
    enum Type { Light, Doser, Compressor };

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
    Type _type = Light;

   public:
    ~Device();
    Device();
    Device(const std::string& name,
           uint8_t pin,
           uint8_t hourOn,
           uint8_t minuteOn,
           uint8_t hourOff,
           uint8_t minuteOff,
           bool state,
           bool enabled,
           const std::string& objectId);
    void setPin(uint8_t pin);
    void setName(const std::string& name);
    void setState(bool state);
    void setEnabled(bool enabled);
    void setObjectId(const std::string& objectId);
    uint8_t getPin();
    std::string& getName();
    uint8_t getHourOn() const;
    uint8_t getMinuteOn() const;
    uint8_t getHourOff() const;
    uint8_t getMinuteOff() const;
    bool getState();
    bool getEnabled();
    std::string& getObjectId();
    std::string serialize();
    void setTimeOn(char* time);
    void setTimeOff(char* time);
    Type getType();
    void setType(Type type);

    void splitTime(char* payload, uint8_t& hour, uint8_t& minute);
};

#endif  // AQUACONTROLLER_DEVICE_H