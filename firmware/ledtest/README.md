# LED Test Bench — standalone WS2812B demo (`firmware/ledtest`)

A **separate, throwaway test program** to confirm the Hub's WS2812B chain works
through the **74HCT125** level shifter. It does **not** touch the Hub code or
any other part of the repo — it only lights up the LEDs in sequence with a
variety of colors (a PC-RGB look) so you can verify every pixel and the wiring.

## Wiring it expects (identical to the Hub's data path)

```
ESP32-S3 GPIO45 -> 74HCT125 pin 2 (1A input)
74HCT125 pin 1 (/1OE) -> GND       (enables channel 1)
74HCT125 pin 3 (1Y output) -> 330 ohm -> Hub S1/J10 pin 7 -> S1 DIN
S1 DOUT (Hub J10 pin 8) -> R61 (220 ohm) -> Hub S2/J11 pin 7 -> S2 DIN
S2 DOUT (Hub J11 pin 8) -> R62 (220 ohm) -> Hub S3/J12 pin 7 -> S3 DIN
... (for S1 + S2 only, S2 DOUT ends unconnected)
```

74HCT125 power:

```
pin 14 VCC -> regulated 5 V
pin 7  GND -> common GND
100 nF ceramic directly across pins 14 and 7
```

Current Hub PCB buffer connections: pins 1 (/1OE) and 4 (/2OE) -> GND;
pins 10 and 13 (/3OE, /4OE) -> +5V; pins 9 and 12 (unused inputs) -> GND;
unused outputs pins 8 and 11 are open. Pins 5/6 are used for SERVO_IN/BUF.

**Share ground** between ESP32, 74HCT125, the LED supply, and every board.
Feed **5 V and GND to every board in parallel** (don't push all the current
through board 1).

## Configure

Open [`src/main.cpp`](src/main.cpp) and set the two lines in the CONFIG block:

```cpp
#define LED_PIN        45   // Current Hub PCB v0.4 / YD-ESP32-S3 N16R8
#define PIXEL_COUNT    16   // S1 + S2, each has 8 pixels, in daisy-chain order
```

This recovered version is configured for 16 pixels: S1/J10 = global pixel
indices 0–7 and S2/J11 = indices 8–15. Keep both stations connected. R61 (220 Ω)
in the hub must pass S1 DOUT to S2 DIN. The startup sweep should light S1 first,
then S2. A two-station test cannot isolate S2 if S1 or the R61 hop is removed.

## Build & flash (PlatformIO)

```sh
/home/user/.platformio/penv/bin/pio run -d firmware/ledtest -t upload --upload-port /dev/ttyACM0
/home/user/.platformio/penv/bin/pio device monitor -d firmware/ledtest --port /dev/ttyACM0 --baud 115200 --dtr 0 --rts 0
```

The serial output prints the configured pin/count and the name of each effect
as it runs.

## What it does (effects, in order, loops forever)

1. **Startup sweep** — lights every pixel one-by-one in white, so you can
   count the chain and confirm order + color. (This is the “light them all,
   but in sequence” test.)
2. **Color wipe** — fills the strip red, then green, then blue.
3. **Comet / Larson scanner** — a bright head races up and down with a fading
   tail (the signature PC-RGB look), in cyan, magenta, and orange.
4. **Rainbow** — a full spectrum sliding along the strip.
5. **Theater-chase rainbow** — sparkly three-pixel chase with a shifting hue.
6. **Breathing** — the whole strip smoothly pulses in purple, then white.

If a pixel, a color, or an entire board is dark while the rest animate, that
isolates the fault to that pixel / that board's data-in / or its DOUT->DIN
link — exactly what a bench test is for.

This original animation source was recovered from commit `a2be33a` on branch
`feature/mp3-rfid-nano-firmware`. Animation functions are unchanged; hardware
settings are adapted for the current PCB (ESP32-S3, GPIO45, 16 pixels, S1+S2).
The older recovered revision assumed GPIO15/56 pixels, which described the old
hardware, not this PCB. The earlier test sketches used different startup,
brightness, and refresh logic; those differences are plausible reasons they
behaved differently, but no single failing cause was proven. The recovered
animation test is now the test target.

Current hub data path for two stations: GPIO45 -> U2 pin 2 -> U2 pin 3 ->
330 Ω -> J10.7/S1 DIN; J10.8/S1 DOUT -> R61 (220 Ω) -> J11.7/S2 DIN. Each
station gets parallel 5V/GND from its own hub connector. Power off before
changing either cable.

## Why the original demo worked when the earlier tests did not

There is not enough evidence to identify one certain cause for the earlier
dark tests; no earlier build's exact waveform was recorded or compared on a
scope. Facts established from the source and test history:

| Difference | What is known |
|---|---|
| Hardware config | The old branch's untouched defaults were GPIO15 / 56 pixels. That configuration did not match this PCB (GPIO45 and 8 pixels per station). It was changed before using the recovered animation on this board. |
| Startup pattern | The recovered animation immediately starts a white, one-pixel-at-a-time sweep and then color wipes. The earlier diagnostic had different start/refresh behavior; at least one variant explicitly booted OFF and required a serial command. |
| Brightness | The recovered source uses brightness 110/255; the S1 diagnostic was deliberately dimmed to 16/255. The larger setting is visibly brighter, but low brightness alone was not proven to be the cause of total darkness. |
| CPU-visible indicator | The demo toggles `LED_BUILTIN` (fallback GPIO2). This indicates the MCU sketch is running; it is independent of the WS2812 data path and does not prove the strip received data. |
| Electrical tests | The recorded DC high/low readings establish static signal levels only, not LED power under load or valid WS2812 timing. |

The original demo's animations are now kept as the test reference, with only
hardware-specific settings corrected for the current PCB. Do not change the
pin, pixel count, or library while comparing S1 with S1+S2.

## Current two-station test

Connect both stations at once, power off before changing cables:

1. Hub J10 -> S1 hub connector (DIN enters S1).
2. Hub J11 -> S2 hub connector (DIN enters S2).
3. Keep PCB hop **R61 (220 Ω)** installed: S1 DOUT -> R61 -> S2 DIN.
4. Power both stations from their own J10/J11 5V and GND pins. Data alone
   passes serially; power is supplied in parallel.

The 16-pixel startup sweep should traverse S1's 8 pixels first, then S2's 8.
If the first eight light and the following eight do not, focus on S1 DOUT,
R61 and S2 DIN/power. If pixel 1–8 stop working as soon as S2 is connected,
check that S2 is connected to J11 in the correct orientation and is not loading
or shorting the shared power/data lines. Serial output confirms test progress,
not physical LED output.
