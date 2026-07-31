#pragma once
#include <Arduino.h>   // brings in uint8_t etc. (and silences the editor linter)
// =====================================================================
//  config.h  —  settings for the Museum Hub (running on the Track ESP32)
//  Edit SSID/password here, then print the matching WiFi QR (README.md).
// =====================================================================

// ---- WiFi SoftAP (the offline network phones join) ----
// Keep the password >= 8 characters. Channel MUST match every ESP-NOW node.
static const char*    HUB_SSID    = "MuseumHub";
static const char*    HUB_PASS    = "museum2026";
static const uint8_t  HUB_CHANNEL = 1;            // ESP-NOW + SoftAP share this
static const uint8_t  HUB_IP[]    = {192, 168, 4, 1};

// ---- Track hardware ----
static const uint8_t  NEOPIXEL_PIN   = 15;        // WS2812 DIN
static const uint8_t  STATION_COUNT  = 7;         // one LED per artifact station
static const uint8_t  BUZZER_PIN     = 4;         // active buzzer (HIGH = on)

// Station ids MUST match webapp/js/data.js (ARTIFACTS[].id).
// ESP-NOW nodes send a numeric stationId 1..N; the Hub maps it to these names.
static const char* STATION_IDS[STATION_COUNT] = {
  "jamdani", "nakshi", "liberation", "terracotta", "ektara", "palki", "tiger"
};
