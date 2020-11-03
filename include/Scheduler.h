//
// Created by IT on 08.09.2020.
//

#ifndef AQUACONTROLLER_ESP8266_SCHEDULER_H
#define AQUACONTROLLER_ESP8266_SCHEDULER_H

#include <Arduino.h>
#include "TimeAlarms.h"
#include "sensor2.h"
class Scheduler {
   private:
    sensor2* _device = nullptr;
    AlarmID_t _on;
    AlarmID_t _off;

   public:
    ~Scheduler();
    Scheduler();
    sensor2* getDevice();
    void setDevice(sensor2* device);
    AlarmID_t getOn();
    void setOn(AlarmID_t on);
    AlarmID_t getOff();
    void setOff(AlarmID_t off);
};
#endif  // AQUACONTROLLER_ESP8266_SCHEDULER_H
