#pragma once
#include <math.h>
#define DHT11 11
extern float fakeTemps[64], fakeHumidity[64];
class DHT {
  int pin;
 public:
  DHT(int p, int type) : pin(p) { assert(type == DHT11 && ((p >= 15 && p <= 18) || p == 38 || p == 39)); }
  void begin() {}
  float readHumidity() { return fakeHumidity[pin]; }
  float readTemperature() { return fakeTemps[pin]; }
};
