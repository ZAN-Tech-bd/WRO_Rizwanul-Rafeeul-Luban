# LED Test Bench — standalone WS2812B demo (`firmware/ledtest`)

A **separate, throwaway test program** to confirm the Hub's WS2812B chain works
through the **74HCT125** level shifter. It does **not** touch the Hub code or
any other part of the repo — it only lights up the LEDs in sequence with a
variety of colors (a PC-RGB look) so you can verify every pixel and the wiring.

## Wiring it expects (identical to the Hub's data path)

```
ESP32 GPIO15  ->  74HCT125 pin 2  (1A input)
74HCT125 pin 1 (/1OE) -> GND       (enables channel 1)
74HCT125 pin 3 (1Y output) -> 330-470 ohm -> board 1 DIN

board 1 DOUT -> board 2 DIN -> ... -> board N DOUT (unconnected)
```

74HCT125 power:

```
pin 14 VCC -> regulated 5 V
pin 7  GND -> common GND
100 nF ceramic directly across pins 14 and 7
```

Unused channels (2/3/4): tie `/OE` (pins 4, 10, 13) HIGH to 5 V, tie inputs
(pins 5, 9, 12) LOW to GND, leave outputs (pins 6, 8, 11) unconnected.

**Share ground** between ESP32, 74HCT125, the LED supply, and every board.
Feed **5 V and GND to every board in parallel** (don't push all the current
through board 1).

## Configure

Open [`src/main.cpp`](src/main.cpp) and set the two lines in the CONFIG block:

```cpp
#define LED_PIN        15   // ESP32 GPIO -> 74HCT125 pin 2
#define PIXEL_COUNT    56   // 7 boards x 8 pixels. Set to YOUR total.
```

`PIXEL_COUNT` **must match the number of pixels you actually wired.** If you
wired all seven 8-pixel boards, leave it at 56. If you wired fewer boards,
lower it (e.g. 8, 16, 24 …) so the animations line up.

## Build & flash (PlatformIO)

```bash
cd firmware/ledtest
pio run -t upload          # flash the ESP32
pio device monitor         # serial monitor at 115200
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
