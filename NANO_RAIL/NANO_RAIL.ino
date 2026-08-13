// Arduino Nano + L298N -- rail follower, serial-controlled.
//
// Motion: FORWARD at the tuned speed (left=150, right=210). Stop = hard BRAKE
// (both IN pins HIGH + enable ON), so the wheel stops hard instead of coasting
// from inertia. The robot never turns right: left speed is clamped <= right.
//
// Serial Monitor @ 9600 baud. Commands are CASE-INSENSITIVE (f == F).
//
//   F            Forward at current speeds (starts at 150/210), keeps running
//   S            BRAKE / stop (hard stop)
//   +   /  -     Raise / lower BOTH wheel speeds by 5
//   M <l> <r>    Set both wheel speeds and go, e.g.  M 150 210   (or  M 180)
//   0            Reset speeds to 150 / 210
//   D            Demo: forward 2s -> brake 2s (once)
//   ?            Show this help
//
//   Status line:  L=150 R=210 FORWARD   (or BRAKE)

#define ENA 10
#define IN1 8
#define IN2 7

#define ENB 5
#define IN3 11
#define IN4 12

const int DEFAULT_LEFT  = 150;
const int DEFAULT_RIGHT = 210;
const int MAX_SPEED     = 255;
const int STEP          = 5;

int driveLeft  = DEFAULT_LEFT;
int driveRight = DEFAULT_RIGHT;
bool moving    = false;          // true while driving forward


void setup() {

  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  Serial.begin(9600);
  Serial.setTimeout(100);        // lets single chars work with "No line ending"

  brake();                       // start braked (safe -- won't lurch on power-up)

  Serial.println("Rail ready.");
  printHelp();
  printStatus();
}


void loop() {

  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    line.trim();
    line.toUpperCase();
    handle(line);
  }
}


// =========================
// COMMAND HANDLING
// =========================

void handle(const String &c) {

  if (c.length() == 0) return;

  // M <left> <right>  (or  M <both>)
  if (c[0] == 'M') {
    String a = c.substring(1);
    a.replace(',', ' ');
    a.trim();

    if (a.length() == 0) {
      Serial.println("Usage: M <left> <right>   e.g. M 150 210");
    } else {
      int sp = a.indexOf(' ');
      if (sp < 0) {
        int v = constrain(a.toInt(), 0, MAX_SPEED);
        driveLeft = driveRight = v;
      } else {
        driveLeft  = constrain(a.substring(0, sp).toInt(), 0, MAX_SPEED);
        String r = a.substring(sp + 1); r.trim();
        driveRight = constrain(r.toInt(), 0, MAX_SPEED);
      }
      if (driveLeft > driveRight) {                 // never turn right
        Serial.println("left>right would curve RIGHT -> clamped left=right.");
        driveLeft = driveRight;
      }
      forward();
    }
  }

  else if (c == "F" || c == "G" || c == "1") {
    forward();
  }
  else if (c == "S" || c == "X" || c == "STOP") {
    brake();
  }
  else if (c == "+") {
    driveLeft  = constrain(driveLeft  + STEP, 0, MAX_SPEED);
    driveRight = constrain(driveRight + STEP, 0, MAX_SPEED);
    reapply();
  }
  else if (c == "-") {
    driveLeft  = constrain(driveLeft  - STEP, 0, MAX_SPEED);
    driveRight = constrain(driveRight - STEP, 0, MAX_SPEED);
    reapply();
  }
  else if (c == "0") {
    driveLeft  = DEFAULT_LEFT;
    driveRight = DEFAULT_RIGHT;
    reapply();
    Serial.println("Speeds reset to 150 / 210.");
  }
  else if (c == "D" || c == "DEMO") {
    runDemo();
    printStatus();
    return;
  }
  else if (c == "?" || c == "H" || c == "HELP") {
    printHelp();
    printStatus();
    return;
  }
  else {
    Serial.print("Unknown: ");
    Serial.println(c);
    printHelp();
    return;
  }

  printStatus();
}


void reapply() {
  if (moving) forward();
  else        brake();
}


// Demo: forward 2s -> brake 2s (once).
void runDemo() {
  Serial.println("DEMO: forward 2s -> brake 2s");
  forward();
  delay(2000);
  brake();
  delay(2000);
}


// =========================
// MOTION
// =========================

void forward() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, driveLeft);
  analogWrite(ENB, driveRight);
  moving = true;
}


// Hard brake: both IN pins HIGH + enable ON -> motor terminals shorted -> stops
// fast. (LOW,LOW with enable OFF would free-wheel and let inertia roll it on.)
void brake() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, HIGH);
  analogWrite(ENA, 255);
  analogWrite(ENB, 255);
  moving = false;
}


// =========================
// STATUS / HELP
// =========================

void printHelp() {
  Serial.println("Commands:  F=forward  S=brake  +/-  M<l r>  0=reset  D=demo  ?=help");
}

void printStatus() {
  Serial.print("L=");
  Serial.print(driveLeft);
  Serial.print(" R=");
  Serial.print(driveRight);
  Serial.print("  ");
  Serial.println(moving ? "FORWARD" : "BRAKE");
}
