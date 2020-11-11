#ifndef TimeAlarms_h
#define TimeAlarms_h

#include <Arduino.h>
#include <time.h>
#include "Sensor.h"
#if !defined(dtNBR_ALARMS)
#if defined(__AVR__)
#define dtNBR_ALARMS 6  // max is 255
#elif defined(ESP8266) || defined(ARDUINO_ARCH_ESP32)
#define dtNBR_ALARMS 30  // for esp8266 chip - max is 255
#else
#define dtNBR_ALARMS 12  // assume non-AVR has more memory
#endif
#endif

#define USE_SPECIALIST_METHODS  // define this for testing

#define SECS_PER_MIN ((time_t)(60UL))
#define SECS_PER_HOUR ((time_t)(3600UL))
#define SECS_PER_DAY ((time_t)(SECS_PER_HOUR * 24UL))
#define DAYS_PER_WEEK ((time_t)(7UL))
#define SECS_PER_WEEK ((time_t)(SECS_PER_DAY * DAYS_PER_WEEK))

#define SECS_PER_YEAR ((time_t)(SECS_PER_DAY * 365UL))  // TODO: ought to handle leap years

#define secs_per_year(y) ((time_t)(SECS_PER_DAY * year_lengths[isleap(y)]))
#define isleap(y) ((((y) % 4) == 0 && ((y) % 100) != 0) || ((y) % 400) == 0)

/* Useful Macros for getting elapsed time */
#define numberOfSeconds(_time_) ((_time_) % SECS_PER_MIN)
#define numberOfMinutes(_time_) (((_time_) / SECS_PER_MIN) % SECS_PER_MIN)
#define numberOfHours(_time_) (((_time_) % SECS_PER_DAY) / SECS_PER_HOUR)
#define dayOfWeek(_time_) ((((_time_) / SECS_PER_DAY + 4) % DAYS_PER_WEEK) + 1)  // 1 = Sunday
#define elapsedDays(_time_) ((_time_) / SECS_PER_DAY)                            // this is number of days since Jan 1 1970
#define elapsedSecsToday(_time_) ((_time_) % SECS_PER_DAY)                       // the number of seconds since last midnight
// The following macros are used in calculating alarms and assume the clock is set to a date later than Jan 1 1971
// Always set the correct time before settting alarms
#define previousMidnight(_time_) (((_time_) / SECS_PER_DAY) * SECS_PER_DAY)  // time at the start of the given day
#define nextMidnight(_time_) (previousMidnight(_time_) + SECS_PER_DAY)       // time at the end of the given day
#define elapsedSecsThisWeek(_time_) \
    (elapsedSecsToday(_time_) + ((dayOfWeek(_time_) - 1) * SECS_PER_DAY))  // note that week starts on day 1
#define previousSunday(_time_) ((_time_)-elapsedSecsThisWeek(_time_))      // time at the start of the week for the given time
#define nextSunday(_time_) (previousSunday(_time_) + SECS_PER_WEEK)        // time at the end of the week for the given time

static const int year_lengths[2] = {365, 366};

typedef enum { dowInvalid, dowSunday, dowMonday, dowTuesday, dowWednesday, dowThursday, dowFriday, dowSaturday } timeDayOfWeek_t;

typedef enum { dtMillisecond, dtSecond, dtMinute, dtHour, dtDay } dtUnits_t;

typedef struct {
    uint8_t alarmType : 4;  // enumeration of daily/weekly (in future:
    // biweekly/semimonthly/monthly/annual)
    // note that the current API only supports daily
    // or weekly alarm periods
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
} dtAlarmPeriod_t;  // in future: dtBiweekly, dtMonthly, dtAnnual

// macro to return true if the given type is a time based alarm, false if timer or not allocated
#define dtIsAlarm(_type_) ((_type_) >= dtExplicitAlarm && (_type_) < dtLastAlarmType)
#define dtUseAbsoluteValue(_type_) ((_type_) == dtTimer || (_type_) == dtExplicitAlarm)

typedef uint8_t AlarmID_t;
typedef AlarmID_t AlarmId;  // Arduino friendly name

#define dtINVALID_ALARM_ID 255
#define dtINVALID_TIME (time_t)(-1)
#define AlarmHMS(_hr_, _min_, _sec_) ((_hr_)*SECS_PER_HOUR + (_min_)*SECS_PER_MIN + (_sec_))

#include <functional>
#include "AquaTypes.h"
#include "Scheduler.h"

typedef std::function<void()> OnTick_t;
typedef std::function<void(uint8_t)> OnTickByte_t;
typedef std::function<void(doser_t&)> OnTickDoser_t;
typedef std::function<void(Sensor*)> onTickDevice_t;

// class defining an alarm instance, only used by dtAlarmsClass
class AlarmClass {
   public:
    AlarmClass();
    OnTick_t onTickHandler;
    OnTickByte_t onTickByteHandler;
    OnTickDoser_t onTickDoserHandler;
    onTickDevice_t onTickDeviceHandler;
    void updateNextTrigger();
    time_t value;
    time_t nextTrigger;
    AlarmMode_t Mode;
    uint8_t param_byte;
    doser_t param_doser;
    Sensor* param_Device = nullptr;
};

// class containing the collection of alarms
class TimeAlarmsClass {
   private:
    AlarmClass Alarm[dtNBR_ALARMS];
    void serviceAlarms();
    bool isServicing;
    uint8_t servicedAlarmId;  // the alarm currently being serviced
    AlarmID_t create(time_t value, OnTick_t onTickHandler, bool isOneShot, dtAlarmPeriod_t alarmType);
    AlarmID_t createbyte(time_t value, OnTickByte_t onTickByteHandler, bool isOneShot, dtAlarmPeriod_t alarmType, uint8_t param);
    AlarmID_t createdoser(time_t value, OnTickDoser_t onTickDoserHandler,
     bool isOneShot, dtAlarmPeriod_t alarmType, doser_t param);
    AlarmID_t createBaseDevice(time_t value,
                               onTickDevice_t onTickDeviceHandler,
                               bool isOneHsot,
                               dtAlarmPeriod_t alarmType,
                               Sensor* param);

   public:
    TimeAlarmsClass();
    // functions to create alarms and timers

    // trigger once at the given time in the future
    AlarmID_t triggerOnce(time_t value, OnTick_t onTickHandler) {
        if (value <= 0)
            return dtINVALID_ALARM_ID;
        return create(value, onTickHandler, true, dtExplicitAlarm);
    }

    // trigger once at given time of day
    AlarmID_t alarmOnce(time_t value, OnTick_t onTickHandler) {
        if (value <= 0 || (unsigned)value > SECS_PER_DAY)
            return dtINVALID_ALARM_ID;
        return create(value, onTickHandler, true, dtDailyAlarm);
    }
    AlarmID_t alarmOnce(const int H, const int M, const int S, OnTick_t onTickHandler) {
        return alarmOnce(AlarmHMS(H, M, S), onTickHandler);
    }

    // trigger once on a given day and time
    AlarmID_t alarmOnce(const timeDayOfWeek_t DOW, const int H, const int M, const int S, OnTick_t onTickHandler) {
        time_t value = (DOW - 1) * SECS_PER_DAY + AlarmHMS(H, M, S);
        if (value <= 0)
            return dtINVALID_ALARM_ID;
        return create(value, onTickHandler, true, dtWeeklyAlarm);
    }

    // trigger daily at given time of day
    AlarmID_t alarmRepeat(time_t value, OnTick_t onTickHandler) {
        if ((unsigned)value > SECS_PER_DAY)
            return dtINVALID_ALARM_ID;
        return create(value, onTickHandler, false, dtDailyAlarm);
    }
    AlarmID_t alarmRepeat(time_t value, OnTickByte_t onTickByteHandler, uint8_t param) {
        if ((unsigned)value > SECS_PER_DAY)
            return dtINVALID_ALARM_ID;
        return createbyte(value, onTickByteHandler, false, dtDailyAlarm, param);
    }
    AlarmID_t alarmRepeat(time_t value, OnTickDoser_t onTickDoserHandler, doser param) {
        if ((unsigned)value > SECS_PER_DAY)
            return dtINVALID_ALARM_ID;
        return createdoser(value, onTickDoserHandler, false, dtDailyAlarm, param);
    }
    AlarmID_t alarmRepeat(time_t value, onTickDevice_t onTickBaseDeviceHandler, Sensor* param) {
        if ((unsigned)value > SECS_PER_DAY)
            return dtINVALID_ALARM_ID;
        return createBaseDevice(value, onTickBaseDeviceHandler, false, dtDailyAlarm, param);
    }
    AlarmID_t alarmRepeat(const int H, const int M, const int S, OnTick_t onTickHandler) {
        return alarmRepeat(AlarmHMS(H, M, S), onTickHandler);
    }
    AlarmID_t alarmRepeat(const int H, const int M, const int S, OnTickByte_t onTickByteHandler, uint8_t param) {
        return alarmRepeat(AlarmHMS(H, M, S), onTickByteHandler, param);
    }
    AlarmID_t alarmRepeat(const int H, const int M, const int S, OnTickDoser_t onTickDoserHandler, doser param) {
        return alarmRepeat(AlarmHMS(H, M, S), onTickDoserHandler, param);
    }
    AlarmID_t alarmRepeat(const int H, const int M, const int S, onTickDevice_t onTickBaseDeviceHandler, Sensor* param) {
        return alarmRepeat(AlarmHMS(H, M, S), onTickBaseDeviceHandler, param);
    }
    // trigger weekly at a specific day and time
    AlarmID_t alarmRepeat(const timeDayOfWeek_t DOW, const int H, const int M, const int S, OnTick_t onTickHandler) {
        time_t value = (DOW - 1) * SECS_PER_DAY + AlarmHMS(H, M, S);
        if (value <= 0)
            return dtINVALID_ALARM_ID;
        return create(value, onTickHandler, false, dtWeeklyAlarm);
    }

    AlarmID_t alarmRepeat(const timeDayOfWeek_t DOW,
                          const int H,
                          const int M,
                          const int S,
                          OnTickByte_t onTickByteHandler,
                          uint8_t param) {
        time_t value = (DOW - 1) * SECS_PER_DAY + AlarmHMS(H, M, S);
        if (value <= 0)
            return dtINVALID_ALARM_ID;
        return createbyte(value, onTickByteHandler, false, dtTimer, param);
    }
    AlarmID_t alarmRepeat(const timeDayOfWeek_t DOW,
                          const int H,
                          const int M,
                          const int S,
                          OnTickDoser_t onTickDoserHandler,
                          doser param) {
        time_t value = (DOW - 1) * SECS_PER_DAY + AlarmHMS(H, M, S);
        if (value <= 0)
            return dtINVALID_ALARM_ID;
        return createdoser(value, onTickDoserHandler, false, dtTimer, param);
    }
    AlarmID_t alarmRepeat(const timeDayOfWeek_t DOW,
                          const int H,
                          const int M,
                          const int S,
                          onTickDevice_t onTickBaseDeviceHandler,
                          Sensor* param) {
        time_t value = (DOW - 1) * SECS_PER_DAY + AlarmHMS(H, M, S);
        if (value <= 0)
            return dtINVALID_ALARM_ID;
        return createBaseDevice(value, onTickBaseDeviceHandler, false, dtTimer, param);
    }

    // trigger once after the given number of seconds
    AlarmID_t timerOnce(time_t value, OnTick_t onTickHandler) {
        if (value <= 0)
            return dtINVALID_ALARM_ID;
        return create(value, onTickHandler, true, dtTimer);
    }
    AlarmID_t timerOnce(const int H, const int M, const int S, OnTick_t onTickHandler) {
        return timerOnce(AlarmHMS(H, M, S), onTickHandler);
    }
    AlarmID_t timerOnce(time_t value, OnTickByte_t onTickByteHandler, uint8_t param) {
        if (value <= 0)
            return dtINVALID_ALARM_ID;
        return createbyte(value, onTickByteHandler, true, dtTimer, param);
    }
    AlarmID_t timerOnce(time_t value, OnTickDoser_t onTickDoserHandler, doser param) {
        if (value <= 0)
            return dtINVALID_ALARM_ID;
        return createdoser(value, onTickDoserHandler, true, dtTimer, param);
    }
    AlarmID_t timerOnce(time_t value, onTickDevice_t onTickBaseDeviceHandler, Sensor* param) {
        if (value <= 0)
            return dtINVALID_ALARM_ID;
        return createBaseDevice(value, onTickBaseDeviceHandler, true, dtTimer, param);
    }
    // trigger at a regular interval
    AlarmID_t timerRepeat(time_t value, OnTick_t onTickHandler) {
        if (value <= 0)
            return dtINVALID_ALARM_ID;
        return create(value, onTickHandler, false, dtTimer);
    }
    AlarmID_t timerRepeat(const int H, const int M, const int S, OnTick_t onTickHandler) {
        return timerRepeat(AlarmHMS(H, M, S), onTickHandler);
    }

    AlarmID_t timerRepeat(time_t value, OnTickByte_t onTickByteHandler, uint8_t param) {
        if (value <= 0)
            return dtINVALID_ALARM_ID;
        return createbyte(value, onTickByteHandler, false, dtTimer, param);
    }
    AlarmID_t timerRepeat(const int H, const int M, const int S, OnTickByte_t onTickByteHandler, uint8_t param) {
        return timerRepeat(AlarmHMS(H, M, S), onTickByteHandler, param);
    }
    AlarmID_t timerRepeat(time_t value, OnTickDoser_t onTickDoserHandler, doser param) {
        if (value <= 0)
            return dtINVALID_ALARM_ID;
        return createdoser(value, onTickDoserHandler, false, dtTimer, param);
    }
    AlarmID_t timerRepeat(time_t value, onTickDevice_t onTickBaseDeviceHandler, Sensor* param) {
        if (value <= 0)
            return dtINVALID_ALARM_ID;
        return createBaseDevice(value, onTickBaseDeviceHandler, false, dtTimer, param);
    }
    AlarmID_t timerRepeat(const int H, const int M, const int S, onTickDevice_t onTickBaseDeviceHandler, Sensor* param) {
        return timerRepeat(AlarmHMS(H, M, S), onTickBaseDeviceHandler, param);
    }
    AlarmID_t timerRepeat(const int H, const int M, const int S, OnTickDoser_t onTickDoserHandler, doser param) {
        return timerRepeat(AlarmHMS(H, M, S), onTickDoserHandler, param);
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

#ifndef USE_SPECIALIST_METHODS
   private:  // the following methods are for testing and are not documented as part of the standard library
#endif
    uint8_t count();                      // returns the number of allocated timers
    time_t getNextTrigger();              // returns the time of the next scheduled alarm
    time_t getNextTrigger(AlarmID_t ID);  // returns the time of scheduled alarm
    bool isAllocated(AlarmID_t ID);       // returns true if this id is allocated
    bool isAlarm(AlarmID_t ID);           // returns true if id is for a time based alarm, false if its a timer or not allocated
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
