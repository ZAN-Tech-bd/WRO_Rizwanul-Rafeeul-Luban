/*
  ESP32 RFID -> DFPlayer Mini + Nano rail controller
  =================================================
  Scans MIFARE cards with an MFRC522 reader (SPI), controls the Nano rail over
  UART1, and plays the MP3 track mapped to that card's UID on DFPlayer UART2.

  RFID reader setup copied from your working MFRC522v2 sketch.
  DFPlayer setup copied from your working ESP32_MP3_LFR sketch.

  ----------------------------------------------------------------------
  Wiring (ESP32)
  ----------------------------------------------------------------------
    MFRC522 (SPI, default VSPI pins):
      SDA / SS  -> GPIO5
      SCK       -> GPIO18
      MOSI      -> GPIO23
      MISO      -> GPIO19
      RST       -> (not used in this MFRC522v2 setup, same as your reader sketch)
      3V3, GND
    DFPlayer Mini (UART2):
      DFPlayer TX -> ESP32 GPIO16 (RX2)
      DFPlayer RX -> ESP32 GPIO17 (TX2)   (1k series resistor recommended)
      VCC -> 5V, GND, speaker to SPK_1/SPK_2

  ----------------------------------------------------------------------
  Nano rail UART
    Nano D1 (TX) --10k--+-- ESP32 GPIO25 (RX)
                         +-- 20k -- GND       (5 V -> 3.3 V divider)
    Nano D0 (RX) <------ ESP32 GPIO26 (TX)
    Common ground. Disconnect both wires while flashing the Nano.

  UID -> Track / rail action table
    Bytes are in the SAME order the reader prints them, i.e.
    "Card UID: 64 14 F6 B3"  ->  {0x64, 0x14, 0xF6, 0xB3}.
    Any mapped audio card -> STOP rail, play audio, START rail after finish
    18 C4 0F BA -> play 0001.mp3, then START rail
    A3 C0 38 15 -> STOP rail latch, no audio, no automatic restart
    The remaining cards map to tracks 2..8. All entries are full 4-byte UIDs.
  ----------------------------------------------------------------------
*/

#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522Debug.h>
#include <DFRobotDFPlayerMini.h>

// ============================================================
// DFPlayer (UART2)  -- proven setup from ESP32_MP3_LFR
// ============================================================
#define DF_RX 16   // ESP32 RX2 <- DFPlayer TX
#define DF_TX 17   // ESP32 TX2 -> DFPlayer RX
HardwareSerial dfSerial(2);
DFRobotDFPlayerMini player;

int currentVolume = 20;   // 0..30
bool audioReady = false;

// Station audio pauses the rail and resumes it only after DFPlayer reports
// playback finished. The explicit end card latches the rail stopped; only the
// start card clears that latch.
bool narrating = false;
bool resumeAfterAudio = false;
bool railStopLatched = true;  // boot is stopped until the START card is read
unsigned long audioStartMs = 0;
const unsigned long AUDIO_MIN_MS = 1500;   // ignore stale finished events
const unsigned long AUDIO_MAX_MS = 90000;  // fail-safe if DFPlayer never reports finish

// ============================================================
// Nano rail controller (UART1)
// ============================================================
#define NANO_RX 25   // ESP32 RX1 <- Nano D1 TX through a divider
#define NANO_TX 26   // ESP32 TX1 -> Nano D0 RX
HardwareSerial nanoSerial(1);
const unsigned long NANO_BAUD = 9600;  // MUST match NANO_LFR_RAIL
char nanoBuf[64];
uint8_t nanoLen = 0;

// ============================================================
// MFRC522 (SPI)
// ============================================================
MFRC522DriverPinSimple ss_pin(5);
MFRC522DriverSPI driver{ss_pin};
MFRC522 mfrc522{driver};

// ============================================================
// RFID UID -> Track mapping
// ============================================================
struct RfidTrack {
  uint8_t uid[7];
  uint8_t uidSize;   // number of meaningful bytes in uid[]
  int    track;
};

const int NO_AUDIO_TRACK = 0;  // mapped action only, used by the STOP card
const int UNKNOWN_TRACK  = -1;

const RfidTrack TRACKS[] = {
  { {0x18, 0xC4, 0x0F, 0xBA}, 4, 1 },   // 18C40FBA -> START + 0001.mp3
  { {0xA3, 0xC0, 0x38, 0x15}, 4, 0 },   // A3C03815 -> STOP, no audio
  { {0x64, 0x14, 0xF6, 0xB3}, 4, 2 },   // 6414F6B3 -> 0002.mp3
  { {0x4A, 0x6B, 0xDF, 0x34}, 4, 3 },   // 4A6BDF34 -> 0003.mp3
  { {0xD8, 0x36, 0xD4, 0xB9}, 4, 4 },   // D836D4B9 -> 0004.mp3
  { {0x94, 0x60, 0x6B, 0xB2}, 4, 5 },   // 94606BB2 -> 0005.mp3
  { {0x18, 0x48, 0xF4, 0xBA}, 4, 6 },   // 1848F4BA -> 0006.mp3
  { {0x94, 0x58, 0x1A, 0xB3}, 4, 7 },   // 94581AB3 -> 0007.mp3
  { {0x09, 0x4C, 0xB1, 0x92}, 4, 8 },   // 094CB192 -> 0008.mp3
};
const int TRACK_COUNT = sizeof(TRACKS) / sizeof(TRACKS[0]);

// Debounce: skip the very same card re-tapped too quickly.
uint8_t lastUid[7];
uint8_t lastUidSize = 0;
unsigned long lastPlayMs = 0;
const unsigned long DEBOUNCE_MS = 1500;

// Forward declaration: the card/audio helpers use the Nano command sender.
void sendNano(const char *cmd);
void finishAudio(bool allowResume);

// ============================================================
// Helpers
// ============================================================
String uidToHex(const uint8_t *uid, uint8_t size) {
  String s;
  for (uint8_t i = 0; i < size; i++) {
    if (uid[i] < 0x10) s += '0';
    s += String(uid[i], HEX);
    s += ' ';
  }
  s.trim();
  s.toUpperCase();
  return s;
}

bool sameUid(const uint8_t *a, uint8_t aSize, const uint8_t *b, uint8_t bSize) {
  if (aSize != bSize) return false;
  for (uint8_t i = 0; i < aSize; i++) {
    if (a[i] != b[i]) return false;
  }
  return true;
}

// Returns the mapped track (1..8), NO_AUDIO_TRACK for a mapped no-audio action,
// or UNKNOWN_TRACK if the UID is not in the table.
int trackForUid(const uint8_t *uid, uint8_t size) {
  for (int i = 0; i < TRACK_COUNT; i++) {
    uint8_t n = TRACKS[i].uidSize;
    if (size < n) continue;                 // scanned card shorter than entry
    bool ok = true;
    for (uint8_t j = 0; j < n; j++) {
      if (uid[j] != TRACKS[i].uid[j]) { ok = false; break; }
    }
    if (ok) return TRACKS[i].track;
  }
  return UNKNOWN_TRACK;
}

void playTrack(int track) {
  Serial.print(F("-> DFPlayer: play track "));
  Serial.println(track);
  player.playMp3Folder(track);   // plays /MP3/000X.mp3
}

void startRailFromCard() {
  railStopLatched = false;
  narrating = false;
  resumeAfterAudio = false;
  if (audioReady) player.stop();
  Serial.println(F("START card -> clearing stop latch; rail starts after audio."));
}

void stopRailFromCard() {
  railStopLatched = true;
  narrating = false;
  resumeAfterAudio = false;
  if (audioReady) player.stop();
  Serial.println(F("STOP card -> rail latched stopped; no audio."));
  sendNano("STOP");
}

void beginAudio(int track, bool resumeAfterFinish) {
  // Every mapped audio card stops the rail before audio. It may resume only
  // after completion if the stop latch is still clear.
  sendNano("STOP");
  if (!audioReady) {
    narrating = false;
    resumeAfterAudio = false;
    Serial.println(F("Audio unavailable -> rail remains stopped."));
    return;
  }

  if (narrating) finishAudio(false);
  player.stop();
  playTrack(track);
  narrating = true;
  resumeAfterAudio = resumeAfterFinish && !railStopLatched;
  audioStartMs = millis();
}

void finishAudio(bool allowResume) {
  if (!narrating) return;

  narrating = false;
  bool shouldResume = allowResume && resumeAfterAudio && !railStopLatched;
  resumeAfterAudio = false;
  if (shouldResume) {
    Serial.println(F("Audio finished -> restarting rail."));
    sendNano("START");
  } else {
    Serial.println(F("Audio finished -> rail remains stopped."));
  }
}

void sendNano(const char *cmd) {
  nanoSerial.print(cmd);
  nanoSerial.print('\n');  // NANO_LFR_RAIL parses newline-terminated commands
  Serial.print(F("-> Nano: "));
  Serial.println(cmd);
}

void pollNano() {
  while (nanoSerial.available()) {
    char c = nanoSerial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      nanoBuf[nanoLen] = '\0';
      if (nanoLen > 0) {
        Serial.print(F("<- Nano: "));
        Serial.println(nanoBuf);
      }
      nanoLen = 0;
      continue;
    }
    if (nanoLen < sizeof(nanoBuf) - 1) nanoBuf[nanoLen++] = c;
  }
}

// ============================================================
// SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println(F("================================"));
  Serial.println(F("ESP32 RFID -> DFPlayer"));
  Serial.println(F("================================"));

  // --- MFRC522 ---
  mfrc522.PCD_Init();
  MFRC522Debug::PCD_DumpVersionToSerial(mfrc522, Serial);
  Serial.println(F("RFID reader ready. Tap a card..."));

  // --- DFPlayer (UART2) ---
  dfSerial.begin(9600, SERIAL_8N1, DF_RX, DF_TX);
  Serial.println(F("Connecting DFPlayer..."));
  if (!player.begin(dfSerial)) {
    Serial.println(F("DFPlayer ERROR! Check wiring / SD card."));
    Serial.println(F("Audio disabled. Serial commands still work."));
    audioReady = false;
  } else {
    Serial.println(F("DFPlayer connected."));
    audioReady = true;
    player.volume(currentVolume);
  }

  // --- Nano rail controller (UART1) ---
  nanoSerial.begin(NANO_BAUD, SERIAL_8N1, NANO_RX, NANO_TX);
  Serial.println(F("Nano rail UART ready (9600, RX=25, TX=26)."));
  sendNano("STOP");  // safe state if the ESP32 rebooted while Nano was moving

  Serial.println(F("Serial: 1..9 = play track, +/- = volume, s = stop audio."));
  Serial.println();
}

// ============================================================
// LOOP
// ============================================================
void loop() {
  // ---------------- RFID ----------------
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    uint8_t size = mfrc522.uid.size;
    String hex   = uidToHex(mfrc522.uid.uidByte, size);

    // Halt so a card held on the reader does not re-trigger continuously;
    // the card must be removed and tapped again to play once more.
    mfrc522.PICC_HaltA();

    Serial.print(F("Card UID: "));
    Serial.println(hex);

    int track = trackForUid(mfrc522.uid.uidByte, size);

    if (track == UNKNOWN_TRACK) {
      Serial.println(F("UID not in map -> no track."));
    } else {
      bool same = sameUid(lastUid, lastUidSize, mfrc522.uid.uidByte, size);
      if (same && (millis() - lastPlayMs < DEBOUNCE_MS)) {
        Serial.println(F("Same card, skipping (debounce)."));
      } else {
        memcpy(lastUid, mfrc522.uid.uidByte, size);
        lastUidSize  = size;
        lastPlayMs   = millis();

        if (track == NO_AUDIO_TRACK) {
          stopRailFromCard();
        } else if (track == 1) {
          // Track 1 is the only card that clears the explicit STOP latch.
          startRailFromCard();
          beginAudio(track, true);   // start rail after intro audio finishes
        } else {
          Serial.print(F("Mapped to track "));
          Serial.println(track);
          beginAudio(track, true);
        }
      }
    }
  }

  // ---------------- DFPlayer status ----------------
  if (player.available()) {
    uint8_t type = player.readType();
    int value = player.read();
    if (type == DFPlayerPlayFinished) {
      if (narrating && millis() - audioStartMs >= AUDIO_MIN_MS) {
        finishAudio(true);
      }
    } else if (type == DFPlayerError) {
      Serial.print(F("DFPlayer error code: "));
      Serial.println(value);
      if (narrating) {
        Serial.println(F("Audio error -> rail remains stopped."));
        finishAudio(false);
      }
    }
  }

  if (narrating && millis() - audioStartMs > AUDIO_MAX_MS) {
    Serial.println(F("Audio timeout -> applying safe stopped state."));
    finishAudio(false);
  }

  // ---------------- Nano rail status ----------------
  pollNano();

  // ---------------- Manual serial control (testing) ----------------
  if (Serial.available()) {
    char c = Serial.read();
    if (c >= '1' && c <= '9') {
      finishAudio(false);
      beginAudio(c - '0', false);
    } else if (c == '+') {
      if (currentVolume < 30) currentVolume++;
      player.volume(currentVolume);
      Serial.print(F("Volume: ")); Serial.println(currentVolume);
    } else if (c == '-') {
      if (currentVolume > 0) currentVolume--;
      player.volume(currentVolume);
      Serial.print(F("Volume: ")); Serial.println(currentVolume);
    } else if (c == 's' || c == 'S') {
      finishAudio(false);
      if (audioReady) player.stop();
      Serial.println(F("Audio stopped; rail state unchanged."));
    }
  }
}
