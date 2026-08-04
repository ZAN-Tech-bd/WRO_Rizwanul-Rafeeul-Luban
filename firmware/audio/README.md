# Audio — DFPlayer Mini bring-up (PlatformIO project)

This ESP32 target drives a **DFPlayer Mini** MP3 module using the prepared
`MSMAUDIO` SD card (`/mp3/0001.mp3` … `/mp3/0014.mp3`).

It is a standalone bring-up node: it talks to the DFPlayer over a hardware
UART and accepts text commands over USB serial for manual testing. Later it
will be wired into the robot's station/language logic.

## Wiring

| ESP32 pin | Connects to | Notes |
|---|---|---|
| GPIO14 (TX) | DFPlayer RX | 3.3 V TX into DFPlayer RX is acceptable |
| GPIO13 (RX) | DFPlayer TX | level-shift if the DFPlayer is powered at 5 V |
| GND | DFPlayer GND | **common ground required** |
| 5 V | DFPlayer VCC | stable 5 V rail; do **not** use the ESP32 3.3 V pin |
| — | DFPlayer SPK_1 / SPK_2 | speaker (typically ≤ 3 W, 4–8 Ω) |

> **Do not use GPIO12 for the DFPlayer TX line.** GPIO12 is the MTDI strapping
> pin that selects the internal flash voltage at boot. Pulling it high (as the
> DFPlayer RX input tends to) forces 1.8 V flash mode, which corrupts the 3.3 V
> flash chip and crashes the ESP32 in a boot loop
> (`spi_flash: Detected size(2k) ... Probe failed`).

> Insert the SD card **with the DFPlayer powered off** (DFRobot warns hot
> insertion can corrupt the index).

## SD card track map

| Track | Language | Station |
|---|---|---|
| `0001`–`0007` | English | 1–7 |
| `0008`–`0014` | Bangla | 1–7 |

`STATION n` in English plays track `n`; in Bangla plays track `n + 7`.

## Serial commands (USB, 115200 baud)

```
LANG 0 | LANG 1        select English / Bangla
STATION <1..7>         play that station in the current language
PLAY <1..14>           play an explicit track directly
STOP                   stop playback
PAUSE | RESUME         pause / resume
VOL <0..30>            set volume
STATE                  show current state
HELP                   list commands
```

## Build & flash

```bash
cd firmware/audio
pio run -t upload          # flash the ESP32
pio device monitor         # USB serial @ 115200
```

## Library

| Library | Why |
|---|---|
| `DFRobotDFPlayerMini` | Drives the DFPlayer Mini over UART; `playMp3Folder(n)` plays `/mp3/000n.mp3` deterministically |
