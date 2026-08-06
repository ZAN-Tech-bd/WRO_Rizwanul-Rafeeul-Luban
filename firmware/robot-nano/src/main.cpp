#include <Arduino.h>
#include <MFRC522.h>
#include <SPI.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "RobotUartProtocol.h"

namespace pins {

constexpr uint8_t kIr[5] = {A0, A1, A2, A3, A4};

// Physical chassis mapping: L298N channel A = left, channel B = right.
// The user's four former D9..D12 direction wires move in order to D2..D5.
constexpr uint8_t kLeftIn1 = 2;      // IN1: old D9 -> D2
constexpr uint8_t kLeftIn2 = 3;      // IN2: old D10 -> D3
constexpr uint8_t kRightIn1 = 4;     // IN3: old D11 -> D4
constexpr uint8_t kRightIn2 = 5;     // IN4: old D12 -> D5
constexpr uint8_t kLeftEnable = 6;   // ENA, hardware PWM
constexpr uint8_t kRightEnable = 9;  // ENB, hardware PWM

constexpr uint8_t kRfidSs = 10;
constexpr uint8_t kRfidRst = A5;

}  // namespace pins

namespace config {

constexpr bool kLineDetectedWhenLow = true;
constexpr int kMaxMotorCommand = 254;

// Match the supplied Arduino behavior: begin line following automatically.
// Keep the chassis wheels raised during the first Nano flash/direction test.
constexpr bool kEnableMotionAfterStartup = true;
constexpr uint32_t kStartupSafetyDelayMs = 1500;

// Initial values come from the physically measured ESP32 baseline. Recheck
// them because AVR analogWrite uses different PWM timers/frequencies.
constexpr int kCurveBaseLeft = 200;
constexpr int kCurveBaseRight = 220;
constexpr int kStraightSpeedLeft = 200;
constexpr int kStraightSpeedRight = 220;
constexpr int kRecoverySpeedLeft = 200;
constexpr int kRecoverySpeedRight = 220;
constexpr int kRightTurnSpeedLeft = 200;
constexpr int kRightTurnSpeedRight = 220;
constexpr uint16_t kRightTurnDurationMs = 250;
constexpr uint8_t kChangedErrorDelayMs = 5;
constexpr int kStraightErrorThreshold = 10;
constexpr float kKp = 1.0F;
constexpr float kKd = 1.0F;
constexpr int kNoLineError = 420;

constexpr uint32_t kTagRetryIntervalMs = 750;
// Long enough for the current 3.3--4.4 second station tracks. The Nano always
// resumes after this bounded stop even if the ESP32 or UART link has failed.
constexpr uint32_t kCardStopDurationMs = 5000;

}  // namespace config

namespace {

enum class LastEdge : uint8_t { kUnknown, kLeft, kRight };
enum class MotionState : uint8_t {
  kRunning,
  kStoppedByCommand,
  kStoppedByCard,
};

MFRC522 rfid(pins::kRfidSs, pins::kRfidRst);

uint8_t sensors[5] = {0, 0, 0, 0, 0};
LastEdge lastEdge = LastEdge::kUnknown;
int lastError = 0;
MotionState motionState = MotionState::kStoppedByCommand;
MotionState motionStateBeforeCard = MotionState::kStoppedByCommand;

uint16_t eventSequence = 0;
uint16_t activeCardSequence = 0;
bool activeCardAcknowledged = false;
uint32_t cardStoppedAtMs = 0;
uint32_t lastTagSentAtMs = 0;
char activeTagFrame[robot_uart::kMaxFrameLength]{};

char linkBuffer[24]{};
size_t linkLength = 0;

int clampMotorCommand(const int command) {
  return constrain(command, -config::kMaxMotorCommand,
                   config::kMaxMotorCommand);
}

void writeMotor(const int command, const uint8_t in1, const uint8_t in2,
                const uint8_t enable) {
  const int limited = clampMotorCommand(command);
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
  analogWrite(enable, abs(limited));
}

void wheel(const int leftCommand, const int rightCommand) {
  writeMotor(leftCommand, pins::kLeftIn1, pins::kLeftIn2,
             pins::kLeftEnable);
  writeMotor(rightCommand, pins::kRightIn1, pins::kRightIn2,
             pins::kRightEnable);
}

void stopMotors() {
  digitalWrite(pins::kLeftIn1, HIGH);
  digitalWrite(pins::kLeftIn2, HIGH);
  digitalWrite(pins::kRightIn1, HIGH);
  digitalWrite(pins::kRightIn2, HIGH);
  analogWrite(pins::kLeftEnable, 0);
  analogWrite(pins::kRightEnable, 0);
}

void initializeMotors() {
  pinMode(pins::kLeftIn1, OUTPUT);
  pinMode(pins::kLeftIn2, OUTPUT);
  pinMode(pins::kRightIn1, OUTPUT);
  pinMode(pins::kRightIn2, OUTPUT);
  pinMode(pins::kLeftEnable, OUTPUT);
  pinMode(pins::kRightEnable, OUTPUT);
  stopMotors();
}

void initializeSensors() {
  for (const uint8_t pin : pins::kIr) {
    pinMode(pin, INPUT);
  }
}

int readSensors() {
  int weightedPosition = 0;
  uint8_t activeCount = 0;

  for (uint8_t index = 0; index < 5U; ++index) {
    const bool rawHigh = digitalRead(pins::kIr[index]) == HIGH;
    const bool detected =
        config::kLineDetectedWhenLow ? !rawHigh : rawHigh;
    sensors[index] = detected ? 1U : 0U;
    if (detected) {
      ++activeCount;
      weightedPosition += static_cast<int>((index + 1U) * 10U);
    }
  }

  if (sensors[0] == 1U) {
    lastEdge = LastEdge::kLeft;
  } else if (sensors[4] == 1U) {
    lastEdge = LastEdge::kRight;
  }

  if (activeCount == 0U) {
    return config::kNoLineError;
  }
  return weightedPosition / activeCount - 30;
}

bool allSensorsActive() {
  return sensors[0] == 1U && sensors[1] == 1U && sensors[2] == 1U &&
         sensors[3] == 1U && sensors[4] == 1U;
}

void turnRight() {
  wheel(config::kRightTurnSpeedLeft, -config::kRightTurnSpeedRight);
  delay(config::kRightTurnDurationMs);
}

void followLine() {
  const int error = readSensors();

  if (error == config::kNoLineError) {
    if (lastEdge == LastEdge::kLeft) {
      wheel(-config::kRecoverySpeedLeft, config::kRecoverySpeedRight);
    } else if (lastEdge == LastEdge::kRight) {
      wheel(config::kRecoverySpeedLeft, -config::kRecoverySpeedRight);
    } else {
      stopMotors();
    }
    return;
  }

  if (allSensorsActive()) {
    turnRight();
    return;
  }

  const float proportional = config::kKp * error;
  const float derivative = config::kKd * (error - lastError);
  const int correction = static_cast<int>(proportional + derivative);

  if (abs(error) < config::kStraightErrorThreshold) {
    wheel(config::kStraightSpeedLeft + correction,
          config::kStraightSpeedRight - correction);
  } else {
    wheel(config::kCurveBaseLeft + correction,
          config::kCurveBaseRight - correction);
  }

  if (error != lastError) {
    delay(config::kChangedErrorDelayMs);
  }
  lastError = error;
}

void sendActiveTag() {
  if (activeTagFrame[0] == '\0') {
    return;
  }
  Serial.print(activeTagFrame);
  lastTagSentAtMs = millis();
}

void finishCardStop() {
  if (motionState != MotionState::kStoppedByCard) {
    return;
  }
  const uint16_t completedSequence = activeCardSequence;
  motionState = motionStateBeforeCard;
  activeCardAcknowledged = false;
  activeTagFrame[0] = '\0';
  (void)completedSequence;
}

void serviceEspLink() {
  while (Serial.available() > 0) {
    const char incoming = static_cast<char>(Serial.read());
    if (incoming == '\r') {
      continue;
    }
    if (incoming == '\n') {
      linkBuffer[linkLength] = '\0';
      uint16_t sequence = 0;
      if (robot_uart::parseAck(linkBuffer, sequence) &&
          motionState == MotionState::kStoppedByCard &&
          sequence == activeCardSequence) {
        activeCardAcknowledged = true;
      }
      linkLength = 0;
      continue;
    }
    if (incoming >= ' ' && incoming <= '~' &&
        linkLength + 1U < sizeof(linkBuffer)) {
      linkBuffer[linkLength++] = incoming;
    } else if (incoming >= ' ') {
      linkLength = 0;
    }
  }
}

void uidToHex(char* output, const size_t capacity) {
  static const char kHex[] = "0123456789ABCDEF";
  if (capacity == 0U) {
    return;
  }
  const size_t uidSize = min(static_cast<size_t>(rfid.uid.size),
                             (capacity - 1U) / 2U);
  for (size_t index = 0; index < uidSize; ++index) {
    output[index * 2U] = kHex[rfid.uid.uidByte[index] >> 4U];
    output[index * 2U + 1U] = kHex[rfid.uid.uidByte[index] & 0x0FU];
  }
  output[uidSize * 2U] = '\0';
}

bool pollRfid() {
  if (motionState == MotionState::kStoppedByCard ||
      !rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return false;
  }

  stopMotors();
  motionStateBeforeCard = motionState;
  motionState = MotionState::kStoppedByCard;
  cardStoppedAtMs = millis();
  activeCardAcknowledged = false;
  ++eventSequence;
  if (eventSequence == 0U) {
    ++eventSequence;
  }
  activeCardSequence = eventSequence;

  robot_uart::RfidEvent event{};
  event.sequence = activeCardSequence;
  uidToHex(event.uidHex, sizeof(event.uidHex));
  event.sak = rfid.uid.sak;
  const MFRC522::PICC_Type piccType = MFRC522::PICC_GetType(rfid.uid.sak);
  event.piccType = static_cast<uint8_t>(piccType);
  if (robot_uart::encodeRfidFrame(event, activeTagFrame,
                                  sizeof(activeTagFrame)) > 0U) {
    sendActiveTag();
  } else {
    activeTagFrame[0] = '\0';
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  return true;
}

void serviceCardStop() {
  if (motionState != MotionState::kStoppedByCard) {
    return;
  }
  stopMotors();
  const uint32_t now = millis();
  if (!activeCardAcknowledged &&
      now - lastTagSentAtMs >= config::kTagRetryIntervalMs) {
    sendActiveTag();
  }
  if (now - cardStoppedAtMs >= config::kCardStopDurationMs) {
    finishCardStop();
  }
}

}  // namespace

void setup() {
  // D0/RX and D1/TX are the Nano's only hardware UART. Disconnect the ESP32
  // wires during Nano upload and USB monitoring, then reconnect for operation.
  Serial.begin(robot_uart::kBaud);
  initializeSensors();
  initializeMotors();
  SPI.begin();
  rfid.PCD_Init();
  stopMotors();
  Serial.println(F("HELLO|1"));
  delay(config::kStartupSafetyDelayMs);
  motionState = config::kEnableMotionAfterStartup
                    ? MotionState::kRunning
                    : MotionState::kStoppedByCommand;
}

void loop() {
  serviceEspLink();
  pollRfid();
  serviceCardStop();

  if (motionState == MotionState::kRunning) {
    followLine();
  } else {
    stopMotors();
  }
}
