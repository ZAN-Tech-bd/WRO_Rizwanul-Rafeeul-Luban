/*
 * ESP32 Five-Sensor Line Following Robot
 * =======================================
 *
 * Ported from the original Arduino UNO sketch to the ESP32-WROOM-32.
 * Preserves the exact line-following algorithm: weighted sensor position,
 * PD correction, last-edge recovery, and all-five-active T-junction right pivot.
 *
 * The only changes from the original are:
 *   1. ESP32 GPIO pin numbers instead of Arduino pins.
 *   2. ESP32 LEDC PWM (ledcSetup / ledcAttachPin / ledcWrite) instead of analogWrite.
 *   3. Motor speeds calibrated for this chassis (left starts at 180, right at 190;
 *      matched pair is 200/220 because the right motor needs ~10% more command).
 *
 * Tested hardware
 * ---------------
 *   Board:   ESP32-WROOM-32 DevKit (30-pin, DOIT-style)
 *   IR array: 5-channel digital, active-low (LOW = line detected), powered at 3.3 V
 *   Driver:   L298N dual H-bridge, ENA/ENB jumpers removed
 *   Motors:   2 x 6 V TT gearmotors
 *
 * Wiring
 * ------
 *   IR sensors (physical far-left to far-right):
 *     GPIO36, GPIO39, GPIO34, GPIO35, GPIO32
 *
 *   Left motor (L298N channel A — physically verified):
 *     ENA = GPIO25, IN1 = GPIO26, IN2 = GPIO27, OUT1/OUT2 -> left motor
 *
 *   Right motor (L298N channel B — physically verified):
 *     ENB = GPIO13, IN3 = GPIO14, IN4 = GPIO33, OUT3/OUT4 -> right motor
 *
 * Arduino IDE setup
 * -----------------
 *   1. Install "ESP32" board support by Espressif in Boards Manager.
 *   2. Select board: "ESP32 Dev Module".
 *   3. Upload and open Serial Monitor at 115200 baud (optional, for debug).
 *
 * GitHub: https://github.com/ZAN-Tech-bd/ESP32_LFR
 */

// =====================================================================
// Pin definitions
// =====================================================================

// IR sensor pins — physical far-left to far-right order.
const int irPins[5] = {36, 39, 34, 35, 32};

// L298N Channel A — LEFT motor (physical calibration confirmed).
#define ENA 25   // Left motor PWM (LEDC channel 0)
#define IN1 26   // Left motor direction A
#define IN2 27   // Left motor direction B

// L298N Channel B — RIGHT motor (physical calibration confirmed).
#define ENB 13   // Right motor PWM (LEDC channel 1)
#define IN3 14   // Right motor direction A
#define IN4 33   // Right motor direction B

// =====================================================================
// PWM configuration (ESP32 LEDC — replaces Arduino analogWrite)
// =====================================================================

#define PWM_FREQ   20000   // 20 kHz — above audible range
#define PWM_RES    8       // 8-bit resolution (duty 0–255)
#define LEFT_CH    0       // LEDC channel for left motor
#define RIGHT_CH   1       // LEDC channel for right motor

// =====================================================================
// Line-following tunable parameters
// =====================================================================

// Sensor array state.
int s[5] = {0, 0, 0, 0, 0};
int i;
int lastSensor;
int lastError;

// Motor speeds — calibrated for this chassis (wheels-up measurements).
// The right motor needs ~10% more PWM than the left to match speed.
int uturn_L   = 200;   // Recovery left-motor speed
int uturn_R   = 220;   // Recovery right-motor speed
int base_L    = 200;   // Curve base speed, left
int base_R    = 220;   // Curve base speed, right
int straight_L = 200;  // Straight-line speed, left
int straight_R = 220;  // Straight-line speed, right

// Right-pivot (T-junction) parameters.
int turnPivot_L = 200;       // Left wheel forward during pivot
int turnPivot_R = 220;       // Right wheel backward during pivot
int turnDelay   = 250;       // Pivot duration in ms (tune for 90 degrees)

// PD controller gains.
float kp = 1;
float kd = 1;


// =====================================================================
// Setup
// =====================================================================

void setup() {
  mot_init();
  other_init();
}

void loop() {
  line_follow();
}


// =====================================================================
// Initialisation
// =====================================================================

void other_init() {
  lastSensor = 0;
  lastError = 0;
}

void mot_init() {
  // Direction pins as outputs.
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // ESP32 LEDC PWM setup — this replaces Arduino's analogWrite().
  ledcSetup(LEFT_CH, PWM_FREQ, PWM_RES);
  ledcSetup(RIGHT_CH, PWM_FREQ, PWM_RES);
  ledcAttachPin(ENA, LEFT_CH);
  ledcAttachPin(ENB, RIGHT_CH);

  // Start with both motors stopped (active brake).
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, HIGH);
  ledcWrite(LEFT_CH, 0);
  ledcWrite(RIGHT_CH, 0);
}


// =====================================================================
// Motor control — same logic as the original Arduino wheel() function.
//
//   lm:  left  motor command  (-254 = full reverse … 0 = brake … +254 = full forward)
//   rm:  right motor command  (-254 = full reverse … 0 = brake … +254 = full forward)
//
// Channel A (IN1/IN2/ENA) drives the LEFT  motor.
// Channel B (IN3/IN4/ENB) drives the RIGHT motor.
// =====================================================================

void wheel(int lm, int rm) {
  // --- Left motor (channel A) ---
  if (lm == 0) {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, HIGH);      // brake
  }
  if (lm > 0) {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);       // forward
  } else if (lm < 0) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);      // reverse
  }

  // --- Right motor (channel B) ---
  if (rm == 0) {
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, HIGH);      // brake
  }
  if (rm > 0) {
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);       // forward
  } else if (rm < 0) {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);      // reverse
  }

  // Clamp to the original ±254 range.
  if (lm > 254)  lm = 254;
  if (lm < -254) lm = -254;
  if (rm > 254)  rm = 254;
  if (rm < -254) rm = -254;

  // Write PWM duty via ESP32 LEDC.
  ledcWrite(LEFT_CH, abs(lm));
  ledcWrite(RIGHT_CH, abs(rm));
}


// =====================================================================
// Sensor reading — same logic as the original Arduino readSensor().
//
// Reads five digital IR sensors, inverts each reading (active-low),
// computes a weighted position error, and remembers the last edge.
//
// Returns:
//   error in range -20..+20, or 420 if no line is detected.
// =====================================================================

int readSensor() {
  s[0] = digitalRead(irPins[0]);
  s[1] = digitalRead(irPins[1]);
  s[2] = digitalRead(irPins[2]);
  s[3] = digitalRead(irPins[3]);
  s[4] = digitalRead(irPins[4]);

  // Invert: active-low array (LOW = line detected -> 1).
  s[0] = 1 - s[0];
  s[1] = 1 - s[1];
  s[2] = 1 - s[2];
  s[3] = 1 - s[3];
  s[4] = 1 - s[4];

  int error, sum;
  sum = s[0] + s[1] + s[2] + s[3] + s[4];

  if (sum != 0) {
    // Weighted average: weights 10,20,30,40,50 minus centre offset 30.
    error = (s[0] * 10 + s[1] * 20 + s[2] * 30 + s[3] * 40 + s[4] * 50) / sum - 30;
  } else {
    error = 420;   // No line detected.
  }

  // Remember which edge was last seen for recovery.
  if (s[0] == 1) lastSensor = 1;       // last edge was left
  else if (s[4] == 1) lastSensor = 2;  // last edge was right

  return error;
}


// =====================================================================
// T-junction right pivot — same logic as the original turnRight().
//
// Pivots right (left forward, right backward) for a fixed duration.
// =====================================================================

void turnRight() {
  wheel(turnPivot_L, -turnPivot_R);  // left forward, right backward = right pivot
  delay(turnDelay);
}


// =====================================================================
// Main line-following loop — same logic as the original line_follow().
// =====================================================================

void line_follow() {
  int error, corr;
  float p, d;

  error = readSensor();

  if (error == 420) {
    // ----- Line lost: recover using last known edge -----
    if (lastSensor == 1)
      wheel(-uturn_L, uturn_R);         // turn left
    else if (lastSensor == 2)
      wheel(uturn_L, -uturn_R);         // turn right
  }
  else if (s[0] == 1 && s[1] == 1 && s[2] == 1 && s[3] == 1 && s[4] == 1) {
    // ----- All five sensors active: T-junction detected -----
    turnRight();
  }
  else {
    // ----- Normal tracking: PD correction -----
    p = kp * error;
    d = kd * (error - lastError);
    corr = p + d;

    if (abs(error) < 10) {
      // Near-centre: straight-line speed.
      wheel(straight_L + corr, straight_R - corr);
    } else {
      // Off-centre: curve base speed.
      wheel(base_L + corr, base_R - corr);
    }

    if ((error - lastError) != 0) delay(5);
    lastError = error;
  }
}
