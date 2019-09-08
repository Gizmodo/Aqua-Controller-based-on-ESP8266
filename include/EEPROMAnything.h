#include <Arduino.h>  // for type definitions
/*#include <EEPROM.h>

template <class T>
int EEPROM_writeAnything(int ee, const T& value) {
    const byte* p = (const byte*)(const void*)&value;
    unsigned int i;
    for (i = 0; i < sizeof(value); i++)
        EEPROM.write(ee++, *p++);
    return i;
}

template <class T>
int EEPROM_readAnything(int ee, T& value) {
    byte* p = (byte*)(void*)&value;
    unsigned int i;
    for (i = 0; i < sizeof(value); i++)
        *p++ = EEPROM.read(ee++);
    return i;
}
*/
std::vector<String> splitStringToVector(const String& msg) {
    std::vector<String> subStrings;
    uint32_t j = 0;
    for (uint32_t i = 0; i < msg.length(); i++) {
        if (msg.charAt(i) == ':') {
            subStrings.push_back(msg.substring(j, i));
            j = i + 1;
        }
    }
    subStrings.push_back(msg.substring(j, msg.length()));  // to grab the last value of the string
    return subStrings;
}
