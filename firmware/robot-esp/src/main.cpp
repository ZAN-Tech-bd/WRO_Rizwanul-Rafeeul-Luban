#include <Arduino.h>
#include <DFRobotDFPlayerMini.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "RobotUartProtocol.h"

namespace pins {

// Nano hardware UART D1/TX -> 10k/20k divider -> ESP32 RX.
constexpr int8_t kNanoRx = 16;
// ESP32 TX -> Nano hardware UART D0/RX.
constexpr int8_t kNanoTx = 17;

constexpr int8_t kDfRx = 25;  // DFPlayer TX -> protected ESP32 RX
constexpr int8_t kDfTx = 26;  // ESP32 TX -> 1 kOhm -> DFPlayer RX

}  // namespace pins

namespace config {

constexpr uint32_t kUsbBaud = 115200;
constexpr uint32_t kDfBaud = 9600;
constexpr uint8_t kPointCount = 7;
constexpr uint8_t kDefaultLanguage = 0;  // 0=English, 1=Bangla
constexpr uint8_t kDefaultVolume = 20;
constexpr uint32_t kDuplicateWindowMs = 10000;
constexpr size_t kUsbCommandSize = 48;

}  // namespace config

namespace {

struct PointIdentity {
  const char* canonicalUid;
  const char* legacyUid;
  const char* name;
};

// Point order and names supplied from the physically scanned cards.
// Point 7 was printed by the legacy sketch without zero padding, so it is
// intentionally matched through legacyUid instead of an assumed byte split.
constexpr PointIdentity kPoints[config::kPointCount] = {
    {"6414F6B3", nullptr, "Terracotta Plaque"},
    {"4A6BDF34", nullptr, "Traditional Wooden Boat"},
    {"D836D4B9", nullptr, "Nakshi Katha"},
    {"94606BB2", nullptr, "Traditional Pottery"},
    {"1848F4BA", nullptr, "Traditional Metal Chair"},
    {"94581AB3", nullptr, "Bamboo and Cane Basket"},
    {nullptr, "94CB192", "Jamdani Saree"},
};

HardwareSerial nanoLink(1);
HardwareSerial dfSerial(2);
DFRobotDFPlayerMini dfPlayer;

bool dfReady = false;
uint8_t currentLanguage = config::kDefaultLanguage;
uint8_t currentVolume = config::kDefaultVolume;
uint8_t currentPoint = 0;

char nanoBuffer[robot_uart::kMaxFrameLength]{};
size_t nanoLength = 0;
char usbBuffer[config::kUsbCommandSize]{};
size_t usbLength = 0;

uint16_t lastSequence = 0;
char lastUid[robot_uart::kUidBufferSize]{};
uint32_t lastAcceptedAtMs = 0;

uint8_t trackForPoint(const uint8_t point) {
  return currentLanguage == 1U ? point + config::kPointCount : point;
}

bool canonicalToLegacy(const char* canonical, char* output,
                       const size_t capacity) {
  if (canonical == nullptr || output == nullptr || capacity == 0U) return false;
  const size_t length = strlen(canonical);
  if (length < 2U || (length % 2U) != 0U) return false;

  static const char kHex[] = "0123456789ABCDEF";
  size_t written = 0;
  for (size_t index = 0; index < length; index += 2U) {
    const int8_t high = robot_uart::hexNibble(canonical[index]);
    const int8_t low = robot_uart::hexNibble(canonical[index + 1U]);
    if (high < 0 || low < 0) return false;
    const uint8_t value = static_cast<uint8_t>((high << 4) | low);
    const size_t needed = value < 0x10U ? 1U : 2U;
    if (written + needed + 1U > capacity) return false;
    if (value >= 0x10U) output[written++] = kHex[value >> 4U];
    output[written++] = kHex[value & 0x0FU];
  }
  output[written] = '\0';
  return true;
}

uint8_t pointForUid(const char* uid) {
  if (uid == nullptr) return 0;
  char legacyUid[robot_uart::kUidBufferSize]{};
  if (!canonicalToLegacy(uid, legacyUid, sizeof(legacyUid))) return 0;

  for (uint8_t index = 0; index < config::kPointCount; ++index) {
    const PointIdentity& point = kPoints[index];
    if ((point.canonicalUid != nullptr &&
         strcmp(uid, point.canonicalUid) == 0) ||
        (point.legacyUid != nullptr &&
         strcmp(legacyUid, point.legacyUid) == 0)) {
      return index + 1U;
    }
  }
  return 0;
}

void sendAck(const uint16_t sequence) {
  char frame[16]{};
  if (robot_uart::encodeAck(sequence, frame, sizeof(frame)) > 0U) {
    nanoLink.print(frame);
  }
}

bool isDuplicate(const robot_uart::RfidEvent& event) {
  return event.sequence == lastSequence && strcmp(event.uidHex, lastUid) == 0 &&
         millis() - lastAcceptedAtMs < config::kDuplicateWindowMs;
}

void playPoint(const uint8_t point) {
  currentPoint = point;
  const uint8_t track = trackForPoint(point);
  if (dfReady) dfPlayer.playMp3Folder(track);

  Serial.print(F("[point] id="));
  Serial.print(point);
  Serial.print(F(" name=\""));
  Serial.print(kPoints[point - 1U].name);
  Serial.print(F("\" track="));
  Serial.print(track);
  Serial.println(dfReady ? F(" playing") : F(" DFPLAYER_NOT_READY"));
}

void acceptRfidEvent(const robot_uart::RfidEvent& event) {
  // ACK every valid frame, including retries, so the Nano can stop retrying.
  sendAck(event.sequence);
  if (isDuplicate(event)) {
    Serial.print(F("[rfid] duplicate retry sequence="));
    Serial.println(event.sequence);
    return;
  }

  lastSequence = event.sequence;
  strncpy(lastUid, event.uidHex, sizeof(lastUid) - 1U);
  lastUid[sizeof(lastUid) - 1U] = '\0';
  lastAcceptedAtMs = millis();

  const uint8_t point = pointForUid(event.uidHex);
  Serial.print(F("[rfid] sequence="));
  Serial.print(event.sequence);
  Serial.print(F(" uid="));
  Serial.print(event.uidHex);
  Serial.print(F(" sak=0x"));
  if (event.sak < 0x10U) Serial.print('0');
  Serial.print(event.sak, HEX);
  Serial.print(F(" type="));
  Serial.print(event.piccType);
  Serial.print(F(" point="));
  Serial.println(point);

  if (point > 0U) {
    playPoint(point);
  } else {
    currentPoint = 0;
    Serial.println(F("[rfid] unknown card; ACK sent, no track played"));
  }
}

void handleNanoLine(char* line) {
  if (robot_uart::isHello(line)) {
    lastSequence = 0;
    lastUid[0] = '\0';
    lastAcceptedAtMs = 0;
    Serial.println(F("[nano] HELLO protocol=1; duplicate state reset"));
    return;
  }

  robot_uart::RfidEvent event{};
  if (robot_uart::parseRfidFrame(line, event)) {
    acceptRfidEvent(event);
  } else {
    Serial.println(F("[nano] rejected invalid frame"));
  }
}

void serviceNanoLink() {
  while (nanoLink.available() > 0) {
    const char incoming = static_cast<char>(nanoLink.read());
    if (incoming == '\r') continue;
    if (incoming == '\n') {
      nanoBuffer[nanoLength] = '\0';
      if (nanoLength > 0U) handleNanoLine(nanoBuffer);
      nanoLength = 0;
      continue;
    }
    if (incoming >= ' ' && incoming <= '~' &&
        nanoLength + 1U < sizeof(nanoBuffer)) {
      nanoBuffer[nanoLength++] = incoming;
    } else if (incoming >= ' ') {
      nanoLength = 0;
      Serial.println(F("[nano] oversized frame discarded"));
    }
  }
}

void printState() {
  Serial.print(F("[state] df_ready="));
  Serial.print(dfReady ? F("yes") : F("no"));
  Serial.print(F(" language="));
  Serial.print(currentLanguage == 1U ? F("Bangla") : F("English"));
  Serial.print(F(" volume="));
  Serial.print(currentVolume);
  Serial.print(F(" point="));
  Serial.print(currentPoint);
  Serial.print(F(" last_uid="));
  Serial.print(lastUid[0] == '\0' ? "none" : lastUid);
  Serial.print(F(" last_sequence="));
  Serial.println(lastSequence);
}

void printHelp() {
  Serial.println(F("[cmd] HELP | STATE | LANG <0|1> | VOL <0..30>"));
  Serial.println(F("[cmd] POINT <1..7> | STOP"));
}

bool parseBoundedNumber(const char* text, const uint8_t maximum,
                        uint8_t& value) {
  if (text == nullptr || *text == '\0') return false;
  char* end = nullptr;
  const unsigned long parsed = strtoul(text, &end, 10);
  if (*end != '\0' || parsed > maximum) return false;
  value = static_cast<uint8_t>(parsed);
  return true;
}

void handleUsbCommand(char* line) {
  char* save = nullptr;
  const char* command = strtok_r(line, " ", &save);
  if (command == nullptr) return;

  if (strcasecmp(command, "HELP") == 0) {
    printHelp();
  } else if (strcasecmp(command, "STATE") == 0) {
    printState();
  } else if (strcasecmp(command, "STOP") == 0) {
    if (dfReady) dfPlayer.stop();
    Serial.println(F("[audio] stopped"));
  } else if (strcasecmp(command, "LANG") == 0) {
    uint8_t language = 0;
    if (!parseBoundedNumber(strtok_r(nullptr, " ", &save), 1, language)) {
      Serial.println(F("[cmd] usage: LANG <0|1>"));
      return;
    }
    currentLanguage = language;
    Serial.println(currentLanguage == 1U ? F("[audio] language=Bangla")
                                         : F("[audio] language=English"));
  } else if (strcasecmp(command, "VOL") == 0) {
    uint8_t volume = 0;
    if (!parseBoundedNumber(strtok_r(nullptr, " ", &save), 30, volume)) {
      Serial.println(F("[cmd] usage: VOL <0..30>"));
      return;
    }
    currentVolume = volume;
    if (dfReady) dfPlayer.volume(currentVolume);
    Serial.print(F("[audio] volume="));
    Serial.println(currentVolume);
  } else if (strcasecmp(command, "POINT") == 0) {
    uint8_t point = 0;
    if (!parseBoundedNumber(strtok_r(nullptr, " ", &save),
                            config::kPointCount, point) ||
        point == 0U) {
      Serial.println(F("[cmd] usage: POINT <1..7>"));
      return;
    }
    playPoint(point);
  } else {
    Serial.println(F("[cmd] unknown; send HELP"));
  }
}

void serviceUsbSerial() {
  while (Serial.available() > 0) {
    const char incoming = static_cast<char>(Serial.read());
    if (incoming == '\r') continue;
    if (incoming == '\n') {
      usbBuffer[usbLength] = '\0';
      handleUsbCommand(usbBuffer);
      usbLength = 0;
      continue;
    }
    if (incoming >= ' ' && incoming <= '~' &&
        usbLength + 1U < sizeof(usbBuffer)) {
      usbBuffer[usbLength++] = incoming;
    } else if (incoming >= ' ') {
      usbLength = 0;
      Serial.println(F("[cmd] line too long; discarded"));
    }
  }
}

void serviceDfPlayer() {
  if (!dfReady || !dfPlayer.available()) return;
  const uint8_t type = dfPlayer.readType();
  const int value = dfPlayer.read();
  if (type == DFPlayerPlayFinished) {
    Serial.print(F("[df] finished track="));
    Serial.println(value);
  } else if (type == DFPlayerError) {
    Serial.print(F("[df] error="));
    Serial.println(value);
  }
}

}  // namespace

void setup() {
  Serial.begin(config::kUsbBaud);
  nanoLink.begin(robot_uart::kBaud, SERIAL_8N1, pins::kNanoRx,
                 pins::kNanoTx);
  dfSerial.begin(config::kDfBaud, SERIAL_8N1, pins::kDfRx, pins::kDfTx);

  Serial.println(F("[robot-esp] Nano RFID receiver + DFPlayer"));
  Serial.println(F("[pins] Nano UART1 RX=16 TX=17 at 9600"));
  Serial.println(F("[pins] DFPlayer UART2 RX=25 TX=26 at 9600"));

  dfPlayer.setTimeOut(500);
  dfReady = dfPlayer.begin(dfSerial, true, false);
  if (dfReady) {
    dfPlayer.volume(currentVolume);
    Serial.println(F("[df] ready"));
  } else {
    Serial.println(F("[df] not detected; RFID events still work"));
  }
  printHelp();
}

void loop() {
  serviceNanoLink();
  serviceUsbSerial();
  serviceDfPlayer();
}
