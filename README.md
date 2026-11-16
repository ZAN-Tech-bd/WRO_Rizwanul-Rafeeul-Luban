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


<p align="center"><img src="media/project_photos/00_final_model_overview.png" width="860"><br>
<sub>The finished model: guide robot, black Museum Hub, orange artifact stations, track with START and END marks.</sub></p>

<p align="center"><a href="https://www.youtube.com/watch?v=wlzYdRGdfmo"><img src="https://img.youtube.com/vi/wlzYdRGdfmo/maxresdefault.jpg" width="640" alt="Watch the project video on YouTube"></a><br>
<sub>▶ <a href="https://www.youtube.com/watch?v=wlzYdRGdfmo">Watch the project video on YouTube</a></sub></p>

---

## 1. The team

| Member | School | Role |
|---|---|---|
| **Syed Rizwanul Alam** | South Breeze School | Software & Programming |
| **Syed Rafeeul Alam** | South Breeze School | 3D Modeling & Mechanical |
| **Tareef Toufique** | South Breeze School | Hardware & Mechanical |

Team name: **Quantrom Physics** · Category: Future Innovators, Elementary (ages 8–12).
The full project report is in [`docs/report/`](docs/report/).

<p align="center"><img src="media/project_photos/24_team_portrait.jpg" width="640"><br><sub>Team Quantrom Physics at WRO Bangladesh 2026 (National) with the robot, the Hub and the artifact stations.</sub></p>

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
| Electronics | Custom team-designed PCBs: Hub, robot and station boards (section 8) |
| Structure | 3D-printed robot, hub, stations and set pieces, plus a bordered plywood track (sections 7 and 9) |

Per-board pin tables are in each firmware README. CAD/3D files are in [`hardware/`](hardware/).

---

## 7. Mechanical design and 3D-printed parts

Every body part was designed in **Autodesk Fusion 360** and printed on a 3D printer, then fitted
with off-the-shelf motors, wheels and boards. The meshes are in
[`hardware/3d_parts/`](hardware/3d_parts/). Sizes below were measured from the STL files (X × Y × Z, mm).

<table>
<tr><td align="center" valign="top"><img src="media/project_photos/01_robot_cad_fusion360.jpg" width="420"><br><sub>Robot in Fusion 360: head with face window, body, arms, motor base with wheels</sub></td><td align="center" valign="top"><img src="media/project_photos/02_robot_base_cad_underside.jpg" width="420"><br><sub>Underside of the base: motor mounts, caster holder, board mounts</sub></td></tr>
<tr><td align="center" valign="top"><img src="media/project_photos/03_motor_driver_holder_cad.jpg" width="420"><br><sub>Motor-driver (L298N) holder legs on the base</sub></td><td align="center" valign="top"><img src="media/project_photos/04_printed_base_vs_cad.jpg" width="420"><br><sub>The printed base next to its CAD model</sub></td></tr>
<tr><td align="center" valign="top"><img src="media/project_photos/16_robot_chassis_wiring_and_head.jpg" width="420"><br><sub>Printed chassis with wiring, and the orange head with the TFT face</sub></td><td align="center" valign="top"><img src="media/project_photos/26_robot_closeup_wheels.jpg" width="420"><br><sub>Finished robot: white body, orange arms and head, blue wheels</sub></td></tr>
</table>

### Robot

| Part (file) | Size (mm) | What it does |
|---|---|---|
| `Robot/Base/Base.stl` | 180 × 120 × 64 | Main chassis: holds the two motors and wheels |
| `Robot/Base/l298 holder legs.stl` | 53 × 53 × 24 | Legs that lift the L298N motor driver above the chassis |
| `Robot/Base/Buck and battery holder.stl` | 105 × 55 × 60 | Holds the buck converters and the LiPo battery |
| `Robot/Base/caster wheel holder.stl` | 68 × 78 × 14 | Mount for the lower ball caster |
| `Robot/Base/Slider clamp.stl` | 55 × 45 × 6 | Clamp for the side guide casters |
| `Robot/Base/Back cover v2.stl` | 22 × 72 × 42 | Rear access cover |
| `Robot/Body/Body V2.stl` | 180 × 127 × 151 | Upper body: speaker, RFID reader, camera and electronics |
| `Robot/Hand/Hands.stl` | 210 × 96 × 96 | The two arms |
| `Robot/Head/Head Front draft.stl`, `Head Back dreft.stl` | 160 × 83 × 116, 160 × 66 × 113 | Two-piece head with a window for the 3.2" TFT face |

### Museum Hub, stations and set pieces

<table>
<tr><td align="center" valign="top"><img src="media/project_photos/06_museum_hub_printed.jpg" width="420"><br><sub>Printed hexagonal Museum Hub: antenna, emergency button, power switch</sub></td><td align="center" valign="top"><img src="media/project_photos/07_exhibit_station_box.jpg" width="420"><br><sub>Printed artifact-station box (orange)</sub></td></tr>
<tr><td align="center" valign="top"><img src="media/project_photos/08_exhibit_station_sensors_inside.jpg" width="420"><br><sub>Inside a station box: IR sensor, MQ-135 gas sensor, buzzer circuit</sub></td><td align="center" valign="top"><img src="media/project_photos/28_station_box_assembly.jpg" width="420"><br><sub>Fitting the sensor into a station box</sub></td></tr>
</table>

| Part (file) | Size (mm) | What it does |
|---|---|---|
| `Hub/Hub Base.stl`, `Hub Top.stl` | 165 × 187 × 80, 165 × 187 × 98 | Hexagonal Hub enclosure (PCB, antenna, emergency button, siren) |
| `Arteffecet/Art effect v2 base.stl` | 70 × 70 × 60 | Artifact-station box: sensors and LED stick inside, the exhibit on top |
| `Arteffecet/Art effect v2 dhakna.stl` | 70 × 70 × 6 | Lid (*dhakna*) for the station box |
| `Exit+sign+RIGHT+A1+Mini_stls/obj_1_Exit sign.svg.stl` | 176 × 91 × 3 | Emergency exit sign |
| `Japanese+Torii+Gate/torii.stl` | as exported | Torii gate marking the START area |
| `QR Stand/QR_Holder v1.3mf` | | Stand that holds the gate QR code |

The robot is white (body) and orange (head, arms); stations are orange or white; the Hub is black.

---

## 8. Custom electronics (PCBs)

The team designed its own boards so the wiring is neat and repeatable, with the team logo printed on them.

<table>
<tr><td align="center" valign="top"><img src="media/project_photos/11_hub_pcb_v04_bare.jpg" width="420"><br><sub>Museum Hub PCB v0.4: ESP32-S3, 8 sensor ports, buzzer, servos, SD, E-stop</sub></td><td align="center" valign="top"><img src="media/project_photos/12_robot_pcb_populated.jpg" width="420"><br><sub>Robot PCB: ESP32-S3, DFPlayer Mini, two amplifiers, XT60 power</sub></td></tr>
<tr><td align="center" valign="top"><img src="media/project_photos/13_station_pcbs_assembled.jpg" width="420"><br><sub>Three station PCBs v0.4: IR sensor, buzzer driver, DHT11, LED stick</sub></td><td align="center" valign="top"><img src="media/project_photos/14_station_pcb_assembled_vs_bare.jpg" width="420"><br><sub>Station PCB: assembled and bare</sub></td></tr>
<tr><td align="center" valign="top"><img src="media/project_photos/09_custom_pcbs.jpg" width="420"><br><sub>A batch of robot, hub and station boards</sub></td><td align="center" valign="top"><img src="media/project_photos/17_robot_electronics_bench_test.jpg" width="420"><br><sub>Bench test: robot PCB powered from the LiPo</sub></td></tr>
</table>

- **Hub PCB v0.4:** ESP32 module, eight sensor ports (5 V, GND, 3.3 V, DHT, MQ, proximity, D-in, D-out), siren buzzer,
  servo headers, SD card socket, emergency-stop input and a 12 V input with battery/adapter switch.
- **Robot PCB:** ESP32 module, DFPlayer Mini, two amplifier modules, RFID reader port, mute and spare buttons, XT60 power inputs, status LEDs.
- **Station PCB v0.4:** one per artifact. It has a cable to the Hub, a DHT11 port, an IR proximity port, an LED-stick port,
  and a buzzer driven by a PNP transistor with a diode and two resistors.

> Note: the PCBs in the photos carry an ESP32-S3 module, while the firmware READMEs describe the ESP32 DevKit pins.
> Check the pin tables in the firmware before moving a build onto the boards.

---

## 9. The track

The robot runs on a bordered oval track cut from plywood. The raised wooden border keeps the robot
aligned and gives a repeatable route. The drawings are in [`hardware/track_road/`](hardware/track_road/).

<table>
<tr><td align="center" valign="top"><img src="media/project_photos/05_plywood_track_pieces.jpg" width="420"><br><sub>Early track pieces and border strips</sub></td><td align="center" valign="top"><img src="media/project_photos/18_track_oval_finished.jpg" width="420"><br><sub>The finished oval track</sub></td></tr>
<tr><td align="center" valign="top"><img src="media/project_photos/19_museum_model_setup.jpg" width="420"><br><sub>The model set up: track, Hub, stations, robot</sub></td></tr>
</table>

---

## 10. How we built it (process)

<table>
<tr><td align="center" valign="top"><img src="media/project_photos/23_team_building_together.jpg" width="420"><br><sub>Building and testing together</sub></td><td align="center" valign="top"><img src="media/project_photos/20_building_station_electronics.jpg" width="420"><br><sub>Wiring a station sensor</sub></td></tr>
<tr><td align="center" valign="top"><img src="media/project_photos/21_testing_pcb_with_multimeter.jpg" width="420"><br><sub>Checking a PCB with a multimeter</sub></td><td align="center" valign="top"><img src="media/project_photos/22_assembling_station_box.jpg" width="420"><br><sub>Assembling a station box</sub></td></tr>
<tr><td align="center" valign="top"><img src="media/project_photos/29_sensor_wiring_station.jpg" width="420"><br><sub>Explaining the station sensor wiring (the notebook lists the RFID card codes)</sub></td><td align="center" valign="top"><img src="media/project_photos/27_holding_robot_face_display.jpg" width="420"><br><sub>Testing the robot face display</sub></td></tr>
<tr><td align="center" valign="top"><img src="media/project_photos/25_team_robot_demo.jpg" width="420"><br><sub>Checking the robot underside: motors, wheels and caster</sub></td><td align="center" valign="top"><img src="media/project_photos/15_robot_pcb_top_view.jpg" width="420"><br><sub>Assembled robot PCB</sub></td></tr>
<tr><td align="center" valign="top"><img src="media/project_photos/32_programming_hub_on_laptop.jpg" width="420"><br><sub>Programming the Hub on a laptop</sub></td><td align="center" valign="top"><img src="media/project_photos/34_pcb_continuity_test_closeup.jpg" width="420"><br><sub>Testing a bare PCB for shorts</sub></td></tr>
<tr><td align="center" valign="top"><img src="media/project_photos/36_team_working_robot_face_on.jpg" width="420"><br><sub>Three teammates working at once</sub></td><td align="center" valign="top"><img src="media/project_photos/40_robot_with_basket_on_head.jpg" width="420"><br><sub>The robot with a bamboo basket artifact on its head</sub></td></tr>
<tr><td align="center" valign="top"><img src="media/project_photos/30_robot_head_face_display_on_bench.jpg" width="420"><br><sub>Head with the animated face on the bench</sub></td><td align="center" valign="top"><img src="media/project_photos/31_station_sensor_modules_held.jpg" width="420"><br><sub>Station sensors: gas, IR and DHT11 modules</sub></td></tr>
</table>

Steps we followed: plan the features → design the parts in Fusion 360 → 3D print → design and order the PCBs →
solder and test each board on its own → assemble the robot, Hub and stations → build the track → write and test
the firmware node by node → run the whole tour again and again. More build photos are in
[`media/project_photos/`](media/project_photos/).

---

## 11. Diagrams

Each diagram has an editable `.drawio` file next to the PNG in [`docs/diagrams/`](docs/diagrams/README.md).

<table>
<tr><td align="center" valign="top"><img src="docs/diagrams/01_working_combo_system_context.png" width="460"><br><sub>1. System context: every board and link</sub></td><td align="center" valign="top"><img src="docs/diagrams/02_board_wiring_map.png" width="460"><br><sub>2. Board wiring map</sub></td></tr>
<tr><td align="center" valign="top"><img src="docs/diagrams/03_radio_and_networks.png" width="460"><br><sub>3. Radio and Wi-Fi networks</sub></td><td align="center" valign="top"><img src="docs/diagrams/04_one_station_stop.png" width="460"><br><sub>4. One station stop</sub></td></tr>
<tr><td align="center" valign="top"><img src="docs/diagrams/05_emergency_path.png" width="460"><br><sub>5. Emergency path</sub></td><td align="center" valign="top"><img src="docs/diagrams/06_robot_states.png" width="460"><br><sub>6. Robot states</sub></td></tr>
<tr><td align="center" valign="top"><img src="docs/diagrams/07_photo_pipeline.png" width="460"><br><sub>7. Photo pipeline</sub></td><td align="center" valign="top"><img src="docs/diagrams/08_hub_two_jobs.png" width="460"><br><sub>8. Hub: two jobs</sub></td></tr>
<tr><td align="center" valign="top"><img src="docs/diagrams/09_message_reference.png" width="460"><br><sub>9. Message reference</sub></td></tr>
</table>

---

## 12. Repository layout

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
│   ├── 3d_parts/                ← STL/3MF: robot, hub, station box, QR stand, exit sign, torii gate
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

## 13. Build and run

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

## 14. Current status and limitations

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

## 15. Credits and licences

- Built by the team above. Mechanical design, wiring, coding and testing were done by the team.
- ChatGPT was used as an assistant for brainstorming, explanations, debugging discussion and report
  wording. See the report's "Sources and Use of AI" section. Claude Code was used for the code
  documentation and for tidying this repository.
- Third-party libraries (DFRobotDFPlayerMini, EspSoftwareSerial, MFRC522v2, jsQR) keep their own
  licences. The face sketch's licence and provenance are in [`firmware/display-standalone/`](firmware/display-standalone/).
- WRO rulebooks in [`docs/references/`](docs/references/) belong to the World Robot Olympiad Association.
