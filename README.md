# Smart Museum Guidance System — Arduino firmware

> **WRO 2026 Future Innovators (Elementary)** · South Breeze School, Bangladesh
> Theme: *Robots Meet Culture* · Slogan: **Guiding Visitors. Protecting Heritage.**

This `arduino` branch holds the **Arduino-IDE-ready sketches** for every microcontroller in the robot + museum system. Each folder below is a self-contained sketch — the `.ino` name matches the folder, so you can open it directly in the Arduino IDE. The original PlatformIO source stays on `main` under `firmware/`.

### Team
| Member | Role |
|---|---|
| Syed Rizwanul Alam | Software & Programming |
| Syed Rafeeul Alam | 3D Modeling & Mechanical |
| Tareef Toufique | Hardware & Mechanical |

## The idea in one paragraph
A small autonomous guide robot travels a prepared, bordered track through a model museum of seven Bangladeshi heritage stations. Visitors scan a QR code and request entry; staff approve from an admin dashboard. At each station the robot stops, plays a bilingual (English + Bangla) narration, and the camera takes a photo. A **Museum Hub** tracks the robot's location, shows environmental readings (temperature, humidity, gas), stores the photo gallery, and — most importantly — owns the **emergency** state: when raised, the normal tour is interrupted and the robot drives straight to the exit.

## The five sketches (this branch)
| Sketch folder | Board | Role | Sketch tabs |
|---|---|---|---|
| [`ESP32_Robot_Main/`](ESP32_Robot_Main/ESP32_Robot_Main.ino) | ESP32 Dev Module | Robot brain: RFID → narration → Nano/Camera control → Hub location; listens for Hub emergency & mute | `ESP32_Robot_Main.ino`, `ControlPage.h`, `MuseumProtocol.h`, `EspNowCompat.h` |
| [`Nano_Robot_Driver/`](Nano_Robot_Driver/Nano_Robot_Driver.ino) | Arduino Nano (ATmega328P) | L298N motor driver: `START` drive / `STOP` hard-brake + PWM tuning | `Nano_Robot_Driver.ino` |
| [`ESP32_CAM/`](ESP32_CAM/ESP32_CAM.ino) | AI Thinker ESP32-CAM (GC2145) | Captures a station photo, JPEG-encodes it, uploads to the Hub | `ESP32_CAM.ino`, `config.h`, `MuseumProtocol.h` |
| [`ESP32_HUB/`](ESP32_HUB/ESP32_HUB.ino) | ESP32 Dev Module | Museum Hub: SoftAP `MuseumHub`, visitor/admin web, ESP-NOW, NeoPixel station LEDs, siren, DHT11/MQ sensors, photo gallery | `ESP32_HUB.ino`, `config.h`, `MuseumState.h`, `PhotoStore.h`/`.cpp`, `HubSensors.h`/`.cpp`, `MuseumProtocol.h` |
| [`MEGA_DISPLAY/`](MEGA_DISPLAY/MEGA_DISPLAY.ino) | Arduino Mega 2560 + 3.2" TFT | Standalone animated robot face + station / menu / emergency screens | `MEGA_DISPLAY.ino`, `display_config.h`, `robot_face.h` |

## How the boards talk to each other
- **Robot ↔ Nano** — UART @9600: Robot sends `START` / `STOP` / `SET ML n` / `SET MR n` / `GET` / `SAVE`; Nano replies `STATE:RUNNING` / `STATE:STOPPED` / `TUNE ML=n MR=n`.
- **Robot → Camera** — one-way UART (GPIO14 → CAM GPIO13, 9600): sends `SHOOT <station>` ~2 s after stopping at a station.
- **Robot → Display** — UART text commands: `FACE`, `LANG`, `STATION n m`, `MENU`, `EMERGENCY`.
- **Camera → Hub** — joins Hub Wi-Fi and `POST /api/image/upload` (multipart), plus a 1-packet ESP-NOW "image ready" ping.
- **Hub ↔ Robot** — ESP-NOW on **Wi-Fi channel 1**: Hub broadcasts emergency + mute; Robot broadcasts its station / movement.

> All ESP-NOW boards must stay on **channel 1**. A channel mismatch fails silently.

## Normal tour (RFID sequence)
1. **START** card → welcome track plays while stopped, then the Robot drives.
2. **Station** card → stop, update Hub location, play that station's bilingual narration.
3. ~2 s after stopping → `SHOOT` to the camera (while still narrating).
4. Narration ends → drive resumes to the next station.
5. **END** card → hard-stop and latch; only the START card restarts the tour.

## Emergency (highest priority)
The Hub is the single source of truth. When emergency is raised the Robot cuts audio, drops any pending photo, and **drives past every station to the END card**, ignoring all cards except END. When cleared, it stays latched stopped until a fresh START card. (Mute is separate — it just sets narration volume to 0 while the tour otherwise continues on the same timing.)

## Build & flash (Arduino IDE)
Open each sketch folder, pick the board, install the libraries, Upload.

| Sketch | Board setting | Libraries (Library Manager) | Notes |
|---|---|---|---|
| `ESP32_Robot_Main` | ESP32 Dev Module | `MFRC522v2` (Arduino_MFRC522v2), `DFRobotDFPlayerMini`, `EspSoftwareSerial` | SoftAP `ESP32-Robot-Main` / `museum2026`; field console at `http://192.168.4.1/` |
| `Nano_Robot_Driver` | Arduino Nano · ATmega328P (new bootloader) | — (EEPROM is built in) | 9600 baud; default ML=150, MR=210 |
| `ESP32_CAM` | AI Thinker ESP32-CAM | ESP32 core: `esp_camera`, `SD_MMC`, `img_converters` | Tools: PSRAM **Enabled**, Partition **Huge APP**, Upload **115200** |
| `ESP32_HUB` | ESP32 Dev Module | `Adafruit NeoPixel` (WiFi/WebServer/DNSServer/LittleFS/esp_now are in the core) | SoftAP `MuseumHub` / `museum2026`; upload `firmware/hub/data/` to LittleFS for the full dashboard |
| `MEGA_DISPLAY` | Arduino Mega 2560 | `MCUFRIEND_kbv`, `Adafruit GFX Library` | 3.2"/3.5" MCUFRIEND ILI9481 shield; the MCUFRIEND `-D` flags are already `#define`d in the `.ino` |

## Board-to-board wiring (quick reference)
Full pin lists are in the comment block at the top of each `.ino` / `config.h`.

| Link | From → To | Notes |
|---|---|---|
| Robot ↔ Nano | ESP32 GPIO26 (TX) → Nano D0 (RX); Nano D1 (TX) → ESP32 GPIO25 (RX) via 10k/20k divider | common GND; unplug while flashing the Nano |
| Robot → Camera | ESP32 GPIO14 (TX) → CAM GPIO13 (RX) | 3.3 V both sides, no divider; unplug while flashing the CAM |
| Robot → Display | ESP32 GPIO17 (TX) → Mega RX1 (pin 19) via 1k | common GND |
| Camera → Hub | over Wi-Fi `MuseumHub` | HTTP POST + ESP-NOW ping |
| Hub ↔ Robot | ESP-NOW (no wire) | channel 1 on both |

## Stations (seven)
Jamdani Saree · Nakshi Kantha · Liberation War · Terracotta · Ektara · Palki · Royal Bengal Tiger — defined in [`MEGA_DISPLAY/display_config.h`](MEGA_DISPLAY/display_config.h) (`STATION_NAMES`). Audio is one bilingual track per station as `/MP3/0002.mp3`…`0008.mp3` on the DFPlayer SD card (`0001.mp3` = welcome, START card).

## Repository
- Arduino-IDE sketches — this `arduino` branch: <https://github.com/ZAN-Tech-bd/WRO_Rizwanul-Rafeeul-Luban/tree/arduino>
- Original PlatformIO source — `main` branch, `firmware/`

## Credits & sources
Built by the South Breeze School team for WRO 2026 Future Innovators. ChatGPT was used as a support tool for brainstorming, explanations and debugging; the team carried out the hardware, wiring, mechanical work, testing and all final decisions. Artifact names represent the Bangladeshi heritage displays in the model museum.

*Guiding Visitors. Protecting Heritage.*
