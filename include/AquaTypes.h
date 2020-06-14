#include "TimeAlarms.h"

enum ledPosition { ONE, TWO, THREE, FOUR, FIVE, SIX };
enum doserType { K, NP, Fe };
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

typedef struct doser_t {
    byte dirPin;
    byte stepPin;
    byte enablePin;
    doserType type;
    String name;
    byte hour, minute;
    AlarmId alarm;
    byte volume;
} doser;