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

// ---- robot motion ----
// How long the Hub keeps showing "moving" after the last movement report.
// The robot node re-sends MSG_LOCATION while it drives, so this only expires
// once the reports stop (or after the admin "Advance robot (test)" button).
// Without it the dashboard/LED would stay stuck on "moving" forever.
static const uint32_t ROBOT_MOVE_SETTLE_MS = 2500;

// Station ids MUST match webapp/js/data.js (ARTIFACTS[].id).
// ESP-NOW nodes send a numeric stationId 1..N; the Hub maps it to these names.
static const char* STATION_IDS[STATION_COUNT] = {
  "jamdani", "nakshi", "liberation", "terracotta", "ektara", "palki", "tiger"
};

// ---- microSD card for the camera gallery (SPI / VSPI) ----
// Wiring matches the already-proven firmware/sdcard bring-up project:
//   MISO -> GPIO19   MOSI -> GPIO23   SCK -> GPIO18   CS -> GPIO5
// None of these clash with the NeoPixel (GPIO15) or the buzzer (GPIO4).
// The card is OPTIONAL: when it is missing or unreadable the Hub keeps the
// gallery in LittleFS instead, so the dashboard works either way.
static const bool     HUB_SD_ENABLED   = true;   // false = never try the card
static const uint8_t  HUB_SD_SCK_PIN   = 18;
static const uint8_t  HUB_SD_MISO_PIN  = 19;
static const uint8_t  HUB_SD_MOSI_PIN  = 23;
static const uint8_t  HUB_SD_CS_PIN    = 5;
static const uint32_t HUB_SD_FREQ_HZ   = 4000000UL;  // 4 MHz is safe on jumpers

// ---- camera gallery ----
// How many photos the dashboard keeps. Older ones are deleted from the card /
// flash automatically, newest first. A QVGA JPEG is ~2 KB, so even the LittleFS
// fallback holds the whole gallery comfortably.
static const uint8_t  GALLERY_MAX_PHOTOS = 12;   // must stay <= 32
static const char*    GALLERY_DIR        = "/photos";
