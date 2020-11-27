#include "TimeAlarms.h"

#define IS_ONESHOT true  // constants used in arguments to create method
#define IS_REPEAT false

//**************************************************************
//* Alarm Class Constructor

AlarmClass::AlarmClass() {
    Mode.isEnabled = Mode.isOneShot = false;
    Mode.alarmType = dtNotAllocated;
    value = nextTrigger = 0;
    value2 = nextTrigger2 = 0;
    flag = false;
    onTickHandler = nullptr;
    onTickSensorHandler = nullptr;
    onTickSensorHandlerNew = nullptr;
}

//**************************************************************
//* Private Methods

void AlarmClass::updateNextTrigger() {
    if (Mode.isEnabled) {
        time_t timenow = time(nullptr);
        if (Mode.alarmType == dtTimer) {
            // its a timer
            nextTrigger =
                time(nullptr) + value;  // add the value to previous time (this ensures delay always at least Value seconds)
        }
        if (dtIsAlarm(Mode.alarmType) && ((nextTrigger <= timenow) || (nextTrigger2 <= timenow))) {
            // update alarm if next trigger is not yet in the future
            if (Mode.alarmType == dtExplicitAlarm) {
                // is the value a specific date and time in the future
                nextTrigger = value;  // yes, trigger on this value
            } else if (Mode.alarmType == dtDailyAlarm) {
                // if this is a daily alarm
                if ((value > 0) && (value2 > 0)) {
                    // this is a two time alarm
                    // new
                    if (value + previousMidnight(time(nullptr)) <= timenow) {
                        // tomorrow
                        nextTrigger = value + nextMidnight(timenow);
                    } else {
                        // today
                        nextTrigger = value + previousMidnight(timenow);
                    }
                    if (value2 + previousMidnight(time(nullptr)) <= timenow) {
                        // tomorrow
                        nextTrigger2 = value2 + nextMidnight(timenow);
                    } else {
                        // today
                        nextTrigger2 = value2 + previousMidnight(timenow);
                    }
                } else {
                    // old
                    if (value + previousMidnight(time(nullptr)) <= timenow) {
                        // if time has passed then set for tomorrow
                        nextTrigger = value + nextMidnight(timenow);
                    } else {
                        // set the date to today and add the time given in value
                        nextTrigger = value + previousMidnight(timenow);
                    }
                }
            } else if (Mode.alarmType == dtWeeklyAlarm) {
                // if this is a weekly alarm
                if ((value + previousSunday(time(nullptr))) <= timenow) {
                    // if day has passed then set for the next week.
                    nextTrigger = value + nextSunday(timenow);
                } else {
                    // set the date to this week today and add the time given in value
                    nextTrigger = value + previousSunday(timenow);
                }
            } else {
                // its not a recognized alarm type - this should not happen
                Mode.isEnabled = false;  // Disable the alarm
            }
        }
    }
}

//**************************************************************
//* Time Alarms Public Methods

TimeAlarmsClass::TimeAlarmsClass() {
    isServicing = false;
    for (uint8_t id = 0; id < ALARMS_COUNT; id++) {
        free(id);  // ensure all Alarms are cleared and available for allocation
    }
}

void TimeAlarmsClass::enable(AlarmID_t ID) {
    if (isAllocated(ID)) {
        if ((!(dtUseAbsoluteValue(Alarm[ID].Mode.alarmType) && (Alarm[ID].value == 0))) &&
            ((Alarm[ID].onTickHandler != nullptr) || (Alarm[ID].onTickSensorHandler != nullptr) ||
             (Alarm[ID].onTickSensorHandlerNew != nullptr))) {
            // only enable if value is non zero and a tick handler has been set
            // (is not NULL, value is non zero ONLY for dtTimer & dtExplicitAlarm
            // (the rest can have 0 to account for midnight))
            Alarm[ID].Mode.isEnabled = true;
            Alarm[ID].updateNextTrigger();  // trigger is updated whenever  this is called, even if already enabled
        } else {
            Alarm[ID].Mode.isEnabled = false;
        }
    }
}

void TimeAlarmsClass::disable(AlarmID_t ID) {
    if (isAllocated(ID)) {
        Alarm[ID].Mode.isEnabled = false;
    }
}

// write the given value to the given alarm
void TimeAlarmsClass::write(AlarmID_t ID, time_t value) {
    if (isAllocated(ID)) {
        Alarm[ID].value = value;    // note: we don't check value as we do it in enable()
        Alarm[ID].nextTrigger = 0;  // clear out previous trigger time (see issue #12)
        enable(ID);                 // update trigger time
    }
}
void TimeAlarmsClass::write2(AlarmID_t ID, time_t value) {
    if (isAllocated(ID)) {
        Alarm[ID].value2 = value;    // note: we don't check value as we do it in enable()
        Alarm[ID].nextTrigger2 = 0;  // clear out previous trigger time (see issue #12)
        enable(ID);                  // update trigger time
    }
}

// return the value for the given alarm ID
time_t TimeAlarmsClass::read(AlarmID_t ID) {
    if (isAllocated(ID)) {
        return Alarm[ID].value;
    } else {
        return INVALID_TIME;
    }
}

time_t TimeAlarmsClass::read2(AlarmID_t ID) {
    if (isAllocated(ID)) {
        return Alarm[ID].value2;
    } else {
        return INVALID_TIME;
    }
}

// return the alarm type for the given alarm ID
dtAlarmPeriod_t TimeAlarmsClass::readType(AlarmID_t ID) {
    if (isAllocated(ID)) {
        return (dtAlarmPeriod_t)Alarm[ID].Mode.alarmType;
    } else {
        return dtNotAllocated;
    }
}

void TimeAlarmsClass::free(AlarmID_t ID) {
    if (isAllocated(ID)) {
        Alarm[ID].Mode.isEnabled = false;
        Alarm[ID].Mode.alarmType = dtNotAllocated;
        Alarm[ID].onTickHandler = nullptr;
        Alarm[ID].onTickSensorHandler = nullptr;
        Alarm[ID].value = 0;
        Alarm[ID].value2 = 0;
        Alarm[ID].nextTrigger = 0;
        Alarm[ID].nextTrigger2 = 0;
        Alarm[ID].flag = false;
    }
}

// returns the number of allocated timers
uint8_t TimeAlarmsClass::count() {
    uint8_t c = 0;
    for (uint8_t id = 0; id < ALARMS_COUNT; id++) {
        if (isAllocated(id))
            c++;
    }
    return c;
}

// returns true only if id is allocated and the type is a time based alarm, returns false if not allocated or if its a timer
bool TimeAlarmsClass::isAlarm(AlarmID_t ID) {
    return (isAllocated(ID) && dtIsAlarm(Alarm[ID].Mode.alarmType));
}

// returns true if this id is allocated
bool TimeAlarmsClass::isAllocated(AlarmID_t ID) {
    return (ID < ALARMS_COUNT && Alarm[ID].Mode.alarmType != dtNotAllocated);
}

// returns the currently triggered alarm id
// returns INVALID_ALARM_ID if not invoked from within an alarm handler
AlarmID_t TimeAlarmsClass::getTriggeredAlarmId() {
    if (isServicing) {
        return servicedAlarmId;  // new private data member used instead of local loop variable i in serviceAlarms();
    } else {
        return INVALID_ALARM_ID;  // valid ids only available when servicing a callback
    }
}

// following functions are not Alarm ID specific.
void TimeAlarmsClass::delay(unsigned long ms) {
    unsigned long start = millis();
    do {
        serviceAlarms();
        yield();
    } while (millis() - start <= ms);
}

void TimeAlarmsClass::waitForDigits(uint8_t Digits, dtUnits_t Units) {
    while (Digits != getDigitsNow(Units)) {
        serviceAlarms();
    }
}

void TimeAlarmsClass::waitForRollover(dtUnits_t Units) {
    // if its just rolled over than wait for another rollover
    while (getDigitsNow(Units) == 0) {
        serviceAlarms();
    }
    waitForDigits(0, Units);
}

uint8_t TimeAlarmsClass::getDigitsNow(dtUnits_t Units) {
    time_t timenow = time(nullptr);

    if (Units == dtSecond)
        return numberOfSeconds(timenow);
    if (Units == dtMinute)
        return numberOfMinutes(timenow);
    if (Units == dtHour)
        return numberOfHours(timenow);
    if (Units == dtDay)
        return dayOfWeek(timenow);
    return 255;  // This should never happen
}

// returns isServicing
bool TimeAlarmsClass::getIsServicing() {
    return isServicing;
}

//***********************************************************
//* Private Methods

void TimeAlarmsClass::serviceAlarms() {
    if (!isServicing) {
        time_t now = time(nullptr);
        isServicing = true;
        for (uint8_t i = 0; i < ALARMS_COUNT; i++) {
            if (Alarm[i].Mode.isEnabled) {
                if ((Alarm[i].value2 > 0) && (Alarm[i].value > 0)) {
                    if (now >= Alarm[i].nextTrigger) {
                        auto TickDeviceHandlerNew = Alarm[i].onTickSensorHandlerNew;
                        if (Alarm[i].Mode.isOneShot) {
                            free(i);
                        } else {
                            Alarm[i].updateNextTrigger();
                        }
                        if (TickDeviceHandlerNew != nullptr) {
                            TickDeviceHandlerNew(Alarm[i].sensor, Alarm[i].flag);
                            Alarm[i].flag = !Alarm[i].flag;
                        }
                    }
                    if (now >= Alarm[i].nextTrigger2) {
                        auto TickDeviceHandlerNew = Alarm[i].onTickSensorHandlerNew;
                        if (Alarm[i].Mode.isOneShot) {
                            free(i);
                        } else {
                            Alarm[i].updateNextTrigger();
                        }
                        if (TickDeviceHandlerNew != nullptr) {
                            TickDeviceHandlerNew(Alarm[i].sensor, Alarm[i].flag);
                            Alarm[i].flag = !Alarm[i].flag;
                        }
                    }
                } else {
                    if (now >= Alarm[i].nextTrigger) {
                        OnTick_t TickHandler = Alarm[i].onTickHandler;
                        onTickSensor_t TickDeviceHandler = Alarm[i].onTickSensorHandler;

                        if (Alarm[i].Mode.isOneShot) {
                            free(i);
                        } else {
                            Alarm[i].updateNextTrigger();
                        }
                        if (TickHandler != nullptr) {
                            TickHandler();
                        }
                        if (TickDeviceHandler != nullptr) {
                            TickDeviceHandler(Alarm[i].sensor);
                        }
                    }
                }
            }
        }
        isServicing = false;
    }
}

// returns the absolute time of the next scheduled alarm, or 0 if none
time_t TimeAlarmsClass::getNextTrigger() {
    auto nextTrigger = (time_t)0xffffffff;  // the max time value

    for (uint8_t id = 0; id < ALARMS_COUNT; id++) {
        if (isAllocated(id)) {
            if (nextTrigger == 0) {
                nextTrigger = Alarm[id].nextTrigger;
            } else if (Alarm[id].nextTrigger < nextTrigger) {
                nextTrigger = Alarm[id].nextTrigger;
            }
        }
    }
    return nextTrigger == (time_t)0xffffffff ? 0 : nextTrigger;
}
time_t TimeAlarmsClass::getNextTrigger(AlarmID_t ID) {
    if (isAllocated(ID)) {
        return Alarm[ID].nextTrigger;
    } else {
        return 0;
    }
}
// attempt to create an alarm and return true if successful
AlarmID_t TimeAlarmsClass::create(time_t value, OnTick_t onTickHandler, bool isOneShot, dtAlarmPeriod_t alarmType) {
    time_t now = time(nullptr);
    if (!((dtIsAlarm(alarmType) && now < SECS_PER_YEAR) || (dtUseAbsoluteValue(alarmType) && (value == 0)))) {
        // only create alarm ids if the time is at least Jan 1 1971
        for (uint8_t id = 0; id < ALARMS_COUNT; id++) {
            if (Alarm[id].Mode.alarmType == dtNotAllocated) {
                // here if there is an Alarm id that is not allocated
                Alarm[id].onTickHandler = onTickHandler;
                Alarm[id].Mode.isOneShot = isOneShot;
                Alarm[id].Mode.alarmType = alarmType;
                Alarm[id].value = value;
                enable(id);
                return id;  // alarm created ok
            }
        }
    }
    return INVALID_ALARM_ID;  // no IDs available or time is invalid
}

AlarmID_t TimeAlarmsClass::createSensorAlarm(time_t value,
                                             onTickSensor_t onTickDeviceHandler,
                                             bool isOneShot,
                                             dtAlarmPeriod_t alarmType,
                                             Sensor* param) {
    time_t now = time(nullptr);

    if (!((dtIsAlarm(alarmType) && now < SECS_PER_YEAR) || (dtUseAbsoluteValue(alarmType) && (value == 0)))) {
        for (uint8_t id = 0; id < ALARMS_COUNT; id++) {
            if (Alarm[id].Mode.alarmType == dtNotAllocated) {
                Alarm[id].onTickSensorHandler = onTickDeviceHandler;
                Alarm[id].sensor = param;
                Alarm[id].Mode.isOneShot = isOneShot;
                Alarm[id].Mode.alarmType = alarmType;
                Alarm[id].value = value;
                enable(id);
                return id;
            }
        }
    }
    return INVALID_ALARM_ID;
}

AlarmID_t TimeAlarmsClass::createSensorAlarmNew(time_t value,
                                                time_t value2,
                                                onTickSensorNew_t onTickDeviceHandler,
                                                bool isOneShot,
                                                dtAlarmPeriod_t alarmType,
                                                Sensor* param,
                                                bool defaultState) {
    time_t now = time(nullptr);
    if (!((dtIsAlarm(alarmType) && now < SECS_PER_YEAR) || (dtUseAbsoluteValue(alarmType) && (value == 0) && (value2 == 0)))) {
        for (uint8_t id = 0; id < ALARMS_COUNT; id++) {
            if (Alarm[id].Mode.alarmType == dtNotAllocated) {
                Alarm[id].onTickSensorHandlerNew = onTickDeviceHandler;
                Alarm[id].sensor = param;
                Alarm[id].Mode.isOneShot = isOneShot;
                Alarm[id].Mode.alarmType = alarmType;
                Alarm[id].value = value;
                Alarm[id].value2 = value2;
                Alarm[id].flag = defaultState;
                enable(id);
                return id;
            }
        }
    }
    return INVALID_ALARM_ID;
}

// make one instance for the user to use
TimeAlarmsClass Alarm = TimeAlarmsClass();