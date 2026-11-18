#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string>
#include <cstdio>
#include <cassert>
#include <type_traits>
#define HIGH 1
#define LOW 0
#define INPUT 1
#define INPUT_PULLUP 2
#define ADC_11db 3

extern uint32_t fakeMillis;
extern int fakeLevels[64];
extern int fakeAdc[64];
inline uint32_t millis() { return fakeMillis; }
inline void pinMode(int pin, int mode) { assert((pin == 41 || pin == 42 || pin == 3) && mode == INPUT_PULLUP); }
inline int digitalRead(int pin) { return fakeLevels[pin]; }
inline void analogReadResolution(int bits) { assert(bits == 12); }
inline void analogSetPinAttenuation(int pin, int) { assert(pin >= 1 && pin <= 7 && pin != 3); }
inline int analogRead(int pin) { analogSetPinAttenuation(pin, ADC_11db); return fakeAdc[pin]; }
inline int analogReadMilliVolts(int pin) { analogSetPinAttenuation(pin, ADC_11db); return pin * 50; }

struct SerialStub {
  template<class... Args> void printf(const char*, Args...) {}
  void println(const char*) {}
};
extern SerialStub Serial;

class String : public std::string {
 public:
  using std::string::string;
  using std::string::operator+=;
  String(const std::string& value) : std::string(value) {}
  String(float value, unsigned digits) {
    char out[64]; std::snprintf(out, sizeof(out), "%.*f", digits, value); assign(out);
  }
  template<class T, typename std::enable_if<std::is_arithmetic<T>::value, int>::type = 0>
  String(T value) : std::string(std::to_string(value)) {}
  String& operator+=(const String& value) { append(value); return *this; }
  template<class T> typename std::enable_if<std::is_arithmetic<T>::value, String&>::type
  operator+=(T value) { append(std::to_string(value)); return *this; }
};
