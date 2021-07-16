#ifndef TimeAlarms_h
#define TimeAlarms_h

#include <Arduino.h>
#include <time.h>
#include "Sensor.h"

#define ALARMS_COUNT 25  // for esp8266 chip - max is 255

#define SECS_PER_MIN ((time_t)(60UL))
#define SECS_PER_HOUR ((time_t)(3600UL))
#define SECS_PER_DAY ((time_t)(SECS_PER_HOUR * 24UL))
#define DAYS_PER_WEEK ((time_t)(7UL))
#define SECS_PER_WEEK ((time_t)(SECS_PER_DAY * DAYS_PER_WEEK))

#define SECS_PER_YEAR ((time_t)(SECS_PER_DAY * 365UL))  // TODO: ought to handle leap years

/* Useful Macros for getting elapsed time */
#define numberOfSeconds(_time_) ((_time_) % SECS_PER_MIN)
#define numberOfMinutes(_time_) (((_time_) / SECS_PER_MIN) % SECS_PER_MIN)
#define numberOfHours(_time_) (((_time_) % SECS_PER_DAY) / SECS_PER_HOUR)
#define dayOfWeek(_time_) ((((_time_) / SECS_PER_DAY + 4) % DAYS_PER_WEEK) + 1)  // 1 = Sunday
#define elapsedSecsToday(_time_) ((_time_) % SECS_PER_DAY)                       // the number of seconds since last midnight
// The following macros are used in calculating alarms and assume the clock is set to a date later than Jan 1 1971
// Always set the correct time before setting alarms
#define previousMidnight(_time_) (((_time_) / SECS_PER_DAY) * SECS_PER_DAY)  // time at the start of the given day
#define nextMidnight(_time_) (previousMidnight(_time_) + SECS_PER_DAY)       // time at the end of the given day
#define elapsedSecsThisWeek(_time_) \
    (elapsedSecsToday(_time_) + ((dayOfWeek(_time_) - 1) * SECS_PER_DAY))  // note that week starts on day 1
#define previousSunday(_time_) ((_time_)-elapsedSecsThisWeek(_time_))      // time at the start of the week for the given time
#define nextSunday(_time_) (previousSunday(_time_) + SECS_PER_WEEK)        // time at the end of the week for the given time

typedef enum { dtSecond, dtMinute, dtHour, dtDay } dtUnits_t;

typedef struct {
    uint8_t alarmType : 4;
    bool isEnabled : 1;  // the timer is only actioned if isEnabled is true
    bool isOneShot : 1;  // the timer will be de-allocated after trigger is processed
} AlarmMode_t;

// new time based alarms should be added just before dtLastAlarmType
typedef enum {
    dtNotAllocated,
    dtTimer,
    dtExplicitAlarm,
    dtDailyAlarm,
    dtWeeklyAlarm,
    dtLastAlarmType
} dtAlarmPeriod_t;  // in the future: dtBiweekly, dtMonthly, dtAnnual

// macro to return true if the given type is a time based alarm, false if timer or not allocated
#define dtIsAlarm(_type_) ((_type_) >= dtExplicitAlarm && (_type_) < dtLastAlarmType)
#define dtUseAbsoluteValue(_type_) ((_type_) == dtTimer || (_type_) == dtExplicitAlarm)

typedef uint8_t AlarmID_t;
typedef AlarmID_t AlarmId;  // Arduino friendly name

#define INVALID_ALARM_ID 255
#define INVALID_TIME (time_t)(-1)
#define AlarmHMS(_hr_, _min_, _sec_) ((_hr_)*SECS_PER_HOUR + (_min_)*SECS_PER_MIN + (_sec_))

#include <functional>
#include "Scheduler.h"

typedef std::function<void()> OnTick_t;
typedef std::function<void(Sensor*, bool)> onTickSensorNew_t;

// class defining an alarm instance, only used by dtAlarmsClass
class AlarmClass {
   public:
    AlarmClass();
    OnTick_t onTickHandler;
    onTickSensorNew_t onTickSensorHandlerNew;
    void updateNextTrigger();
    time_t value;
    time_t value2;
    time_t nextTrigger;
    time_t nextTrigger2;
    AlarmMode_t Mode;
    Sensor* sensor = nullptr;
    bool flag = false;
};

// class containing the collection of alarms
class TimeAlarmsClass {
   private:
    AlarmClass Alarm[ALARMS_COUNT];
    void serviceAlarms();
    bool isServicing;
    uint8_t servicedAlarmId;  // the alarm currently being serviced
    AlarmID_t create(time_t value, OnTick_t onTickHandler, bool isOneShot, dtAlarmPeriod_t alarmType);
    AlarmID_t createSensorAlarmNew(time_t value,
                                   time_t value2,
                                   onTickSensorNew_t onTickDeviceHandler,
                                   bool isOneShot,
                                   dtAlarmPeriod_t alarmType,
                                   Sensor* param);
    AlarmID_t createSensorTimerNew(time_t value,
                                   onTickSensorNew_t onTickDeviceHandler,
                                   bool isOneShot,
                                   dtAlarmPeriod_t alarmType,
                                   Sensor* param);

   public:
    TimeAlarmsClass();
    // functions to create alarms and timers

    // trigger once at the given time in the future
    AlarmID_t triggerOnce(time_t value, OnTick_t onTickHandler) {
        if (value <= 0)
            return INVALID_ALARM_ID;
        return create(value, onTickHandler, true, dtExplicitAlarm);
    }

    // trigger once at given time of day
    AlarmID_t alarmOnce(time_t value, OnTick_t onTickHandler) {
        if (value <= 0 || value > SECS_PER_DAY)
            return INVALID_ALARM_ID;
        return create(value, onTickHandler, true, dtDailyAlarm);
    }
    AlarmID_t alarmOnce(const uint8_t H, const uint8_t M, const uint8_t S, OnTick_t onTickHandler) {
        return alarmOnce(AlarmHMS(H, M, S), onTickHandler);
    }

    // trigger daily at given time of day
    AlarmID_t alarmRepeat(time_t value, OnTick_t onTickHandler) {
        if (value > SECS_PER_DAY)
            return INVALID_ALARM_ID;
        return create(value, onTickHandler, false, dtDailyAlarm);
    }
    AlarmID_t alarmRepeat(time_t value, time_t value2, onTickSensorNew_t onTickBaseDeviceHandler, Sensor* param) {
        if ((value > SECS_PER_DAY) && (value2 > SECS_PER_DAY))
            return INVALID_ALARM_ID;
        return createSensorAlarmNew(value, value2, onTickBaseDeviceHandler, false, dtDailyAlarm, param);
    }
    AlarmID_t alarmRepeat(const uint8_t H1,
                          const uint8_t M1,
                          const uint8_t H2,
                          const uint8_t M2,
                          onTickSensorNew_t onTickBaseDeviceHandler,
                          Sensor* param) {
        return alarmRepeat(AlarmHMS(H1, M1, 0), AlarmHMS(H2, M2, 0), onTickBaseDeviceHandler, param);
    }
    AlarmID_t alarmRepeat(const uint8_t H, const uint8_t M, const uint8_t S, OnTick_t onTickHandler) {
        return alarmRepeat(AlarmHMS(H, M, S), onTickHandler);
    }
    // trigger at a regular interval
    AlarmID_t timerRepeat(time_t value, OnTick_t onTickHandler) {
        if (value <= 0)
            return INVALID_ALARM_ID;
        return create(value, onTickHandler, false, dtTimer);
    }
    AlarmID_t timerRepeat(const uint8_t H, const uint8_t M, const uint8_t S, OnTick_t onTickHandler) {
        return timerRepeat(AlarmHMS(H, M, S), onTickHandler);
    }
    AlarmID_t timerRepeat(time_t value, onTickSensorNew_t onTickHandler, Sensor* param) {
        if (value <= 0)
            return INVALID_ALARM_ID;
        return createSensorTimerNew(value, onTickHandler, false, dtTimer, param);
    }
    AlarmID_t timerRepeat(const uint8_t H,
                          const uint8_t M,
                          const uint8_t S,
                          onTickSensorNew_t onTickBaseDeviceHandler,
                          Sensor* param) {
        return timerRepeat(AlarmHMS(H, M, S), onTickBaseDeviceHandler, param);
    }
    void delay(unsigned long ms);

    // utility methods
    uint8_t getDigitsNow(dtUnits_t Units);  // returns the current digit value for the given time unit
    void waitForDigits(uint8_t Digits, dtUnits_t Units);
    void waitForRollover(dtUnits_t Units);

    // low level methods
    void enable(AlarmID_t ID);               // enable the alarm to trigger
    void disable(AlarmID_t ID);              // prevent the alarm from triggering
    AlarmID_t getTriggeredAlarmId();         // returns the currently triggered  alarm id
    bool getIsServicing();                   // returns isServicing
    void write(AlarmID_t ID, time_t value);  // write the value (and enable) the alarm with the given ID
    time_t read(AlarmID_t ID);               // return the value for the given timer
    dtAlarmPeriod_t readType(AlarmID_t ID);  // return the alarm type for the given alarm ID

    void free(AlarmID_t ID);  // free the id to allow its reuse

    uint8_t count();                      // returns the number of allocated timers
    time_t getNextTrigger();              // returns the time of the next scheduled alarm
    time_t getNextTrigger(AlarmID_t ID);  // returns the time of scheduled alarm
    bool isAllocated(AlarmID_t ID);       // returns true if this id is allocated
    bool isAlarm(AlarmID_t ID);           // returns true if id is for a time based alarm, false if it's a timer or not allocated
    time_t read2(AlarmID_t ID);
    void write2(AlarmID_t ID, time_t value);
};

extern TimeAlarmsClass Alarm;  // make an instance for the user

/*==============================================================================
   MACROS
  ============================================================================*/

/* public */
#define waitUntilThisSecond(_val_) waitForDigits(_val_, dtSecond)
#define waitUntilThisMinute(_val_) waitForDigits(_val_, dtMinute)
#define waitUntilThisHour(_val_) waitForDigits(_val_, dtHour)
#define waitUntilThisDay(_val_) waitForDigits(_val_, dtDay)
#define waitMinuteRollover() waitForRollover(dtSecond)
#define waitHourRollover() waitForRollover(dtMinute)
#define waitDayRollover() waitForRollover(dtHour)

#endif /* TimeAlarms_h */
