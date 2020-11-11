//
// Created by IT on 08.09.2020.
//

#ifndef AQUACONTROLLER_ESP8266_SCHEDULER_H
#define AQUACONTROLLER_ESP8266_SCHEDULER_H

#include <Arduino.h>
#include "Sensor.h"
#include "TimeAlarms.h"
class Scheduler {
   private:
    Sensor* _device = nullptr;
    AlarmID_t _on = 255;
    AlarmID_t _off = 255;

   public:
    ~Scheduler();
    Scheduler();
    Sensor* getDevice();
    void setDevice(Sensor* device);
    AlarmID_t getOn();
    void setOn(AlarmID_t on);
    AlarmID_t getOff();
    void setOff(AlarmID_t off);
};
#endif  // AQUACONTROLLER_ESP8266_SCHEDULER_H
