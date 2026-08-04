// =====================================================================
//  main.cpp — Robot Face & Info Display (Arduino Mega 2560 + TFT shield)
//
//  The Mega is a DEDICATED RENDERER. It does not talk Wi-Fi/ESP-NOW.
//  The ESP32 robot brain sends short text commands over UART (Serial1):
//
//    FACE                          -> show the idle animated face
//    FACE <emotion>                -> happy | neutral | alert | sleepy
//    LANG <0|1>                    -> 0=English audio, 1=Bangla audio
//    STATION <id> <moving 0|1>     -> narrating/moving at artifact id 1..7
//    MENU                          -> show language-selection menu
//    EMERGENCY                     -> emergency evacuation screen
//
//  Wiring: ESP32 GPIO17 (TX) -> 1k resistor -> Mega RX1 (pin 19).
//          Common GND required.
//
//  Build & flash:  cd firmware/display && pio run -t upload
// =====================================================================

#include <Arduino.h>
#include <MCUFRIEND_kbv.h>
#include <Adafruit_GFX.h>
#include "display_config.h"
#include "robot_face.h"

MCUFRIEND_kbv tft;
RobotFace* face = nullptr;

// ---- Runtime state -------------------------------------------------
uint8_t currentScreen = SCREEN_BOOT;
uint8_t currentEmotion = EMOTION_HAPPY;
uint8_t currentLanguage = LANG_ENGLISH;
uint8_t currentStation = 0;   // 0 = none / not at a station
uint8_t robotMoving = 0;

// Separate command buffers so a floating Serial1 RX pin cannot corrupt
// USB test commands (and vice versa).
String linkLine = "";   // from Serial1 (ESP32)
String usbLine = "";    // from Serial  (USB debug/test)

// Menu selection cursor (cycle with future buttons or repeated LANG cmds).
uint8_t menuCursor = 0;

// Palette shared by info screens.
static const uint16_t COL_BG      = 0x0264;  // dark teal
static const uint16_t COL_ACCENT  = 0xD366;  // terracotta
static const uint16_t COL_GREEN   = 0x37E0;
static const uint16_t COL_RED     = 0xD800;
static const uint16_t COL_YELLOW  = 0xFD20;
static const uint16_t COL_WHITE   = 0xFFFF;
static const uint16_t COL_TEXT    = 0xFFFF;

// ---- Forward declarations ------------------------------------------
void showBoot();
void showMenu();
void showFace();
void showStation(uint8_t id, uint8_t moving);
void showEmergency();
void handleCommand(const String& line);

// =====================================================================
//  SETUP
// =====================================================================
void setup() {
  // USB serial for debugging on the laptop.
  Serial.begin(9600);

  // Serial1 = the UART link from the ESP32 robot brain (pins 18/19).
  Serial1.begin(LINK_BAUD);

  uint16_t id = tft.readID();
  // Write-only MCUFRIEND shields (no /RD pin) return 0xD3D3 or 0x00D3.
  // The library's own examples map these to ILI9481. Also catch 0/0xFFFF.
  if (id == 0x0 || id == 0xFFFF || id == 0xD3D3 || id == 0x00D3) id = 0x9481;
  tft.begin(id);
  tft.setRotation(1);   // landscape 480x320
  tft.fillScreen(COL_BG);

  face = new RobotFace(tft);
  randomSeed(analogRead(A15));   // floating pin for entropy

  Serial.println(F("[display] Mega TFT ready."));
  Serial.print(F("[display] Controller ID: 0x"));
  Serial.println(id, HEX);

  showBoot();
  delay(1200);

  // Default to the animated face.
  showFace();
}

// =====================================================================
//  LOOP — drives face animation + drains the ESP32 command channel
// =====================================================================
void loop() {
  // Read one line from the ESP32 link.
  while (Serial1.available()) {
    char c = Serial1.read();
    if (c == '\n') {
      linkLine.trim();
      if (linkLine.length() > 0) {
        handleCommand(linkLine);
      }
      linkLine = "";
    } else if (c != '\r' && c >= 32) {   // ignore control chars / noise
      linkLine += c;
      if (linkLine.length() > 64) linkLine = "";  // overflow guard
    }
  }

  // Keep the face alive. RobotFace owns a deliberate fixed-rate gaze and
  // eased blink choreography; the main loop does not continuously drive it.
  if (currentScreen == SCREEN_FACE && face) {
    face->tick();
  }

  // Also accept test commands from the USB serial (handy without an ESP32).
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      usbLine.trim();
      if (usbLine.length() > 0) {
        Serial.print(F("[usb] "));
        Serial.println(usbLine);
        handleCommand(usbLine);
      }
      usbLine = "";
    } else if (c != '\r') {
      usbLine += c;
    }
  }
}

// =====================================================================
//  Screen renderers
// =====================================================================
void showBoot() {
  currentScreen = SCREEN_BOOT;
  tft.fillScreen(COL_BG);
  tft.setTextColor(COL_ACCENT);
  tft.setTextSize(4);
  tft.setCursor(70, 120);
  tft.print(F("SMART MUSEUM"));
  tft.setTextColor(COL_WHITE);
  tft.setTextSize(2);
  tft.setCursor(130, 175);
  tft.print(F("Robot Display"));
  tft.setTextSize(1);
  tft.setCursor(150, 210);
  tft.setTextColor(COL_TEXT);
  tft.print(F("Mega 2560 + TFT loading..."));
}

void showFace() {
  currentScreen = SCREEN_FACE;
  if (face) face->drawFull(currentEmotion, currentLanguage);
}

void showMenu() {
  currentScreen = SCREEN_MENU;
  tft.fillScreen(COL_BG);

  // Title bar.
  tft.fillRect(0, 0, TFT_W, 50, COL_ACCENT);
  tft.setTextColor(COL_WHITE);
  tft.setTextSize(3);
  tft.setCursor(40, 14);
  tft.print(F("Select Language"));

  // Option 1: English.
  bool enSel = (currentLanguage == LANG_ENGLISH);
  tft.fillRoundRect(40, 90, 400, 80, 12, enSel ? COL_GREEN : COL_WHITE);
  tft.setTextColor(enSel ? COL_WHITE : 0x4228);
  tft.setTextSize(3);
  tft.setCursor(80, 118);
  tft.print(F("1. English"));
  if (enSel) {
    tft.setTextSize(2);
    tft.setCursor(360, 122);
    tft.print(F("[ON]"));
  }

  // Option 2: Bangla (audio; romanized label per D3).
  bool bnSel = (currentLanguage == LANG_BANGLA);
  tft.fillRoundRect(40, 190, 400, 80, 12, bnSel ? COL_GREEN : COL_WHITE);
  tft.setTextColor(bnSel ? COL_WHITE : 0x4228);
  tft.setTextSize(3);
  tft.setCursor(80, 218);
  tft.print(F("2. Bangla"));
  if (bnSel) {
    tft.setTextSize(2);
    tft.setCursor(340, 222);
    tft.print(F("[AUDIO]"));
  }

  // Footer hint.
  tft.setTextColor(COL_TEXT);
  tft.setTextSize(1);
  tft.setCursor(40, 295);
  tft.print(F("Send LANG 0 or LANG 1. Bangla plays via speaker (audio only)."));
}

void showStation(uint8_t id, uint8_t moving) {
  currentScreen = SCREEN_STATION;
  currentStation = id;
  robotMoving = moving;
  tft.fillScreen(COL_BG);

  const char* name = (id >= 1 && id <= STATION_COUNT) ? STATION_NAMES[id - 1] : "Unknown";
  const char* tag  = (id >= 1 && id <= STATION_COUNT) ? STATION_EMOJI[id - 1] : "?";

  // Header.
  tft.fillRect(0, 0, TFT_W, 60, moving ? COL_YELLOW : COL_GREEN);
  tft.setTextColor(moving ? 0x4228 : COL_WHITE);
  tft.setTextSize(2);
  tft.setCursor(20, 12);
  tft.print(moving ? F("MOVING TO") : F("NOW EXPLAINING"));

  // Big station number badge.
  tft.fillCircle(80, 180, 50, COL_ACCENT);
  tft.setTextColor(COL_WHITE);
  tft.setTextSize(4);
  tft.setCursor(62, 164);
  tft.print(id);

  // Station name.
  tft.setTextColor(COL_WHITE);
  tft.setTextSize(3);
  tft.setCursor(150, 150);
  tft.print(name);

  // Tag.
  tft.setTextColor(COL_ACCENT);
  tft.setTextSize(2);
  tft.setCursor(150, 195);
  tft.print(F("["));
  tft.print(tag);
  tft.print(F("]"));

  // Language reminder.
  tft.setTextColor(COL_TEXT);
  tft.setTextSize(2);
  tft.setCursor(150, 240);
  tft.print(F("Audio: "));
  tft.print(currentLanguage == LANG_BANGLA ? F("Bangla") : F("English"));
}

void showEmergency() {
  currentScreen = SCREEN_EMERGENCY;
  // Flash handled by loop drawing; here draw the static emergency screen.
  tft.fillScreen(COL_RED);
  tft.setTextColor(COL_WHITE);
  tft.setTextSize(5);
  tft.setCursor(60, 90);
  tft.print(F("EMERGENCY"));
  tft.setTextSize(2);
  tft.setCursor(40, 160);
  tft.print(F("Follow the robot to the"));
  tft.setCursor(40, 190);
  tft.print(F("nearest EXIT."));
  tft.setTextSize(2);
  tft.setCursor(40, 250);
  tft.print(F("Stay calm. Keep moving."));
}

// =====================================================================
//  Command handler — parses one text line from the ESP32
// =====================================================================
void handleCommand(const String& line) {
  // Uppercase the command word for case-insensitive matching.
  String cmd = line;
  cmd.toUpperCase();

  if (cmd.startsWith("FACE")) {
    // Optional emotion argument.
    currentEmotion = EMOTION_HAPPY;
    if (cmd.indexOf("HAPPY") > 0) currentEmotion = EMOTION_HAPPY;
    else if (cmd.indexOf("ALERT") > 0) currentEmotion = EMOTION_ALERT;
    else if (cmd.indexOf("SLEEPY") > 0) currentEmotion = EMOTION_SLEEPY;
    else if (cmd.indexOf("NEUTRAL") > 0) currentEmotion = EMOTION_NEUTRAL;
    showFace();
  }
  else if (cmd.startsWith("LANG")) {
    // LANG 0 = English, LANG 1 = Bangla (audio).
    int v = line.substring(4).toInt();
    currentLanguage = (v == 1) ? LANG_BANGLA : LANG_ENGLISH;
    // Reflect immediately on whatever screen is showing.
    if (currentScreen == SCREEN_MENU) showMenu();
    else if (currentScreen == SCREEN_FACE) showFace();
    else if (currentScreen == SCREEN_STATION) showStation(currentStation, robotMoving);
  }
  else if (cmd.startsWith("STATION")) {
    // STATION <id> <moving>
    int firstSpace = line.indexOf(' ');
    int secondSpace = line.indexOf(' ', firstSpace + 1);
    uint8_t id = 0;
    uint8_t moving = 0;
    if (firstSpace > 0) {
      id = line.substring(firstSpace + 1, secondSpace > 0 ? secondSpace : 100).toInt();
      if (secondSpace > 0) moving = line.substring(secondSpace + 1).toInt() ? 1 : 0;
    }
    currentEmotion = moving ? EMOTION_NEUTRAL : EMOTION_HAPPY;
    showStation(id, moving);
  }
  else if (cmd.startsWith("MENU")) {
    showMenu();
  }
  else if (cmd.startsWith("EMERGENCY")) {
    showEmergency();
  }
  else if (cmd.startsWith("BOOT") || cmd.startsWith("RESET")) {
    showBoot();
    delay(600);
    showFace();
  }
  else {
    // Unknown command — ignore but log to USB.
    Serial.print(F("[display] unknown: "));
    Serial.println(line);
  }
}
