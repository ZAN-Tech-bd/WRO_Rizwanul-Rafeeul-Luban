/*
  ESP32 Museum Guide Controller
  =============================
  Merge of ESP32_MP3_LFR (DFPlayer + WiFi web UI + Nano line-follower link)
  and ESP32_RFID_MP3 (MFRC522 card reader -> DFPlayer track).

  Tour behaviour
  --------------
    Tap a mapped station card -> the ESP32 tells the Nano to STOP, plays that
    card's track, and tells the Nano to START again when the track finishes.
    The START card is intentionally silent for now and starts the Nano at once.
    The auto-resume is only armed when it is safe to move:
      * the robot was already line-following when the card was tapped, or
      * the card is the START card (tour intro -> begin the tour).
    So tapping a station card on the bench plays audio but does NOT drive off.
    Pressing Stop or LFR STOP cancels a pending auto-resume.

  Hub link (ESP-NOW, channel 1)
  -----------------------------
    Outbound: every card scan / start / stop sends a MSG_LOCATION packet, plus
    a 1 s heartbeat, so the Hub dashboard shows which station the robot is at
    and whether it is moving. Station id = track - 1 (tracks 2..8 -> 1..7).

    Inbound: the Hub broadcasts the authoritative emergency state every 2 s.
    When it turns ON the robot enters EVACUATION mode:
      * narration is cut and the line follower starts immediately,
      * every station card is IGNORED (no stop, no audio, no photo) -- the
        robot just reports passing it and keeps driving,
      * only the EXIT card (station 7, the last one) stops it.
    When the Hub clears the emergency the robot stops and waits for a normal
    LFR START / START card. Because the Hub re-sends the state on a timer, a
    robot that reboots or misses a packet re-syncs within ~2 s.

    ESP-NOW needs both boards on the SAME WiFi channel. This sketch's SoftAP
    and the Hub's SoftAP are both pinned to channel 1 -- do not change one
    without the other, or the link goes silent with no error.

  ----------------------------------------------------------------------
  Wiring (ESP32-WROOM-32 DevKit) -- the three peripherals share no pins
  ----------------------------------------------------------------------
    MFRC522 (SPI, default VSPI pins):
      SDA / SS  -> GPIO5
      SCK       -> GPIO18
      MOSI      -> GPIO23
      MISO      -> GPIO19
      RST       -> (not used in this MFRC522v2 setup)
      3V3, GND
    DFPlayer Mini (UART2):
      DFPlayer TX -> ESP32 GPIO16 (RX2)
      DFPlayer RX -> ESP32 GPIO17 (TX2)   (1k series resistor recommended)
      VCC -> 5V, GND, speaker to SPK_1/SPK_2
    Arduino Nano line follower (UART1):
      Nano D1 (TX) --10k--+-- ESP32 GPIO25 (RX)   <- Nano talks here
                           +-- 20k -- GND           (5V->3.3V divider)
      Nano D0 (RX) <------ ESP32 GPIO26 (TX)       -> Nano listens here
      Common ground. Disconnect both wires while flashing the Nano.
    ESP32-CAM photo trigger (software serial, TX only):
      ESP32 GPIO27 (TX) -------> ESP32-CAM GPIO13 (RX)
      Common ground. Both sides are 3.3V, so no divider is needed.
      All three hardware UARTs are taken, so this link is software serial;
      it is write-only (the cam's replies go to its own USB console).
      Disconnect this wire while flashing the cam.

  ----------------------------------------------------------------------
  UID -> Track table
    Bytes are in the SAME order the reader prints them, i.e.
    "Card UID: 64 14 F6 B3"  ->  {0x64, 0x14, 0xF6, 0xB3}.
    The START card (18 C4 0F BA) is silent for now; the rest map to tracks 2..8.
    All entries are full 4-byte UIDs.
  ----------------------------------------------------------------------
*/

#include <WiFi.h>
#include <WebServer.h>
#include <DFRobotDFPlayerMini.h>
#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522Debug.h>
#include <SoftwareSerial.h>
#include "TunerPage.h"
#include "RailPage.h"
#include <esp_now.h>
#include "EspNowCompat.h"     // one recv-callback signature for core 2.x and 3.x
#include "MuseumProtocol.h"   // shared packet: firmware/common (symlinked here)

// ============================================================
// Web UI mode  -- which page answers "/"
// ============================================================
// WEB_TOUR   the MP3 / RFID control page. The demo and competition build:
//            tap a card, audio plays, the robot resumes line following when
//            the track ends.
// WEB_TUNER  the live LFR calibration page. Use this while tuning.
//
// This ONLY chooses the landing page. Both pages are always compiled and both
// are always reachable at /mp3 and /tuner, and the RFID -> audio -> resume
// tour logic runs identically either way. Flip this one line and reflash.
#define WEB_TOUR   0
#define WEB_TUNER  1
#define WEB_MODE   WEB_TUNER

#if WEB_MODE == WEB_TUNER
const char* firmwareVersion = "RFID-MP3-LFR-v3-tuner";
#else
const char* firmwareVersion = "RFID-MP3-LFR-v3-silent-start";
#endif

// ============================================================
// DFPlayer (UART2)
// ============================================================
#define DF_RX 16   // ESP32 RX2 <- DFPlayer TX
#define DF_TX 17   // ESP32 TX2 -> DFPlayer RX
HardwareSerial dfSerial(2);
DFRobotDFPlayerMini player;

bool audioReady    = false;   // false -> DFPlayer missing, everything else still works
int  currentVolume = 20;      // 0..30
const int MAX_TRACK = 14;

// ============================================================
// Nano line-follower link (UART1)
// ============================================================
#define NANO_RX 25   // <- Nano D1 (TX), through the divider
#define NANO_TX 26   // -> Nano D0 (RX)
HardwareSerial nanoSerial(1);
const unsigned long NANO_BAUD = 9600;   // MUST match the Nano sketch

bool    lfrRunning = false;   // set optimistically, corrected by the Nano's STATE: replies
char    nanoBuf[96];       // holds a complete S1..S5/error/motion telemetry line
uint8_t nanoLen = 0;

// Latest lines from the Nano, kept for the tuner page to poll.
char    lastTelemetry[96] = "waiting for Nano...";
char    lastTuneLine[160] = "";        // "MIN=70 BASE=110 CO=95 ..."
char    lastNanoMsg[64]   = "";        // RAMP PWM=.., OK MIN=.., SAVED, ERR:..

// ============================================================
// ESP32-CAM photo trigger (software serial, TX only)
//   UART0/1/2 are all taken (USB, Nano, DFPlayer), so this one link is
//   bit-banged. It only ever writes ~10 bytes per station, so 9600 TX-only
//   is comfortably reliable even with the WiFi AP running.
// ============================================================
#define CAM_TX 27    // -> ESP32-CAM GPIO13 (RX)
const unsigned long CAM_BAUD = 9600;   // MUST match ROBOT_LINK_BAUD on the cam
SoftwareSerial camSerial;

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
  uint8_t uid[10];   // MIFARE UIDs are 4, 7 or 10 bytes
  uint8_t uidSize;   // number of meaningful bytes in uid[]
  int     track;
  const char* name;  // human label shown on Serial and the web page
};

const RfidTrack TRACKS[] = {
  { {0x18, 0xC4, 0x0F, 0xBA}, 4, 1, "START"   },  // 18C40FBA -> silent tour start
  { {0x64, 0x14, 0xF6, 0xB3}, 4, 2, "Track 2" },  // 6414F6B3 -> 0002.mp3
  { {0x4A, 0x6B, 0xDF, 0x34}, 4, 3, "Track 3" },  // 4A6BDF34 -> 0003.mp3
  { {0xD8, 0x36, 0xD4, 0xB9}, 4, 4, "Track 4" },  // D836D4B9 -> 0004.mp3
  { {0x94, 0x60, 0x6B, 0xB2}, 4, 5, "Track 5" },  // 94606BB2 -> 0005.mp3
  { {0x18, 0x48, 0xF4, 0xBA}, 4, 6, "Track 6" },  // 1848F4BA -> 0006.mp3
  { {0x94, 0x58, 0x1A, 0xB3}, 4, 7, "Track 7" },  // 94581AB3 -> 0007.mp3
  { {0x09, 0x4C, 0xB1, 0x92}, 4, 8, "Track 8" },  // 094CB192 -> 0008.mp3
};
const int TRACK_COUNT = sizeof(TRACKS) / sizeof(TRACKS[0]);

// The card that launches the tour: its track always arms the auto-start.
const int TOUR_START_TRACK = 1;

// Debounce: skip the very same card re-tapped too quickly.
uint8_t lastUid[10];
uint8_t lastUidSize = 0;
unsigned long lastPlayMs = 0;
const unsigned long DEBOUNCE_MS = 1500;

// ============================================================
// Narration state (station stop -> play -> resume)
// ============================================================
bool narrating           = false;   // a card-triggered track is playing
bool resumeAfterNarration = false;  // send START to the Nano when it finishes
unsigned long narrationStartMs = 0;

// DFPlayer clones sometimes emit a stale "play finished" right after a new
// play command; ignore anything that arrives sooner than this.
const unsigned long NARRATION_MIN_MS = 1500;
// Safety net: if the DFPlayer never reports the end, resume anyway.
const unsigned long NARRATION_MAX_MS = 90000;

// ============================================================
// Hub link (ESP-NOW)
// ============================================================
const uint8_t ROBOT_NODE_ID = 1;    // identifies this node in every packet

// The last card of the tour. During an emergency this is the only card that
// stops the robot -- it is the exit. Station id = track - 1, so station 7 is
// the "Track 8" card (09 4C B1 92).
const uint8_t EXIT_STATION = 7;

// The Hub drops the robot back to "not moving" after ROBOT_MOVE_SETTLE_MS
// (2.5 s) without a report, so re-send well inside that window.
const unsigned long LOCATION_HEARTBEAT_MS = 1000;

bool     espnowReady    = false;
uint8_t  currentStation = 0;        // 0 = no station reached yet
uint16_t espnowSeq      = 0;
unsigned long lastLocationMs = 0;

// Emergency / evacuation. espnowEmergencyWanted is written from the ESP-NOW
// receive callback (another task), so it is only ever read/copied in loop() --
// all the real work happens there, never in the callback.
volatile bool espnowEmergencyWanted = false;
bool emergencyActive = false;       // the state we have actually applied
bool atExit          = false;       // evacuation finished (exit card reached)

// ============================================================
// Web server
// ============================================================
const char* ssid     = "ESP32-MP3";
const char* password = "12345678";

WebServer server(80);

String lastWebStatus = "Ready";
String lastCardText  = "No card scanned yet";
String lastCamText   = "No photo triggered yet";
int    lastStation   = 1;   // station id of the most recent photo trigger


// =====================================================
// WEB PAGE
// =====================================================
const char webpage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>

<meta name="viewport" content="width=device-width, initial-scale=1">
<meta http-equiv="Cache-Control" content="no-store, no-cache, must-revalidate">

<title>ESP32 Museum Guide</title>

<style>

body {
    font-family: Arial;
    text-align: center;
    background: #101820;
    color: white;
    margin: 0;
    padding: 20px;
}

.container {
    max-width: 500px;
    margin: auto;
}

h1 {
    color: #00d4ff;
}

.card {
    background: #1b2733;
    padding: 20px;
    border-radius: 15px;
    margin-top: 20px;
}

.inline {
    display: inline-block;
    margin: 0;
}

button {
    width: 130px;
    padding: 15px;
    margin: 8px;
    font-size: 17px;
    border: none;
    border-radius: 10px;
    cursor: pointer;
}

button:active {
    transform: scale(0.96);
}

.play {
    background: #00c853;
    color: white;
}

.control {
    background: #2979ff;
    color: white;
}

.stop {
    background: #ff1744;
    color: white;
}

.volume {
    background: #ff9800;
    color: white;
}

.lfr {
    background: #7c4dff;
    color: white;
}

.version {
    color: #8fa6b8;
    font-size: 13px;
}

.state {
    color: #00d4ff;
    font-size: 15px;
    margin: 4px;
}

</style>

</head>

<body>

<div class="container">

<h1>ESP32 Museum Guide</h1>
<p class="version">Firmware: {{FW}}</p>

<div class="card">

<h2>Status</h2>

<h3 id="status">{{STATUS}}</h3>
<p class="state">Line follower: <b>{{LFR}}</b></p>
<p class="state">Station: <b>{{STN}}</b> &nbsp;|&nbsp; Hub link: {{LINK}}</p>
<p class="state">Emergency: <b>{{EMG}}</b></p>
<p class="state">Last card: {{CARD}}</p>
<p class="state">Last photo: {{CAM}}</p>
<p class="state">Volume: {{VOL}} &nbsp;|&nbsp; Audio: {{AUDIO}}</p>

<form class="inline" method="get" action="/">
<button class="control" type="submit">Refresh</button>
</form>

</div>


<div class="card">

<h2>Tracks</h2>

<form class="inline" method="get" action="/play">
<button class="play" type="submit" name="track" value="1">Play 0001</button>
</form>

<form class="inline" method="get" action="/play">
<button class="play" type="submit" name="track" value="2">Play 0002</button>
</form>

<form class="inline" method="get" action="/play">
<button class="play" type="submit" name="track" value="3">Play 0003</button>
</form>

<form class="inline" method="get" action="/play">
<button class="play" type="submit" name="track" value="4">Play 0004</button>
</form>

<form class="inline" method="get" action="/play">
<button class="play" type="submit" name="track" value="5">Play 0005</button>
</form>

<form class="inline" method="get" action="/play">
<button class="play" type="submit" name="track" value="6">Play 0006</button>
</form>

<form class="inline" method="get" action="/play">
<button class="play" type="submit" name="track" value="7">Play 0007</button>
</form>

<form class="inline" method="get" action="/play">
<button class="play" type="submit" name="track" value="8">Play 0008</button>
</form>

</div>


<div class="card">

<h2>Playback Control</h2>

<form class="inline" method="get" action="/pause">
<button class="control" type="submit">Pause</button>
</form>

<form class="inline" method="get" action="/resume">
<button class="control" type="submit">Resume</button>
</form>

<form class="inline" method="get" action="/stop">
<button class="stop" type="submit">Stop</button>
</form>

</div>


<div class="card">

<h2>Volume</h2>

<form class="inline" method="get" action="/volumedown">
<button class="volume" type="submit">Volume -</button>
</form>

<form class="inline" method="get" action="/volumeup">
<button class="volume" type="submit">Volume +</button>
</form>

</div>


<div class="card">

<h2>Line Follower (Robot)</h2>

<form class="inline" method="get" action="/lfr">
<button class="lfr" type="submit" name="state" value="on">LFR START</button>
</form>

<form class="inline" method="get" action="/lfr">
<button class="stop" type="submit" name="state" value="off">LFR STOP</button>
</form>

<p class="version">Stop also cancels a pending auto-resume.</p>

</div>


<div class="card">

<p class="version">These controls use normal browser forms and work without JavaScript.</p>

</div>

</div>


</body>
</html>
)rawliteral";


// =====================================================
// Hub link (ESP-NOW)
// =====================================================
// Tell the Hub where we are and whether we are driving. stationId 0 means
// "no station change" -- the Hub then updates only the moving flag, which is
// exactly what we want before the first card is ever read.
void sendLocation(uint8_t stationId, bool moving) {
    if (!espnowReady) return;

    museum::MuseumMessage m = museum::makeMessage(museum::MSG_LOCATION, ROBOT_NODE_ID);
    m.stationId = stationId;
    m.flags     = museum::FLAG_DEVICE_ONLINE;
    if (moving)          m.flags |= museum::FLAG_ROBOT_MOVING;
    if (emergencyActive) m.flags |= museum::FLAG_EMERGENCY_ACTIVE;
    m.sequence  = ++espnowSeq;
    m.uptimeMs  = millis();
    museum::finalizeMessage(m);      // fields changed since makeMessage: re-checksum

    esp_now_send(museum::kBroadcastAddress, (const uint8_t*)&m, sizeof(m));
    lastLocationMs = millis();
}


// Report wherever we are right now. Used by the heartbeat and by every
// start/stop, so the dashboard never lags behind the robot.
void reportLocation() {
    sendLocation(currentStation, lfrRunning);
}


// Runs in the WiFi task: keep it to a copy + a flag, no serial, no motors.
void handleEspNowMessage(const uint8_t* data, int len) {
    if (len != (int)sizeof(museum::MuseumMessage)) return;

    museum::MuseumMessage m;
    memcpy(&m, data, sizeof(m));
    if (!museum::isValidMessage(m)) return;

    if (m.messageType == museum::MSG_EMERGENCY) {
        espnowEmergencyWanted = (m.flags & museum::FLAG_EMERGENCY_ACTIVE) != 0;
    }
}

// EspNowRecvInfo hides the core 2.x / 3.x signature difference (EspNowCompat.h).
void onEspNowRecv(const EspNowRecvInfo*, const uint8_t* data, int len) {
    handleEspNowMessage(data, len);
}


// Call AFTER WiFi.softAP(): ESP-NOW rides on the AP interface and inherits its
// channel, which must match the Hub's (both are pinned to channel 1).
bool initEspNow() {
    if (esp_now_init() != ESP_OK) {
        Serial.println(F("ESP-NOW init FAILED (Hub link disabled)"));
        return false;
    }

    esp_now_register_recv_cb(onEspNowRecv);

    esp_now_peer_info_t peer{};
    memcpy(peer.peer_addr, museum::kBroadcastAddress, 6);
    peer.channel = museum::kEspNowChannel;
    peer.ifidx   = WIFI_IF_AP;      // this node is AP-only, exactly like the Hub
    peer.encrypt = false;

    if (esp_now_add_peer(&peer) != ESP_OK) {
        Serial.println(F("ESP-NOW add_peer FAILED (Hub link disabled)"));
        return false;
    }

    Serial.println(F("ESP-NOW ready (broadcast, channel 1)"));
    return true;
}


// =====================================================
// Nano link helpers
// =====================================================
void sendNano(const char* cmd) {
    nanoSerial.print(cmd);
    nanoSerial.print('\n');          // Nano parses newline-terminated commands
    Serial.print(F("-> Nano: "));
    Serial.println(cmd);
}


// Deterministic commands: the Nano also accepts a toggle, but never use it --
// the ESP32 must always know which state it asked for.
void lfrStart() {
    sendNano("START");
    lfrRunning = true;               // corrected by the Nano's STATE: reply
    reportLocation();                // dashboard should not wait for the heartbeat
}


void lfrStop() {
    sendNano("STOP");
    lfrRunning = false;
    reportLocation();
}


void handleNanoLine(const char* line) {
    if (line[0] == '\0') return;

    // Telemetry lands 8x a second. Capture it for the tuner page but keep it
    // out of the serial console, which it would otherwise flood.
    if (strncmp(line, "S1=", 3) == 0) {
        strlcpy(lastTelemetry, line, sizeof(lastTelemetry));
        return;
    }
    if (strncmp(line, "TUNE ", 5) == 0) {
        strlcpy(lastTuneLine, line + 5, sizeof(lastTuneLine));
        return;
    }

    // Everything else (RAMP PWM=.., OK .., SAVED, ERR:..) is worth showing in
    // both places.
    strlcpy(lastNanoMsg, line, sizeof(lastNanoMsg));

    Serial.print(F("<- Nano: "));
    Serial.println(line);

    if (strcmp(line, "STATE:RUNNING") == 0) {
        lfrRunning = true;
    } else if (strcmp(line, "STATE:STOPPED") == 0 ||
               strcmp(line, "READY:STOPPED") == 0) {
        lfrRunning = false;
    }
}


void pollNano() {
    while (nanoSerial.available()) {
        char c = nanoSerial.read();

        if (c == '\r') continue;

        if (c == '\n') {
            nanoBuf[nanoLen] = '\0';
            handleNanoLine(nanoBuf);
            nanoLen = 0;
            continue;
        }

        if (nanoLen < sizeof(nanoBuf) - 1) {
            nanoBuf[nanoLen++] = c;
        }
    }
}


// =====================================================
// ESP32-CAM trigger
// =====================================================
// "SHOOT <station>" makes the cam join MuseumHub, capture, and POST the JPEG
// to the Hub dashboard. Fire and forget: the cam takes 1-3 s to do all that,
// but none of it happens here -- this call only writes ~10 bytes.
void shootStation(int station) {
    lastStation = station;

    camSerial.print("SHOOT ");
    camSerial.println(station);

    lastCamText = "station " + String(station) + " (sent)";

    Serial.print(F("-> Cam: SHOOT "));
    Serial.println(station);
}


// =====================================================
// RFID helpers
// =====================================================
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


// Returns the index in TRACKS[] for a scanned UID, or -1 if unknown.
// The length must match exactly, so a 7-byte card cannot collide with a
// 4-byte entry that happens to share its first four bytes.
int findTrackIndex(const uint8_t *uid, uint8_t size) {
    for (int i = 0; i < TRACK_COUNT; i++) {
        if (size != TRACKS[i].uidSize) continue;
        bool ok = true;
        for (uint8_t j = 0; j < size; j++) {
            if (uid[j] != TRACKS[i].uid[j]) { ok = false; break; }
        }
        if (ok) return i;
    }
    return -1;
}


// =====================================================
// Playback
// =====================================================
void playTrack(int track) {
    Serial.print(F("-> DFPlayer: play track "));
    Serial.println(track);

    if (!audioReady) {
        Serial.println(F("(audio disabled - DFPlayer not connected)"));
        return;
    }

    player.playMp3Folder(track);   // plays /MP3/000X.mp3
}


// Leave the narration state. resume=true honours a pending auto-resume;
// resume=false cancels it (manual Stop / LFR STOP took over).
void endNarration(bool resume) {
    narrating = false;

    if (resume && resumeAfterNarration) {
        Serial.println(F("Narration done -> resuming line follower"));
        lfrStart();
    }

    resumeAfterNarration = false;
}


// A mapped card was presented: stop at the station, narrate, then resume.
void onCardScanned(int idx) {
    const RfidTrack& entry = TRACKS[idx];

    // Tracks 2..8 are the seven exhibits; the tour-start card is not a station.
    const uint8_t station = (entry.track >= 2) ? (uint8_t)(entry.track - 1) : 0;

    // Decide before stopping: only resume if the robot was already moving,
    // or if this is the card that starts the tour.
    bool armResume = lfrRunning || (entry.track == TOUR_START_TRACK);

    lastCardText = uidToHex(entry.uid, entry.uidSize) +
                   " -> " + String(entry.name) +
                   " (track " + String(entry.track) + ")";

    Serial.print(F("Mapped: "));
    Serial.print(entry.name);
    Serial.print(F(" -> track "));
    Serial.println(entry.track);

    // Remember where we are before any report goes out, so the Hub is told the
    // new station rather than the previous one.
    if (station) currentStation = station;

    // ---------------- EVACUATION ----------------
    // Drive straight through every station until the exit card. No stop, no
    // narration, no photo -- the robot only reports that it passed.
    if (emergencyActive) {
        if (station == EXIT_STATION) {
            lfrStop();                  // reports station 7, not moving
            atExit        = true;
            lastWebStatus = "EMERGENCY - arrived at exit";
            Serial.println(F("EMERGENCY: exit card reached -> stopped."));
        } else {
            lastWebStatus = "EMERGENCY - passing " + String(entry.name);
            Serial.print(F("EMERGENCY: skipping "));
            Serial.println(entry.name);
            sendLocation(station, true);   // still driving to the exit
        }
        return;
    }
    // --------------------------------------------

    // The START card is a motion command, not an exhibit. For now it must not
    // play track 0001 or enter narration state: cancel any old narration/audio
    // and launch the line follower immediately.
    if (entry.track == TOUR_START_TRACK) {
        if (audioReady) player.stop();
        narrating            = false;
        resumeAfterNarration = false;
        lastWebStatus        = "START card scanned - LFR started (audio skipped)";
        Serial.println(F("START card -> audio skipped; starting line follower now."));
        lfrStart();
        return;
    }

    if (lfrRunning) {
        lfrStop();                      // stop at the station before narrating
    } else {
        reportLocation();               // bench tap: still tell the Hub where we are
    }

    // Photograph every station except the tour-start card, which is not an
    // exhibit. Sent while the robot is halted so the frame is not smeared.
    if (entry.track != TOUR_START_TRACK) {
        shootStation(entry.track - 1);  // tracks 2..8 -> stations 1..7
    }

    playTrack(entry.track);

    if (audioReady) {
        narrating            = true;
        resumeAfterNarration = armResume;
        narrationStartMs     = millis();
        lastWebStatus        = String(entry.name) + " narrating";
    } else {
        // Nothing to wait for -- go straight on if the tour should continue.
        narrating            = false;
        resumeAfterNarration = false;
        lastWebStatus        = String(entry.name) + " scanned (no audio)";
        if (armResume) lfrStart();
    }
}


void pollRfid() {
    if (!mfrc522.PICC_IsNewCardPresent()) return;
    if (!mfrc522.PICC_ReadCardSerial())   return;

    uint8_t size = mfrc522.uid.size;
    if (size > sizeof(lastUid)) size = sizeof(lastUid);

    String hex = uidToHex(mfrc522.uid.uidByte, size);

    // Halt so a card held on the reader does not re-trigger continuously;
    // the card must be removed and tapped again to play once more.
    mfrc522.PICC_HaltA();

    Serial.print(F("Card UID: "));
    Serial.println(hex);

    int idx = findTrackIndex(mfrc522.uid.uidByte, size);

    if (idx < 0) {
        Serial.println(F("UID not in map -> no track."));
        lastCardText  = hex + " -> unknown card";
        lastWebStatus = "Unknown card " + hex;
        return;
    }

    if (sameUid(lastUid, lastUidSize, mfrc522.uid.uidByte, size) &&
        (millis() - lastPlayMs < DEBOUNCE_MS)) {
        Serial.println(F("Same card, skipping (debounce)."));
        return;
    }

    memcpy(lastUid, mfrc522.uid.uidByte, size);
    lastUidSize = size;
    lastPlayMs  = millis();

    onCardScanned(idx);
}


void pollPlayer() {
    if (!audioReady) return;

    if (player.available()) {
        uint8_t type  = player.readType();
        int     value = player.read();

        if (type == DFPlayerPlayFinished) {
            if (narrating && millis() - narrationStartMs >= NARRATION_MIN_MS) {
                endNarration(true);
            }
        } else if (type == DFPlayerError) {
            Serial.print(F("DFPlayer error code: "));
            Serial.println(value);
        }
    }

    if (narrating && millis() - narrationStartMs > NARRATION_MAX_MS) {
        Serial.println(F("Narration timeout (no end-of-track report)."));
        endNarration(true);
    }
}


// =====================================================
// Emergency / evacuation
// =====================================================
// The Hub owns the emergency state and re-broadcasts it every 2 s, so this is
// called constantly with the same value: act only on a real transition.
void applyEmergencyState() {
    const bool wanted = espnowEmergencyWanted;   // volatile: read once
    if (wanted == emergencyActive) return;

    emergencyActive = wanted;

    if (emergencyActive) {
        // Cut whatever is playing and go, regardless of what we were doing.
        if (audioReady) player.stop();
        narrating            = false;
        resumeAfterNarration = false;
        atExit               = false;

        lastWebStatus = "EMERGENCY - driving to exit";
        Serial.println(F("!! EMERGENCY -> evacuation: skipping stations, driving to exit"));

        lfrStart();          // also reports the new state to the Hub
    } else {
        // Cleared. Stop rather than silently carrying on -- after an
        // evacuation the robot is off-tour and should be repositioned, so a
        // human restarts it with LFR START or the tour-start card.
        atExit = false;

        lastWebStatus = "Emergency cleared - robot stopped";
        Serial.println(F("Emergency cleared -> stopped. Use LFR START or the START card."));

        lfrStop();
    }
}


// =====================================================
// Web Server
// =====================================================
void addNoCacheHeaders() {
    server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    server.sendHeader("Pragma", "no-cache");
    server.sendHeader("Expires", "0");
}


void sendControlPage() {
    String lfrText = lfrRunning ? "RUNNING" : "STOPPED";
    if (narrating) {
        lfrText += resumeAfterNarration ? " (narrating, will resume)"
                                        : " (narrating)";
    }

    String emgText = "normal";
    if (emergencyActive) {
        emgText = atExit ? "ACTIVE - at exit (stopped)"
                         : "ACTIVE - skipping to exit";
    }

    String page = FPSTR(webpage);
    page.replace("{{FW}}",     firmwareVersion);
    page.replace("{{STATUS}}", lastWebStatus);
    page.replace("{{LFR}}",    lfrText);
    page.replace("{{STN}}",    currentStation ? String(currentStation) : String("not started"));
    page.replace("{{LINK}}",   espnowReady ? "ESP-NOW ready" : "off");
    page.replace("{{EMG}}",    emgText);
    page.replace("{{CARD}}",   lastCardText);
    page.replace("{{CAM}}",    lastCamText);
    page.replace("{{VOL}}",    String(currentVolume));
    page.replace("{{AUDIO}}",  audioReady ? "OK" : "not connected");

    addNoCacheHeaders();
    server.send(200, "text/html; charset=utf-8", page);
}


void redirectToControls(const String& status) {
    lastWebStatus = status;
    addNoCacheHeaders();
    // /mp3 answers in both modes, so this redirect is mode-independent.
    server.sendHeader("Location", "/mp3", true);
    server.send(303, "text/plain", status);
}


void handleRoot() {
    sendControlPage();
}


// =====================================================
// LFR tuner page (root)
// =====================================================
// Calibrating used to mean editing constants and reflashing the Nano, which
// also meant unplugging the ESP32 from D0/D1 every round. This page edits the
// Nano's tunables live over the existing UART link instead, and shows the
// telemetry stream so a change can be judged on the next lap.
// tunerPage[] lives in TunerPage.h -- see the note there.


void handleTuner() {
    addNoCacheHeaders();
    server.send_P(200, "text/html; charset=utf-8", tunerPage);
}


void handleRail() {
    addNoCacheHeaders();
    server.send_P(200, "text/html; charset=utf-8", railPage);
}


// The Nano protocol is newline-delimited, so a query argument must never be
// able to smuggle one through and inject a second command.
String sanitizeArg(const String& in) {
    String out;
    for (unsigned int i = 0; i < in.length() && out.length() < 12; i++) {
        char ch = in[i];
        if (isalnum(ch) || ch == '-') out += ch;
    }
    return out;
}


void handleTuneData() {
    String j = "{\"t\":\"";
    j += lastTelemetry;
    j += "\",\"v\":\"";
    j += lastTuneLine;
    j += "\",\"m\":\"";
    j += lastNanoMsg;
    j += "\",\"r\":";
    j += lfrRunning ? "true" : "false";
    j += "}";

    addNoCacheHeaders();
    server.send(200, "application/json", j);
}


void handleTuneSet() {
    String k = sanitizeArg(server.arg("k"));
    String v = sanitizeArg(server.arg("v"));
    if (k.length() == 0 || v.length() == 0) {
        server.send(400, "text/plain", "missing k/v");
        return;
    }

    String cmd = "SET " + k + " " + v;
    sendNano(cmd.c_str());
    server.send(204, "text/plain", "");
}


void handleTuneCmd() {
    String c = sanitizeArg(server.arg("c"));

    if (c == "START")      lfrStart();      // keeps lfrRunning in step
    else if (c == "STOP")  lfrStop();
    else if (c == "RAMP" || c == "SAVE" || c == "LOAD" || c == "GET" ||
             c == "MAN")
        sendNano(c.c_str());
    else {
        server.send(400, "text/plain", "bad cmd");
        return;
    }
    server.send(204, "text/plain", "");
}


void handlePlay() {
    if (!server.hasArg("track")) {
        redirectToControls("Missing track number");
        return;
    }

    int track = server.arg("track").toInt();
    if (track < 1 || track > MAX_TRACK) {
        redirectToControls("Invalid track number");
        return;
    }

    endNarration(false);        // manual playback takes over from a card
    playTrack(track);
    redirectToControls("Playing track " + String(track));
}


void handlePause() {
    player.pause();
    Serial.println(F("Audio Paused"));
    redirectToControls("Audio Paused");
}


void handleResume() {
    player.start();
    Serial.println(F("Audio Resumed"));
    redirectToControls("Audio Resumed");
}


void handleStop() {
    player.stop();
    endNarration(false);        // cancel any pending auto-resume
    Serial.println(F("Audio Stopped"));
    redirectToControls("Audio Stopped");
}


void handleVolumeUp() {
    if (currentVolume < 30)
        currentVolume++;

    player.volume(currentVolume);

    Serial.print(F("Volume: "));
    Serial.println(currentVolume);

    redirectToControls("Volume: " + String(currentVolume));
}


void handleVolumeDown() {
    if (currentVolume > 0)
        currentVolume--;

    player.volume(currentVolume);

    Serial.print(F("Volume: "));
    Serial.println(currentVolume);

    redirectToControls("Volume: " + String(currentVolume));
}


void handleLfr() {
    if (!server.hasArg("state")) {
        redirectToControls("Missing LFR state");
        return;
    }

    String state = server.arg("state");

    if (state == "on") {
        resumeAfterNarration = false;   // already running, nothing to resume
        lfrStart();
        redirectToControls("LFR Started");

    } else if (state == "off") {
        resumeAfterNarration = false;   // an explicit stop wins over a card
        lfrStop();
        redirectToControls("LFR Stopped");

    } else {
        redirectToControls("Bad LFR state");
    }
}


// =====================================================
// SETUP
// =====================================================
void setup() {

    Serial.begin(115200);

    delay(1000);

    Serial.println();
    Serial.println(F("=================================="));
    Serial.print(F("ESP32 Museum Guide Starting... "));
    Serial.println(firmwareVersion);
    Serial.println(F("=================================="));


    // --------------------------------
    // MFRC522 (SPI)
    // --------------------------------
    mfrc522.PCD_Init();
    MFRC522Debug::PCD_DumpVersionToSerial(mfrc522, Serial);
    Serial.println(F("RFID reader ready."));


    // --------------------------------
    // Nano line-follower link (UART1)
    // --------------------------------
    nanoSerial.begin(
        NANO_BAUD,
        SERIAL_8N1,
        NANO_RX,   // 25  RX  <- Nano D1 (TX)
        NANO_TX    // 26  TX  -> Nano D0 (RX)
    );
    Serial.println(F("Nano UART link ready (9600, RX=25, TX=26)"));

    // If the ESP32 reset while the Nano kept driving, put it back in a known
    // stopped state instead of inheriting whatever it was doing.
    lfrStop();


    // --------------------------------
    // ESP32-CAM trigger (software serial, TX only)
    // --------------------------------
    camSerial.begin(
        CAM_BAUD,
        SWSERIAL_8N1,
        -1,        // no RX: the cam never answers on this wire
        CAM_TX     // 27  TX -> cam GPIO13 (RX)
    );
    Serial.println(F("Cam trigger ready (9600, TX=27, write-only)"));


    // --------------------------------
    // DFPlayer (UART2)
    // --------------------------------
    dfSerial.begin(9600, SERIAL_8N1, DF_RX, DF_TX);

    Serial.println(F("Connecting DFPlayer..."));

    if (!player.begin(dfSerial)) {
        Serial.println(F("DFPlayer ERROR! Check wiring / SD card."));
        Serial.println(F("Audio disabled. RFID, web UI and LFR still work."));
        audioReady = false;
    } else {
        Serial.println(F("DFPlayer connected."));
        audioReady = true;
        player.volume(currentVolume);
        delay(500);
    }


    // --------------------------------
    // ESP32 WiFi Hotspot
    // --------------------------------
    Serial.println();
    Serial.println(F("Starting WiFi Hotspot..."));

    WiFi.mode(WIFI_AP);
    WiFi.setSleep(false);
    WiFi.softAP(
        ssid,
        password,
        1,
        false,
        4
    );

    IPAddress IP = WiFi.softAPIP();

    Serial.println();
    Serial.println(F("=================================="));
    Serial.println(F("WiFi Ready!"));
    Serial.print(F("WiFi Name: "));
    Serial.println(ssid);

    Serial.print(F("Password: "));
    Serial.println(password);

    Serial.print(F("Open Browser: http://"));
    Serial.println(IP);

    Serial.println(F("=================================="));


    // --------------------------------
    // Hub link (ESP-NOW) -- must come AFTER softAP so it inherits channel 1
    // --------------------------------
    espnowReady = initEspNow();
    if (espnowReady) {
        reportLocation();   // announce ourselves; the Hub shows us immediately
    }


    // --------------------------------
    // Web routes
    // --------------------------------
    // Both pages always answer at their own path, whatever WEB_MODE says, so
    // links and redirects never depend on the build.
    server.on("/mp3", handleRoot);
    server.on("/tuner", handleTuner);
    server.on("/rail", handleRail);     // drivetrain characterisation rig
#if WEB_MODE == WEB_TUNER
    server.on("/", handleTuner);
#else
    server.on("/", handleRoot);
#endif

    server.on("/tunedata", handleTuneData);
    server.on("/tuneset", handleTuneSet);
    server.on("/tunecmd", handleTuneCmd);

    server.on("/play", handlePlay);
    server.on("/pause", handlePause);
    server.on("/resume", handleResume);
    server.on("/stop", handleStop);
    server.on("/volumeup", handleVolumeUp);
    server.on("/volumedown", handleVolumeDown);
    server.on("/lfr", handleLfr);

    server.onNotFound([]() {
        server.send(404, "text/plain", "Not Found");
    });

    server.begin();

    Serial.println(F("Web Server Started!"));

    Serial.println();
    Serial.println(F("Serial Commands:"));
    Serial.println(F("1..9 = Play 000X.mp3"));
    Serial.println(F("p = Pause"));
    Serial.println(F("r = Resume"));
    Serial.println(F("s = Stop"));
    Serial.println(F("+ = Volume Up"));
    Serial.println(F("- = Volume Down"));
    Serial.println(F("f = LFR Start"));
    Serial.println(F("g = LFR Stop"));
    Serial.println(F("c = Test photo (SHOOT last station)"));
    Serial.println(F("e = Toggle emergency locally (bench test, Hub overrides)"));
    Serial.println(F("? = Status"));
    Serial.println();
    Serial.println(F("Tap a card..."));
}


// =====================================================
// LOOP
// =====================================================
void loop() {

    // Web Server
    server.handleClient();

    // Card reader -> stop, narrate, resume
    pollRfid();

    // DFPlayer end-of-track / error reports
    pollPlayer();

    // Nano STATE:/READY: replies
    pollNano();

    // Hub emergency broadcast -> enter/leave evacuation mode
    applyEmergencyState();

    // Keep the Hub dashboard fresh (it forgets "moving" after 2.5 s of silence).
    // Skipped entirely when ESP-NOW is down, so a Hub-less bench run does not
    // re-enter this on every single pass.
    if (espnowReady && (millis() - lastLocationMs >= LOCATION_HEARTBEAT_MS)) {
        reportLocation();
    }


    // ================================
    // Serial Monitor Control
    // ================================
    if (Serial.available()) {

        char command = Serial.read();

        if (command >= '1' && command <= '9') {

            endNarration(false);          // manual playback takes over
            playTrack(command - '0');

        }

        else if (command == 'p') {

            player.pause();
            Serial.println(F("Paused"));

        }

        else if (command == 'r') {

            player.start();
            Serial.println(F("Resumed"));

        }

        else if (command == 's' || command == 'S') {

            player.stop();
            endNarration(false);          // cancel any pending auto-resume
            Serial.println(F("Stopped"));

        }

        else if (command == '+') {

            if (currentVolume < 30)
                currentVolume++;

            player.volume(currentVolume);

            Serial.print(F("Volume: "));
            Serial.println(currentVolume);

        }

        else if (command == '-') {

            if (currentVolume > 0)
                currentVolume--;

            player.volume(currentVolume);

            Serial.print(F("Volume: "));
            Serial.println(currentVolume);

        }

        else if (command == 'f' || command == 'F') {

            resumeAfterNarration = false;
            lfrStart();
            Serial.println(F("LFR Start"));

        }

        else if (command == 'g' || command == 'G') {

            resumeAfterNarration = false;
            lfrStop();
            Serial.println(F("LFR Stop"));

        }

        else if (command == 'c' || command == 'C') {

            shootStation(lastStation);   // bench test without tapping a card

        }

        else if (command == 'e' || command == 'E') {

            // Bench test with no Hub powered up. The Hub re-broadcasts the real
            // state every 2 s, so with a Hub present this is overridden almost
            // immediately -- that is the Hub being authoritative, not a bug.
            espnowEmergencyWanted = !espnowEmergencyWanted;
            Serial.print(F("Local emergency test -> "));
            Serial.println(espnowEmergencyWanted ? F("ON") : F("OFF"));

        }

        else if (command == '?') {

            Serial.print(F("LFR: "));
            Serial.print(lfrRunning ? F("RUNNING") : F("STOPPED"));
            Serial.print(F(" | narrating: "));
            Serial.print(narrating ? F("yes") : F("no"));
            Serial.print(F(" | resume armed: "));
            Serial.print(resumeAfterNarration ? F("yes") : F("no"));
            Serial.print(F(" | audio: "));
            Serial.print(audioReady ? F("OK") : F("off"));
            Serial.print(F(" | volume: "));
            Serial.print(currentVolume);
            Serial.print(F(" | station: "));
            Serial.print(currentStation);
            Serial.print(F(" | emergency: "));
            Serial.print(emergencyActive ? (atExit ? F("AT EXIT") : F("EVACUATING"))
                                         : F("no"));
            Serial.print(F(" | hub link: "));
            Serial.print(espnowReady ? F("ESP-NOW ready") : F("off"));
            Serial.print(F(" | last photo: "));
            Serial.println(lastCamText);

        }
    }
}
