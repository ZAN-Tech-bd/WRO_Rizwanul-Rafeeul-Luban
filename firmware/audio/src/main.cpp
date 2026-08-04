// =====================================================================
//  main.cpp — DFPlayer Mini bring-up for the Smart Museum audio node.
//
//  Wired to a hardware UART (NOT SoftwareSerial) for ESP32 reliability:
//      ESP32 GPIO14 (TX) -> DFPlayer RX
//      ESP32 GPIO13 (RX) <- DFPlayer TX
//      common GND required
//
//  Do NOT use GPIO12 for the DFPlayer TX line: GPIO12 is the MTDI strapping
//  pin that selects internal flash voltage at boot. Pulling it high forces
//  1.8V flash mode and crashes the 3.3V flash chip.
//
//  The SD card is the prepared "MSMAUDIO" card:
//      /mp3/0001.mp3 .. 0007.mp3  -> English, stations 1..7
//      /mp3/0008.mp3 .. 0014.mp3  -> Bangla,  stations 1..7
//
//  USB serial commands (115200 baud, one per line, case-insensitive):
//      LANG 0 | LANG 1        select English / Bangla
//      STATION <1..7>         play that station in the current language
//      PLAY <1..14>           play an explicit track directly
//      STOP                    stop playback
//      PAUSE | RESUME          pause / resume
//      VOL <0..30>             set volume
//      STATE                   print player state + current selection
//      HELP                    list commands
// =====================================================================

#include <Arduino.h>
#include <DFRobotDFPlayerMini.h>

// --- DFPlayer track <-> station mapping (matches MSMAUDIO card) -------
static const uint8_t STATION_COUNT  = 7;
static const char* const STATION_NAMES[STATION_COUNT] = {
  "Jamdani Saree", "Nakshi Kantha", "Liberation War", "Terracotta",
  "Ektara", "Palki", "Royal Bengal Tiger"
};

// English track = station id; Bangla track = station id + 7.
static uint8_t trackFor(uint8_t station, uint8_t language) {
  return (language == 1) ? (station + 7) : station;
}

// --- Runtime state ---------------------------------------------------
DFRobotDFPlayerMini df;
HardwareSerial dfSerial(1);

static uint8_t currentLanguage = 0;   // 0 = English, 1 = Bangla
static uint8_t currentStation  = 0;   // 0 = none
static uint8_t currentVolume   = 20;  // comfortable default
static bool    dfReady         = false;

String inputLine = "";

// --- Helper ----------------------------------------------------------
static void printHelp() {
  Serial.println(F("\n--- DFPlayer audio node ---"));
  Serial.println(F("LANG 0 | LANG 1       select English / Bangla"));
  Serial.println(F("STATION <1..7>        play station in current language"));
  Serial.println(F("PLAY <1..14>          play explicit track"));
  Serial.println(F("STOP                  stop"));
  Serial.println(F("PAUSE | RESUME        pause / resume"));
  Serial.println(F("VOL <0..30>           set volume"));
  Serial.println(F("STATE                 show current state"));
  Serial.println(F("HELP                  show this help"));
}

static void printState() {
  Serial.println(F("--- state ---"));
  Serial.print(F("DFPlayer ready : ")); Serial.println(dfReady ? F("yes") : F("no"));
  Serial.print(F("language       : ")); Serial.println(currentLanguage == 1 ? F("Bangla") : F("English"));
  Serial.print(F("station        : "));
  if (currentStation >= 1 && currentStation <= STATION_COUNT) {
    Serial.print(currentStation);
    Serial.print(F(" ("));
    Serial.print(STATION_NAMES[currentStation - 1]);
    Serial.println(F(")"));
  } else {
    Serial.println(F("none"));
  }
  Serial.print(F("volume         : ")); Serial.println(currentVolume);
  if (dfReady) {
    Serial.print(F("readType()     : 0x")); Serial.println(df.readType(), HEX);
    Serial.print(F("readState()    : ")); Serial.println(df.readState());
  }
}

// --- Command handler -------------------------------------------------
static void handleCommand(const String& raw) {
  String line = raw;
  line.trim();
  if (line.length() == 0) return;

  String cmd = line;
  cmd.toUpperCase();

  if (cmd == "HELP") {
    printHelp();
    return;
  }

  if (cmd == "STATE") {
    printState();
    return;
  }

  if (cmd == "STOP") {
    if (dfReady) { df.stop(); Serial.println(F("[ok] stop")); }
    else Serial.println(F("[err] DFPlayer not ready"));
    return;
  }

  if (cmd == "PAUSE") {
    if (dfReady) { df.pause(); Serial.println(F("[ok] pause")); }
    else Serial.println(F("[err] DFPlayer not ready"));
    return;
  }

  if (cmd == "RESUME") {
    if (dfReady) { df.start(); Serial.println(F("[ok] resume")); }
    else Serial.println(F("[err] DFPlayer not ready"));
    return;
  }

  if (cmd.startsWith("LANG")) {
    int v = line.substring(4).toInt();
    currentLanguage = (v == 1) ? 1 : 0;
    Serial.print(F("[ok] language = "));
    Serial.println(currentLanguage == 1 ? F("Bangla") : F("English"));
    return;
  }

  if (cmd.startsWith("VOL")) {
    int v = line.substring(3).toInt();
    if (v < 0) v = 0;
    if (v > 30) v = 30;
    currentVolume = (uint8_t)v;
    if (dfReady) { df.volume(currentVolume); Serial.print(F("[ok] volume = ")); Serial.println(currentVolume); }
    else Serial.println(F("[err] DFPlayer not ready"));
    return;
  }

  if (cmd.startsWith("PLAY")) {
    int t = line.substring(4).toInt();
    if (t < 1 || t > 14) { Serial.println(F("[err] track must be 1..14")); return; }
    if (!dfReady) { Serial.println(F("[err] DFPlayer not ready")); return; }
    df.playMp3Folder(t);
    Serial.print(F("[ok] play track ")); Serial.println(t);
    return;
  }

  if (cmd.startsWith("STATION")) {
    int s = line.substring(7).toInt();
    if (s < 1 || s > STATION_COUNT) { Serial.println(F("[err] station must be 1..7")); return; }
    if (!dfReady) { Serial.println(F("[err] DFPlayer not ready")); return; }
    currentStation = (uint8_t)s;
    uint8_t track = trackFor(currentStation, currentLanguage);
    df.playMp3Folder(track);
    Serial.print(F("[ok] station ")); Serial.print(currentStation);
    Serial.print(F(" (")); Serial.print(STATION_NAMES[currentStation - 1]); Serial.print(F(") -> track "));
    Serial.println(track);
    return;
  }

  Serial.print(F("[err] unknown: ")); Serial.println(line);
}

// =====================================================================
//  SETUP
// =====================================================================
void setup() {
  // USB serial for commands and logging.
  Serial.begin(115200);

  // DFPlayer on a hardware UART.
  //   ESP32 GPIO14 = TX -> DFPlayer RX
  //   ESP32 GPIO13 = RX <- DFPlayer TX
  dfSerial.begin(9600, SERIAL_8N1, DF_RX_PIN, DF_TX_PIN);

  Serial.println(F("\n[audio] DFPlayer bring-up starting..."));
  Serial.print(F("[audio] DF TX pin = ")); Serial.println(DF_TX_PIN);
  Serial.print(F("[audio] DF RX pin = ")); Serial.println(DF_RX_PIN);

  // Begin with ACK enabled, no amplifier echo.
  // Set a short timeOut so begin() does not block forever if the module
  // is slow to respond; we still accept commands after init.
  df.setTimeOut(500);
  dfReady = df.begin(dfSerial, /*ACK*/ true, /*echo*/ false);

  if (!dfReady) {
    Serial.println(F("[audio] DFPlayer NOT detected. Check wiring/power/SD card."));
    Serial.println(F("[audio] Commands will still be accepted but playback will fail."));
  } else {
    Serial.println(F("[audio] DFPlayer detected."));
    df.volume(currentVolume);
    Serial.print(F("[audio] volume set to ")); Serial.println(currentVolume);
  }

  printHelp();
  Serial.println(F("[audio] ready. Type a command and press Enter."));
}

// =====================================================================
//  LOOP
// =====================================================================
void loop() {
  // Drain the DFPlayer status/event channel so ACKs and errors are consumed.
  if (dfReady && df.available()) {
    uint8_t type = df.readType();
    int value = df.read();
    if (type == DFPlayerPlayFinished) {
      Serial.print(F("[df] finished track slot ")); Serial.println(value);
    } else if (type == DFPlayerError) {
      Serial.print(F("[df] ERROR code ")); Serial.println(value);
    }
  }

  // Read one line from USB serial.
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      handleCommand(inputLine);
      inputLine = "";
    } else if (c != '\r') {
      inputLine += c;
      if (inputLine.length() > 64) inputLine = "";  // overflow guard
    }
  }
}
