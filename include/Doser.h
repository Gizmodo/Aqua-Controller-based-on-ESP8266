
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