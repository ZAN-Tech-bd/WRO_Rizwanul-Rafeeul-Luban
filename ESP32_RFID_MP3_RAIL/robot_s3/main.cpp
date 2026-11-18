#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <DFRobotDFPlayerMini.h>
#include <WiFi.h>
#include <esp_ota_ops.h>
#include <atomic>
#include "BleHub.h"
#include "EspNowCompat.h"
#include "MuseumProtocol.h"

// =====================================
// PINS (ESP32-S3)
// =====================================
// L298N
#define ENA  4    // Left speed (PWM)
#define IN1  5
#define IN2  6
#define IN3  7
#define IN4  15
#define ENB  16   // Right speed (PWM)

// RFID RC522
#define RFID_SS    10
#define RFID_RST   9
#define RFID_SCK   12
#define RFID_MOSI  11
#define RFID_MISO  13

// DFPlayer
#define DF_RX_PIN  17
#define DF_TX_PIN  14
#define BUSY_PIN   8     // LOW = playing

// IR obstacle
#define IR_PIN     2     // LOW = obstacle
#define CAM_TX_PIN 18    // Robot J8.3 -> ESP-CAM GPIO13

// =====================================
// CARDS
// =====================================
struct CardMap { const char* uid; int track; uint8_t station; };

const CardMap cards[] = {
  { "64 14 F6 B3", 1, 1 },
  { "4A 6B DF 34", 2, 2 },
  { "D8 36 D4 B9", 3, 3 },
  { "94 60 6B B2", 4, 4 },
  { "18 48 F4 BA", 5, 5 },
  { "94 58 1A B3", 6, 6 },
  { "83 54 6A 28", 8, 0 },   // Start/intro card; no station photo.
};

const char* END_CARD  = "13 8C 5A 28";   // stop audio + stop robot
const char* EXIT_CARD = "D8 36 D4 B9";   // S3 also stops an emergency run.
const int START_TRACK = 1;
const int IR_TRACK    = 9;
constexpr uint8_t ROBOT_NODE_ID = 20;
constexpr uint32_t PHOTO_DELAY_MS = 2000;

// =====================================
// SETTINGS
// =====================================
int speedValue  = 200;                    // 0-255
int volumeLevel = 20;                     // 0-30
const unsigned long MAX_AUDIO_MS = 90000; // safety: max clip length

// =====================================
// OBJECTS
// =====================================
MFRC522 rfid(RFID_SS, RFID_RST);
HardwareSerial dfSerial(1);
HardwareSerial camSerial(2);
DFRobotDFPlayerMini player;

// =====================================
// STATE
// =====================================
enum State { IDLE, PLAYING, MOVING, OBSTACLE };
State state = IDLE;
std::atomic<bool> idleForOta{true};
std::atomic<bool> hubEmergency{false}, hubMute{false};
bool emergency = false, muted = false, espNowReady = false, canReport = false;
uint8_t currentStation = 0;
uint32_t photoArmedAt = 0;
bool photoPending = false;
uint16_t photoTriggers = 0;

int lastCardTrack = 0;
unsigned long moveStartTime = 0;

// audio
bool audioActive  = false;
bool audioStarted = false;
int  audioTrack   = 0;
int  audioRetry   = 0;
unsigned long audioCmdTime = 0;
unsigned long busyHighTime = 0;

// RFID
String lastUID = "";
unsigned long lastUIDTime = 0;

// IR
int irStable = HIGH, irLastRaw = HIGH;
unsigned long irChangeTime = 0, irClearTime = 0;

// =====================================
// MOTORS
// =====================================
void forward(int spd) {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, spd);
  analogWrite(ENB, spd);
}

void stopMotors() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
}

// =====================================
// AUDIO
// =====================================
void playTrack(int t) {
  player.playMp3Folder(t);
  audioTrack   = t;
  audioActive  = true;
  audioStarted = false;
  busyHighTime = 0;
  audioCmdTime = millis();
  Serial.printf(">> Play %04d.mp3\n", t);
}

void startAudio(int t) {
  audioRetry = 0;
  playTrack(t);
}

void stopAudio() {
  player.stop();
  audioActive = false;
}

void audioDone(const char* reason) {
  audioActive = false;
  Serial.printf("   Audio %04d ended (%s)\n", audioTrack, reason);
}

// Detects audio end 3 ways: BUSY pin, DFPlayer "finished" msg, timeout
void updateAudio() {
  bool finishedMsg = false;

  if (player.available()) {
    uint8_t type = player.readType();
    int value = player.read();
    if (type == DFPlayerPlayFinished && millis() - audioCmdTime > 1000) {
      finishedMsg = true;
    } else if (type == DFPlayerError) {
      Serial.printf("   DFPlayer ERROR %d\n", value);
    }
  }

  if (!audioActive) return;

  unsigned long elapsed = millis() - audioCmdTime;

  if (finishedMsg)            { audioDone("finish msg"); return; }
  if (elapsed > MAX_AUDIO_MS) { audioDone("timeout");    return; }
  if (elapsed < 300) return;                  // let DFPlayer react

  bool busy = (digitalRead(BUSY_PIN) == LOW);

  if (!audioStarted) {
    if (busy) {
      audioStarted = true;
      Serial.printf("   Audio %04d started\n", audioTrack);
    } else if (elapsed > 2000) {
      if (audioRetry < 1) {
        audioRetry++;
        Serial.println("   Audio did not start -> retry");
        delay(100);
        playTrack(audioTrack);
      } else {
        audioDone("FAILED to start");
      }
    }
    return;
  }

  // BUSY HIGH for 200ms = finished
  if (busy) {
    busyHighTime = 0;
  } else {
    if (busyHighTime == 0) busyHighTime = millis();
    if (millis() - busyHighTime > 200) audioDone("busy pin");
  }
}

// =====================================
// IR
// =====================================
void updateIR() {
  int raw = digitalRead(IR_PIN);
  if (raw != irLastRaw) { irLastRaw = raw; irChangeTime = millis(); }
  if (millis() - irChangeTime > 50 && raw != irStable) {
    irStable = raw;
    if (irStable == HIGH) irClearTime = millis();
    Serial.println(irStable == LOW ? "IR: obstacle" : "IR: clear");
  }
}

bool obstacle() { return irStable == LOW; }

// =====================================
// ROBOT ACTIONS
// =====================================
void goForward() {
  if (obstacle()) {
    stopMotors();
    if (!emergency) startAudio(IR_TRACK);
    state = OBSTACLE;
    Serial.println("[OBSTACLE] waiting for clear path");
    return;
  }
  forward(speedValue);
  moveStartTime = millis();
  state = MOVING;
  Serial.printf("[MOVING] forward, speed %d\n", speedValue);
}

void stopAndPlay(int t) {
  stopMotors();                    // stop instantly
  lastCardTrack = t;
  startAudio(t);
  state = PLAYING;                 // audio end -> goForward()
  Serial.printf("[STOP] card %d -> playing audio\n", t);
}

void endTour() {
  stopMotors();
  stopAudio();
  lastCardTrack = 0;
  currentStation = 0;
  photoPending = false;
  state = IDLE;
  idleForOta.store(!emergency);
  Serial.println("#### END CARD - audio OFF, robot STOPPED ####");
  Serial.println("Scan any card to run again");
}

// =====================================
// RFID
// =====================================
String readUID() {
  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(rfid.uid.uidByte[i], HEX);
    if (i < rfid.uid.size - 1) uid += " ";
  }
  uid.toUpperCase();
  return uid;
}

const CardMap* findCard(const String &uid) {
  for (const auto& card : cards) if (uid == card.uid) return &card;
  return nullptr;
}

bool armTour() {
  idleForOta.store(false);
  if (!bleHubTransferActive()) return true;
  idleForOta.store(state == IDLE && !emergency);
  return false;
}

void onCard(const String &uid) {
  Serial.printf("Card: %s\n", uid.c_str());

  // END card -> stop everything
  if (uid == END_CARD || (emergency && uid == EXIT_CARD)) {
    endTour();
    return;
  }

  if (emergency) return;
  const CardMap* card = findCard(uid);
  if (!card) {
    Serial.println("   Unknown card");
    return;
  }

  if (state != IDLE && card->track == lastCardTrack &&
      (state == PLAYING || millis() - moveStartTime < 4000)) {
    return;
  }

  if (!armTour()) return;
  currentStation = card->station;
  stopAndPlay(card->track);
  photoArmedAt = millis();
  photoPending = currentStation != 0;
}

void handleRFID() {
  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  String uid = readUID();
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  if (uid == lastUID && millis() - lastUIDTime < 1500) {
    lastUIDTime = millis();
    return;
  }
  lastUID = uid;
  lastUIDTime = millis();
  onCard(uid);
}

// =====================================
// STATE MACHINE
// =====================================
void updateRobot() {
  switch (state) {
    case IDLE:
      break;

    case PLAYING:                      // audio ends -> go forward
      if (!audioActive) goForward();
      break;

    case MOVING:
      if (obstacle()) {
        stopMotors();
        if (!emergency) startAudio(IR_TRACK);
        state = OBSTACLE;
        Serial.println("[OBSTACLE] stopped");
      }
      break;

    case OBSTACLE:
      if (!obstacle() && !audioActive && millis() - irClearTime > 1000) {
        Serial.println("Path clear -> resume");
        goForward();
      }
      break;
  }
}

// =====================================
// SERIAL COMMANDS
// 0-255 speed | x stop | g start | p3 play | v20 volume
// =====================================
void handleSerial() {
  if (!Serial.available()) return;
  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  cmd.toLowerCase();
  if (cmd.length() == 0) return;
  if ((emergency || bleHubTransferActive()) && cmd != "x") return;

  if (isDigit(cmd.charAt(0))) {
    speedValue = constrain(cmd.toInt(), 0, 255);
    Serial.printf("Speed = %d\n", speedValue);
    if (state == MOVING) forward(speedValue);
  } else if (cmd == "x") {
    endTour();
  } else if (cmd == "g") {
    if (armTour()) {
      currentStation = 0;
      photoPending = false;
      stopAndPlay(START_TRACK);
    }
  } else if (cmd.charAt(0) == 'p') {
    startAudio(cmd.substring(1).toInt());
  } else if (cmd.charAt(0) == 'v') {
    volumeLevel = constrain(cmd.substring(1).toInt(), 0, 30);
    player.volume(muted ? 0 : volumeLevel);
    Serial.printf("Volume = %d\n", volumeLevel);
  }
}

void onEspNowReceived(const EspNowRecvInfo*, const uint8_t* data, int size) {
  if (size != sizeof(museum::MuseumMessage)) return;
  museum::MuseumMessage message;
  memcpy(&message, data, sizeof(message));
  if (!museum::isValidMessage(message) || message.nodeId != 10) return;
  if (message.messageType == museum::MSG_EMERGENCY)
    hubEmergency.store((message.flags & museum::FLAG_EMERGENCY_ACTIVE) != 0);
  else if (message.messageType == museum::MSG_AUDIO)
    hubMute.store((message.flags & museum::FLAG_AUDIO_MUTED) != 0);
}

void startEspNow() {
  // Use the same AP-mode radio setup as the Hub while BLE runs alongside it.
  if (!WiFi.mode(WIFI_AP) ||
      !WiFi.softAP("MuseumRobot", "museum2026", museum::kEspNowChannel, true) ||
      esp_now_init() != ESP_OK) return;
  espNowReady = esp_now_register_recv_cb(onEspNowReceived) == ESP_OK;
  esp_now_peer_info_t peer{};
  memcpy(peer.peer_addr, museum::kBroadcastAddress, sizeof(peer.peer_addr));
  peer.channel = museum::kEspNowChannel;
  peer.ifidx = WIFI_IF_AP;
  canReport = esp_now_add_peer(&peer) == ESP_OK;
}

void applyHubControls() {
  const bool alarm = hubEmergency.load();
  if (alarm != emergency) {
    emergency = alarm;
    // A robot already touring runs toward an exit card, subject to its IR
    // obstacle stop. An idle robot stays parked and an alarm clear stops it.
    photoPending = false;
    if (emergency && state != IDLE && !bleHubTransferActive()) {
      idleForOta.store(false);
      stopAudio();
      goForward();
    } else {
      endTour();
    }
    Serial.printf("Hub emergency %s\n", emergency ? "ON" : "OFF");
  }
  const bool mute = hubMute.load();
  if (mute != muted) {
    muted = mute;
    player.volume(muted ? 0 : volumeLevel);
  }
}

void reportLocation() {
  if (!canReport) return;
  static uint8_t lastStation = 255;
  static bool lastMoving = false;
  static uint32_t lastSent = 0;
  static uint16_t sequence = 0;
  const bool moving = state == MOVING;
  const uint32_t now = millis();
  if (currentStation == lastStation && moving == lastMoving && now - lastSent < 1000) return;
  auto message = museum::makeMessage(museum::MSG_LOCATION, ROBOT_NODE_ID);
  message.stationId = currentStation;
  message.flags = moving ? museum::FLAG_ROBOT_MOVING : museum::FLAG_NONE;
  message.uptimeMs = now;
  message.sequence = ++sequence;
  museum::finalizeMessage(message);
  esp_now_send(museum::kBroadcastAddress, reinterpret_cast<const uint8_t*>(&message), sizeof(message));
  lastSent = now;
  lastStation = currentStation;
  lastMoving = moving;
}

void updateCamera() {
  if (!photoPending) return;
  if (state != PLAYING || emergency || !audioActive) {
    photoPending = false;
    return;
  }
  if (millis() - photoArmedAt < PHOTO_DELAY_MS) return;
  camSerial.printf("SHOOT %u\n", unsigned(currentStation));
  Serial.printf("Camera: SHOOT %u\n", unsigned(currentStation));
  photoTriggers++;
  photoPending = false;
}

bool allowOta() { return idleForOta.load() && !hubEmergency.load(); }

void publishDiagnostics() {
  static const uint32_t boot = esp_random();
  static const auto* partition = esp_ota_get_running_partition();
  static const uint8_t* build = esp_ota_get_app_description()->app_elf_sha256;
  static const char* const states[] = {"idle", "playing", "moving", "obstacle"};
  static uint32_t last = 0, lastSerial = 0;
  const uint32_t now = millis();
  if (last && now - last < 250) return;
  last = now;
  char status[256];
  snprintf(status, sizeof(status),
           "NOV_ROBOT boot=%08lx slot=%s build=%02x%02x%02x%02x up_ms=%lu "
           "state=%s station=%u moving=%u emergency=%s muted=%u ir=%s espnow=%s tx=%s photos=%u",
           static_cast<unsigned long>(boot), partition ? partition->label : "unknown",
           unsigned(build[0]), unsigned(build[1]), unsigned(build[2]), unsigned(build[3]),
           static_cast<unsigned long>(now), states[state], unsigned(currentStation), unsigned(state == MOVING),
           emergency ? "on" : "off", unsigned(muted), obstacle() ? "blocked" : "clear",
           espNowReady ? "ready" : "failed", canReport ? "ready" : "failed", unsigned(photoTriggers));
  bleHubSetDiagnostics(status);
  if (!lastSerial || now - lastSerial >= 5000) {
    Serial.println(status);
    lastSerial = now;
  }
}

// =====================================
// SETUP
// =====================================
void setup() {
  Serial.begin(115200);
  Serial.setTimeout(20);

  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENB, OUTPUT);
  stopMotors();

  pinMode(BUSY_PIN, INPUT_PULLUP);
  pinMode(IR_PIN, INPUT_PULLUP);
  irStable = irLastRaw = digitalRead(IR_PIN);
  irClearTime = millis();

  delay(500);
  Serial.println();
  Serial.println("===== MUSEUM ROBOT =====");

  camSerial.begin(9600, SERIAL_8N1, -1, CAM_TX_PIN);

  SPI.begin(RFID_SCK, RFID_MISO, RFID_MOSI, RFID_SS);
  rfid.PCD_Init();
  Serial.println("RFID ready");

  dfSerial.begin(9600, SERIAL_8N1, DF_RX_PIN, DF_TX_PIN);
  delay(1500);
  if (!player.begin(dfSerial)) {
    Serial.println("DFPlayer no ACK -> clone mode");
    player.begin(dfSerial, false);
    delay(1000);
  }
  delay(200);
  player.volume(volumeLevel);
  Serial.println("DFPlayer ready");

  startEspNow();
  Serial.println(bleHubBegin("MuseumRobot-BLE-OTA", allowOta) ? "Bluetooth OTA ready." : "Bluetooth OTA failed.");

  // A card already under the reader during a reset must be removed and tapped again.
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }

  Serial.println("Commands: 0-255 speed | x stop | g start | p3 play | v20 volume");
  Serial.println("Scan any card to begin...");
}

// =====================================
// LOOP
// =====================================
void loop() {
  updateIR();
  applyHubControls();
  if (state == IDLE && bleHubTransferActive()) {
    reportLocation();
    publishDiagnostics();
    bleHubPoll(!allowOta());
    delay(2);
    return;
  }
  handleSerial();
  handleRFID();
  updateAudio();
  updateCamera();
  updateRobot();
  reportLocation();
  publishDiagnostics();
  delay(2);
}
