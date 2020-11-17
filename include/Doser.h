#pragma clang diagnostic push
#pragma ide diagnostic ignored "modernize-use-nodiscard"
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#ifndef FACTORYMETHOD_DOSER_H
#define FACTORYMETHOD_DOSER_H

#include <ArduinoJson.h>
#include <string>
#include <utility>
#include "Mediator.h"
#include "Sensor.h"
#define UNDEFINED (-1)

class Doser : public Sensor {
   public:
    enum DoserType { K, NP, Fe };

   private:
    // TODO Mediator unused - doserHandler do the same things!!!
    Mediator<Doser> mMediator;
    uint8_t _dirPin = UNDEFINED;
    uint8_t _stepPin = UNDEFINED;
    uint8_t _enablePin = UNDEFINED;
    uint8_t _sleepPin = UNDEFINED;
    DoserType _doserType;
    uint8_t _volume = UNDEFINED;
    uint8_t _mode0_pin = UNDEFINED;
    uint8_t _mode1_pin = UNDEFINED;
    uint8_t _mode2_pin = UNDEFINED;
    uint8_t _index = UNDEFINED;
    uint16_t _steps = UNDEFINED;

    static std::string tail(std::string const& source, size_t const length) {
        if (length >= source.size()) {
            return source;
        }
        return source.substr(source.size() - length);
    }

   public:
    Doser(Mediator<Doser> mediator, std::string name, DoserType doserType) : Sensor(std::move(name), SensorType::doser) {
        mMediator = mediator;
        _doserType = doserType;
    };

    ~Doser() = default;

    void setMediator(const Mediator<Doser>& mediator) {
        mMediator = mediator;
    }

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

    std::string DoserInfo() {
        const uint8_t sz = 10;
        char bufferOn[sz];
        time_t time1 = this->getOn();
        const tm* tmOn = localtime(&time1);
        strftime(bufferOn, sz, "%T", tmOn);

        std::string buffer;
        buffer = "[Название] " + this->getName() + " [Тип] Дозатор" + " [ON] " + bufferOn + " [dir] " +
                 std::to_string(this->getDirPin()) + " [enable] " + std::to_string(this->getEnablePin()) + " [index] " +
                 std::to_string(this->getIndex()) + " [m0] " + std::to_string(this->getMode0Pin()) + " [m1] " +
                 std::to_string(this->getMode1Pin()) + " [m2] " + std::to_string(this->getMode2Pin()) + " [sleep] " +
                 std::to_string(this->getSleepPin()) + " [step] " + std::to_string(this->getStepPin()) + " [steps] " +
                 std::to_string(this->getSteps()) + " [ENABLED] " + std::to_string(this->getEnabled()) + " [OBJECTID] ..." +
                 tail(this->getObjectID(), 5);
        return buffer;
    }
};

#endif  // FACTORYMETHOD_DOSER_H
#pragma clang diagnostic pop