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

  Speed: set SPEED_PERCENT below (100 = the original tuned speeds).

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

#define ena 10
#define inA 8
#define inB 7
#define inC 11
#define inD 12
#define enb 5
const int irPins[5] = {A0, A1, A2, A3, A4};

void wheel(int lm, int rm);
int s[5] = {0, 0, 0, 0, 0};
int i, lastSensor, lastError;

/* ------------------------- Speed scaling (added) -------------------------
   SPEED_PERCENT = 100 means the original tuned speeds (u-turn 180, curve
   base 160, straight 150, T-junction 100 out of 255). Lower it to slow the
   whole robot down; raise it to speed back up. This is the ONLY number to
   change -- every speed below, including the PD correction, scales with it,
   so the steering keeps the same shape at any setting.
   -------------------------------------------------------------------------- */
const int SPEED_PERCENT = 35;

int uturn          = 180 * SPEED_PERCENT / 100;   // 35% -> 63
int base_L         = 160 * SPEED_PERCENT / 100;   // 35% -> 56
int base_R         = 160 * SPEED_PERCENT / 100;   // 35% -> 56
int straight_speed = 150 * SPEED_PERCENT / 100;   // 35% -> 52
int turn_speed     = 100 * SPEED_PERCENT / 100;   // 35% -> 35

// Scaled with the speeds so the correction stays proportional to the base
// throttle; an unscaled kp/kd would steer far harder at reduced speed.
float kp = 1.0 * SPEED_PERCENT / 100;             // 35% -> 0.35
float kd = 1.0 * SPEED_PERCENT / 100;             // 35% -> 0.35

/* ---------------------- ESP32 link state (added) ---------------------- */
const unsigned long LINK_BAUD = 9600;   // MUST match the ESP32 side
bool running = false;                   // robot starts STOPPED by default
char rxBuf[16];                         // buffer for one command line
uint8_t rxLen = 0;
/* ---------------------------------------------------------------------- */

void setup() {
  Serial.begin(LINK_BAUD);              // Nano D0/D1 hardware UART <-> ESP32
  mot_init();
  other_init();
  wheel(0, 0);                          // make sure we are stopped on boot
  Serial.println(F("READY:STOPPED"));   // tell the ESP32 we are alive & idle
}

void loop() {
  handleSerial();                       // check for start/stop commands every loop
  if (running) {
    line_follow();
  } else {
    wheel(0, 0);                        // stay stopped
  }
}

/* ===================== ESP32 command handling (added) ===================== */

void processCommand(const char *cmd) {
  if (cmd[0] == '\0') {
    return;                             // empty line -> ignore
  }

  if (strcmp(cmd, "STOP") == 0 || strcmp(cmd, "0") == 0) {
    running = false;
  } else if (strcmp(cmd, "START") == 0 || strcmp(cmd, "1") == 0) {
    if (!running) {
      lastError = 0;                    // avoid a derivative spike on restart
      running = true;
    }
  } else {
    // any other signal (GO, T, TOGGLE, ...) flips the running state
    running = !running;
    if (running) {
      lastError = 0;
    }
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
  lastError = 0;
}

void mot_init() {
  pinMode(inA, OUTPUT);
  pinMode(inB, OUTPUT);
  pinMode(inC, OUTPUT);
  pinMode(inD, OUTPUT);
  pinMode(ena, OUTPUT);
  pinMode(enb, OUTPUT);
}

void wheel(int lm, int rm) {
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
  if (lm > 254) lm = 254;
  if (lm < -254) lm = -254;
  if (rm > 254) rm = 254;
  if (rm < -254) rm = -254;

  analogWrite(ena, abs(rm));
  analogWrite(enb, abs(lm));
}

int readSensor() {
  s[0] = digitalRead(irPins[0]);
  s[1] = digitalRead(irPins[1]);
  s[2] = digitalRead(irPins[2]);
  s[3] = digitalRead(irPins[3]);
  s[4] = digitalRead(irPins[4]);
  s[0] = 1 - s[0];
  s[1] = 1 - s[1];
  s[2] = 1 - s[2];
  s[3] = 1 - s[3];
  s[4] = 1 - s[4];
  int error, sum;
  sum = s[0] + s[1] + s[2] + s[3] + s[4];
  if (sum != 0) {
    error = (s[0] * 10 + s[1] * 20 + s[2] * 30 + s[3] * 40 + s[4] * 50) / sum - 30;
  } else {
    error = 420;
  }

  if (s[0] == 1) lastSensor = 1;
  else if (s[4] == 1) lastSensor = 2;
  return error;
}

void turnRight() {
  // This function will turn the robot to the right.
  // Adjust the turning duration and speed as needed.
  wheel(turn_speed, -turn_speed);       // turn right
  delay(500);                           // adjust the delay for a proper right turn
}

void line_follow() {
  int error, corr;
  float p, d;
  error = readSensor();

  if (error == 420) {
    if (lastSensor == 1) wheel(-uturn, uturn);       // make a slight left turn
    else if (lastSensor == 2) wheel(uturn, -uturn);  // make a slight right turn
  } else if (s[0] == 1 && s[1] == 1 && s[2] == 1 && s[3] == 1 && s[4] == 1) {
    // All sensors are active - 'T' shape detected
    turnRight();
  } else {
    p = kp * error;
    d = kd * (error - lastError);
    corr = p + d;

    // Check for straight line detection (you can adjust the threshold as needed)
    if (abs(error) < 10) {
      // Straight line detected - run at the straight-line speed
      wheel(straight_speed + corr, straight_speed - corr);
    } else {
      // Not a straight line, use the base speed
      wheel(base_L + corr, base_R - corr);
    }

    if ((error - lastError) != 0) delay(5);
    lastError = error;
  }
}
