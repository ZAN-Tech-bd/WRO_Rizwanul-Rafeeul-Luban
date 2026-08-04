# Display — Robot Face & Info Screen (PlatformIO project)

This is a **separate microcontroller** from the ESP32 nodes: an **Arduino Mega 2560**
(8-bit AVR, no Wi-Fi) driving a **3.2" MCUFRIEND parallel-bus TFT shield** (ILI9481,
480×320, 16-bit). The shield plugs directly onto the Mega headers — no wiring needed.

The Mega is a **dedicated renderer**. It cannot join the ESP-NOW bus, so the ESP32
robot brain sends short text commands over a UART wire and the Mega draws them.

> **D3 locked decision:** the TFT shows **English text only**. Bengali conjunct/matra
> shaping is not done on the display. The "Bangla" menu option selects the **audio
> language** (played by the DFPlayer speaker); the label is shown in romanized form.
> Full Bengali rendering lives on the phone dashboard, where the browser shapes it.

## Features

- **Animated robot face** (default idle screen): synchronized eased blinking,
  deterministic left/right gaze, cheeks, a Jamdani-inspired forehead mark, and
  four emotions (`happy`, `neutral`, `alert`, `sleepy`). The face does not show
  an audio-language badge; language remains visible in the menu and station UI.
- **Language-selection menu**: two large cards for English / Bangla audio.
- **Station screen**: shows the current artifact name + number, and whether the
  robot is moving or narrating.
- **Emergency screen**: full red evacuation alert.

All drawing is **procedural** — no bitmap assets or SD card are needed. The eye
animation uses pupil dirty regions and incremental eyelid scanlines to avoid
tearing on the Mega's 16-bit parallel display.

## Wiring

| Mega pin | Connects to | Notes |
|----------|-------------|-------|
| TFT shield | Mega headers | plugs directly on top |
| RX1 (pin 19) | ESP32 GPIO17 (TX) | through a **1 kΩ** resistor (3.3→5 V safety) |
| TX1 (pin 18) | *(optional)* ESP32 GPIO16 (RX) | if the robot wants replies |
| GND | ESP32 GND | **common ground required** |

> The shield uses the Mega's parallel port registers, so pins 22–53 + A0–A5 are
> consumed by the display. Use Serial1 (pins 18/19) for the ESP32 link, never Serial0
> (pins 0/1) which is shared with USB upload.

## Serial command protocol (ESP32 → Mega, 9600 baud)

Send one command per line (terminated by `\n`). Case-insensitive.

| Command | Effect |
|---------|--------|
| `FACE` | Show the idle animated face (happy) |
| `FACE HAPPY` / `FACE ALERT` / `FACE SLEEPY` / `FACE NEUTRAL` | Face with that emotion |
| `LANG 0` | Audio = English |
| `LANG 1` | Audio = Bangla |
| `MENU` | Show the language-selection menu |
| `STATION 3 0` | At station 3, not moving (narrating) |
| `STATION 3 1` | Heading to station 3, moving |
| `EMERGENCY` | Emergency evacuation screen |
| `BOOT` | Re-show the splash, then return to face |

## Testing without an ESP32

You can drive every screen from the **USB serial monitor** (9600 baud). The firmware
accepts the same commands on `Serial` as on `Serial1`, so you can type e.g.:

```
MENU
LANG 1
FACE HAPPY
STATION 4 0
EMERGENCY
FACE
```

This lets you verify the display works before the robot firmware exists.

## Build & flash

```bash
cd firmware/display
pio run -t upload          # flash the Mega (select its port if needed)
pio device monitor         # USB serial @ 9600
```

## Libraries

| Library | Why |
|---------|-----|
| `MCUFRIEND_kbv` | Drives the 16-bit parallel bus; write-only IDs are mapped to the physically verified ILI9481 controller |
| `Adafruit GFX Library` | Drawing primitives (ellipses, circles, text, lines) |
