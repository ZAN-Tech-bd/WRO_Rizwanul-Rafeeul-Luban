/*
  ESP32 Museum Guide Controller
  =============================
  Merge of ESP32_MP3_LFR (DFPlayer + WiFi web UI + Nano line-follower link)
  and ESP32_RFID_MP3 (MFRC522 card reader -> DFPlayer track).

  Tour behaviour
  --------------
    Tap a mapped card -> the ESP32 tells the Nano to STOP, plays that card's
    track, and tells the Nano to START again when the track finishes.
    The auto-resume is only armed when it is safe to move:
      * the robot was already line-following when the card was tapped, or
      * the card is the START card (tour intro -> begin the tour).
    So tapping a station card on the bench plays audio but does NOT drive off.
    Pressing Stop or LFR STOP cancels a pending auto-resume.

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

  ----------------------------------------------------------------------
  UID -> Track table
    Bytes are in the SAME order the reader prints them, i.e.
    "Card UID: 64 14 F6 B3"  ->  {0x64, 0x14, 0xF6, 0xB3}.
    The START card (18 C4 0F BA) plays 0001.mp3 (tour intro); the rest
    map to tracks 2..8. All entries are full 4-byte UIDs.
  ----------------------------------------------------------------------
*/

#include <WiFi.h>
#include <WebServer.h>
#include <DFRobotDFPlayerMini.h>
#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522Debug.h>

const char* firmwareVersion = "RFID-MP3-LFR-v1";

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
char    nanoBuf[32];
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
  uint8_t uid[10];   // MIFARE UIDs are 4, 7 or 10 bytes
  uint8_t uidSize;   // number of meaningful bytes in uid[]
  int     track;
  const char* name;  // human label shown on Serial and the web page
};

const RfidTrack TRACKS[] = {
  { {0x18, 0xC4, 0x0F, 0xBA}, 4, 1, "START"   },  // 18C40FBA -> 0001.mp3 (tour start)
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
// Web server
// ============================================================
const char* ssid     = "ESP32-MP3";
const char* password = "12345678";

WebServer server(80);

String lastWebStatus = "Ready";
String lastCardText  = "No card scanned yet";


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
<p class="state">Last card: {{CARD}}</p>
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
}


void lfrStop() {
    sendNano("STOP");
    lfrRunning = false;
}


void handleNanoLine(const char* line) {
    if (line[0] == '\0') return;

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

    if (lfrRunning) {
        lfrStop();                      // stop at the station before narrating
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

    String page = FPSTR(webpage);
    page.replace("{{FW}}",     firmwareVersion);
    page.replace("{{STATUS}}", lastWebStatus);
    page.replace("{{LFR}}",    lfrText);
    page.replace("{{CARD}}",   lastCardText);
    page.replace("{{VOL}}",    String(currentVolume));
    page.replace("{{AUDIO}}",  audioReady ? "OK" : "not connected");

    addNoCacheHeaders();
    server.send(200, "text/html; charset=utf-8", page);
}


void redirectToControls(const String& status) {
    lastWebStatus = status;
    addNoCacheHeaders();
    server.sendHeader("Location", "/", true);
    server.send(303, "text/plain", status);
}


void handleRoot() {
    sendControlPage();
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
    // Web routes
    // --------------------------------
    server.on("/", handleRoot);
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
            Serial.println(currentVolume);

        }
    }
}
