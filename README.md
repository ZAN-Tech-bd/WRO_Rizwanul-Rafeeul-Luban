# Smart Museum Guidance System

**WRO 2026 · Theme "Robots Meet Culture" · Elementary category (ages 8–12)**
Team: WRO_Rizwanul-Rafeeul-Luban · Regional: WROBd

A line-following guide robot walks visitors around a small museum, stopping at
each artifact to tell its story (Bangla or English), while a track of ESP32
"nodes" lights the current station, watches every artifact's temperature /
humidity / gas level, counts visitors at the gate, and serves a live phone
dashboard — all **fully offline** (no venue Wi‑Fi, no cloud). This document is
the map of the whole repository: what every part is *for*, what hardware you
need, how it's wired, and how the boards talk to each other.

> Theme areas targeted: **Area 1 — Protecting/Preserving Cultural Heritage**
> (per‑artifact environment monitoring) and **Area 3 — Experiencing Art &
> History with Robots** (the guided tour itself).

---

## 1. Table of contents

1. [Table of contents](#1-table-of-contents)
2. [Repository map](#2-repository-map)
3. [System architecture — who talks to whom](#3-system-architecture--who-talks-to-whom)
4. [Equipment list (Bill of Materials), by function](#4-equipment-list-bill-of-materials-by-function)
5. [Wiring — pin map for every board](#5-wiring--pin-map-for-every-board)
6. [Communication — how the boards understand each other](#6-communication--how-the-boards-understand-each-other)
7. [Power & grounding](#7-power--grounding)
8. [Build & flash each node](#8-build--flash-each-node)
9. [Project status](#9-project-status)
10. [Further reading](#10-further-reading)

---

## 2. Repository map

```
WRO_Rizwanul-Rafeeul-Luban/
├── BUILD_PLAN.md                  ← THE canonical plan: scope, budget, phases, risks
├── CLAUDE_RECOMMENDATIONS.md      ← decision log (what we chose/rejected and why)
├── findings.md / progress.md      ← running engineering audit log (dated entries)
├── task_plan.md                   ← active migration/implementation checklist
├── UI_TEXT_PATCH_HELD.md          ← a held (not-yet-applied) UI text patch, for reference
│
├── firmware/                      ← one PlatformIO project per microcontroller
│   ├── common/                    ← protocol headers shared by every board (§6)
│   ├── hub/                       ← ESP32 — Track Hub: SoftAP + web server + ESP‑NOW + LEDs  ✅
│   ├── robot-nano/                ← Arduino Nano — line following + RFID station stops       ✅
│   ├── robot-esp/                 ← ESP32 — RFID bridge + Bangla/English narration (DFPlayer) ✅
│   ├── robot/                     ← legacy single‑ESP32 robot skeleton (superseded by the
│   │                                 robot-nano + robot-esp split above; kept for reference)
│   ├── display/                   ← Arduino Mega 2560 + TFT — animated robot face / station UI ✅
│   ├── health/                    ← ESP32 — per‑artifact DHT11 + MQ‑135 (temp/humid/gas)      ⬜ skeleton
│   ├── emergency/                 ← ESP32 — physical emergency pushbutton                     ⬜ skeleton
│   ├── cam/                       ← ESP32‑CAM — photo capture to its own microSD              ✅ (offline)
│   └── sdcard/                    ← standalone microSD bring‑up/diagnostic tool
│
├── webapp/                        ← the visitor + admin dashboard (plain HTML/CSS/JS)
│   ├── index.html                 ← gate: QR check‑in → admin approval
│   ├── visitor.html               ← visitor dashboard (station, queue, emergency banner)
│   ├── admin.html                 ← admin dashboard (count, health table, robot, emergency)
│   └── js/ , css/                 ← app logic, mock/real Hub client, styling
│
├── plans/                         ← deep-dive wiring/engineering plans (one per subsystem)
│   ├── robot-wiring.md            ← legacy direct‑ESP32 robot wiring (superseded, kept for history)
│   ├── robot-nano-esp-wiring.md   ← the active Nano + ESP32 split wiring design
│   └── hub-sensor-wiring.md       ← forward-looking track upgrade (56 LEDs, 4 real sensor pairs, servos)
│
├── tools/
│   ├── generate_qr.py             ← generates the 3 gate QR codes (Wi‑Fi join + check‑in)
│   └── dfplayer_audio/            ← placeholder MP3 track set + generator for the DFPlayer SD card
│
├── ESP32_LFR/                     ← standalone Arduino‑IDE line‑follower sketch (bring‑up reference)
├── .standalone-display-repo/      ← the animated‑face sketch as its own mini‑repo (for sharing/credit)
└── *.pdf                          ← official WRO rulebooks (regional + general + team info)
```

**Where to start reading, in order:** this file → [`BUILD_PLAN.md`](BUILD_PLAN.md) (why the
project is scoped the way it is) → [`firmware/README.md`](firmware/README.md) (firmware
overview) → the individual `firmware/<node>/README.md` files (exact pins/commands) → `plans/`
(deep engineering detail behind those pins).

---

## 3. System architecture — who talks to whom

There are **7 physical microcontrollers** (8 counting the Nano/ESP32 pair on the robot as one
"robot brain unit"), one shared wireless bus, and two client devices (phones/laptop). Nothing
touches the internet.

```
                         ESP-NOW  —  Wi-Fi channel 1  —  no router, no internet
   ┌─────────────┬───────────────┬────────────────┬────────────────┐
   │             │               │                │                │
[Robot ESP32] [Health ESP32] [Emergency ESP32] [ESP32-CAM]      [Hub = Track ESP32]
 (via Nano)    DHT11+MQ135    big pushbutton    (HTTP client,    SoftAP "MuseumHub" ch1
 RFID station  per artifact   → EMERGENCY       uploads a photo)  + web server + /api
 stops, audio     broadcast    broadcast            │             + WS2812 station LEDs
     │                                               └──HTTP POST──────┘
     │ UART (9600, text frames)                                        │
     ▼                                                                  │  phones/laptop join
[Nano]  ← line follower + RC522 (RFID)             [Display: Mega 2560] │  this Wi-Fi (clients)
 owns motors, IR array, RFID reads     UART (9600) ← robot ESP32        │
                                        drives the face/station TFT     ▼
                                                              http://192.168.4.1
                                                        (QR code at the gate)
```

**The bridge rule (locked design decision):** only the **Hub** hosts Wi‑Fi/HTTP. Every other
ESP32 is either an ESP‑NOW peer (tiny broadcast packets) or, for the camera, an HTTP *client*
that uploads to the Hub. **Nothing on a phone or laptop ever talks directly to a sensor or the
robot** — the Hub is the single bridge in both directions:

```
sensor/robot node  --ESP-NOW-->  Hub  --HTTP-->  phone/laptop screen
phone/laptop screen --HTTP-->  Hub  --ESP-NOW-->  sensor/robot node
```

This keeps Wi‑Fi/HTTP traffic from ever glitching the motors, the LEDs, or the camera.

| Node | Board | Job | Talks to the bus how |
|---|---|---|---|
| **Hub** | ESP32 (Track ESP32) | SoftAP `MuseumHub` (ch 1) + web server + ESP‑NOW aggregator + WS2812 station LEDs + buzzer | Hosts Wi‑Fi/HTTP; receives ESP‑NOW from every other node |
| **Robot — Nano** | Arduino Nano | Line following (5‑ch IR array), drive motors (L298N), reads RC522 RFID tags at each station | UART link to the robot's ESP32 only (not on ESP‑NOW) |
| **Robot — ESP32** | ESP32 | Receives RFID station events from the Nano, plays the matching Bangla/English track (DFPlayer Mini); will broadcast `MSG_LOCATION` over ESP‑NOW once wired in | UART to Nano; ESP‑NOW to Hub (next integration step) |
| **Display** | Arduino Mega 2560 + TFT shield | Animated robot face / language menu / station name / emergency screen | UART **from** the robot ESP32 only — not Wi‑Fi capable, not on the ESP‑NOW bus |
| **Health** | ESP32 (× per artifact) | DHT11 (temp/humidity) + MQ‑135 (gas) at one artifact; broadcasts `MSG_HEALTH` | ESP‑NOW to Hub |
| **Emergency** | ESP32 | Physical pushbutton; broadcasts `MSG_EMERGENCY` to everyone | ESP‑NOW broadcast (not just to Hub — every node reacts directly) |
| **Cam** | ESP32‑CAM (GC2145) | Captures a JPEG, saves to its own microSD; later HTTP‑uploads it to the Hub | HTTP client of the Hub (joins `MuseumHub` as a Wi‑Fi client only) |
| Visitor phone / Admin laptop | — | Browser | HTTP client of the Hub only |

---

## 4. Equipment list (Bill of Materials), by function

Equipment is grouped by the **function** it serves in the system, not by which board it plugs
into — the same function (e.g. "line following") may span a controller *and* its sensors.
Full pricing/quantities and the reasoning behind each cut are in
[`BUILD_PLAN.md §5`](BUILD_PLAN.md#5-revised-bom--20000-tk); this table is the reference list.

### 4.1 Controllers (the "brains")

| Qty | Part | Function |
|---:|---|---|
| 4–5 | **ESP32 DevKit V1** (DOIT‑style, 30‑pin, ESP‑WROOM‑32) | Hub, robot companion, health (×N), emergency, +1 spare |
| 1 | **Arduino Nano** (ATmega328P) | Robot motion controller: line following + RFID reads |
| 1 | **Arduino Mega 2560** | Dedicated display driver (no Wi‑Fi — text commands only) |
| 1 | **ESP32‑CAM (GC2145)** on an **ESP32‑CAM‑MB** programmer | Photo capture at a station |

### 4.2 Drivetrain & line following (robot base)

| Qty | Part | Function |
|---:|---|---|
| 2 | 6 V TT gear motors | Drive wheels (left/right) |
| 1 | Caster wheel | Third, unpowered support point |
| 1 | **L298N** dual H‑bridge motor driver | Converts Nano PWM/direction signals into motor current |
| 1 | 5‑channel digital IR line‑sensor array (3.3 V) | Detects the black track line under the chassis |
| — | Clamps/couplers, wheels | Mechanical drive coupling |

### 4.3 Station identification & audio narration (robot)

| Qty | Part | Function |
|---:|---|---|
| 1–2 | **RC522** RFID reader (13.56 MHz, SPI, 3.3 V only) | Detects the tag taped at each station → tells the robot to stop |
| 7 | MIFARE RFID cards/tags | One per artifact station |
| 1–2 | **DFPlayer Mini** MP3 module | Plays the Bangla/English narration track for the current station |
| 1–2 | 3 W / 4–8 Ω speaker | Audio output (DFPlayer's onboard amp drives it directly — no separate amp needed) |
| 1–2 | microSD card, **8–16 GB, FAT32** | Stores narration tracks as `/mp3/0001.mp3` … `0014.mp3` (see §6.4) |

### 4.4 Indicators — direction & station lighting

| Qty | Part | Function |
|---:|---|---|
| 1 | WS2812B 8‑pixel stick (robot) | Shows the robot's direction/status |
| 7 (or 56 in the track‑upgrade plan) | WS2812B pixels along the track | Lights the station the robot is currently at; all blink red on emergency |
| 2 | 0.96" OLED SSD1306 | Station name readout (robot) |
| 1 | 3.2" TFT (ILI9481, 480×320, on the Mega) | Animated robot face, language menu, station/emergency screens |

### 4.5 Artifact protection — environment monitoring ("Area 1" story)

| Qty | Part | Function |
|---:|---|---|
| 3–4 | **DHT11** temperature/humidity sensor | One per monitored artifact — feeds `MSG_HEALTH` |
| 3–4 | **MQ‑135** gas/air‑quality sensor | One per monitored artifact — relative gas trend, not calibrated ppm |

### 4.6 Visitor flow & safety

| Qty | Part | Function |
|---:|---|---|
| 2–4 | IR obstacle/beam sensor modules | Front‑of‑robot "stop for people" + gate visitor counter |
| 1 | Big push‑button (emergency) | Autonomous emergency trigger (WRO rule 5.1.3 — no remote control) |
| 4 | Buzzers (active) | Track alarm / restricted‑area alert / emergency |

### 4.7 Power & protection

| Qty | Part | Function |
|---:|---|---|
| 1 | 3S LiPo battery + B3 charger | Main robot power source |
| 2–3 | **LM2596** adjustable buck converters | Separate **logic**, **motor**, and (if per‑artifact gas sensors) **heater** rails — this is the #1 fix for ESP32 brownout resets |
| 1 | 74HCT125 quad buffer (DIP‑14) | 3.3 V→5 V push‑pull level shift for WS2812B data (locked choice — not the HW‑209 module) |
| — | Fuses, master switch, star‑ground distribution block | Short‑circuit/overcurrent protection and a single common ground reference |
| — | Assorted resistors/capacitors (decoupling, dividers, level shifting) | See each node's wiring table in §5 |

### 4.8 Structure

| Qty | Part | Function |
|---:|---|---|
| 1 | 5 mm PVC board | Robot chassis (cut, not 3D‑printed — faster to iterate) |
| — | Breadboards, jumpers, wire, zip ties | Prototyping and harnessing |

---

## 5. Wiring — pin map for every board

Each table below is the **currently implemented** pin map (matches the firmware source, not
just a plan). Where a `plans/*.md` document proposes a future upgrade instead of the current
build, that is called out explicitly.

### 5.1 Hub — Track ESP32 (`firmware/hub/`) ✅

| Part | ESP32 pin | Notes |
|---|---|---|
| WS2812 strip **DIN** | GPIO15 | 1 LED per station × 7 (56‑pixel upgrade path in [`plans/hub-sensor-wiring.md`](plans/hub-sensor-wiring.md)) |
| Active buzzer **+** | GPIO4 | **−** → GND; use a 2N2222 driver if it draws >20 mA |
| microSD **SCK** | GPIO18 | VSPI, optional (camera photo storage) |
| microSD **MISO** | GPIO19 | VSPI |
| microSD **MOSI** | GPIO23 | VSPI |
| microSD **CS** | GPIO5 | VSPI |
| Power | 5 V (USB or buck) | Low‑power node — logic/motor rail separation matters on the *robot*, not here |

Pins live in [`firmware/hub/include/config.h`](firmware/hub/include/config.h). The microSD is
optional: without a card the Hub falls back to storing photos in LittleFS.

### 5.2 Robot — Arduino Nano (`firmware/robot-nano/`) ✅ — motors, line sensor, RFID

| Nano pin | Connects to | Notes |
|---|---|---|
| A0–A4 | IR array, far‑left → far‑right | 3.3 V array; physical position order, not silkscreen order |
| D2, D3 | L298N `IN1`, `IN2` | Left‑motor direction |
| D4, D5 | L298N `IN3`, `IN4` | Right‑motor direction |
| D6 | L298N `ENA` | Left‑motor PWM (hardware PWM pin) |
| D9 | L298N `ENB` | Right‑motor PWM (hardware PWM pin) |
| D10 | RC522 `SDA/SS` | Chip select |
| D11, D12, D13 | RC522 `MOSI`, `MISO`, `SCK` | ATmega328P's fixed hardware‑SPI pins — cannot be moved |
| A5 | RC522 `RST` | |
| D0 (RX), D1 (TX) | ESP32 GPIO17 (TX) / GPIO16 (RX) | Hardware UART link to the robot's ESP32, through a resistor divider (5 V→3.3 V safety) — **not USB** once wired in; disconnect during flashing |

RC522 runs at **3.3 V only** — never feed it 5 V. Full rationale and the D9–D12 → D2–D5 motor
remap history (forced by the RC522 needing the Nano's fixed SPI pins) is in
[`plans/robot-nano-esp-wiring.md`](plans/robot-nano-esp-wiring.md) and
[`findings.md`](findings.md).

### 5.3 Robot — ESP32 companion (`firmware/robot-esp/`) ✅ — RFID bridge + audio

| ESP32 pin | Connects to | Notes |
|---|---|---|
| GPIO16 (RX) | Nano TX (D1), via divider | Receives `RFID|…` station‑stop frames |
| GPIO17 (TX) | Nano RX (D0), via resistor | Sends `ACK`/`RESUME` back to the Nano |
| GPIO25 | DFPlayer **TX** | Protect/measure before connecting (module‑dependent voltage) |
| GPIO26 | DFPlayer **RX**, via 1 kΩ series resistor | |
| — | WS2812 arrow (robot), ESP‑NOW to Hub | **Planned next step** — not implemented in this file yet; see §9 |

### 5.4 Display — Arduino Mega 2560 + TFT shield (`firmware/display/`) ✅

| Mega pin | Connects to | Notes |
|---|---|---|
| TFT shield | Mega header stack | Plugs straight on — no manual wiring (consumes pins 22–53 + A0–A5) |
| RX1 (pin 19) | Robot ESP32 GPIO17 (TX) | Through a **1 kΩ** resistor (3.3 V→5 V safety) |
| TX1 (pin 18) | *(optional)* Robot ESP32 GPIO16 (RX) | Only if the robot wants replies |
| GND | ESP32 GND | Common ground is mandatory |

Use `Serial1` (pins 18/19) for this link — never `Serial0` (pins 0/1), which is shared with USB
upload. The Mega is **not** an ESP‑NOW peer; it only understands the text commands in §6.3.

### 5.5 Health — per‑artifact monitor (`firmware/health/`) ⬜ planned

| Part | ESP32 pin | Notes |
|---|---|---|
| DHT11 data | one GPIO per board | 3.3/5 V per module; 10 kΩ pull‑up |
| MQ‑135 `AO` | GPIO32–39 (ADC1 only — keep WiFi off ADC2 pins) | **Own buck rail** (~150 mA heater, needs 24–48 h burn‑in) |
| Power | Separate buck for heaters | Never share the ESP32 logic rail |

`THIS_STATION_ID` in the firmware selects which artifact this board watches (1=jamdani,
2=nakshi, 3=liberation, 4=terracotta, 5=ektara, 6=palki, 7=tiger — must match the Hub's
`STATION_IDS`).

### 5.6 Emergency — pushbutton node (`firmware/emergency/`) ⬜ planned

| Part | ESP32 pin | Notes |
|---|---|---|
| Big push button | GPIO… (TBD) | To GND, `INPUT_PULLUP`, debounced in software |
| Status LED | GPIO… (TBD) | Armed/active indicator |
| Buzzer (optional) | GPIO… (TBD) | Local sound |

### 5.7 Cam — ESP32‑CAM / GC2145 (`firmware/cam/`) ✅ (capture‑to‑SD only, no network yet)

| Part | Notes |
|---|---|
| Camera (GC2145) | Ribbon connector; **not** a JPEG‑native sensor — this firmware converts RGB565→JPEG in software |
| microSD | Runs in **SD_MMC 1‑bit mode** — only GPIO2 (D0) / GPIO14 (CLK) / GPIO15 (CMD); this deliberately frees GPIO4 (flash LED) and every camera pin |
| Power | Stable 5 V via the CAM‑MB USB‑C, data‑capable cable required |

### 5.8 Track upgrade plan (not yet built — reference only)

[`plans/hub-sensor-wiring.md`](plans/hub-sensor-wiring.md) documents a **future** Hub revision
that folds 56 WS2812B pixels, 4 real DHT11/MQ‑135 pairs (with 3 simulated stations), 4 SG90
servos, a 74HCT125 driver, and a full fused power‑distribution plan directly onto the Track
ESP32 — a denser alternative to running separate `health/` nodes per artifact. It is **not**
the currently wired Hub; read it before changing the Hub's GPIO map.

---

## 6. Communication — how the boards understand each other

There are exactly **four** communication links in this system. Every board uses at most two of
them.

### 6.1 ESP‑NOW bus — the wireless backbone (Hub, Health, Emergency, robot ESP32 once wired)

- **Fixed Wi‑Fi channel 1** on every node — this must match the Hub's SoftAP channel or ESP‑NOW
  silently fails. It is the single most common "worked yesterday, dead today" bug in this
  project.
- No router, no pairing dance, no credentials — each ESP32 sends a small struct to a MAC
  address (or the broadcast address `FF:FF:FF:FF:FF:FF` for emergencies).
- **Packet format** — defined once in
  [`firmware/common/MuseumProtocol.h`](firmware/common/MuseumProtocol.h) and included
  identically by every ESP‑NOW node (`build_flags = -I ../common`). It is a fixed‑width,
  packed, 24‑byte struct (`museum::MuseumMessage`) — no floats, so every board agrees on the
  byte layout regardless of compiler:

  | Field | Type | Meaning |
  |---|---|---|
  | `magic` | uint16 | Always `0x4D53` ("MS") — rejects garbage packets |
  | `version` | uint8 | Protocol version (currently 1) |
  | `messageType` | uint8 | `MSG_LOCATION` `MSG_EMERGENCY` `MSG_HEALTH` `MSG_VISITOR_COUNT` `MSG_IMAGE_READY` `MSG_COMMAND` `MSG_HEARTBEAT` |
  | `nodeId` | uint8 | Which physical node sent it |
  | `stationId` | uint8 | 1..N artifact station (0 = none) |
  | `flags` | uint8 | Bitmask: emergency active / obstacle / alert / device online / robot moving |
  | `tempX10`, `humidX10` | int16 | Temperature/humidity ×10 (26.4 °C is stored as `264`) |
  | `gasRaw` | uint16 | Raw MQ‑135 reading (relative trend, not ppm) |
  | `visitorCount` | uint16 | Running gate count |
  | `sequence` | uint16 | Increments per message |
  | `uptimeMs` | uint32 | Sender's uptime |
  | `checksum` | uint16 | Sum of all preceding bytes — `isValidMessage()` rejects anything that doesn't match |

  **Who sends what:** Robot → `MSG_LOCATION` (current station). Health nodes → `MSG_HEALTH`
  (temp/humidity/gas for their one artifact). Emergency node → `MSG_EMERGENCY` broadcast to
  *everyone*, so the system reacts even if the Hub reboots. Cam → `MSG_IMAGE_READY` after its
  HTTP upload. The Hub aggregates all of it into one shared state (`gState`) that the web app
  reads.

### 6.2 Nano ↔ Robot‑ESP32 UART — RFID station events

A short, checksummed text protocol defined in
[`firmware/common/RobotUartProtocol.h`](firmware/common/RobotUartProtocol.h), running at
**9600 baud, 8N1** over a resistor‑divided hardware UART link (§5.2/§5.3). Every frame is
`$PAYLOAD*HH\n` in spirit — concretely `KIND|field|field|…|CHECKSUM\n`, where `CHECKSUM` is the
two‑digit hex XOR of every preceding byte.

**Station‑stop handshake:**

1. Nano reads a new RC522 tag → **brakes both motors immediately**.
2. Nano sends `RFID|1|<sequence>|<UID_HEX>|<SAK_HEX>|<PICC_TYPE>|<XOR>\n`.
3. ESP32 validates the checksum/fields, replies `ACK|<sequence>\n`.
4. ESP32 does its station work (plays the Bangla/English track, updates the display, and — once
   wired — sends `MSG_LOCATION` over ESP‑NOW), then sends `RESUME|<sequence>\n`.
5. Nano replies `RESUMED|<sequence>\n` and restarts line following.
6. **Fail‑safe:** the Nano auto‑resumes after **15 seconds** even if no `RESUME` ever arrives —
   a dead ESP32 or broken wire cannot strand the robot on the track. Presenting the same card
   again after removing it creates a brand‑new sequence number/event.

The RFID UID/SAK/type are card‑identification metadata for matching a station — **not** a
security credential.

### 6.3 Robot‑ESP32 → Display (Mega) UART — screen commands

Plain text, one command per line, **9600 baud**, case‑insensitive, terminated by `\n`. The Mega
has no idea what ESP‑NOW or Wi‑Fi is — it just draws whatever it's told:

| Command | Effect |
|---|---|
| `FACE` / `FACE HAPPY` / `FACE ALERT` / `FACE SLEEPY` / `FACE NEUTRAL` | Idle animated face with that emotion |
| `LANG 0` / `LANG 1` | Set displayed audio‑language indicator: English / Bangla |
| `MENU` | Show the language‑selection menu |
| `STATION <n> 0` | At station `n`, narrating (not moving) |
| `STATION <n> 1` | Heading to station `n`, moving |
| `EMERGENCY` | Full‑screen red evacuation alert |
| `BOOT` | Replay the splash screen, then return to the face |

Every command also works typed into the Mega's own USB serial monitor, so the display can be
tested with no robot connected at all.

### 6.4 Hub Wi‑Fi + HTTP — the phone/laptop dashboard

The Hub runs a SoftAP named **`MuseumHub`** (password `museum2026`, channel 1) and a web server
at `http://192.168.4.1`. Phones and the admin laptop join it as ordinary Wi‑Fi clients — no
internet, no venue router involved.

**Entry flow:** a QR code at the gate either (a) auto‑joins the phone to `MuseumHub`, or
(b) is scanned in‑page with the offline `jsQR` library, or (c) is a plain URL scanned with the
phone's normal camera that hits `GET /api/enter` directly. All three land the visitor on the
same waiting → admin‑approve → dashboard flow.

**Key endpoints** (full list in [`firmware/hub/README.md`](firmware/hub/README.md)):

| Method + path | Purpose |
|---|---|
| `GET /api/state` | Everything the dashboards poll every ~2 s: robot station/status, emergency flag, per‑station health, visitor count, pending requests, latest photo |
| `POST /api/checkin` / `GET /api/enter` | Visitor requests entry |
| `POST /api/approve` / `POST /api/deny` | Admin decision on a pending visitor |
| `POST /api/emergency?on=1|0` | Toggle emergency (also settable by the physical button via ESP‑NOW) |
| `POST /api/robot/advance` | Test‑only: nudge the robot to the next station with no hardware attached |
| `POST /api/image/upload?station=N` | ESP32‑CAM's photo upload (`multipart/form‑data`) |
| `GET /api/gallery` | Photo list for the admin thumbnail strip |

Both the visitor and admin pages are the **same static files**, served from the Hub's flash
(LittleFS), auto‑refreshing every ~2 s with a plain `fetch()` — no WebSockets, no build step, so
a 10‑year‑old can read and explain the code. During Phase 0 development the *same* `webapp/`
files run standalone against a mock Hub in the browser's `localStorage`
([`webapp/README.md`](webapp/README.md)) — only the data source changes, not the pages.

### 6.5 End‑to‑end example — "robot arrives at Jamdani station"

```
1. Nano reads the RC522 tag → stops motors → RFID|1|3|...*HH  → (UART) → robot ESP32
2. robot ESP32 → ACK|3  → (UART) → Nano
3. robot ESP32 → STATION 1 0  → (UART) → Mega          [display shows "Jamdani, narrating"]
4. robot ESP32 plays track 0001.mp3 (English) or 0008.mp3 (Bangla) via DFPlayer
5. robot ESP32 → MSG_LOCATION{stationId=1} → (ESP-NOW, ch1) → Hub
6. Hub lights WS2812 pixel(s) for station 1, updates gState
7. Meanwhile: Health ESP32 #1 → MSG_HEALTH{stationId=1, temp, humid, gas} → (ESP-NOW) → Hub
8. Visitor's phone (already on MuseumHub) polls GET /api/state → sees "at Jamdani" +
   Jamdani's live temperature/humidity/gas status (green/yellow/red)
9. robot ESP32 → RESUME|3 → (UART) → Nano → Nano resumes line following
```

If the emergency button is pressed at any point, step 5 is replaced by a **broadcast**
`MSG_EMERGENCY` that every node (Hub, robot, health, cam) reacts to independently — the Hub does
not need to be reachable for the evacuation behavior to work.

---

## 7. Power & grounding

- **Separate bucks for logic vs. motors vs. sensor heaters.** ESP32 brownout‑reset when the
  motors kick in is the single most common "worked at home, died at the venue" failure — do not
  share a rail between the L298N motor supply and any microcontroller's logic supply.
- **RC522 and MQ‑135 heaters are the two easy-to-miss voltage traps:** RC522 is strictly 3.3 V
  (never 5 V); MQ‑135 heaters draw ~150 mA each continuously and need their own buck, separate
  from ESP32 logic.
- **One common (star) ground.** Every board's GND — Nano, every ESP32, the Mega, L298N, RC522,
  DFPlayer, sensors — must join at one point. High‑current motor return should go directly to
  the distribution point, not daisy‑chained through logic‑board grounds.
- **LiPo safety:** 3S LiPo, adult‑owned charging routine (LiPo bag, never unattended), main
  power switch + inline fuse on the robot.

Full budgets, fuse sizing, and bring‑up/acceptance sequences are in
[`plans/robot-wiring.md`](plans/robot-wiring.md),
[`plans/robot-nano-esp-wiring.md`](plans/robot-nano-esp-wiring.md), and
[`plans/hub-sensor-wiring.md`](plans/hub-sensor-wiring.md).

---

## 8. Build & flash each node

All ESP32/Nano firmware is **PlatformIO**; the display is **Arduino IDE or PlatformIO** for the
Mega. From each node's folder:

```bash
cd firmware/<node>          # hub | robot-nano | robot-esp | display | health | emergency | cam | sdcard
pio run -t upload           # flash
pio device monitor          # serial monitor (baud rate noted in that node's own README)
```

The Nano has two PlatformIO environments in [`firmware/robot-nano/platformio.ini`](firmware/robot-nano/platformio.ini) —
use `nano_new` (default) or `nano_old` if a CH340 clone only accepts the "old bootloader" option.

The Hub additionally needs its web app copied into `firmware/hub/data/` and uploaded with
`pio run -t uploadfs` — see [`firmware/hub/README.md`](firmware/hub/README.md).

**Rule that bites everyone once:** ESP‑NOW and the Hub's SoftAP must share **channel 1**. It's
set per‑node in each `platformio.ini`/`config.h` — if you change it, change it on every board.

---

## 9. Project status

| Node | Built? | What works today |
|---|---|---|
| Hub (Track ESP32) | ✅ | SoftAP, web server, `/api`, WS2812 station LEDs, buzzer, optional microSD photo gallery |
| Robot — Nano (motion + RFID) | ✅ | Line following, RC522 station stops, UART handshake to the ESP32 |
| Robot — ESP32 (audio bridge) | ✅ | Receives RFID events from the Nano, plays Bangla/English DFPlayer tracks. **Not yet wired:** ESP‑NOW `MSG_LOCATION` broadcast to the Hub, WS2812 arrow, display UART |
| Display (Mega + TFT) | ✅ | Full command set (§6.3), testable standalone over USB |
| Health (per‑artifact) | ⬜ skeleton | Pin plan documented; firmware not yet implemented (Phase 6) |
| Emergency (pushbutton) | ⬜ skeleton | Pin plan documented; firmware not yet implemented (Phase 5) |
| Cam (ESP32‑CAM) | ✅ offline | Capture‑to‑own‑SD works; Hub upload (`/api/image/upload`) is a documented, not‑yet‑wired seam |
| Web app | ✅ (mock data) | Full visitor/admin flow against `localStorage`; switches to live Hub data with no UI changes when Phase 6 lands |
| Legacy `firmware/robot/` (single‑ESP32) | ⬜ superseded | Kept for reference only — replaced by the Nano + ESP32 split above |

See [`BUILD_PLAN.md §6`](BUILD_PLAN.md#6-build-order-robot-first-esp-now-folds-in-at-the-track-phase)
for the full phase‑by‑phase build order and exit criteria, and
[`progress.md`](progress.md) / [`findings.md`](findings.md) for the dated engineering log.

---

## 10. Further reading

| Document | What's in it |
|---|---|
| [`BUILD_PLAN.md`](BUILD_PLAN.md) | Canonical scope, budget, architecture rationale, build phases, demo‑day risk list |
| [`CLAUDE_RECOMMENDATIONS.md`](CLAUDE_RECOMMENDATIONS.md) | Decision log — what was chosen/rejected and why |
| [`firmware/README.md`](firmware/README.md) | One‑paragraph overview of every firmware project |
| `firmware/<node>/README.md` | Exact pinout, commands, and status for that one board |
| `plans/*.md` | Full electrical engineering detail: voltage math, current budgets, bring‑up sequences |
| [`webapp/README.md`](webapp/README.md) | How to run/test the dashboard, and how it becomes "real" in Phase 6 |
| [`tools/dfplayer_audio/README.md`](tools/dfplayer_audio/README.md) | The placeholder audio track set and how to regenerate/replace it |
| [`findings.md`](findings.md) / [`progress.md`](progress.md) | Dated engineering audit trail |
