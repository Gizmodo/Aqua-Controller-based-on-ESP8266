#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>
#include "TimeAlarms.h"
enum ledPosition { ONE, TWO, THREE, FOUR, FIVE, SIX };
enum doserType { K, NP, Fe };
enum deviceType { Light, Compressor };
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
    deviceType device;
} ledDescription;

typedef struct doser_t {
    byte dirPin;
    byte stepPin;
    byte enablePin;
    byte sleepPin;
    doserType type;
    String name;
    byte hour, minute;
    AlarmId alarm;
    byte volume;
    byte mode0_pin;
    byte mode1_pin;
    byte mode2_pin;
    byte index;
    word steps;
} doser;

inline const char* ToString(doserType v) {
    switch (v) {
        case K:
            return "K";
        case NP:
            return "NP";
        case Fe:
            return "Fe";
        default:
            return "Unknown";
    }
}