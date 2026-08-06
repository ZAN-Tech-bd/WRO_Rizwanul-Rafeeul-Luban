#include <Arduino.h>

#include <array>
#include <cstdint>

namespace pins {

// Physical sensor order when looking in the robot's forward direction.
constexpr std::array<uint8_t, 5> kIr = {36, 39, 34, 35, 32};

// Physical calibration confirmed the installed chassis wiring:
// L298N channel A (OUT1/OUT2) drives the robot's LEFT motor.
constexpr uint8_t kLeftEnable = 25;  // ENA
constexpr uint8_t kLeftIn1 = 26;     // IN1
constexpr uint8_t kLeftIn2 = 27;     // IN2

// L298N channel B (OUT3/OUT4) drives the robot's RIGHT motor.
constexpr uint8_t kRightEnable = 13;  // ENB
constexpr uint8_t kRightIn1 = 14;     // IN3
constexpr uint8_t kRightIn2 = 33;     // IN4

}  // namespace pins

namespace config {

constexpr uint32_t kSerialBaud = 115200;

// The supplied Arduino code inverted digitalRead(), so LOW meant line found.
// Change only this value if the measured array uses the opposite polarity.
constexpr bool kLineDetectedWhenLow = true;

constexpr uint8_t kLeftPwmChannel = 0;
constexpr uint8_t kRightPwmChannel = 1;
constexpr uint32_t kPwmFrequencyHz = 20000;
constexpr uint8_t kPwmResolutionBits = 8;
constexpr int kMaxMotorCommand = 254;

// Physical motor calibration: left starts by 180, right by 190, and the
// approximately matched pair is left=200/right=220. Start floor tuning there.
constexpr int kCurveBaseLeft = 200;
constexpr int kCurveBaseRight = 220;
constexpr int kStraightSpeedLeft = 200;
constexpr int kStraightSpeedRight = 220;
constexpr int kRecoverySpeedLeft = 200;
constexpr int kRecoverySpeedRight = 220;
constexpr int kRightTurnSpeedLeft = 200;
constexpr int kRightTurnSpeedRight = 220;
// The original 500 ms is unsafe with the now-higher calibrated pivot command;
// start at 250 ms and tune from physical angle measurements.
constexpr uint32_t kRightTurnDurationMs = 250;
constexpr uint32_t kChangedErrorDelayMs = 5;
constexpr int kStraightErrorThreshold = 10;
constexpr float kKp = 1.0F;
constexpr float kKd = 1.0F;

constexpr int kNoLineError = 420;
constexpr uint32_t kSensorLogIntervalMs = 250;
constexpr uint32_t kStartupSafetyDelayMs = 1500;
// Keep false during calibration. Motor output can then be enabled only by an
// explicit serial ARM command; use true only for the final autonomous image.
constexpr bool kEnableMotorsAfterStartup = false;
constexpr size_t kCommandBufferSize = 64;
constexpr uint32_t kCommandTimeoutMs = 1500;
constexpr uint32_t kMaxCalibrationAutoDurationMs = 5000;

}  // namespace config

namespace {

enum class LastEdge : uint8_t {
  kUnknown = 0,
  kLeft = 1,
  kRight = 2,
};

enum class FollowState : uint8_t {
  kTracking,
  kRecoverLeft,
  kRecoverRight,
  kStoppedNoHistory,
  kTurningRight,
};

struct SensorReading {
  std::array<uint8_t, 5> detected{};
  uint8_t activeCount = 0;
  int error = config::kNoLineError;

  bool allActive() const { return activeCount == detected.size(); }
};

SensorReading readSensors();

LastEdge lastEdge = LastEdge::kUnknown;
int lastError = 0;
FollowState followState = FollowState::kStoppedNoHistory;
uint32_t lastSensorLogMs = 0;
bool motorsEnabled = false;
bool autonomousMode = false;
uint32_t autonomousRunStartedMs = 0;
uint32_t autonomousRunLimitMs = 0;
char commandBuffer[config::kCommandBufferSize]{};
size_t commandLength = 0;

int clampMotorCommand(const int command) {
  return constrain(command, -config::kMaxMotorCommand,
                   config::kMaxMotorCommand);
}

void writeMotor(const int command, const uint8_t in1, const uint8_t in2,
                const uint8_t pwmChannel) {
  const int limited = clampMotorCommand(command);

  // Match the original sketch: HIGH/HIGH gives active braking at zero.
  if (limited == 0) {
    digitalWrite(in1, HIGH);
    digitalWrite(in2, HIGH);
  } else if (limited > 0) {
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
  } else {
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
  }

  ledcWrite(pwmChannel, static_cast<uint32_t>(abs(limited)));
}

// Signed command convention: positive = forward, negative = reverse.
void wheel(const int leftCommand, const int rightCommand) {
  if (!motorsEnabled) {
    ledcWrite(config::kLeftPwmChannel, 0);
    ledcWrite(config::kRightPwmChannel, 0);
    return;
  }
  writeMotor(leftCommand, pins::kLeftIn1, pins::kLeftIn2,
             config::kLeftPwmChannel);
  writeMotor(rightCommand, pins::kRightIn1, pins::kRightIn2,
             config::kRightPwmChannel);
}

void stopMotors() {
  // This must work even before the autonomous motor gate opens.
  digitalWrite(pins::kLeftIn1, HIGH);
  digitalWrite(pins::kLeftIn2, HIGH);
  digitalWrite(pins::kRightIn1, HIGH);
  digitalWrite(pins::kRightIn2, HIGH);
  ledcWrite(config::kLeftPwmChannel, 0);
  ledcWrite(config::kRightPwmChannel, 0);
}

void disarmMotors() {
  autonomousMode = false;
  autonomousRunLimitMs = 0;
  motorsEnabled = false;
  stopMotors();
  Serial.println(F("[motor] DISARMED; PWM forced to zero"));
}

bool parseMotorValue(const char* text, int& value) {
  if (text == nullptr || *text == '\0') {
    return false;
  }
  char* end = nullptr;
  const long parsed = strtol(text, &end, 10);
  if (*end != '\0' || parsed < -config::kMaxMotorCommand ||
      parsed > config::kMaxMotorCommand) {
    return false;
  }
  value = static_cast<int>(parsed);
  return true;
}

void runTimedMotorTest(const int leftCommand, const int rightCommand,
                       const uint32_t durationMs) {
  if (!motorsEnabled || autonomousMode) {
    Serial.println(F("[motor] REJECTED; send ARM first and keep AUTO off"));
    return;
  }
  Serial.print(F("[motor] TEST left="));
  Serial.print(leftCommand);
  Serial.print(F(" right="));
  Serial.print(rightCommand);
  Serial.print(F(" duration_ms="));
  Serial.println(durationMs);
  wheel(leftCommand, rightCommand);
  delay(durationMs);
  stopMotors();
  Serial.println(F("[motor] TEST complete; stopped"));
}

void printCommandHelp() {
  Serial.println(F("[cmd] HELP | SENSORS | ARM | DISARM | STOP"));
  Serial.println(F("[cmd] AUTO <milliseconds 1..5000>"));
  Serial.println(F("[cmd] MOTOR <left -254..254> <right -254..254> [ms]"));
  Serial.println(F("[cmd] Test example: MOTOR 100 0 500"));
}

void handleCommand(char* line) {
  char* save = nullptr;
  const char* command = strtok_r(line, " ", &save);
  if (command == nullptr) {
    return;
  }

  if (strcasecmp(command, "HELP") == 0) {
    printCommandHelp();
  } else if (strcasecmp(command, "SENSORS") == 0) {
    const SensorReading reading = readSensors();
    Serial.print(F("[sensor] bits="));
    for (const uint8_t bit : reading.detected) {
      Serial.print(bit);
    }
    Serial.println();
  } else if (strcasecmp(command, "ARM") == 0) {
    stopMotors();
    autonomousMode = false;
    autonomousRunLimitMs = 0;
    motorsEnabled = true;
    Serial.println(F("[motor] ARMED in manual mode; motors remain stopped"));
  } else if (strcasecmp(command, "DISARM") == 0) {
    disarmMotors();
  } else if (strcasecmp(command, "STOP") == 0) {
    autonomousMode = false;
    autonomousRunLimitMs = 0;
    stopMotors();
    Serial.println(F("[motor] STOPPED; still armed if previously armed"));
  } else if (strcasecmp(command, "AUTO") == 0) {
    if (!motorsEnabled) {
      Serial.println(F("[motor] REJECTED; send ARM before AUTO"));
      return;
    }

    const char* durationText = strtok_r(nullptr, " ", &save);
    char* end = nullptr;
    const unsigned long parsed =
        durationText == nullptr ? 0UL : strtoul(durationText, &end, 10);
    if (durationText == nullptr || *durationText == '\0' || *end != '\0' ||
        parsed == 0UL || parsed > config::kMaxCalibrationAutoDurationMs) {
      Serial.println(F("[motor] usage: AUTO <milliseconds 1..5000>"));
      return;
    }

    stopMotors();
    lastError = 0;
    lastEdge = LastEdge::kUnknown;
    autonomousRunStartedMs = millis();
    autonomousRunLimitMs = static_cast<uint32_t>(parsed);
    autonomousMode = true;
    Serial.print(F("[motor] autonomous line following ENABLED for "));
    Serial.print(autonomousRunLimitMs);
    Serial.println(F(" ms; timeout will stop and disarm"));
  } else if (strcasecmp(command, "MOTOR") == 0) {
    int left = 0;
    int right = 0;
    if (!parseMotorValue(strtok_r(nullptr, " ", &save), left) ||
        !parseMotorValue(strtok_r(nullptr, " ", &save), right)) {
      Serial.println(F("[motor] usage: MOTOR <left -254..254> <right -254..254> [ms]"));
      return;
    }
    uint32_t duration = 500;
    const char* durationText = strtok_r(nullptr, " ", &save);
    if (durationText != nullptr) {
      char* end = nullptr;
      const unsigned long parsed = strtoul(durationText, &end, 10);
      if (*end != '\0' || parsed == 0 || parsed > config::kCommandTimeoutMs) {
        Serial.println(F("[motor] duration must be 1..1500 ms"));
        return;
      }
      duration = static_cast<uint32_t>(parsed);
    }
    runTimedMotorTest(left, right, duration);
  } else {
    Serial.print(F("[cmd] unknown: "));
    Serial.println(command);
    printCommandHelp();
  }
}

void serviceSerialCommands() {
  while (Serial.available() > 0) {
    const char incoming = static_cast<char>(Serial.read());
    if (incoming == '\r') {
      continue;
    }
    if (incoming == '\n') {
      commandBuffer[commandLength] = '\0';
      handleCommand(commandBuffer);
      commandLength = 0;
      continue;
    }
    if (commandLength + 1U < sizeof(commandBuffer) &&
        incoming >= ' ' && incoming <= '~') {
      commandBuffer[commandLength++] = incoming;
    } else if (incoming >= ' ') {
      commandLength = 0;
      Serial.println(F("[cmd] line too long; discarded"));
    }
  }
}

void initializeMotors() {
  // Establish a disabled/braking state before attaching PWM to the EN pins.
  pinMode(pins::kLeftIn1, OUTPUT);
  pinMode(pins::kLeftIn2, OUTPUT);
  pinMode(pins::kRightIn1, OUTPUT);
  pinMode(pins::kRightIn2, OUTPUT);
  digitalWrite(pins::kLeftIn1, HIGH);
  digitalWrite(pins::kLeftIn2, HIGH);
  digitalWrite(pins::kRightIn1, HIGH);
  digitalWrite(pins::kRightIn2, HIGH);

  ledcSetup(config::kLeftPwmChannel, config::kPwmFrequencyHz,
            config::kPwmResolutionBits);
  ledcSetup(config::kRightPwmChannel, config::kPwmFrequencyHz,
            config::kPwmResolutionBits);
  ledcAttachPin(pins::kLeftEnable, config::kLeftPwmChannel);
  ledcAttachPin(pins::kRightEnable, config::kRightPwmChannel);
  ledcWrite(config::kLeftPwmChannel, 0);
  ledcWrite(config::kRightPwmChannel, 0);
}

void initializeSensors() {
  for (const uint8_t pin : pins::kIr) {
    // GPIO34/35/36/39 have no internal pulls. The 3.3 V array must actively
    // drive them, or each open-collector output needs an external 10 k pull-up.
    pinMode(pin, INPUT);
  }
}

SensorReading readSensors() {
  SensorReading reading;
  int weightedPosition = 0;

  for (size_t index = 0; index < pins::kIr.size(); ++index) {
    const bool rawHigh = digitalRead(pins::kIr[index]) == HIGH;
    const bool detected =
        config::kLineDetectedWhenLow ? !rawHigh : rawHigh;
    reading.detected[index] = detected ? 1U : 0U;

    if (detected) {
      ++reading.activeCount;
      // Preserve the original integer arithmetic: weights 10..50, then -30.
      weightedPosition += static_cast<int>((index + 1U) * 10U);
    }
  }

  if (reading.activeCount > 0U) {
    reading.error = weightedPosition / reading.activeCount - 30;
  }

  // Preserve original precedence if both edge sensors see the line.
  if (reading.detected[0] == 1U) {
    lastEdge = LastEdge::kLeft;
  } else if (reading.detected[4] == 1U) {
    lastEdge = LastEdge::kRight;
  }

  return reading;
}

const __FlashStringHelper* stateName(const FollowState state) {
  switch (state) {
    case FollowState::kTracking:
      return F("TRACK");
    case FollowState::kRecoverLeft:
      return F("RECOVER_LEFT");
    case FollowState::kRecoverRight:
      return F("RECOVER_RIGHT");
    case FollowState::kStoppedNoHistory:
      return F("STOP_NO_HISTORY");
    case FollowState::kTurningRight:
      return F("TURN_RIGHT");
  }
  return F("UNKNOWN");
}

void logSensorsPeriodically(const SensorReading& reading, const int correction,
                            const int leftCommand,
                            const int rightCommand) {
  const uint32_t now = millis();
  if (now - lastSensorLogMs < config::kSensorLogIntervalMs) {
    return;
  }
  lastSensorLogMs = now;

  Serial.print(F("[lfr] bits="));
  for (const uint8_t bit : reading.detected) {
    Serial.print(bit);
  }
  Serial.print(F(" error="));
  if (reading.error == config::kNoLineError) {
    Serial.print(F("LOST"));
  } else {
    Serial.print(reading.error);
  }
  Serial.print(F(" corr="));
  Serial.print(correction);
  Serial.print(F(" left="));
  Serial.print(leftCommand);
  Serial.print(F(" right="));
  Serial.print(rightCommand);
  Serial.print(F(" state="));
  Serial.println(stateName(followState));
}

void turnRight() {
  followState = FollowState::kTurningRight;
  // Left forward, right reverse = right pivot.
  wheel(config::kRightTurnSpeedLeft, -config::kRightTurnSpeedRight);
  delay(config::kRightTurnDurationMs);
}

void followLine() {
  const SensorReading reading = readSensors();
  int correction = 0;
  int leftCommand = 0;
  int rightCommand = 0;

  // Keep the supplied order: no-line recovery is evaluated before the
  // all-active T-junction case.
  if (reading.error == config::kNoLineError) {
    if (lastEdge == LastEdge::kLeft) {
      followState = FollowState::kRecoverLeft;
      leftCommand = -config::kRecoverySpeedLeft;
      rightCommand = config::kRecoverySpeedRight;
      wheel(leftCommand, rightCommand);
    } else if (lastEdge == LastEdge::kRight) {
      followState = FollowState::kRecoverRight;
      leftCommand = config::kRecoverySpeedLeft;
      rightCommand = -config::kRecoverySpeedRight;
      wheel(leftCommand, rightCommand);
    } else {
      // The Arduino sketch left the previous output active here. Stopping is
      // safer at boot because no edge direction has ever been observed.
      followState = FollowState::kStoppedNoHistory;
      stopMotors();
    }
  } else if (reading.allActive()) {
    turnRight();
    leftCommand = config::kRightTurnSpeedLeft;
    rightCommand = -config::kRightTurnSpeedRight;
  } else {
    followState = FollowState::kTracking;
    const float proportional = config::kKp * reading.error;
    const float derivative = config::kKd * (reading.error - lastError);
    correction = static_cast<int>(proportional + derivative);

    if (abs(reading.error) < config::kStraightErrorThreshold) {
      leftCommand = config::kStraightSpeedLeft + correction;
      rightCommand = config::kStraightSpeedRight - correction;
    } else {
      leftCommand = config::kCurveBaseLeft + correction;
      rightCommand = config::kCurveBaseRight - correction;
    }

    wheel(leftCommand, rightCommand);
    if (reading.error != lastError) {
      delay(config::kChangedErrorDelayMs);
    }
    lastError = reading.error;
  }

  logSensorsPeriodically(reading, correction,
                         clampMotorCommand(leftCommand),
                         clampMotorCommand(rightCommand));
}

void printConfiguration() {
  Serial.println();
  Serial.println(F("[robot] ESP32 five-sensor line follower"));
  Serial.println(F("[pins] IR far-left..far-right = 36,39,34,35,32"));
  Serial.println(F("[pins] LEFT  = ENA25 IN1=26 IN2=27 OUT1/OUT2"));
  Serial.println(F("[pins] RIGHT = ENB13 IN3=14 IN4=33 OUT3/OUT4"));
  Serial.print(F("[sensor] line detected when input is "));
  Serial.println(config::kLineDetectedWhenLow ? F("LOW") : F("HIGH"));
  Serial.print(F("[pwm] "));
  Serial.print(config::kPwmFrequencyHz);
  Serial.print(F(" Hz, "));
  Serial.print(config::kPwmResolutionBits);
  Serial.println(F(" bit, max command 254"));
  Serial.println(F("[safety] Motors stay stopped during startup delay."));
  Serial.println(F("[safety] Raise wheels and verify direction before track test."));
  Serial.print(F("[safety] Autonomous motor output after delay: "));
  Serial.println(config::kEnableMotorsAfterStartup ? F("ENABLED")
                                                  : F("DISABLED"));
  printCommandHelp();
}

}  // namespace

void setup() {
  Serial.begin(config::kSerialBaud);
  initializeSensors();
  initializeMotors();
  stopMotors();
  printConfiguration();
  delay(config::kStartupSafetyDelayMs);
  motorsEnabled = config::kEnableMotorsAfterStartup;
  autonomousMode = config::kEnableMotorsAfterStartup;
  Serial.println(autonomousMode ? F("[robot] line following active")
                                : F("[robot] calibration mode; motors disabled"));
}

void loop() {
  serviceSerialCommands();

  if (autonomousMode && autonomousRunLimitMs > 0U &&
      millis() - autonomousRunStartedMs >= autonomousRunLimitMs) {
    Serial.println(F("[motor] AUTO timeout; stopping and disarming"));
    disarmMotors();
  }

  if (autonomousMode) {
    followLine();
  } else {
    const SensorReading reading = readSensors();
    logSensorsPeriodically(reading, 0, 0, 0);
    delay(1);
  }
}
