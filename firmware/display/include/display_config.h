#pragma once
// =====================================================================
//  display_config.h — settings for the robot-face display controller.
//  Arduino Mega 2560 + MCUFRIEND parallel TFT shield (ILI9481 480x320).
// =====================================================================
#include <Arduino.h>

// ---- Serial link with the ESP32 robot brain -------------------------
// The ESP32 TX connects to Mega RX (Serial1 / pin 19) through a 1 kOhm
// resistor (3.3 V -> 5 V level safety). The Mega replies on TX1 / pin 18.
// Baud must match robot firmware. Keep slow for AVR reliability.
static const unsigned long LINK_BAUD = 9600;

// ---- Stations: MUST match firmware/hub/include/config.h order --------
// The ESP32 sends a 1..N number; the Mega shows this English name.
// (D3 locked decision: TFT shows ENGLISH ONLY — no Bangla shaping.)
static const uint8_t STATION_COUNT = 7;
static const char* const STATION_NAMES[STATION_COUNT] = {
  "Jamdani Saree", "Nakshi Kantha", "Liberation War", "Terracotta",
  "Ektara", "Palki", "Royal Bengal Tiger"
};
static const char* const STATION_EMOJI[STATION_COUNT] = {
  "WEAVE", "STITCH", "FLAG", "CLAY", "MUSIC", "CROWN", "TIGER"
};

// ---- Robot face geometry (480x320 landscape) ------------------------
static const int16_t TFT_W = 480;
static const int16_t TFT_H = 320;

// Compact eye capsules leave breathing room for the Jamdani forehead mark,
// cheeks, and mouth. Audio language is shown on menu/station screens, not face.
static const int16_t EYE_CY       = 126;
static const int16_t EYE_LEFT_CX  = 142;
static const int16_t EYE_RIGHT_CX = TFT_W - EYE_LEFT_CX;
static const int16_t MOUTH_Y      = 228;

// ---- Display state machine ------------------------------------------
enum DisplayScreen : uint8_t {
  SCREEN_BOOT = 0,      // splash while starting
  SCREEN_MENU = 1,      // language-selection menu
  SCREEN_FACE = 2,      // idle animated face (default running state)
  SCREEN_STATION = 3,   // narrating at an artifact
  SCREEN_EMERGENCY = 4, // emergency evacuation
};

enum RobotEmotion : uint8_t {
  EMOTION_NEUTRAL = 0,
  EMOTION_HAPPY   = 1,
  EMOTION_ALERT   = 2,
  EMOTION_SLEEPY  = 3,
};

enum RobotLanguage : uint8_t {
  LANG_ENGLISH = 0,
  LANG_BANGLA  = 1,   // audio language; menu/station label is romanized on TFT
};
