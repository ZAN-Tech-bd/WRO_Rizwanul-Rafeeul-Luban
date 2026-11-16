/*
  ESP32 RFID -> DFPlayer Mini + Nano rail controller
  =================================================
  Scans MIFARE cards with an MFRC522 reader (SPI), controls the Nano rail over
  UART1, plays the MP3 track mapped to that card's UID on DFPlayer UART2, and
  asks the ESP32-CAM to photograph each exhibit two seconds after stopping.

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

  ESP32-CAM trigger (software serial, TX only)
    ESP32 GPIO14 (TX) -------> ESP32-CAM GPIO13 (RX)
    Common ground. Both sides are 3.3 V, so no divider is needed.
    Disconnect this wire while flashing the ESP32-CAM.

  Service hotspot
    Join ESP32-MP3-RAIL / museum2026, then open http://192.168.4.1/
    The page tunes DFPlayer volume and shows a bounded robot event log.

  Hub emergency link (ESP-NOW, channel 1)
    The Hub owns the emergency state: its push switch (or the admin dashboard)
    raises it and re-broadcasts it, so this robot needs no wiring to the button.
    An emergency OUTRANKS the whole card map:
      * narration is cut, any pending photo is dropped, and the rail is started
        so the robot drives on,
      * every card is IGNORED except the END card (A3 C0 38 15) -- the tour
        does not stop for stations while the alarm is up,
      * the END card stops the rail and latches it, exactly as it always does.
    When the Hub clears the emergency the rail stays latched stopped; only the
    START card resumes the tour. Because the Hub re-sends the state on a timer,
    a robot that reboots or misses a packet re-syncs within ~2 s.

    ESP-NOW needs both boards on the SAME WiFi channel. This SoftAP and the
    Hub's are both pinned to channel 1 -- do not change one without the other,
    or the link goes silent with no error.

  Hub mute switch (ESP-NOW, MSG_AUDIO)
    The admin dashboard's blue mute button forces the DFPlayer to volume 0. The
    tour is otherwise UNCHANGED: cards still stop the rail, narration still
    plays and still reports finished, and the rail still restarts on time -- the
    robot simply runs the whole tour silently. That is why mute sets the volume
    instead of calling player.stop(): a stopped track never reports finished, so
    the rail would sit at the station until the 90 s fail-safe fired.
    The Hub latches the state and re-broadcasts it like the emergency, so a
    robot that reboots mid-tour comes back muted if that is still the setting.

  UID -> Track / rail action table
    Bytes are in the SAME order the reader prints them, i.e.
    "Card UID: 64 14 F6 B3"  ->  {0x64, 0x14, 0xF6, 0xB3}.
    Any mapped audio card -> STOP rail, play audio, START rail after finish
     18 C4 0F BA -> play 0009.mp3, then START rail
    A3 C0 38 15 -> STOP rail latch, no audio, no automatic restart
    The remaining cards map to tracks 10..16. All entries are full 4-byte UIDs.
  ----------------------------------------------------------------------
*/

#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522Debug.h>
#include <DFRobotDFPlayerMini.h>
#include <SoftwareSerial.h>
#include <WiFi.h>
#include <WebServer.h>
#include <esp_now.h>
#include "ControlPage.h"
#include "MuseumProtocol.h"   // shared packet layout: firmware/common
#include "EspNowCompat.h"     // one recv-callback signature for core 2.x and 3.x

// ============================================================
// DFPlayer (UART2)  -- proven setup from ESP32_MP3_LFR
// ============================================================
#define DF_RX 16   // ESP32 RX2 <- DFPlayer TX
#define DF_TX 17   // ESP32 TX2 -> DFPlayer RX
HardwareSerial dfSerial(2);
DFRobotDFPlayerMini player;

int currentVolume = 25;   // 0..30 (boot default; the field console can retune it)
bool audioReady = false;

// Hub mute (MSG_AUDIO). currentVolume above always holds the level the robot
// plays at UNMUTED -- the mute is a separate latch on top of it, so unmuting
// restores whatever level the field console was last set to.
bool audioMuted = false;

// Every volume change goes through this one call so a later volume tweak (web
// console or USB +/-) cannot quietly undo a mute the Hub is still holding.
void applyVolumeToPlayer() {
  if (audioReady) player.volume(audioMuted ? 0 : currentVolume);
}

// Send the level a second time, a moment after it was set.
//
// MEASURED, so that the next person does not re-derive it: press 'v' and the
// module reports its real volume register (0x43). It reads the correct value at
// boot AND after a mute/unmute cycle, so the ESP32 is never sending a wrong
// number -- what the module STORES is always right. Some DFPlayer modules still
// do not fully re-apply output gain from the single command that brings them
// back from 0, which a second identical command does fix.
//
// One deferred packet, non-blocking, and a no-op on a module that already
// obeyed the first one. Use 'v' to confirm the register at any time.
unsigned long volumeReassertAtMs = 0;   // 0 = nothing scheduled

void scheduleVolumeReassert(unsigned long delayMs) {
  volumeReassertAtMs = millis() + delayMs;
  if (volumeReassertAtMs == 0) volumeReassertAtMs = 1;   // 0 means "unscheduled"
}

// Station audio pauses the rail and resumes it only after DFPlayer reports
// playback finished. The explicit end card latches the rail stopped; only the
// start card clears that latch.
bool narrating = false;
bool resumeAfterAudio = false;
bool railStopLatched = true;  // boot is stopped until the START card is read
unsigned long audioStartMs = 0;
const unsigned long AUDIO_MIN_MS = 1500;   // ignore stale finished events
const unsigned long AUDIO_MAX_MS = 90000;  // fail-safe if DFPlayer never reports finish

// When the DFPlayer is absent at boot (audioReady == false) the tour still runs
// SILENTLY. beginAudio() arms a "synthetic" narration: the rail stops for a fixed
// dwell (so the station photo still fires), then finishAudio(true) resumes it --
// exactly the normal flow but on a timer, because a disconnected module never
// sends the "play finished" event the normal path waits for. Without this the
// first audio card would STOP the rail and never resume it (deadlock).
bool narratingSynthetic = false;
const unsigned long AUDIO_OFFLINE_DWELL_MS = 3000;  // >= CAM_CAPTURE_DELAY_MS so the photo still goes out

// ============================================================
// Nano rail controller (UART1)
// ============================================================
#define NANO_RX 25   // ESP32 RX1 <- Nano D1 TX through a divider
#define NANO_TX 26   // ESP32 TX1 -> Nano D0 RX
HardwareSerial nanoSerial(1);
const unsigned long NANO_BAUD = 9600;  // MUST match NANO_LFR_RAIL
char nanoBuf[64];
uint8_t nanoLen = 0;

// Last speeds positively reported by the Nano's "TUNE ML=n MR=n" response.
// The web UI never pretends a requested value is active until this reply arrives.
uint8_t motorLeft = 150;
uint8_t motorRight = 210;
bool nanoTuneConfirmed = false;
bool motorCommandPending = false;
bool railRunning = false;
uint8_t requestedMotorLeft = 150;
uint8_t requestedMotorRight = 210;

// ============================================================
// ESP32-CAM trigger (GPIO14 software TX -> camera GPIO13 RX)
// ============================================================
// UART0 is the USB console, UART1 belongs to the Nano, and UART2 belongs to the
// DFPlayer. This short, write-only 9600-baud link therefore uses software TX.
#define CAM_TX 14
const unsigned long CAM_BAUD = 9600;  // MUST match ROBOT_LINK_BAUD on the cam
const unsigned long CAM_CAPTURE_DELAY_MS = 2000;
SoftwareSerial camSerial;

bool cameraTriggerPending = false;
uint8_t pendingCameraStation = 0;
unsigned long cameraTriggerDueMs = 0;

// ============================================================
// Service SoftAP + bounded robot event log
// ============================================================
const char *AP_SSID = "ESP32-MP3-RAIL";
const char *AP_PASSWORD = "museum2026";
const uint8_t AP_CHANNEL = 1;
WebServer server(80);

// ============================================================
// Hub emergency link (ESP-NOW)
// ============================================================
// ESP-NOW rides on the SoftAP interface and inherits its channel, so
// AP_CHANNEL above MUST equal museum::kEspNowChannel and the Hub's channel.
static_assert(AP_CHANNEL == museum::kEspNowChannel,
              "SoftAP channel must match the Hub's ESP-NOW channel");

// This node only listens: the Hub is the single source of truth for the
// emergency and nothing here needs to answer it.
bool espnowReady = false;

// Written from the ESP-NOW receive callback, which runs in the WiFi task. The
// callback does nothing but store it; every reaction happens in loop().
volatile bool hubEmergencyWanted = false;
volatile bool hubMuteWanted      = false;

bool emergencyActive      = false;  // the state we have actually applied
bool emergencyStopReached = false;  // END card seen during THIS emergency

// A small chronological text tape is enough for field diagnosis and cannot
// grow until it exhausts RAM. Whole oldest lines are evicted first.
const size_t ROBOT_LOG_CAPACITY = 4096;
char robotLog[ROBOT_LOG_CAPACITY];
size_t robotLogLength = 0;
uint32_t robotLogSequence = 0;

String lastCardText = "No RFID scanned";
String lastCameraText = "No photo triggered";

// Web handlers below can send tuning commands before the helper's definition.
void sendNano(const char *cmd);

void appendRobotLog(const String &event) {
  char prefix[24];
  snprintf(prefix, sizeof(prefix), "[%10lu ms] ", (unsigned long)millis());
  String line = String(prefix) + event + '\n';

  // A pathological caller cannot monopolize the entire tape.
  if (line.length() >= ROBOT_LOG_CAPACITY) {
    line = line.substring(line.length() - (ROBOT_LOG_CAPACITY - 2));
    line += '\n';
  }

  const size_t needed = line.length();
  while (robotLogLength + needed >= ROBOT_LOG_CAPACITY && robotLogLength > 0) {
    size_t remove = 0;
    while (remove < robotLogLength && robotLog[remove] != '\n') remove++;
    if (remove < robotLogLength) remove++;
    memmove(robotLog, robotLog + remove, robotLogLength - remove);
    robotLogLength -= remove;
  }

  memcpy(robotLog + robotLogLength, line.c_str(), needed);
  robotLogLength += needed;
  robotLog[robotLogLength] = '\0';
  robotLogSequence++;
}

void addNoCacheHeaders() {
  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "0");
}

String jsonEscape(const char *text) {
  String out;
  if (!text) return out;
  out.reserve(strlen(text) + 32);
  for (const unsigned char *p = (const unsigned char *)text; *p; ++p) {
    switch (*p) {
      case '\\': out += F("\\\\"); break;
      case '"':  out += F("\\\""); break;
      case '\n': out += F("\\n"); break;
      case '\r': out += F("\\r"); break;
      case '\t': out += F("\\t"); break;
      default:
        if (*p >= 0x20) out += (char)*p;
        break;
    }
  }
  return out;
}

void handleControlPage() {
  addNoCacheHeaders();
  server.send_P(200, "text/html; charset=utf-8", controlPage);
}

void handleApiState() {
  String response;
  response.reserve(robotLogLength + 320);
  response = F("{\"volume\":");
  response += currentVolume;
  response += F(",\"muted\":");
  response += audioMuted ? F("true") : F("false");
  response += F(",\"audio\":");
  response += audioReady ? F("true") : F("false");
  response += F(",\"motorL\":");
  response += motorLeft;
  response += F(",\"motorR\":");
  response += motorRight;
  response += F(",\"nanoTuned\":");
  response += nanoTuneConfirmed ? F("true") : F("false");
  response += F(",\"motorPending\":");
  response += motorCommandPending ? F("true") : F("false");
  response += F(",\"railRunning\":");
  response += railRunning ? F("true") : F("false");
  response += F(",\"emergency\":");
  response += emergencyActive ? F("true") : F("false");
  response += F(",\"hubLink\":");
  response += espnowReady ? F("true") : F("false");
  response += F(",\"rail\":\"");
  if (emergencyActive) {
    // Which card it was is in the "card" field; this only says whether the
    // evacuation run has ended yet.
    response += emergencyStopReached ? F("EMERGENCY - stopped at a stop card")
                                     : F("EMERGENCY - running to a stop card");
  } else {
    response += railStopLatched ? F("latched stopped") : (narrating ? F("stopped for audio") : F("running / armed"));
  }
  response += F("\",\"playback\":\"");
  response += narrating ? F("station narration") : F("idle");
  response += F("\",\"card\":\"");
  response += jsonEscape(lastCardText.c_str());
  response += F("\",\"camera\":\"");
  response += jsonEscape(lastCameraText.c_str());
  response += F("\",\"logSequence\":");
  response += robotLogSequence;
  response += F(",\"log\":\"");
  response += jsonEscape(robotLog);
  response += F("\"}");

  addNoCacheHeaders();
  server.send(200, "application/json; charset=utf-8", response);
}

void handleApiVolume() {
  if (!server.hasArg("v")) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"missing_volume\"}");
    return;
  }

  const String raw = server.arg("v");
  char *end = nullptr;
  const long requested = strtol(raw.c_str(), &end, 10);
  if (raw.length() == 0 || *end != '\0' || requested < 0 || requested > 30) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"volume_must_be_0_to_30\"}");
    return;
  }

  currentVolume = (int)requested;
  // While the Hub holds the mute this only records the level to return to; the
  // DFPlayer stays at 0 until the Hub releases it.
  applyVolumeToPlayer();
  appendRobotLog("WEB volume set to " + String(currentVolume) +
                 (audioMuted ? " (held at 0 by Hub mute)" : ""));
  Serial.print(F("[web] volume set to "));
  Serial.println(currentVolume);

  addNoCacheHeaders();
  server.send(200, "application/json", String("{\"ok\":true,\"volume\":") + currentVolume +
              ",\"muted\":" + (audioMuted ? "true" : "false") + "}");
}

bool parsePwmArg(const char *name, uint8_t &value) {
  if (!server.hasArg(name)) return false;
  const String raw = server.arg(name);
  char *end = nullptr;
  const long parsed = strtol(raw.c_str(), &end, 10);
  if (raw.length() == 0 || *end != '\0' || parsed < 0 || parsed > 255) return false;
  value = (uint8_t)parsed;
  return true;
}

// Set both channels as one web operation. The Nano applies each SET live while
// driving and emits a TUNE snapshot after each one; the final GET gives one
// authoritative pair for the dashboard.
bool sendMotorPairFromRequest() {
  uint8_t left = 0;
  uint8_t right = 0;
  if (!parsePwmArg("ml", left) || !parsePwmArg("mr", right)) return false;

  requestedMotorLeft = left;
  requestedMotorRight = right;
  motorCommandPending = true;
  sendNano((String("SET ML ") + left).c_str());
  sendNano((String("SET MR ") + right).c_str());
  sendNano("GET");
  appendRobotLog("WEB motor request ML=" + String(left) + " MR=" + String(right));
  return true;
}

void handleApiMotors() {
  if (!sendMotorPairFromRequest()) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"ml_and_mr_must_be_0_to_255\"}");
    return;
  }
  server.send(202, "application/json", String("{\"ok\":true,\"ml\":") + requestedMotorLeft +
              ",\"mr\":" + requestedMotorRight + ",\"pending\":true}");
}

void handleApiMotorsSave() {
  if (!sendMotorPairFromRequest()) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"ml_and_mr_must_be_0_to_255\"}");
    return;
  }
  sendNano("SAVE");
  appendRobotLog("WEB requested Nano EEPROM save");
  server.send(202, "application/json", String("{\"ok\":true,\"ml\":") + requestedMotorLeft +
              ",\"mr\":" + requestedMotorRight + ",\"pending\":true,\"save\":true}");
}

void handleApiLogClear() {
  robotLogLength = 0;
  robotLog[0] = '\0';
  robotLogSequence++;
  appendRobotLog("WEB event tape cleared");
  server.send(204, "text/plain", "");
}

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

// A second card that ends an evacuation run, so the rail has a stop point
// before the far END card. It is a NORMAL station card on a normal tour --
// D836D4B9 plays 0012.mp3 and takes its photo exactly as before -- and only
// doubles as a stop while the Hub alarm is up.
const int EMERGENCY_STOP_TRACK = 12;   // D836D4B9

const RfidTrack TRACKS[] = {
  { {0x18, 0xC4, 0x0F, 0xBA}, 4, 9 },   // 18C40FBA -> START + 0009.mp3
  { {0xA3, 0xC0, 0x38, 0x15}, 4, 0 },   // A3C03815 -> STOP, no audio
  { {0x64, 0x14, 0xF6, 0xB3}, 4, 10 },  // 6414F6B3 -> 0010.mp3
  { {0x4A, 0x6B, 0xDF, 0x34}, 4, 11 },  // 4A6BDF34 -> 0011.mp3
  { {0xD8, 0x36, 0xD4, 0xB9}, 4, 12 },  // D836D4B9 -> 0012.mp3 (+ STOP during emergency)
  { {0x94, 0x60, 0x6B, 0xB2}, 4, 13 },  // 94606BB2 -> 0013.mp3
  { {0x18, 0x48, 0xF4, 0xBA}, 4, 14 },  // 1848F4BA -> 0014.mp3
  { {0x94, 0x58, 0x1A, 0xB3}, 4, 15 },  // 94581AB3 -> 0015.mp3
  { {0x09, 0x4C, 0xB1, 0x92}, 4, 16 },  // 094CB192 -> 0016.mp3
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

void cancelPendingPhoto(const char *reason) {
  if (!cameraTriggerPending) return;

  cameraTriggerPending = false;
  pendingCameraStation = 0;
  Serial.print(F("[cam] pending photo cancelled: "));
  Serial.println(reason);
  appendRobotLog(String("CAM pending photo cancelled: ") + reason);
}

// Tracks 2..8 map to exhibit stations 1..7. Scheduling is deliberately
// non-blocking: RFID, DFPlayer, and Nano status handling continue during the
// two-second settling period.
void scheduleStationPhoto(uint8_t station) {
  if (station < 1 || station > 7) return;

  pendingCameraStation = station;
  cameraTriggerDueMs = millis() + CAM_CAPTURE_DELAY_MS;
  cameraTriggerPending = true;

  Serial.print(F("[cam] station "));
  Serial.print(station);
  Serial.println(F(" photo scheduled in 2000 ms"));
  appendRobotLog("CAM station " + String(station) + " scheduled in 2000 ms");
}

void sendCameraShoot(uint8_t station) {
  camSerial.print(F("SHOOT "));
  camSerial.println(station);

  Serial.print(F("-> Cam: SHOOT "));
  Serial.print(station);
  Serial.println(F(" (rail stopped; audio playing)"));
  lastCameraText = "SHOOT " + String(station) + " sent";
  appendRobotLog(lastCameraText);
}

void pumpCameraTrigger() {
  if (!cameraTriggerPending) return;

  // A short/error-stopped narration must not take a late photo after motion
  // may have resumed. The requirement is to shoot while station audio plays.
  if (!narrating) {
    cancelPendingPhoto("audio is no longer playing");
    return;
  }

  // Signed subtraction is wrap-safe across the millis() rollover.
  if ((int32_t)(millis() - cameraTriggerDueMs) < 0) return;

  const uint8_t station = pendingCameraStation;
  cameraTriggerPending = false;  // clear first: exactly one command per RFID tap
  pendingCameraStation = 0;
  sendCameraShoot(station);
}

void playTrack(int track) {
  Serial.print(F("-> DFPlayer: play track "));
  Serial.println(track);
  if (!audioReady) {
    appendRobotLog("AUDIO play rejected: DFPlayer offline");
    return;
  }
  player.playMp3Folder(track);   // plays /MP3/000X.mp3
  appendRobotLog("AUDIO playing MP3/" + String(track) +
                 (audioMuted ? " (MUTED by Hub)" : " (volume " + String(currentVolume) + ")"));
}

void startRailFromCard() {
  cancelPendingPhoto("START card scanned");
  railStopLatched = false;
  narrating = false;
  resumeAfterAudio = false;
  if (audioReady) player.stop();
  Serial.println(F("START card -> clearing stop latch; rail starts after audio."));
  appendRobotLog("RFID START card: stop latch cleared");
}

// label names the card that caused the stop ("END card" / "emergency stop
// card") so the serial log, the event tape and the field console all agree on
// which one it was.
void stopRailFromCard(const char *label) {
  cancelPendingPhoto(label);
  railStopLatched = true;
  narrating = false;
  resumeAfterAudio = false;
  if (audioReady) player.stop();
  Serial.print(label);
  Serial.println(F(" -> rail latched stopped; no audio."));
  sendNano("STOP");
  appendRobotLog(String("RFID ") + label + ": rail latched stopped");
}

void beginAudio(int track, bool resumeAfterFinish) {
  // Every mapped audio card stops the rail before audio. It may resume only
  // after completion if the stop latch is still clear.
  sendNano("STOP");
  if (!audioReady) {
    // DFPlayer offline (e.g. a TX/RX wire disconnected). Do NOT strand the rail:
    // run the tour silently on a timer. The rail stays stopped for the dwell so
    // the station photo is still taken, then finishAudio(true) resumes it just as
    // a real "play finished" event would have. resumeAfterAudio is armed here so
    // finishAudio() knows to send START when the dwell elapses.
    narrating = true;
    narratingSynthetic = true;
    resumeAfterAudio = resumeAfterFinish && !railStopLatched;
    audioStartMs = millis();
    Serial.println(F("Audio offline -> silent timed dwell; rail resumes after it."));
    appendRobotLog("AUDIO offline: silent timed dwell, rail resumes automatically");
    return;
  }

  if (narrating) finishAudio(false);
  player.stop();
  playTrack(track);
  narrating = true;
  narratingSynthetic = false;
  resumeAfterAudio = resumeAfterFinish && !railStopLatched;
  audioStartMs = millis();
}

void finishAudio(bool allowResume) {
  if (!narrating) return;

  narrating = false;
  narratingSynthetic = false;   // also ends any offline synthetic dwell
  cancelPendingPhoto("station audio finished");
  bool shouldResume = allowResume && resumeAfterAudio && !railStopLatched;
  resumeAfterAudio = false;
  if (shouldResume) {
    Serial.println(F("Audio finished -> restarting rail."));
    sendNano("START");
    appendRobotLog("AUDIO finished: Nano START sent");
  } else {
    Serial.println(F("Audio finished -> rail remains stopped."));
    appendRobotLog("AUDIO finished: rail remains stopped");
  }
}

// ============================================================
// Hub emergency link (ESP-NOW)
// ============================================================
// Runs in the WiFi task: keep it to a copy + a flag. No serial, no rail, no
// DFPlayer -- loop() does all of that from applyEmergencyState().
void handleEspNowMessage(const uint8_t *data, int len) {
  if (len != (int)sizeof(museum::MuseumMessage)) return;

  museum::MuseumMessage m;
  memcpy(&m, data, sizeof(m));
  if (!museum::isValidMessage(m)) return;

  if (m.messageType == museum::MSG_EMERGENCY) {
    hubEmergencyWanted = (m.flags & museum::FLAG_EMERGENCY_ACTIVE) != 0;
  } else if (m.messageType == museum::MSG_AUDIO) {
    hubMuteWanted = (m.flags & museum::FLAG_AUDIO_MUTED) != 0;
  }
}

// EspNowRecvInfo hides the core 2.x / 3.x signature difference (EspNowCompat.h).
void onEspNowRecv(const EspNowRecvInfo *, const uint8_t *data, int len) {
  handleEspNowMessage(data, len);
}

// Call AFTER WiFi.softAP(): ESP-NOW rides on the AP interface and inherits its
// channel, which must match the Hub's (both are pinned to channel 1).
bool initEspNow() {
  if (esp_now_init() != ESP_OK) {
    Serial.println(F("ESP-NOW init FAILED (Hub emergency link disabled)"));
    appendRobotLog("HUB ESP-NOW init FAILED: no emergency link");
    return false;
  }

  esp_now_register_recv_cb(onEspNowRecv);

  // Receiving broadcasts needs no peer; the Hub's broadcast address is
  // registered anyway so this node can transmit later without re-plumbing.
  // A failure here costs nothing today, so it must not disable reception.
  esp_now_peer_info_t peer{};
  memcpy(peer.peer_addr, museum::kBroadcastAddress, 6);
  peer.channel = museum::kEspNowChannel;
  peer.ifidx   = WIFI_IF_AP;      // this node is AP-only, exactly like the Hub
  peer.encrypt = false;
  if (esp_now_add_peer(&peer) != ESP_OK)
    Serial.println(F("ESP-NOW add_peer failed (receive still works)"));

  Serial.println(F("ESP-NOW ready (broadcast, channel 1): listening for Hub emergency."));
  return true;
}

// The Hub owns the emergency state and re-broadcasts it, so this is called
// constantly with the same value: act only on a real transition.
void applyEmergencyState() {
  const bool wanted = hubEmergencyWanted;   // volatile: read once
  if (wanted == emergencyActive) return;

  emergencyActive = wanted;

  if (emergencyActive) {
    // Top priority: nothing the tour is doing may hold the robot at a station.
    // Cut audio, drop the pending photo, clear the stop latch, and drive -- the
    // card handler ignores everything but a stop card from here on.
    cancelPendingPhoto("emergency raised");
    narrating            = false;
    resumeAfterAudio     = false;
    emergencyStopReached = false;
    railStopLatched      = false;
    if (audioReady) player.stop();
    Serial.println(F("!! EMERGENCY from Hub -> every card except a stop card is ignored; running to the next stop card."));
    appendRobotLog("EMERGENCY raised by Hub: cards ignored until a stop card");
    sendNano("START");
  } else {
    // Cleared. Do not silently carry on: after an emergency the robot is
    // off-tour and a human repositions it, so it stays stopped until the START
    // card is tapped -- the same card that starts every normal tour.
    cancelPendingPhoto("emergency cleared");
    narrating            = false;
    resumeAfterAudio     = false;
    emergencyStopReached = false;
    railStopLatched      = true;
    if (audioReady) player.stop();
    Serial.println(F("Emergency cleared -> rail latched stopped. Tap the START card to resume the tour."));
    appendRobotLog("EMERGENCY cleared: rail latched stopped, START card required");
    sendNano("STOP");
  }
}

// Like the emergency, the Hub re-broadcasts the mute constantly, so this is
// called with the same value over and over: act only on a real transition.
//
// Mute is deliberately ORTHOGONAL to everything else. It changes the DFPlayer
// volume and nothing more: no rail command, no cancelled photo, no interrupted
// narration. A muted tour runs on exactly the same timing as a loud one, which
// is what makes it safe to press at any moment during a demo.
void applyMuteState() {
  const bool wanted = hubMuteWanted;   // volatile: read once
  if (wanted == audioMuted) return;

  audioMuted = wanted;
  applyVolumeToPlayer();
  // Coming back from 0 is the case where a module can under-apply the gain, so
  // the level is sent once more shortly after. See scheduleVolumeReassert().
  scheduleVolumeReassert(250);

  if (audioMuted) {
    Serial.println(F("Hub mute ON -> DFPlayer volume 0; the tour continues silently."));
    appendRobotLog("HUB mute ON: volume forced to 0");
  } else {
    Serial.print(F("Hub mute OFF -> volume restored to "));
    Serial.println(currentVolume);
    appendRobotLog("HUB mute OFF: volume restored to " + String(currentVolume));
  }
}

void sendNano(const char *cmd) {
  nanoSerial.print(cmd);
  nanoSerial.print('\n');  // NANO_LFR_RAIL parses newline-terminated commands
  Serial.print(F("-> Nano: "));
  Serial.println(cmd);
  appendRobotLog(String("NANO <- ") + cmd);
}

void handleNanoLine(const char *line) {
  appendRobotLog(String("NANO -> ") + line);

  if (strcmp(line, "STATE:RUNNING") == 0) {
    railRunning = true;
    return;
  }
  if (strcmp(line, "STATE:STOPPED") == 0 || strcmp(line, "READY:STOPPED") == 0) {
    railRunning = false;
    return;
  }

  int left = -1;
  int right = -1;
  if (sscanf(line, "TUNE ML=%d MR=%d", &left, &right) == 2 &&
      left >= 0 && left <= 255 && right >= 0 && right <= 255) {
    motorLeft = (uint8_t)left;
    motorRight = (uint8_t)right;
    nanoTuneConfirmed = true;
    if (motorCommandPending &&
        motorLeft == requestedMotorLeft &&
        motorRight == requestedMotorRight) {
      motorCommandPending = false;
    }
    return;
  }

  if (strncmp(line, "ERR:", 4) == 0) motorCommandPending = false;
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
        handleNanoLine(nanoBuf);
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
  robotLog[0] = '\0';
  appendRobotLog("BOOT ESP32 RFID/MP3 rail controller");

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
    appendRobotLog("AUDIO DFPlayer not connected");
  } else {
    Serial.println(F("DFPlayer connected."));
    audioReady = true;
    applyVolumeToPlayer();
    scheduleVolumeReassert(1500);   // belt and braces while the module settles
    appendRobotLog("AUDIO DFPlayer connected; volume " + String(currentVolume));
  }

  // --- Nano rail controller (UART1) ---
  nanoSerial.begin(NANO_BAUD, SERIAL_8N1, NANO_RX, NANO_TX);
  Serial.println(F("Nano rail UART ready (9600, RX=25, TX=26)."));
  sendNano("STOP");  // safe state if the ESP32 rebooted while Nano was moving
  sendNano("GET");   // populate ML/MR from Nano RAM/EEPROM for the web console

  // --- ESP32-CAM trigger (software serial, TX only) ---
  camSerial.begin(CAM_BAUD, SWSERIAL_8N1, -1, CAM_TX);
  Serial.println(F("Camera trigger ready (9600, TX=14 -> CAM RX=13)."));

  // --- Service hotspot + web console ---
  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);
  const bool apReady = WiFi.softAP(AP_SSID, AP_PASSWORD, AP_CHANNEL, false, 4);
  if (apReady) {
    appendRobotLog(String("WEB SoftAP ready at ") + WiFi.softAPIP().toString());
    Serial.print(F("Web console: join "));
    Serial.print(AP_SSID);
    Serial.print(F(" / "));
    Serial.print(AP_PASSWORD);
    Serial.print(F(" then open http://"));
    Serial.println(WiFi.softAPIP());
  } else {
    appendRobotLog("WEB SoftAP start FAILED");
    Serial.println(F("Web console SoftAP FAILED."));
  }

  // --- Hub emergency link (after softAP: ESP-NOW inherits the AP channel) ---
  espnowReady = initEspNow();
  if (espnowReady) appendRobotLog("HUB ESP-NOW listening on channel 1");

  server.on("/", HTTP_GET, handleControlPage);
  server.on("/api/state", HTTP_GET, handleApiState);
  server.on("/api/volume", HTTP_POST, handleApiVolume);
  server.on("/api/motors", HTTP_POST, handleApiMotors);
  server.on("/api/motors/save", HTTP_POST, handleApiMotorsSave);
  server.on("/api/log/clear", HTTP_POST, handleApiLogClear);
  server.onNotFound([]() { server.send(404, "text/plain", "Not found"); });
  server.begin();
  appendRobotLog("WEB server started");

  Serial.println(F("Serial: 1..9 = play track, +/- = volume, s = stop audio,"));
  Serial.println(F("        v = read the DFPlayer's ACTUAL volume,"));
  Serial.println(F("        e = toggle emergency, m = toggle mute"));
  Serial.println(F("        (e/m are bench tests; the Hub overrides them)."));
  Serial.println();
}

// ============================================================
// LOOP
// ============================================================
void loop() {
  server.handleClient();

  // ---------------- Hub emergency ----------------
  // First in the pass: an alarm raised since the last loop must reach the rail
  // before this pass can act on a card or resume audio.
  applyEmergencyState();

  // ---------------- Hub mute ----------------
  // Ahead of the card reader too, so a card tapped in this same pass starts its
  // narration already at the right level rather than one loop late.
  applyMuteState();

  // ---------------- RFID ----------------
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    uint8_t size = mfrc522.uid.size;
    String hex   = uidToHex(mfrc522.uid.uidByte, size);

    // Halt so a card held on the reader does not re-trigger continuously;
    // the card must be removed and tapped again to play once more.
    mfrc522.PICC_HaltA();

    Serial.print(F("Card UID: "));
    Serial.println(hex);
    lastCardText = hex;
    appendRobotLog("RFID read " + hex);

    int track = trackForUid(mfrc522.uid.uidByte, size);

    if (track == UNKNOWN_TRACK) {
      Serial.println(F("UID not in map -> no track."));
      lastCardText += " (unknown)";
      appendRobotLog("RFID unknown: no action");
    } else {
      bool same = sameUid(lastUid, lastUidSize, mfrc522.uid.uidByte, size);
      if (same && (millis() - lastPlayMs < DEBOUNCE_MS)) {
        Serial.println(F("Same card, skipping (debounce)."));
        appendRobotLog("RFID duplicate ignored by debounce");
      } else {
        memcpy(lastUid, mfrc522.uid.uidByte, size);
        lastUidSize  = size;
        lastPlayMs   = millis();

        // The Hub emergency outranks the whole card map. Only a stop card is
        // acted on, so the robot drives past every station to the first one it
        // meets and stops there. The START card is ignored too: while the alarm
        // is up there is nothing to restart into.
        //
        // Two cards can end the run: the END card always, and the
        // EMERGENCY_STOP_TRACK station card only while the alarm is up -- on a
        // normal tour it falls through to the station branch below and behaves
        // like any other station.
        const bool endCard = (track == NO_AUDIO_TRACK);
        const bool stopsHere = endCard ||
                               (emergencyActive && track == EMERGENCY_STOP_TRACK);

        if (stopsHere) {
          const char *label = endCard ? "END card" : "emergency stop card";
          lastCardText += endCard ? " (END)" : " (emergency stop)";
          stopRailFromCard(label);
          if (emergencyActive) {
            emergencyStopReached = true;
            Serial.print(F("EMERGENCY: "));
            Serial.print(label);
            Serial.println(F(" reached -> rail stopped."));
            appendRobotLog(String("EMERGENCY: ") + label + " reached, rail stopped");
          }
        } else if (emergencyActive) {
          lastCardText += " (ignored: emergency)";
          Serial.println(F("EMERGENCY: card ignored; only a stop card acts now."));
          appendRobotLog("EMERGENCY: card ignored (stop card required)");
        } else if (track == 9) {
          lastCardText += " (START)";
          // Track 9 is the only card that clears the explicit STOP latch.
          startRailFromCard();
          beginAudio(track, true);   // start rail after intro audio finishes
        } else {
          lastCardText += " (station " + String(track - 9) + ")";
          Serial.print(F("Mapped to track "));
          Serial.println(track);
          beginAudio(track, true);
          // Only station tracks 10..16 take photos. Track 9 is START and track 0
          // is END, so neither reaches this branch. If audio failed to start,
          // narrating is false and no supposedly "while playing" shot is queued.
          if (narrating) scheduleStationPhoto((uint8_t)(track - 9));
        }
      }
    }
  }

  // ---------------- deferred volume re-assert ----------------
  // See scheduleVolumeReassert(). Signed compare is wrap-safe across rollover.
  if (audioReady && volumeReassertAtMs &&
      (int32_t)(millis() - volumeReassertAtMs) >= 0) {
    volumeReassertAtMs = 0;
    applyVolumeToPlayer();
    Serial.print(F("[audio] volume re-asserted: "));
    Serial.println(audioMuted ? 0 : currentVolume);
  }

  // ---------------- DFPlayer status ----------------
  if (audioReady && player.available()) {
    uint8_t type = player.readType();
    int value = player.read();
    if (type == DFPlayerPlayFinished) {
      if (narrating && millis() - audioStartMs >= AUDIO_MIN_MS) {
        finishAudio(true);
      }
    } else if (type == DFPlayerError) {
      Serial.print(F("DFPlayer error code: "));
      Serial.println(value);
      appendRobotLog("AUDIO error " + String(value));
      if (narrating) {
        Serial.println(F("Audio error -> rail remains stopped."));
        finishAudio(false);
      }
    }
  }

  // ---------------- offline-audio timed dwell ----------------
  // A disconnected DFPlayer never sends a "play finished" event, so the normal
  // block above never fires. beginAudio() instead armed a synthetic narration;
  // once its fixed dwell elapses we complete it here, which (because
  // resumeAfterAudio was armed) restarts the rail and keeps the silent tour
  // moving. The dwell is >= the photo delay, so the station shot still goes out.
  if (narrating && narratingSynthetic &&
      millis() - audioStartMs >= AUDIO_OFFLINE_DWELL_MS) {
    narratingSynthetic = false;
    Serial.println(F("Silent dwell elapsed -> resuming rail (DFPlayer offline)."));
    appendRobotLog("AUDIO offline dwell elapsed: rail resumes");
    finishAudio(true);
  }

  if (narrating && millis() - audioStartMs > AUDIO_MAX_MS) {
    Serial.println(F("Audio timeout -> applying safe stopped state."));
    appendRobotLog("AUDIO timeout: safe stopped state");
    finishAudio(false);
  }

  // ---------------- Nano rail status ----------------
  pollNano();

  // ---------------- Manual serial control (testing) ----------------
  if (Serial.available()) {
    char c = Serial.read();
    if (c >= '1' && c <= '9') {
      // beginAudio() stops the rail, which would strand the robot mid-track
      // while the alarm is up.
      if (emergencyActive) {
        Serial.println(F("EMERGENCY active -> manual playback refused."));
        appendRobotLog("EMERGENCY: USB play refused");
      } else {
        finishAudio(false);
        beginAudio(c - '0', false);
      }
    } else if (c == 'e' || c == 'E') {
      // Bench test without the Hub. The Hub re-broadcasts its own state every
      // ~2 s, so a real Hub overrides this on its next packet.
      hubEmergencyWanted = !hubEmergencyWanted;
      Serial.print(F("Local emergency test -> "));
      Serial.println(hubEmergencyWanted ? F("ON") : F("OFF"));
    } else if (c == 'v' || c == 'V') {
      // Ask the MODULE what volume it is really at, instead of trusting the
      // number we last sent. This is what proves whether a volume command
      // actually landed (see scheduleVolumeReassert()).
      //
      // The query consumes one DFPlayer response, so it is REFUSED while a
      // narration is running: swallowing the "play finished" event would leave
      // the rail parked at the station until the 90 s fail-safe.
      if (!audioReady) {
        Serial.println(F("DFPlayer offline; no volume to read."));
      } else if (narrating) {
        Serial.println(F("Refused: narration playing (the query would eat the finish event)."));
      } else {
        const int actual = player.readVolume();
        const int expected = audioMuted ? 0 : currentVolume;
        Serial.print(F("DFPlayer reports volume "));
        if (actual < 0) Serial.print(F("(no answer)")); else Serial.print(actual);
        Serial.print(F("; we believe "));
        Serial.print(expected);
        Serial.println(actual == expected ? F("  [match]") : F("  [MISMATCH]"));
        appendRobotLog("USB volume readback: module=" + String(actual) +
                       " expected=" + String(expected));
      }
    } else if (c == 'm' || c == 'M') {
      // Bench test without the Hub, same as 'e'. The Hub re-broadcasts its own
      // mute state every ~2 s, so a real Hub overrides this on its next packet.
      hubMuteWanted = !hubMuteWanted;
      Serial.print(F("Local mute test -> "));
      Serial.println(hubMuteWanted ? F("MUTED") : F("UNMUTED"));
    } else if (c == '+') {
      if (currentVolume < 30) currentVolume++;
      applyVolumeToPlayer();
      Serial.print(F("Volume: ")); Serial.println(currentVolume);
      appendRobotLog("USB volume set to " + String(currentVolume));
    } else if (c == '-') {
      if (currentVolume > 0) currentVolume--;
      applyVolumeToPlayer();
      Serial.print(F("Volume: ")); Serial.println(currentVolume);
      appendRobotLog("USB volume set to " + String(currentVolume));
    } else if (c == 's' || c == 'S') {
      finishAudio(false);
      if (audioReady) player.stop();
      Serial.println(F("Audio stopped; rail state unchanged."));
      appendRobotLog("USB audio stop; rail state unchanged");
    }
  }

  // Non-blocking two-second station-photo delay. Keep this after audio/event
  // handling so an audio finish/error in this pass cancels the shot first.
  pumpCameraTrigger();
}
