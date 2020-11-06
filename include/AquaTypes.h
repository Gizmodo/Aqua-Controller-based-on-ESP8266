#include <memory>
#include "TimeAlarms.h"
#ifdef ARDUINO_ARCH_ESP32
namespace std_esp32 {
template <class T>
struct _Unique_if {
    typedef std::unique_ptr<T> _Single_object;
};

template <class T>
struct _Unique_if<T[]> {
    typedef std::unique_ptr<T[]> _Unknown_bound;
};

template <class T, size_t N>
struct _Unique_if<T[N]> {
    typedef void _Known_bound;
};

template <class T, class... Args>
typename _Unique_if<T>::_Single_object make_unique(Args&&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template <class T>
typename _Unique_if<T>::_Unknown_bound make_unique(size_t n) {
    typedef typename std::remove_extent<T>::type U;
    return std::unique_ptr<T>(new U[n]());
}

template <class T, class... Args>
typename _Unique_if<T>::_Known_bound make_unique(Args&&...) = delete;
}  // namespace std_esp32
#endif
enum doserType { K, NP, Fe };

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