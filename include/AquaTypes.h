#include "TimeAlarms.h"

enum ledPosition { LEFT, CENTER, RIGHT };
typedef struct ledStruct_t {
    byte HOn, HOff;
    byte MOn, MOff;
    byte enabled;
    bool currentState;
    AlarmId on, off;
    byte pin;
} ledStruct;

typedef struct ledDescription_t {
    ledPosition position;
    String name;
    ledStruct_t led;
} ledDescription;

typedef struct ledState_t {
    String name;
    bool state;
} ledState;