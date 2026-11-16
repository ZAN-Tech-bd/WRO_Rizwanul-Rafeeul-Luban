# Smart Museum Guidance System

**Guiding Visitors. Protecting Heritage.**

> WRO Bangladesh 2026 · Future Innovators · Elementary · Theme **"Robots Meet Culture"**
> Team **Quantrom Physics**

A small autonomous guide robot leads visitors around a model Bangladeshi museum. It stops at
each artifact, tells its story in **English and Bangla**, and takes a photo. Meanwhile a central
**Museum Hub** lets visitors request entry with a QR code, lets staff approve them, shows where the
robot is, watches temperature / humidity / gas at the exhibits, and runs an **emergency mode**.
Everything works **offline**. There is no internet and no venue Wi-Fi: the Hub creates its own
Wi-Fi network.

Theme areas targeted: **Area 1 – Protecting & sharing cultural heritage** (artifact monitoring)
and **Area 3 – Experiencing art & history with robots** (the guided tour).

---

## 1. The team

| Member | School | Role |
|---|---|---|
| **Syed Rizwanul Alam** | South Breeze School | Software & Programming |
| **Syed Rafeeul Alam** | South Breeze School | 3D Modeling & Mechanical |
| **Tareef Toufique** | South Breeze School | Hardware & Mechanical |

Team name: **Quantrom Physics** · Category: Future Innovators, Elementary (ages 8–12).
The full project report is in [`docs/report/`](docs/report/).

---

## 2. The problem and our answer

| Problem | Our approach |
|---|---|
| Visitors need guidance and information | Guide robot + visitor web app |
| Museum capacity needs control | QR entry request + admin approval / waiting queue |
| Visitors get separated from the group | The app shows the robot's current station |
| Artifacts need environmental monitoring | DHT11 (temperature/humidity) + gas sensors at the exhibits |
| Visitors get too close to displays | IR proximity node + warning buzzer |
| The robot needs reliable locations | RFID tag at every station |
| The robot needs a repeatable route | Bordered track + three low-friction ball casters |
| An emergency must take priority | Wireless emergency signal + siren + exit behavior |

---

## 3. Features

**For visitors**
- Scan a QR code at the gate, request entry, wait for approval.
- Live dashboard: which station the robot is at, artifact info, emergency banner.
- Bangla / English interface.

**For museum staff (admin dashboard)**
- Approve or deny entry requests and see the visitor count.
- Robot location and status (moving / narrating).
- Live temperature, humidity and gas readings per artifact, with green / yellow / red status.
- Photo gallery from the robot's camera at each station.
- Emergency on/off and a **Mute robot** button.

**The guide robot**
- Drives along the track and **stops at each of 7 artifact stations** (RFID card detected).
- Plays one **bilingual narration** (English + Bangla) per station through a DFPlayer Mini.
- Triggers the ESP32-CAM about 2 s after stopping and uploads the photo to the Hub.
- Reports its location to the Hub over ESP-NOW.
- Has a separate animated **robot-face display** (Arduino Mega + 3.2" TFT).

**Safety**
- **Emergency mode** outranks everything: narration stops, pending photos are cancelled, the robot
  skips all stations and drives to the **END card**, stops and stays latched. Track LEDs flash red
  and a siren sounds. Triggered by the Hub's big button or the admin dashboard; cleared by holding
  the button for 3 s or from the dashboard. After an emergency the robot only restarts on a fresh
  **START card**.
- **Proximity nodes** (IR sensor + PNP transistor + buzzer) warn when someone gets too close.
- **Fail-safes:** the Hub re-broadcasts the emergency every 2 s, so a robot that reboots or misses a
  packet re-syncs. A narration time-out stops the robot from waiting forever.

**Extra prototype:** a standalone LEGO-based **AI staff-authentication** module (camera, buttons,
red/green LEDs) that demonstrates staff access control.

---

## 4. The seven stations

Jamdani Saree · Nakshi Kantha · Terracotta Plaque · Traditional Pottery · Traditional Wooden Boat ·
Traditional Metal Chair · Bamboo & Cane Basket

The tour is: **START card → welcome track → 7 stations → END card.** The recorded narration voices
are in [`assets/narration_voice/`](assets/narration_voice/).

---

## 5. How it works

```
                 ESP-NOW radio, Wi-Fi channel 1 (no router needed)
        ┌───────────────────────┬─────────────────────────────┐
  [Robot ESP32]  ◄── UART ──►  [Arduino Nano]         [ESP32-CAM]
  RFID RC522                    L298N motor driver      photo → SD
  DFPlayer + speaker            START / STOP            + upload
        │ ESP-NOW: location                                  │ Wi-Fi (HTTP)
        ▼                                                    ▼
                     [ MUSEUM HUB  (ESP32) ]
        SoftAP "MuseumHub" · web server · station LEDs · siren
        emergency button · DHT11 / gas sensors · photo gallery (SD)
                              │ Wi-Fi
              ┌───────────────┴───────────────┐
        Visitor phone                    Admin laptop
        http://192.168.4.1               http://192.168.4.1/admin.html

  [Arduino Mega + 3.2" TFT]  → standalone animated robot face
```

**One station stop, step by step**
1. The robot reads a station RFID card and the ESP32 tells the Nano `STOP` (active hard brake).
2. The ESP32 reports the station to the Hub, which lights that station's LED.
3. The matching bilingual narration plays.
4. About 2 s in, the ESP32 sends `SHOOT` to the camera, which uploads a photo to the Hub gallery.
5. When the audio ends, the ESP32 sends `START` and the robot moves on.

**Design rules**
- **Only the Hub hosts Wi-Fi and web pages.** Phones never talk to the robot or sensors directly.
- **ESP-NOW and the Hub must all use Wi-Fi channel 1.** If one board is changed, the link goes silent
  with no error.
- **Motors and logic have separate power rails** to avoid ESP32 brownout resets, with a common ground.

Diagrams (editable `.drawio` plus PNG) are in [`docs/diagrams/`](docs/diagrams/README.md):
system context, board wiring, radio networks, station stop, emergency path, robot states, photo
pipeline, hub design and message reference.

---

## 6. Hardware

| Subsystem | Parts |
|---|---|
| Robot brain | ESP32, MFRC522 (RC522) RFID reader, DFPlayer Mini + microSD + speaker |
| Robot drive | Arduino Nano, L298N driver, 2 DC motors and wheels (calibrated PWM L=150 / R=210), 1 lower + 2 side ball casters |
| Camera | ESP32-CAM (GC2145 sensor) on a CAM-MB programmer, microSD |
| Display | Arduino Mega 2560 + 3.2" 480×320 TFT shield |
| Museum Hub | ESP32, WS2812 LED strip (7 boards × 8 pixels), siren buzzer, emergency push button, microSD |
| Exhibit sensors | DHT11 (temperature / humidity), MQ-135 gas sensor, IR proximity sensor, PNP transistor, buzzer |
| Staff security | LEGO-based AI camera module (standalone) |
| Power | Buck converters (separate logic and motor rails), switch, fuse, common ground |
| Structure | 3D-printed robot, hub, stands and parts, plus a bordered track |

Per-board pin tables are in each firmware README. CAD/3D files are in [`hardware/`](hardware/).

---

## 7. Repository layout

```
.
├── README.md                    ← you are here
├── firmware/                    ← all code that runs on a microcontroller
│   ├── hub/                     ← Museum Hub (ESP32): Wi-Fi AP, web server, LEDs, siren, sensors
│   │   └── data/                ← web app files uploaded to the Hub's flash
│   ├── robot-esp32-rfid-mp3/    ← robot brain (ESP32): RFID, audio, camera trigger, emergency
│   ├── robot-nano-rail/         ← robot motor driver (Arduino Nano)
│   ├── cam/                     ← ESP32-CAM photo node
│   ├── display/                 ← robot face on Arduino Mega + TFT
│   ├── display-standalone/      ← the face sketch as a separate mini-repo (licence + credits)
│   └── common/                  ← shared ESP-NOW packet + protocol headers
├── webapp/                      ← visitor + admin web app (HTML/CSS/JS), mock-hub for PC testing
├── assets/
│   ├── narration_voice/         ← recorded narration MP3s (7 stations + welcome, emergency, obstacle)
│   └── dfplayer_audio/          ← numbered SD-card track set, track map and generator script
├── hardware/
│   ├── 3d_parts/                ← STL/3MF: robot, hub, QR stand, artifact stand, exit sign, torii gate
│   └── track_road/              ← the museum track/road drawings (AI, DWG, PDF, PNG)
├── tools/                       ← generate_qr.py (gate QR codes)
├── media/project_photos/        ← photos taken during the build
└── docs/
    ├── report/                  ← the full project report (DOCX + PDF)
    ├── diagrams/                ← system, wiring, state and message diagrams
    ├── plans/                   ← detailed wiring / electrical plans
    ├── planning/                ← build plan, decisions, pseudocode, dev log (findings, progress)
    ├── references/              ← WRO rulebooks and the ESP32-CAM datasheet
    └── notebooklm_sources/      ← architecture overview used for AI-assisted summaries
```

**Read in this order:** this README → [`docs/report/`](docs/report/) → [`docs/diagrams/`](docs/diagrams/README.md)
→ [`firmware/README.md`](firmware/README.md) → each `firmware/<node>/README.md`.

---

## 8. Build and run

**Try the web app on a PC (no hardware needed)**
```bash
cd webapp
python -m http.server 8000
# Admin:   http://localhost:8000/admin.html
# Visitor: http://localhost:8000/index.html
```

**Flash the firmware** (PlatformIO; the Nano, Hub and ESP32 projects each have a `platformio.ini`)
```bash
cd firmware/hub              # or robot-esp32-rfid-mp3, robot-nano-rail, cam, display
pio run -t upload            # flash
pio run -t uploadfs          # hub only: upload the web app in data/
pio device monitor
```
The robot ESP32 sketch can also be built in the Arduino IDE; see
[`firmware/robot-esp32-rfid-mp3/INSTALL_ARDUINO_IDE.txt`](firmware/robot-esp32-rfid-mp3/INSTALL_ARDUINO_IDE.txt).

**Audio SD card:** copy the files from [`assets/dfplayer_audio/mp3/`](assets/dfplayer_audio/) to the
DFPlayer's microSD (FAT32). The track map is in `assets/dfplayer_audio/TRACK_MAP.csv`.

**Gate QR codes:**
```bash
pip install -r tools/requirements.txt
python tools/generate_qr.py
```

**On demo day**
1. Power the Hub. Phones join Wi-Fi **`MuseumHub`** (password in
   [`firmware/hub/include/config.h`](firmware/hub/include/config.h)), then open `http://192.168.4.1`.
2. Power the robot. Tap the **START card** to begin the tour.
3. Open the admin page to approve visitors, watch sensors and trigger or clear an emergency.

Notes:
- The Nano's UART pins D0/D1 are shared with USB, so **disconnect the two ESP32 wires** before
  flashing it.
- The robot ESP32 also hosts a small field-tuning page (volume, motor trim, event log) on its own
  Wi-Fi network. Use it for bench work, not as the demo path.

---

## 9. Current status and limitations

| Area | Status |
|---|---|
| Robot tour (RFID, audio, motors, camera trigger) | ✅ working |
| Hub (Wi-Fi AP, dashboard, LEDs, siren, emergency, gallery) | ✅ working |
| Display (animated robot face) | ✅ working |
| Visitor and admin web app | ✅ working |
| Environmental sensors | ✅ wired to the Hub; readings need calibration |
| LEGO AI staff-authentication | ✅ standalone prototype |

**Known limitations and future work:** the robot needs a prepared track; the emergency route goes to
one fixed END point; sensor thresholds need calibration; the camera records photos but does not yet
detect damage automatically; Wi-Fi loss limits live information. Planned improvements are image
comparison, calibrated sensors, more languages, a visitor map and smarter navigation.

---

## 10. Credits and licences

- Built by the team above. Mechanical design, wiring, coding and testing were done by the team.
- ChatGPT was used as an assistant for brainstorming, explanations, debugging discussion and report
  wording. See the report's "Sources and Use of AI" section. Claude Code was used for the code
  documentation and for tidying this repository.
- Third-party libraries (DFRobotDFPlayerMini, EspSoftwareSerial, MFRC522v2, jsQR) keep their own
  licences. The face sketch's licence and provenance are in [`firmware/display-standalone/`](firmware/display-standalone/).
- WRO rulebooks in [`docs/references/`](docs/references/) belong to the World Robot Olympiad Association.
