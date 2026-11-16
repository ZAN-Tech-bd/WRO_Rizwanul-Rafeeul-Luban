# Smart Museum Guidance Robot — System Architecture (Mind Map Source)

A multi-robot, multi-node museum tour guide for the WRO Future Innovators "Smart Museum"
project. A single tour robot drives along a line, stops at 7 exhibit stations, narrates
each artifact in the visitor's chosen language, photographs it, and reports its location
to a dashboard. A hardware emergency can override everything and evacuate visitors.

## Central node (root of the mind map)
**Tour Robot Brain = ESP32_RFID_MP3_LFR** (ESP32-WROOM-32)
- Role: the master controller on the robot. Owns the tour state machine.
- Board: ESP32-WROOM-32 DevKit, WiFi AP "ESP32-MP3" on channel 1, web UI on port 80.
- Firmware version string: `RFID-MP3-LFR-v2-espnow`.

## The five modules and how they connect

### 1. ESP32_RFID_MP3_LFR — Robot Brain / Tour Controller
- Reads station RFID cards (MFRC522 over SPI) and maps each UID -> an MP3 track.
- Plays narration on a DFPlayer Mini (audio) over UART2.
- Tells the Arduino Nano line-follower to STOP at a station, narrate, then START again.
- Triggers the ESP32-CAM to photograph each station (software serial, TX only).
- Talks to the Hub over ESP-NOW: sends MSG_LOCATION (station + moving flag) + 1 s heartbeat.
- Listens for MSG_EMERGENCY from the Hub -> enters EVACUATION mode.
- Web UI (plain HTML, no JS): tracks 1-8, pause/resume/stop, volume, LFR start/stop, status.
- Serial monitor commands (115200): 1-9 play, p/r/s, +/-, f/g, c photo, e emergency, ? status.

  Key behaviours:
  - Tour flow: tap START card -> plays 0001.mp3 intro -> arms auto-resume -> line-following begins.
  - At a station card: STOP -> photo -> narrate -> (if resume armed) START.
  - Auto-resume only armed if robot was already moving OR it is the START card (so bench taps don't drive).
  - Evacuation: cut audio, drive, IGNORE every station card except station 7 (EXIT), then stop.

  Wiring (peripheral pins share nothing):
  - MFRC522: SS=5, SCK=18, MOSI=23, MISO=19 (VSPI).
  - DFPlayer: ESP32 GPIO16(RX2)<-DF TX, GPIO17(TX2)->DF RX (via 1k). UART2 @9600.
  - Nano link: ESP32 GPIO25(RX)<-Nano D1(TX) via 10k/20k divider; GPIO26(TX)->Nano D0(RX). UART1 @9600.
  - Cam trigger: ESP32 GPIO27(TX)->CAM GPIO13(RX). Software serial, TX only @9600.
  - Hub link: ESP-NOW broadcast, channel 1, AP interface.

  UID -> Track map: 18C40FBA=1(START), 6414F6B3=2, 4A6BDF34=3, D836D4B9=4, 94606BB2=5,
  1848F4BA=6, 94581AB3=7, 094CB192=8(EXIT/station7). Station id = track - 1.

### 2. NANO_LFR — Arduino Nano Line Follower (motor + IR sensors)
- Role: the robot's "legs". Drives the motors and reads 5 IR sensors; follows the line.
- Board: Arduino Nano (ATmega328), ATmega AVR, no WiFi.
- Default state: STOPPED. Only moves when the ESP32 sends START.
- Command protocol (9600 8N1, newline-terminated): START/1=on, STOP/0=off, GO/T/TOGGLE=flip.
- Replies READY:STOPPED at boot, STATE:RUNNING / STATE:STOPPED after every change.
- Line-follow algorithm: weighted 5-sensor error + PD (kp/kd) correction; u-turn recovery when
  line is lost (uses lastSensor); right turn at T-junctions (all sensors active).
- SPEED_PERCENT = 45 scales every speed (u-turn, base, straight, turn) and kp/kd together.
- Pins: motors ena=10,enb=5,inA=8,inB=7,inC=11,inD=12; IR sensors A0-A4.

### 3. firmware/hub — The Track ESP32 (Hub / Dashboard server)
- Role: the system's brain off-robot. Serves the web dashboard + REST API + owns the emergency state.
- Board: ESP32. Dual-core split:
  - Core 0 (webTask): WiFi SoftAP "MuseumHub" (channel 1) + ESP-NOW receive + WebServer +
    captive-portal DNS. Serves visitor.html / admin.html / /api/* from LittleFS.
  - Core 1 (loop): drains ESP-NOW queue + NeoPixel LED show + passive buzzer siren +
    emergency broadcast every 2 s.
- Shared state gState guarded by a mutex; LEDs via RMT (Adafruit NeoPixel) so WiFi won't glitch them.
- Emergency flow: GPIO21 button (latching, HIGH when pressed) OR POST /api/emergency?on=1.
  Hold 3 s to clear. On emergency: LEDs blink red, buzzer two-tone wail, broadcast MSG_EMERGENCY.
- Robot status: receives MSG_LOCATION, maps stationId->STATION_IDS; robotMoving lapses to
  "narrating" after ROBOT_MOVE_SETTLE_MS (2.5 s).
- Camera photos: receives JPEG POST at /api/image/upload (multipart, field "image"), stores
  under /photos on microSD (VSPI) or LittleFS; keeps newest GALLERY_MAX_PHOTOS (12).
- Visitor check-in: QR codes -> /api/checkin or /api/enter -> admin approve/deny -> dashboard.
- Wiring: WS2812 DIN=GPIO15 (7 LEDs), passive buzzer +=GPIO4, emergency button=GPIO21,
  microSD MISO=19/MOSI=23/SCK=18/CS=5.
- Key API: GET /api/state, POST /api/emergency, POST /api/image/upload, GET /api/gallery,
  GET /api/image?seq=N, POST /api/robot/advance (test), POST /api/approve|deny.

### 4. firmware/cam — ESP32S-CAM (GC2145) Photo Node
- Role: captures a station photo and uploads it to the Hub dashboard.
- Board: ESP32S-CAM on an ESP32-CAM-MB (CH340) Type-C programmer. Sensor GC2145 (PID 0x2145).
- GC2145 caveat: cannot do native JPEG. Grabs RGB565 frames, converts to JPEG in software via
  frame2jpg(). Settings: XCLK 10 MHz, PIXFORMAT_RGB565, QVGA 320x240, 1 framebuffer in PSRAM.
- SD_MMC in 1-bit mode (GPIO2/14/15) so it never collides with the camera or GPIO4 flash LED.
- Two command sources: USB console @115200, and the robot trigger wire UART1 GPIO13 (RX only) @9600.
- Trigger: robot sends "SHOOT <station>" on each station RFID read (except the START card).
- Flow on SHOOT: ensure WiFi -> capture -> frame2jpg -> optional SD backup -> POST to Hub
  /api/image/upload -> ESP-NOW MSG_IMAGE_READY ping.
- Photos saved as /photos/IMG_####.JPG (zero-padded, never overwriting; counter survives reboot
  by scanning the folder for the max index).
- Captures never block on the network: if Hub unreachable, photo stays SD/RAM-only.
- Commands: SHOOT, CAPTURE, SEND, WIFI, AUTO <ms>, STATUS, LIST, INIT, FLASH ON/OFF, HELP.

### 5. firmware/display — Robot Face & Info Screen
- Role: a dedicated renderer for the robot's TFT screen (animated face + info UI).
- Board: Arduino Mega 2560 (8-bit AVR, no WiFi) + 3.2" MCUFRIEND parallel TFT (ILI9481, 480x320, 16-bit).
- Cannot join ESP-NOW, so the ESP32 brain sends short text commands over UART (Serial1, 9600).
- Shield plugs directly onto the Mega headers (pins 22-53 + A0-A5 used by the display bus).
- All drawing is procedural (no bitmaps/SD): animated robot face with eased blinking, left/right
  gaze, cheeks, Jamdani forehead mark, 4 emotions (happy/neutral/alert/sleepy).
- Screens: boot splash, animated face, language menu, station screen, full-red emergency screen.
- Command protocol (case-insensitive, newline-terminated): FACE [emotion], LANG 0|1, MENU,
  STATION <id> <moving>, EMERGENCY, BOOT.
- Wiring: Mega RX1(pin19) <- ESP32 GPIO17(TX) via 1k resistor; common GND. English text only
  on the TFT (Bengali shaping is done in the phone browser dashboard).

## Shared backbone: firmware/common/MuseumProtocol.h
One fixed-layout ESP-NOW packet every node agrees on (packed struct, 26.4 C stored as tempX10).
- Channel pinned to 1 on all boards (mismatch fails silently -> always keep all APs on channel 1).
- MessageType: MSG_LOCATION, MSG_EMERGENCY, MSG_HEALTH, MSG_VISITOR_COUNT, MSG_IMAGE_READY,
  MSG_COMMAND, MSG_HEARTBEAT.
- Flags: EMERGENCY_ACTIVE, OBSTACLE_DETECTED, ALERT_ACTIVE, DEVICE_ONLINE, ROBOT_MOVING.
- Has magic (0x4D53 "MS"), version, checksum (sum of bytes up to the checksum field).
- EspNowCompat.h aliases the recv callback type so one sketch compiles on ESP32 core 2.x and 3.x.

## Key data flows (edges of the mind map)
1. RFID tap -> ESP32 brain -> Nano (STOP) -> DFPlayer (narrate) -> Cam (SHOOT) -> Hub (photo+location).
2. Track finishes -> ESP32 -> Nano (START) -> robot moves on (if resume armed).
3. Hub emergency -> ESP-NOW MSG_EMERGENCY -> ESP32 brain -> Nano drives to exit, ignoring stations.
4. Cam photo -> HTTP POST /api/image/upload -> Hub gallery -> phone dashboard thumbnail strip.
5. Visitor QR -> Hub /api/enter or /api/checkin -> admin approve -> dashboard unlock.
6. ESP32 brain -> display Mega (UART text) -> face/station/emergency screens.

## Timing & safety constants
- DEBOUNCE_MS=1500 (same card re-tap), NARRATION_MIN_MS=1500 (ignore stale play-finished),
  NARRATION_MAX_MS=90000 (resume anyway if no end-of-track report).
- LOCATION_HEARTBEAT_MS=1000 (must be < Hub ROBOT_MOVE_SETTLE_MS=2500).
- EMERGENCY_BROADCAST_MS=2000 (Hub re-sends so rebooted nodes re-sync).
- Two WiFi APs coexist: robot "ESP32-MP3" and Hub "MuseumHub", BOTH on channel 1 for ESP-NOW.
