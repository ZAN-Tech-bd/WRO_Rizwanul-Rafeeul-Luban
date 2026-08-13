#pragma once
#include <Arduino.h>
#include <esp_now.h>

// =====================================================================
//  EspNowCompat.h -- one ESP-NOW receive callback for both ESP32 cores.
//
//  Arduino-ESP32 3.x changed the callback's first argument from the plain
//  sender MAC to a richer info struct:
//     2.x:  void cb(const uint8_t *mac,             const uint8_t *data, int len)
//     3.x:  void cb(const esp_now_recv_info_t *info, const uint8_t *data, int len)
//
//  Aliasing the type lets the sketch define the callback ONCE and compile on
//  either core (PlatformIO here resolves 2.x; the Arduino IDE ships 3.x).
//
//  It lives in a header rather than an #if inside the .ino on purpose: the
//  Arduino builder auto-generates prototypes for every function it finds in
//  the .ino, ignoring preprocessor conditionals, so a guarded definition makes
//  it emit a prototype naming a type that does not exist on the other core.
//  Types pulled in from a header are always visible before those prototypes.
// =====================================================================

#if ESP_ARDUINO_VERSION_MAJOR >= 3
typedef esp_now_recv_info_t EspNowRecvInfo;
#else
typedef uint8_t             EspNowRecvInfo;   // core 2.x hands over the sender MAC
#endif
