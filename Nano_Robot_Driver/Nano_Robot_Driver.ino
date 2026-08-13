/*
  Arduino Nano - ESP32 RFID start/stop motor drive  (SENSOR-FREE)
  ==============================================================
  No IR sensors. No line following. No telemetry. The ESP32 (ESP32_Robot_Main,
  UNCHANGED) reads an RFID card and sends START or STOP over UART; this Nano
  drives the two L298N channels forward at the calibrated pair, or hard-brakes.

  ---------------- UART contract (matches the existing ESP32) -------------
     9600 baud, 8N1, newline-terminated, case-insensitive.
     "START"/"F"/"G"/"W"/"1" -> drive forward at ML/MR  (replies STATE:RUNNING)
     "STOP"/"S"/"X"/"0"      -> hard brake             (replies STATE:STOPPED)
     At boot: brake, then print READY:STOPPED.
  Anything else is NOT motion: unknown input prints ERR:UNKNOWN and changes
  nothing. This build never toggles on random text.

  ---------------- Optional tuning (ESP32 /drive page, still unchanged) ----
     "GET"                       -> "TUNE ML=<v> MR=<v>"
     "SET ML <v>" / "SET MR <v>" -> set a channel PWM (0..255), applied live
     "MAN"                       -> drive (= START)
     "SAVE" / "LOAD"             -> persist / reload ML,MR to EEPROM
  (There is no RAMP and no sensor/telemetry output in this build.)

  ---------------- Wiring (calibrated physical L298N channels) -----------
     Channel A ("150" side):  ENA=D10, IN1=D8,  IN2=D7
     Channel B ("210" side):  ENB=D5,  IN3=D11, IN4=D12
     Remove ENA/ENB jumpers; common ground with the L298N.
     NOTE: D0/D1 are also USB upload/monitor -- disconnect the two ESP32 wires
           while flashing the Nano, then reconnect.

  BRAKE is active: both IN pins HIGH + ENA/ENB=255 shorts each motor and stops
  it hard. (Writing PWM 0 would disable the bridge and let the robot coast.)
*/

#include <EEPROM.h>

// ---- L298N physical channels ----
#define ENA 10   // channel A PWM (ML) -> the "150" side
#define IN1 8
#define IN2 7
#define ENB 5    // channel B PWM (MR) -> the "210" side
#define IN3 11
#define IN4 12

// Calibrated on this chassis. Absolute PWM 0..255, NOT %.
int man_left  = 150;   // ML -> channel A (ENA)
int man_right = 210;   // MR -> channel B (ENB)
const int MAX_PWM = 255;

const unsigned long LINK_BAUD = 9600;   // MUST match the ESP32 side
bool driving = false;                   // true while propelling forward

char rxBuf[24];
uint8_t rxLen = 0;

// ---- tunable table (GET / SET / SAVE / LOAD) ----
struct TuneEntry { const char *key; int *value; };
const TuneEntry tuneTable[] = {
  {"ML", &man_left},
  {"MR", &man_right},
};
const uint8_t TUNE_COUNT = sizeof(tuneTable) / sizeof(tuneTable[0]);
// Bump if the layout changes; a mismatch ignores stale EEPROM.
const uint16_t TUNE_MAGIC = 0x524C;   // 'RL'


void setup() {
  Serial.begin(LINK_BAUD);

  pinMode(ENA, OUTPUT); pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT); pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);

  brake();                              // start stopped (safe)
  tuneLoad();                           // reload last saved ML/MR if any

  Serial.println(F("READY:STOPPED"));   // tells the ESP32 we are alive & idle
}


void loop() {
  handleSerial();
  // Motors hold their last state (set in the command handlers). Nothing else
  // to do here -- this is a pure START/STOP slave with no sensors.
}


// ============================
// UART command handling
// ============================
void handleSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      rxBuf[rxLen] = '\0';
      for (uint8_t i = 0; i < rxLen; i++)            // case-insensitive dispatch
        rxBuf[i] = toupper((unsigned char)rxBuf[i]);
      processCommand(rxBuf);
      rxLen = 0;
      continue;
    }
    if (rxLen < sizeof(rxBuf) - 1) rxBuf[rxLen++] = c;   // guard the buffer
  }
}


void processCommand(const char *cmd) {
  if (cmd[0] == '\0') return;

  // --- motion (the only thing that moves the robot) ---
  // ESP32 sends "START"/"STOP"; the short aliases (F/G/W, S/X) are for manual
  // testing from a Serial Monitor.
  if (strcmp(cmd, "START") == 0 || strcmp(cmd, "MAN") == 0 ||
      strcmp(cmd, "1") == 0 || strcmp(cmd, "F") == 0 ||
      strcmp(cmd, "G") == 0 || strcmp(cmd, "W") == 0) {
    driving = true;
    driveChannels(man_left, man_right);
    Serial.println(F("STATE:RUNNING"));
    return;
  }
  if (strcmp(cmd, "STOP") == 0 || strcmp(cmd, "0") == 0 ||
      strcmp(cmd, "S") == 0 || strcmp(cmd, "X") == 0) {
    driving = false;
    brake();
    Serial.println(F("STATE:STOPPED"));
    return;
  }

  // --- optional tuning (does not move the robot unless already driving) ---
  if (strncmp(cmd, "SET ", 4) == 0) { tuneSet(cmd + 4); return; }
  if (strcmp(cmd, "GET") == 0)      { tuneReport(); return; }
  if (strcmp(cmd, "SAVE") == 0)     { tuneSave(); Serial.println(F("SAVED")); return; }
  if (strcmp(cmd, "LOAD") == 0)     {
    Serial.println(tuneLoad() ? F("LOADED") : F("LOAD:NONE"));
    tuneReport();
    return;
  }

  // Unknown / malformed: report and DO NOT change motion state.
  Serial.print(F("ERR:UNKNOWN "));
  Serial.println(cmd);
}


// ============================
// tuning table
// ============================
void tuneReport() {
  Serial.print(F("TUNE"));
  for (uint8_t i = 0; i < TUNE_COUNT; i++) {
    Serial.print(' ');
    Serial.print(tuneTable[i].key);
    Serial.print('=');
    Serial.print(*tuneTable[i].value);
  }
  Serial.println();
}

void tuneSet(const char *arg) {
  char key[8];
  uint8_t k = 0;
  while (arg[k] && arg[k] != ' ' && k < sizeof(key) - 1) { key[k] = arg[k]; k++; }
  key[k] = '\0';

  if (arg[k] != ' ') { Serial.println(F("ERR:SYNTAX")); return; }
  int value = atoi(arg + k + 1);

  for (uint8_t i = 0; i < TUNE_COUNT; i++) {
    if (strcmp(key, tuneTable[i].key) == 0) {
      *tuneTable[i].value = constrain(value, 0, MAX_PWM);
      Serial.print(F("OK "));
      Serial.print(key);
      Serial.print('=');
      Serial.println(*tuneTable[i].value);
      if (driving) driveChannels(man_left, man_right);   // live retune
      tuneReport();
      return;
    }
  }
  Serial.println(F("ERR:KEY"));
}

void tuneSave() {
  int addr = 0;
  EEPROM.put(addr, TUNE_MAGIC);              // put() uses update(): no needless wear
  addr += sizeof(TUNE_MAGIC);
  for (uint8_t i = 0; i < TUNE_COUNT; i++) {
    int16_t v = *tuneTable[i].value;
    EEPROM.put(addr, v);
    addr += sizeof(v);
  }
}

bool tuneLoad() {
  int addr = 0;
  uint16_t magic = 0;
  EEPROM.get(addr, magic);
  if (magic != TUNE_MAGIC) return false;     // never saved, or layout changed
  addr += sizeof(magic);
  for (uint8_t i = 0; i < TUNE_COUNT; i++) {
    int16_t v = 0;
    EEPROM.get(addr, v);
    *tuneTable[i].value = constrain(v, 0, MAX_PWM);
    addr += sizeof(v);
  }
  return true;
}


// ============================
// motion  (channel A = ENA/IN1/IN2 = ML, channel B = ENB/IN3/IN4 = MR)
// ============================
// Forward only. Both channels forward at the given PWM.
void driveChannels(int pwmA, int pwmB) {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  analogWrite(ENA, constrain(pwmA, 0, MAX_PWM));
  analogWrite(ENB, constrain(pwmB, 0, MAX_PWM));
}

// Active hard brake: both IN pins HIGH + enable ON -> motor terminals shorted
// -> stops fast. (PWM 0 would disable the bridge and let the robot coast.)
void brake() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, HIGH);
  analogWrite(ENA, 255);
  analogWrite(ENB, 255);
}
