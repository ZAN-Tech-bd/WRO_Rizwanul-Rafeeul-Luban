/*
  Arduino Nano - ESP32-controlled Line Follower
  =============================================
  Default behaviour: the robot STAYS STOPPED.

  When the ESP32 (connected over the Nano's hardware UART, D0/D1) sends a
  command, the robot toggles between STOPPED and FOLLOWING THE LINE.

  Command protocol  (9600 baud, 8N1, each command terminated with '\n'
                    -> use Serial.println() on the ESP32 side):
    "GO" / "T" / "TOGGLE"  -> toggle start/stop   (this is the "signal")
    "START" or "1"         -> force line following ON
    "STOP"  or "0"         -> force stop
    Any other non-empty text -> also toggles

  The Nano prints back "STATE:RUNNING" / "STATE:STOPPED" after every change
  and "READY:STOPPED" once at startup, so the ESP32 can confirm the state.

  Speed: tune MIN_DRIVE first (the lowest PWM your motors actually turn at),
         then base_speed for how fast the robot travels. See the block below.

  Sensor telemetry (9600 baud, rate-limited so START/STOP remains responsive):
    S1=0 S2=0 S3=1 S4=0 S5=0 E=0 M=STRAIGHT L=110 R=110
  S1 is far-left, S5 is far-right, and 1 means that sensor sees the line.

  Wiring (as provided):
    Nano D1 (TX) --10k--+ (node) -- ESP32 D25 (RX)
                        + (node) --20k-- GND        (5V -> ~3.3V divider)
    Nano D0 (RX) ------------------- ESP32 D26 (TX)
    Nano GND  ---------------------- ESP32 GND

  NOTE: D0/D1 are the same pins used for USB upload/monitor, so disconnect
        the two ESP32 wires while flashing the Nano, then reconnect.

  Example ESP32 sender (in loop() or on a button press):
    // to start (or toggle):
    Serial2.println("GO");      // GPIO26 = TX -> Nano RX,  GPIO25 = RX <- Nano TX
    // to stop:
    Serial2.println("STOP");
*/

#include <EEPROM.h>

#define ena 10
#define inA 8
#define inB 7
#define inC 11
#define inD 12
#define enb 5
const int irPins[5] = {A0, A1, A2, A3, A4};

void wheel(int lm, int rm);
void wheelRaw(int lm, int rm);
int s[5] = {0, 0, 0, 0, 0};
int lastSensor;

/* ======================= TUNABLES (live-editable) =======================
   Every value below is a runtime variable, not a constant: the ESP32 tuner
   page edits them over the UART link with "SET <key> <value>", and "SAVE"
   writes them to EEPROM so they reload on the next boot. Calibrating no
   longer means reflashing the Nano.

   Values are absolute PWM (0-254), NOT percentages. Percent-scaling the whole
   speed table is what broke the old version: PWM -> wheel RPM is not linear
   near zero, it is flat zero below the motor's start threshold.
   -------------------------------------------------------------------------- */

/* MIN  -- the smallest PWM at which YOUR motors actually rotate while carrying
   the robot's weight. Below it the L298N + gearbox only buzz. This is what
   stalled the robot on every turn: the old code asked for an inner wheel of 27
   (and 6 on sharp turns), which the motor could not start.

   This chassis makes it worse, so MEASURE IT -- do not trust the 70 below:
     * ~1 kg of robot on N20 gearmotors is a lot of load per motor,
     * the L298N is a BJT bridge and drops ~2-3 V before the motors see it,
     * two N20s share each channel, so they split the available current.
   Send "RAMP" and watch the wheels. Expect a higher number than 70. */
int min_drive = 70;

/* BASE -- straight-line travel speed. Keep it well above MIN or every command
   collapses onto the floor and the robot drives straight again. */
int base_speed = 130;

/* Gentle curve: both wheels still drive forward, the inner one sits at the
   floor value -- that is already as slow as a wheel can usefully go, so the
   differential is made by raising CO, not by lowering CI. */
int curve_outer = 145;   // CO
int curve_inner = 70;    // CI

/* Sharp turn / corner: the inner wheel REVERSES. On a 4-wheel skid-steer
   chassis this is not optional -- rotating means dragging all four tyres
   sideways across the floor, and a merely-slower inner wheel cannot do it.
   A reversed inner wheel also keeps both magnitudes above MIN. If it
   over-rotates and wobbles, lower SO or shrink the reverse magnitude of SI. */
int sharp_outer =  180;  // SO
int sharp_inner = -120;  // SI  (negative = reverse)

/* RB -- this track is a rectangle and the robot ONLY turns right, so right
   turns are real 90-degree corners while left turns are just recovery from
   overshoot. Right turns therefore get extra push on both the outer wheel and
   the inner wheel's reverse. Raise RB if right corners are still lazy; set it
   to 0 to make both directions symmetric. */
int right_turn_boost = 30;

/* Searching and junction-crossing used to hardcode a pure counter-rotation
   (outer +N, inner -N). This chassis CANNOT pivot from a standstill -- 1 kg on
   4-wheel skid steer has to drag every tyre sideways, and it just strains and
   stops. So the inner side is now its own tunable rather than a fixed negation:
     negative -> tighter rotation, needs the robot to already be rolling
     positive -> a forward arc, always moves but sweeps much wider
   Keep the outer/inner pair asymmetric enough to actually yaw. */
int search_speed   = 120;  // SRCH  (outer side)
int search_inner   = -40;  // SRCHI (inner side)
int junction_speed = 120;  // JUNC  (outer side)
int junction_inner = -40;  // JUNCI (inner side)

/* LDIR -- where to hunt when the line is lost and no edge has been seen yet.
   0 = stop, 1 = search left, 2 = search right. On a right-only rectangular
   track the line is almost always to the right after an overshoot, so 2 is
   the sensible default. Once an outer sensor HAS seen the line, that memory
   wins over this setting. */
int lost_dir = 2;

/* ML / MR -- rail-test mode. Drives each side at a fixed PWM and ignores the
   line sensors entirely, so the drivetrain can be characterised on a straight
   rail rather than inferred from line-following behaviour. Two things worth
   measuring here:
     * the PWM at which each side actually starts (they are rarely equal),
     * the ML/MR pair that tracks straight -- mismatched motors are a common
       cause of veer that looks like a steering bug.
   These deliberately BYPASS the min_drive floor: a characterisation rig must
   report exactly what was asked for, not a silently corrected value. */
int man_left  = 0;   // ML
int man_right = 0;   // MR

// Complete one-line samples are useful for diagnosis, but printing every
// control-loop pass would saturate the shared 9600-baud ESP32 link. At 125 ms,
// each ~60-character line finishes comfortably before the next sample.
const unsigned long SENSOR_REPORT_MS = 125;
unsigned long lastSensorReportMs = 0;
int lastLeftCommand = 0;
int lastRightCommand = 0;
const char* motionMode = "STOP";

/* ------------------------- Sensor polarity (added) -------------------------
   How do YOUR IR sensors report "on the line"?
     LINE_IS_LOW = true   -> sensor outputs LOW  (0) when it sits over the LINE
                             (most common for TCRT5000-style digital modules).
     LINE_IS_LOW = false  -> sensor outputs HIGH (1) when it sits over the LINE.
   If the robot ignores the path and just drives straight, your sensors are
   the opposite of what the code assumes: flip this one #define and reflash.
   -------------------------------------------------------------------------- */
#define LINE_IS_LOW true

/* ---------------------- ESP32 link state (added) ---------------------- */
const unsigned long LINK_BAUD = 9600;   // MUST match the ESP32 side
bool running = false;                   // robot starts STOPPED by default
bool rampMode = false;                  // MIN_DRIVE ramp test in progress
bool manualMode = false;                // rail test: fixed ML/MR, sensors ignored
char rxBuf[24];                         // fits the longest "SET <key> <value>"
uint8_t rxLen = 0;
/* ---------------------------------------------------------------------- */

void setup() {
  Serial.begin(LINK_BAUD);              // Nano D0/D1 hardware UART <-> ESP32
  mot_init();
  other_init();
  wheel(0, 0);                          // make sure we are stopped on boot

  // Saved calibration wins over the defaults compiled in above.
  Serial.println(tuneLoad() ? F("TUNE:EEPROM") : F("TUNE:DEFAULTS"));
  Serial.println(F("READY:STOPPED"));   // tell the ESP32 we are alive & idle
  tuneReport();                         // let the tuner page populate its fields
}

void loop() {
  handleSerial();                       // check for start/stop commands every loop
  if (rampMode) {
    readSensor();                       // keep the telemetry line meaningful
    rampStep();
  } else if (manualMode) {
    readSensor();                       // sensors reported but NOT acted on
    motionMode = "MANUAL";
    wheelRaw(man_left, man_right);      // raw: no floor, no steering logic
  } else if (running) {
    line_follow();
  } else {
    readSensor();                       // keep diagnostics alive while stopped
    motionMode = "STOP";
    wheel(0, 0);                        // stay stopped
  }
  reportSensors();
}

/* ===================== ESP32 command handling (added) ===================== */

/* ---------------------- Live tuning over the UART ----------------------
   Short keys keep every command inside the small rx buffer: "SET SRCH 110"
   is 12 characters. The ESP32 shows friendly labels and sends these keys. */
struct TuneEntry {
  const char *key;
  int *value;
};

const TuneEntry tuneTable[] = {
  {"MIN",  &min_drive},
  {"BASE", &base_speed},
  {"CO",   &curve_outer},
  {"CI",   &curve_inner},
  {"SO",   &sharp_outer},
  {"SI",   &sharp_inner},
  {"RB",   &right_turn_boost},
  {"SRCH", &search_speed},
  {"SRCHI", &search_inner},
  {"JUNC", &junction_speed},
  {"JUNCI", &junction_inner},
  {"LDIR", &lost_dir},
  {"ML", &man_left},
  {"MR", &man_right},
};
const uint8_t TUNE_COUNT = sizeof(tuneTable) / sizeof(tuneTable[0]);

// Bump TUNE_MAGIC whenever the table layout changes: a mismatch makes the
// Nano ignore the stale EEPROM image and fall back to the defaults above.
const uint16_t TUNE_MAGIC = 0x4C04;   // bumped: ML/MR added to the table

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

void tuneSave() {
  int addr = 0;
  EEPROM.put(addr, TUNE_MAGIC);
  addr += sizeof(TUNE_MAGIC);
  for (uint8_t i = 0; i < TUNE_COUNT; i++) {
    int16_t v = *tuneTable[i].value;
    EEPROM.put(addr, v);                // put() uses update(), so no needless wear
    addr += sizeof(v);
  }
}

bool tuneLoad() {
  int addr = 0;
  uint16_t magic = 0;
  EEPROM.get(addr, magic);
  if (magic != TUNE_MAGIC) return false;   // never saved, or layout changed
  addr += sizeof(magic);
  for (uint8_t i = 0; i < TUNE_COUNT; i++) {
    int16_t v = 0;
    EEPROM.get(addr, v);
    *tuneTable[i].value = v;
    addr += sizeof(v);
  }
  return true;
}

void tuneSet(const char *arg) {
  char key[8];
  uint8_t k = 0;
  while (arg[k] && arg[k] != ' ' && k < sizeof(key) - 1) {
    key[k] = toupper(arg[k]);
    k++;
  }
  key[k] = '\0';

  if (arg[k] != ' ') {                  // no value supplied
    Serial.println(F("ERR:SYNTAX"));
    return;
  }
  int value = atoi(arg + k + 1);

  for (uint8_t i = 0; i < TUNE_COUNT; i++) {
    if (strcmp(key, tuneTable[i].key) == 0) {
      *tuneTable[i].value = constrain(value, -254, 254);
      Serial.print(F("OK "));
      Serial.print(key);
      Serial.print('=');
      Serial.println(*tuneTable[i].value);
      // Echo the whole table so the tuner page always shows what the Nano
      // really holds. Without this the page displays whatever was typed, and
      // a brownout reload from EEPROM goes completely unnoticed.
      tuneReport();
      return;
    }
  }
  Serial.println(F("ERR:KEY"));
}

/* ------------------------- MIN_DRIVE ramp test -------------------------
   Drives both wheels from a PWM that is certainly too low, climbing 1 count
   at a time, and announces each step. Watch the wheels: the number printed
   when they FIRST turn steadily is your min_drive (add ~5 for margin).

   This deliberately bypasses the min_drive floor via wheelRaw() -- it has to
   command values below the floor, that is the entire point of the test.
   Non-blocking, so STOP still lands immediately. */
int rampPwm = 0;
unsigned long rampLastMs = 0;
const unsigned long RAMP_STEP_MS = 200;
const int RAMP_START = 25;
const int RAMP_MAX = 150;

void rampStop() {
  rampMode = false;
  wheelRaw(0, 0);
  Serial.println(F("RAMP:END"));
}

void rampStart() {
  running = false;
  rampMode = true;
  rampPwm = RAMP_START;
  rampLastMs = millis();
  motionMode = "RAMP";
  wheelRaw(rampPwm, rampPwm);
  Serial.println(F("RAMP:START"));
}

void rampStep() {
  unsigned long now = millis();
  if (now - rampLastMs < RAMP_STEP_MS) return;
  rampLastMs = now;

  rampPwm++;
  if (rampPwm > RAMP_MAX) {
    rampStop();
    return;
  }
  wheelRaw(rampPwm, rampPwm);
  lastLeftCommand = rampPwm;
  lastRightCommand = rampPwm;
  Serial.print(F("RAMP PWM="));
  Serial.println(rampPwm);
}

void processCommand(const char *cmd) {
  if (cmd[0] == '\0') {
    return;                             // empty line -> ignore
  }

  // A ramp is aborted by anything that changes state, but NOT by read-only
  // queries: the tuner page polls GET every few seconds, and treating that as
  // an abort silently killed every ramp a few counts after it started.
  if (rampMode && strcmp(cmd, "RAMP") != 0 && strcmp(cmd, "GET") != 0) {
    rampStop();
  }

  if (strncmp(cmd, "SET ", 4) == 0) {
    tuneSet(cmd + 4);
    return;
  }
  if (strcmp(cmd, "GET") == 0) {
    tuneReport();
    return;
  }
  if (strcmp(cmd, "SAVE") == 0) {
    tuneSave();
    Serial.println(F("SAVED"));
    return;
  }
  if (strcmp(cmd, "LOAD") == 0) {
    Serial.println(tuneLoad() ? F("LOADED") : F("LOAD:NONE"));
    tuneReport();
    return;
  }
  if (strcmp(cmd, "RAMP") == 0) {
    rampStart();
    return;
  }
  if (strcmp(cmd, "MAN") == 0) {
    running = false;
    manualMode = true;
    Serial.println(F("MANUAL:ON"));
    return;
  }

  if (strcmp(cmd, "STOP") == 0 || strcmp(cmd, "0") == 0) {
    running = false;
    manualMode = false;                 // STOP always means all motors idle
  } else if (strcmp(cmd, "START") == 0 || strcmp(cmd, "1") == 0) {
    running = true;
    manualMode = false;                 // line following takes over from the rig
  } else {
    // any other signal (GO, T, TOGGLE, ...) flips the running state
    running = !running;
  }

  if (!running) {
    wheel(0, 0);
  }
  Serial.println(running ? F("STATE:RUNNING") : F("STATE:STOPPED"));
}

void handleSerial() {
  while (Serial.available()) {
    char c = Serial.read();

    if (c == '\r') {
      continue;                         // ignore carriage returns
    }
    if (c == '\n') {
      rxBuf[rxLen] = '\0';              // terminate the line
      processCommand(rxBuf);
      rxLen = 0;                        // reset for the next command
      continue;
    }
    if (rxLen < sizeof(rxBuf) - 1) {    // guard the buffer
      rxBuf[rxLen++] = c;
    }
  }
}

/* ============================ original logic ============================ */

void other_init() {
  lastSensor = 0;
}

void mot_init() {
  pinMode(inA, OUTPUT);
  pinMode(inB, OUTPUT);
  pinMode(inC, OUTPUT);
  pinMode(inD, OUTPUT);
  pinMode(ena, OUTPUT);
  pinMode(enb, OUTPUT);
}

/* Push any non-zero request up to at least min_drive. Without this, a command
   like "inner wheel = 27" is silently executed by the hardware as "inner
   wheel = 0" and the robot halts mid-turn instead of steering. Zero still
   means zero, so STOP is unaffected. */
int applyMinDrive(int v) {
  if (v == 0) return 0;
  if (v > 0) return (v < min_drive) ? min_drive : v;
  return (v > -min_drive) ? -min_drive : v;
}

/* Normal path: everything the line follower asks for goes through the floor. */
void wheel(int lm, int rm) {
  wheelRaw(applyMinDrive(lm), applyMinDrive(rm));
}

/* RAMP calibration needs to command PWM values BELOW min_drive -- that is the
   whole point of the test -- so it bypasses the floor and drives here. */
void wheelRaw(int lm, int rm) {
  if (lm > 254) lm = 254;
  if (lm < -254) lm = -254;
  if (rm > 254) rm = 254;
  if (rm < -254) rm = -254;

  lastLeftCommand = lm;
  lastRightCommand = rm;

  if (lm == 0) {
    digitalWrite(inC, HIGH);
    digitalWrite(inD, HIGH);
  }
  if (lm > 0) {
    digitalWrite(inC, HIGH);
    digitalWrite(inD, LOW);
  } else if (lm < 0) {
    digitalWrite(inC, LOW);
    digitalWrite(inD, HIGH);
  }
  if (rm == 0) {
    digitalWrite(inA, HIGH);
    digitalWrite(inB, HIGH);
  }
  if (rm > 0) {
    digitalWrite(inA, HIGH);
    digitalWrite(inB, LOW);
  } else if (rm < 0) {
    digitalWrite(inA, LOW);
    digitalWrite(inB, HIGH);
  }
  analogWrite(ena, abs(rm));
  analogWrite(enb, abs(lm));
}

int readSensor() {
  s[0] = digitalRead(irPins[0]);
  s[1] = digitalRead(irPins[1]);
  s[2] = digitalRead(irPins[2]);
  s[3] = digitalRead(irPins[3]);
  s[4] = digitalRead(irPins[4]);
  // Normalise so that s[i] == 1 means "this sensor is ON the line".
  // Only invert when the module reports LOW over the line.
  if (LINE_IS_LOW) {
    s[0] = 1 - s[0];
    s[1] = 1 - s[1];
    s[2] = 1 - s[2];
    s[3] = 1 - s[3];
    s[4] = 1 - s[4];
  }
  int error, sum;
  sum = s[0] + s[1] + s[2] + s[3] + s[4];
  if (sum != 0) {
    error = (s[0] * 10 + s[1] * 20 + s[2] * 30 + s[3] * 40 + s[4] * 50) / sum - 30;
  } else {
    error = 420;
  }

  // Update direction history only from an unambiguous outer edge. In
  // particular, 11111 must preserve the approach direction instead of always
  // becoming LEFT merely because S1 is checked first.
  if (s[0] == 1 && s[4] == 0) lastSensor = 1;
  else if (s[4] == 1 && s[0] == 0) lastSensor = 2;
  return error;
}

void reportSensors() {
  unsigned long now = millis();
  if (now - lastSensorReportMs < SENSOR_REPORT_MS) return;
  lastSensorReportMs = now;

  Serial.print(F("S1=")); Serial.print(s[0]);
  Serial.print(F(" S2=")); Serial.print(s[1]);
  Serial.print(F(" S3=")); Serial.print(s[2]);
  Serial.print(F(" S4=")); Serial.print(s[3]);
  Serial.print(F(" S5=")); Serial.print(s[4]);
  Serial.print(F(" E="));

  int sum = s[0] + s[1] + s[2] + s[3] + s[4];
  if (sum == 0) {
    Serial.print(F("LOST"));
  } else {
    Serial.print((s[0] * 10 + s[1] * 20 + s[2] * 30 + s[3] * 40 + s[4] * 50) / sum - 30);
  }

  Serial.print(F(" M=")); Serial.print(motionMode);
  Serial.print(F(" L=")); Serial.print(lastLeftCommand);
  Serial.print(F(" R=")); Serial.println(lastRightCommand);
}

void line_follow() {
  int error = readSensor();

  if (error == 420) {
    // Hunt in the direction the line was last seen. With no memory yet, fall
    // back to lost_dir (2 = right, correct for this right-only rectangle).
    int dir = (lastSensor != 0) ? lastSensor : lost_dir;
    if (dir == 1) {
      motionMode = "LOST_LEFT";
      wheel(search_inner, search_speed);
    } else if (dir == 2) {
      motionMode = "LOST_RIGHT";
      wheel(search_speed, search_inner);
    } else {
      motionMode = "LOST_STOP";
      wheel(0, 0);
    }
  } else if (s[0] == 1 && s[4] == 1) {
    // Both outer sensors lit: a crossing or T, which is directionally
    // ambiguous. Keep rotating toward the edge most recently seen; preserve
    // the old default-right behaviour when there is no usable history. This is
    // non-blocking, so STOP is still received while the sensors hunt.
    if (lastSensor == 1) {
      motionMode = "JUNCTION_LEFT";
      wheel(junction_inner, junction_speed);
    } else {
      motionMode = "JUNCTION_RIGHT";
      wheel(junction_speed, junction_inner);
    }
  } else if (s[0] == 1) {
    // Far-LEFT sensor on the line (10000, 11000, 11100, 11110). Decide from
    // the outer sensor, not from the weighted centroid: a corner lights four
    // sensors and averages back to error -5, which the old code read as
    // "centred" and drove straight off the corner.
    motionMode = "SHARP_LEFT";
    wheel(sharp_inner, sharp_outer);
  } else if (s[4] == 1) {
    // Far-RIGHT sensor on the line (00001, 00011, 00111, 01111). On this track
    // that is a real 90-degree corner, so it gets the boost on both wheels.
    motionMode = "SHARP_RIGHT";
    wheel(sharp_outer + right_turn_boost, sharp_inner - right_turn_boost);
  } else if (error <= -5) {
    // Drifting left, including the 01100 straddle the old bands ignored.
    motionMode = "LEFT";
    wheel(curve_inner, curve_outer);
  } else if (error >= 5) {
    // Drifting right, including the 00110 straddle.
    motionMode = "RIGHT";
    wheel(curve_outer + right_turn_boost, curve_inner);
  } else {
    motionMode = "STRAIGHT";
    wheel(base_speed, base_speed);
  }
}
