# Current museum firmware and bench setup

This is the runbook for the six-station build using the YD-ESP32-S3 Hub and
direct-drive YD-ESP32-S3 robot. The system runs on the Hub's local Wi-Fi.

| Device | PlatformIO project and environment | Update path | Role |
|---|---|---|---|
| Hub | [NOV_HUB](NOV_HUB/README.md), `NOV_HUB` | USB and signed BLE OTA | Station sensors, 48 LEDs, alarm, offline dashboard and live camera gallery |
| Robot | [ESP32_RFID_MP3_RAIL](../ESP32_RFID_MP3_RAIL/README.md), `robot_s3` | USB and signed BLE OTA while parked | Direct L298N control, RFID, audio, obstacle sensing and station reports |
| Camera | [cam](cam/README.md), `cam` | USB at 115200 baud via CAM-MB | GC2145 capture, optional microSD backup and HTTP upload to the Hub |

The older [ESP32 Hub](hub/README.md), the robot's `esp32dev` environment with
the Nano, and the [Mega display](display/README.md) are separate firmware
targets. See the [Hub carrier](../PCB%20design/hub/WIRING-PLAN.md) and
[robot carrier](../PCB%20design/Robot/WIRING-PLAN.md) wiring for connector and
power details.

## How the boards talk

- The Hub hosts `MuseumHub` on channel 1 at `192.168.4.1` with the bench
  password `museum2026`. The visitor page is `/`, the dashboard is
  `/admin.html`, and station diagnostics are at `/scan`.
- The robot and Hub use [ESP-NOW channel 1](common/MuseumProtocol.h).
  The robot sends station and motion updates on change and once per second.
  Station 0 clears the visitor stop. The Hub sends emergency and mute state.
  An emergency keeps a parked robot parked. An active tour follows its exit
  handling, with IR obstacle stopping still in effect. Clearing the alarm
  parks the robot.
- After two seconds of station narration, the robot sends one
  `SHOOT <station>\n` at 9600 baud, 8N1. The signal runs **robot GPIO18 →
  robot J8.3 → camera GPIO13**, with a common ground. The separate intro and
  end cards send no photo command. The camera joins `MuseumHub` and sends a
  JPEG in multipart field `image` to `/api/image/upload?station=N`.
  Successful uploads enter the live gallery. The optional ESP-NOW
  `MSG_IMAGE_READY` packet carries metadata only.
- The robot's DFPlayer uses UART1 on RX17/TX14 with BUSY on GPIO8. The RC522
  uses SCK12, MOSI11, MISO13, SS10 and RST9. Active-low obstacle sensing uses
  GPIO2. L298N pins are ENA4, IN1 5, IN2 6, IN3 7, IN4 15 and ENB16.

## Hub station behavior

| Station pair | Shared active-low proximity | DHT11 GPIOs | MQ analog GPIOs |
|---|---|---|---|
| S1–S2 | GPIO41 | 15, 16 | 4, 5 |
| S3–S4 | GPIO42 | 17, 18 | 6, 7 |
| S5–S6 | GPIO3 | 38, 39 | 1, 2 |

GPIO45 drives one 48-pixel chain, eight LEDs per board from J10 through J15.
Idle runs a color sweep. The robot's station is green during narration and
blue while moving. Proximity blinks the corresponding pair amber. An emergency
overrides the other patterns with a red double-flash and buzzer. The Hub
button is on GPIO21 and the buzzer is on GPIO47.

Each MQ channel learns its own baseline during steady conditions and shows
Normal, Elevated or High. Missing DHT readings use nearby measurements and
display an approximation mark. Diagnostics retain the raw GPIO/ADC readings.
If the powered-station signal is absent, the Hub waits to learn the baseline
and leaves unavailable proximity inputs inactive.

## Build, flash and verify

From the repository root:

```sh
pio run -d firmware/NOV_HUB
pio run -d ESP32_RFID_MP3_RAIL -e robot_s3
pio run -d firmware/cam -e cam
```

Identify the board by its USB device and chip before adding
`-t upload --upload-port /dev/serial/by-id/...`. All USB consoles use
115200 baud. The CAM-MB programmer ignores esptool's DTR/RTS auto-reset:
hold IO0, tap RST, release IO0 while esptool retries to enter download
mode, then tap RST again with BOOT released after the write to boot the
camera. Boot the Hub with station power, join MuseumHub, and bring up
the camera. Use **`SHOOT 0`** on the camera console to check capture and HTTP
upload without a station assignment. For an integrated check with motor
power off, present a station RFID card and check both `[link] <- "SHOOT N"`
on the camera console and the station-tagged image on the admin dashboard.

The Hub advertises `MuseumHub-BLE-OTA` at `10:20:BA:74:72:81`. The robot
advertises `MuseumRobot-BLE-OTA` at `7C:E8:B1:AB:2D:E5`. Use their explicit
BLE addresses in the [OTA tool](NOV_HUB/tools/hub_ble.py) when both are nearby.
Each project keeps its own private `.ota-signing.key`. Camera programming
uses the CAM-MB USB adapter.

| Device | Confirmed on 2026-11-18 | Integrated bench status |
|---|---|---|
| Hub | Build `ade145dd` flashed by USB. BLE confirmed Wi-Fi, ESP-NOW, six learned MQ baselines on station power and three clear proximity zones. | Check proximity activation and the live gallery on the assembled tour. |
| Robot | Build `de53b55a` flashed by USB and booted from `app1` after a signed BLE update. ESP-NOW was ready at idle. | RFID-to-Hub motion reporting and the GPIO18 camera trigger need an assembled test. |
| Camera | The `cam` build passed. | USB programming and the first live JPEG upload are pending. |
