#ifndef FACTORYMETHOD_DOSER_H
#define FACTORYMETHOD_DOSER_H

#include <ArduinoJson.h>
#include <string>
#include <utility>
#include "Mediator.h"
#include "Sensor.h"

class Doser : public Sensor {
   public:
    enum DoserType { K, NP, Fe };

    Doser(Mediator<Doser> mediator, std::string name, DoserType doserType) : Sensor(name, SensorType::doser) {
        mMediator = mediator;
        _doserType = doserType;
    };

    ~Doser() = default;

    void setMediator(const Mediator<Doser>& mediator) {
        mMediator = mediator;
    }

   public:
    void setEnable(bool value) {
        this->setEnabled(value);
        mMediator.Send("1", *this);
    }

    void setDirPin(uint8_t dirPin) {
        _dirPin = dirPin;
    }

    void setStepPin(uint8_t stepPin) {
        _stepPin = stepPin;
    }

    void setEnablePin(uint8_t enablePin) {
        _enablePin = enablePin;
    }

    void setSleepPin(uint8_t sleepPin) {
        _sleepPin = sleepPin;
    }

    DoserType getDoserType() const {
        return _doserType;
    }

    void setVolume(uint8_t volume) {
        _volume = volume;
    }

    void setMode0Pin(uint8_t mode0Pin) {
        _mode0_pin = mode0Pin;
    }

    void setMode1Pin(uint8_t mode1Pin) {
        _mode1_pin = mode1Pin;
    }

    void setMode2Pin(uint8_t mode2Pin) {
        _mode2_pin = mode2Pin;
    }

    uint8_t getIndex() const {
        return _index;
    }

    void setIndex(uint8_t index) {
        _index = index;
    }

    uint16_t getSteps() const {
        return _steps;
    }

    void setSteps(uint16_t steps) {
        _steps = steps;
    }

    uint8_t getDirPin() const {
        return _dirPin;
    }

    uint8_t getStepPin() const {
        return _stepPin;
    }

    uint8_t getEnablePin() const {
        return _enablePin;
    }

    uint8_t getSleepPin() const {
        return _sleepPin;
    }

    uint8_t getVolume() const {
        return _volume;
    }

    uint8_t getMode0Pin() const {
        return _mode0_pin;
    }

    uint8_t getMode1Pin() const {
        return _mode1_pin;
    }

    uint8_t getMode2Pin() const {
        return _mode2_pin;
    };

   private:
    Mediator<Doser> mMediator;
    uint8_t _dirPin;
    uint8_t _stepPin;
    uint8_t _enablePin;
    uint8_t _sleepPin;
    DoserType _doserType;
    uint8_t _volume;
    uint8_t _mode0_pin;
    uint8_t _mode1_pin;
    uint8_t _mode2_pin;
    uint8_t _index;
    uint16_t _steps;
};

#endif  // FACTORYMETHOD_DOSER_H