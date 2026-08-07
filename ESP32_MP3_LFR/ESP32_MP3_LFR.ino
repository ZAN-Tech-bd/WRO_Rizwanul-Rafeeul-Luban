#include <WiFi.h>
#include <WebServer.h>
#include <DFRobotDFPlayerMini.h>

// ================================
// DFPlayer  (UART2, pins 16/17)
// ================================
HardwareSerial mySerial(2);
DFRobotDFPlayerMini player;

// ESP32 pins
#define DF_RX 16
#define DF_TX 17

// ================================
// Nano line-follower link  (UART1, remapped)
//   Wiring (as installed):
//     Nano D1 (TX) --10k--+-- ESP32 D25 (RX)   <- Nano talks here
//                          +-- 20k -- GND        (5V->3.3V divider)
//     Nano D0 (RX) <------ ESP32 D26 (TX)       -> Nano listens here
//   => ESP32 D25 = RX, D26 = TX
// ================================
HardwareSerial nanoSerial(1);
#define NANO_RX 25
#define NANO_TX 26
const unsigned long NANO_BAUD = 9600;   // MUST match the Nano sketch

// ================================
// WiFi Access Point
// ================================
const char* ssid = "ESP32-MP3";
const char* password = "12345678";

WebServer server(80);

// Current volume
int currentVolume = 20;
String lastWebStatus = "Ready";

const char* firmwareVersion = "WEB-FORMS-v2";


// =====================================================
// WEB PAGE
// =====================================================
const char webpage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>

<meta name="viewport" content="width=device-width, initial-scale=1">
<meta http-equiv="Cache-Control" content="no-store, no-cache, must-revalidate">

<title>ESP32 MP3 Player</title>

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

</style>

</head>

<body>

<div class="container">

<h1>ESP32 MP3 Player</h1>
<p class="version">Firmware: WEB-FORMS-v2</p>

<div class="card">

<h2>Music</h2>

<form class="inline" method="get" action="/play">
<button class="play" type="submit" name="track" value="1">Play 0001</button>
</form>

<form class="inline" method="get" action="/play">
<button class="play" type="submit" name="track" value="2">Play 0002</button>
</form>

<form class="inline" method="get" action="/play">
<button class="play" type="submit" name="track" value="3">Play 0003</button>
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

</div>


<div class="card">

<h3 id="status">{{STATUS}}</h3>
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
    Serial.print("-> Nano: ");
    Serial.println(cmd);
}


// =====================================================
// Play MP3
// =====================================================
void playTrack(int track) {

    Serial.print("Playing track: ");
    Serial.println(track);

    player.playMp3Folder(track);
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
    String page = FPSTR(webpage);
    page.replace("{{STATUS}}", lastWebStatus);
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
    if (track < 1 || track > 14) {
        redirectToControls("Invalid track number");
        return;
    }

    playTrack(track);
    redirectToControls("Playing track " + String(track));
}


void handlePause() {

    player.pause();

    Serial.println("Audio Paused");

    redirectToControls("Audio Paused");
}


void handleResume() {

    player.start();

    Serial.println("Audio Resumed");

    redirectToControls("Audio Resumed");
}


void handleStop() {

    player.stop();

    Serial.println("Audio Stopped");

    redirectToControls("Audio Stopped");
}


void handleVolumeUp() {

    if (currentVolume < 30)
        currentVolume++;

    player.volume(currentVolume);

    Serial.print("Volume: ");
    Serial.println(currentVolume);

    redirectToControls("Volume: " + String(currentVolume));
}


void handleVolumeDown() {

    if (currentVolume > 0)
        currentVolume--;

    player.volume(currentVolume);

    Serial.print("Volume: ");
    Serial.println(currentVolume);

    redirectToControls("Volume: " + String(currentVolume));
}


void handleLfr() {

    if (server.hasArg("state")) {

        String state = server.arg("state");

        if (state == "on") {

            sendNano("START");      // deterministic: force line-follow ON
            redirectToControls("LFR Started");

        } else if (state == "off") {

            sendNano("STOP");       // deterministic: force STOP
            redirectToControls("LFR Stopped");

        } else {
            redirectToControls("Bad LFR state");

        }

    } else {
        redirectToControls("Missing LFR state");

    }
}


// =====================================================
// SETUP
// =====================================================
void setup() {

    Serial.begin(115200);

    delay(1000);

    Serial.println();
    Serial.println("==============================");
    Serial.print("ESP32 MP3 Player Starting... ");
    Serial.println(firmwareVersion);
    Serial.println("==============================");


    // --------------------------------
    // Nano line-follower link (UART1)
    // --------------------------------
    nanoSerial.begin(
        NANO_BAUD,
        SERIAL_8N1,
        NANO_RX,   // 25  RX  <- Nano D1 (TX)
        NANO_TX    // 26  TX  -> Nano D0 (RX)
    );
    Serial.println("Nano UART link ready (9600, RX=25, TX=26)");


    // --------------------------------
    // DFPlayer (UART2)
    // --------------------------------

    mySerial.begin(
        9600,
        SERIAL_8N1,
        DF_RX,
        DF_TX
    );

    Serial.println("Connecting DFPlayer...");

    if (!player.begin(mySerial)) {

        Serial.println("DFPlayer ERROR!");
        Serial.println("Check wiring / SD card.");

        while (true) {
            delay(1000);
        }
    }

    Serial.println("DFPlayer Connected!");

    player.volume(currentVolume);

    delay(500);


    // --------------------------------
    // ESP32 WiFi Hotspot
    // --------------------------------

    Serial.println();
    Serial.println("Starting WiFi Hotspot...");

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
    Serial.println("==============================");
    Serial.println("WiFi Ready!");
    Serial.print("WiFi Name: ");
    Serial.println(ssid);

    Serial.print("Password: ");
    Serial.println(password);

    Serial.print("Open Browser: http://");
    Serial.println(IP);

    Serial.println("==============================");


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

    Serial.println("Web Server Started!");
    Serial.print("Web firmware: ");
    Serial.println(firmwareVersion);

    Serial.println();
    Serial.println("Serial Commands:");
    Serial.println("1 = Play 0001.mp3");
    Serial.println("2 = Play 0002.mp3");
    Serial.println("3 = Play 0003.mp3");
    Serial.println("p = Pause");
    Serial.println("r = Resume");
    Serial.println("s = Stop");
    Serial.println("+ = Volume Up");
    Serial.println("- = Volume Down");
    Serial.println("f = LFR Start");
    Serial.println("g = LFR Stop");
    Serial.println();
}


// =====================================================
// LOOP
// =====================================================
void loop() {

    // Web Server
    server.handleClient();


    // ================================
    // Serial Monitor Control
    // ================================

    if (Serial.available()) {

        char command = Serial.read();

        if (command == '1') {

            playTrack(1);

        }

        else if (command == '2') {

            playTrack(2);

        }

        else if (command == '3') {

            playTrack(3);

        }

        else if (command == 'p') {

            player.pause();

            Serial.println("Paused");

        }

        else if (command == 'r') {

            player.start();

            Serial.println("Resumed");

        }

        else if (command == 's') {

            player.stop();

            Serial.println("Stopped");

        }

        else if (command == '+') {

            if (currentVolume < 30)
                currentVolume++;

            player.volume(currentVolume);

            Serial.print("Volume: ");
            Serial.println(currentVolume);

        }

        else if (command == '-') {

            if (currentVolume > 0)
                currentVolume--;

            player.volume(currentVolume);

            Serial.print("Volume: ");
            Serial.println(currentVolume);

        }

        else if (command == 'f' || command == 'F') {

            sendNano("START");
            Serial.println("LFR Start");

        }

        else if (command == 'g' || command == 'G') {

            sendNano("STOP");
            Serial.println("LFR Stop");

        }
    }


    // --------------------------------
    // Forward any Nano replies to USB Serial
    // (e.g. "READY:STOPPED", "STATE:RUNNING")
    // --------------------------------
    while (nanoSerial.available()) {
        Serial.write(nanoSerial.read());
    }
}
