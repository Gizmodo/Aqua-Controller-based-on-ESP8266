//
// Created by IT on 08.09.2020.
//

#ifndef AQUACONTROLLER_ESP8266_SCHEDULER_H
#define AQUACONTROLLER_ESP8266_SCHEDULER_H

#include <Arduino.h>
#include "Device.h"
#include "TimeAlarms.h"
class Scheduler {
   private:
    Device* _device = nullptr;
    AlarmID_t _on;
    AlarmID_t _off;

   public:
    ~Scheduler();
    Scheduler();
    Device* getDevice();
    void setDevice(Device* device);
    AlarmID_t getOn();
    void setOn(AlarmID_t on);
    AlarmID_t getOff();
    void setOff(AlarmID_t off);
};
#endif  // AQUACONTROLLER_ESP8266_SCHEDULER_H
