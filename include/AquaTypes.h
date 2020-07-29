#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>
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
// BackendLess Strings
const char backEndLess_0[] PROGMEM = "https://api.backendless.com";
const char backEndLess_1[] PROGMEM = "2B9D61E8-C989-5520-FFEB-A720A49C0C00";
const char backEndLess_2[] PROGMEM = "078C7D14-D7FF-42E1-95FA-A012EB826621";
const char backEndLess_3[] PROGMEM = "property";
const char backEndLess_4[] PROGMEM = "=";
const char backEndLess_5[] PROGMEM = "/";
const char backEndLess_6[] PROGMEM = "?";
const char backEndLess_7[] PROGMEM = "&";
const char backEndLess_8[] PROGMEM = "Light";
const char backEndLess_9[] PROGMEM = "enabled";
const char backEndLess_10[] PROGMEM = "name";
const char backEndLess_11[] PROGMEM = "off";
const char backEndLess_12[] PROGMEM = "on";
const char backEndLess_13[] PROGMEM = "pin";
const char backEndLess_14[] PROGMEM = "state";
const char backEndLess_15[] PROGMEM = "where";
const char backEndLess_16[] PROGMEM = "'";
const char backEndLess_17[] PROGMEM = "data";
const char* const backEndLessStrings[] PROGMEM = {backEndLess_0,  backEndLess_1,  backEndLess_2,  backEndLess_3,  backEndLess_4,
                                                  backEndLess_5,  backEndLess_6,  backEndLess_7,  backEndLess_8,  backEndLess_9,
                                                  backEndLess_10, backEndLess_11, backEndLess_12, backEndLess_13, backEndLess_14,
                                                  backEndLess_15, backEndLess_16, backEndLess_17};
const char led_0[] PROGMEM ="One";
#if __cplusplus < 201402L
namespace std {
template <class T>
struct _Unique_if {
    typedef unique_ptr<T> _Single_object;
};

template <class T>
struct _Unique_if<T[]> {
    typedef unique_ptr<T[]> _Unknown_bound;
};

template <class T, size_t N>
struct _Unique_if<T[N]> {
    typedef void _Known_bound;
};

template <class T, class... Args>
typename _Unique_if<T>::_Single_object make_unique(Args&&... args) {
    return unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template <class T>
typename _Unique_if<T>::_Unknown_bound make_unique(size_t n) {
    typedef typename remove_extent<T>::type U;
    return unique_ptr<T>(new U[n]());
}

template <class T, class... Args>
typename _Unique_if<T>::_Known_bound make_unique(Args&&...) = delete;
}  // namespace std
#endif

template <typename C, C beginVal, C endVal>
class Iterator {
    typedef typename std::underlying_type<C>::type val_t;
    int val;

   public:
    Iterator(const C& f) : val(static_cast<val_t>(f)) {
    }
    Iterator() : val(static_cast<val_t>(beginVal)) {
    }
    Iterator operator++() {
        ++val;
        return *this;
    }
    C operator*() {
        return static_cast<C>(val);
    }
    Iterator begin() {
        return *this;
    }  // default ctor is good
    Iterator end() {
        static const Iterator endIter = ++Iterator(endVal);  // cache it
        return endIter;
    }
    bool operator!=(const Iterator& i) {
        return val != i.val;
    }
};