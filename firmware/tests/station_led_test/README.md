# S1-only LED bench test (v3: LED + DHT + MQ + PROX + web)

For unpowered probe-by-probe checks, use the
[LED continuity checklist](LED-CONTINUITY-CHECKLIST.md), including U2/C1/R4/R6,
the hub/LED cables, unused buffer pins, and optional later-station hops.

Test every physical station **one at a time on hub S1/J10**. The sketch never
selects S2–S8. It sends only eight pixel slots, starts a dim red → green → blue
cycle (two seconds per colour), and refreshes frames every 250 ms. This also
handles LED power arriving after the microcontroller boots.

The old station-selector sketch is superseded here. Serial replies prove only
that software received a command, not that the LED stick received power/data.
This bench sketch replaces the emergency-button test when uploaded and does
not implement emergency stopping or robot control.

## v3 additions: all S1 data on serial + web page

The sketch now also reads every station-S1 signal from the hub v0.4 pin map and
serves all of it live:

| Signal | Hub GPIO | Where it comes from |
|---|---|---|
| LED data | GPIO45 | buffer U2 → R6 → J10.7 (unchanged v2 behaviour) |
| DHT11 | GPIO15 | J10.4 → station J3 |
| MQ-5 gas | GPIO4 (ADC1) | J10.5 → station 68k/100k divider (raw + V at pin + AO ×1.68) |
| PROX zone Z1 (S1+S2) | GPIO41 | J10.6; LOW = detected |

- **Serial**: one data row per second with LED state, DHT11, MQ-5 and PROX.
- **Web**: the board opens SoftAP `S1Bench` (password `museum2026`); connect a
  phone/laptop and open `http://192.168.4.1/` for live values plus LED buttons
  (same commands as the serial `r/g/b/w`, `0`, `a`, `t`). JSON is at
  `/api/data`; LED control at `/led?c=r|g|b|w|0|a|t`.
- DHT11 shows `-- (no module?)` until the station's J3 cable is plugged in.
- MQ values are raw ADC only, not calibrated ppm; the sensor needs warm-up.
- The web page is [web_page.h](StationLedTest/web_page.h); it lives outside the
  .ino so the Arduino prototype generator does not misread its JavaScript.

## Upload, then open VS Code terminal monitor

Close the existing monitor with **Ctrl+C**; close Arduino IDE's monitor too.
From the workspace root:

```sh
/home/user/.platformio/penv/bin/pio run -d firmware/tests/station_led_test -t upload --upload-port /dev/ttyACM0
/home/user/.platformio/penv/bin/pio device monitor --port /dev/ttyACM0 --baud 115200 --dtr 0 --rts 0
```

The startup banner must say **S1-ONLY LED TEST v2**. If it still says STATION LED
TEST and shows S2/J11 after selecting 2, the old sketch is still flashed.

Arduino IDE: open [StationLedTest.ino](StationLedTest/StationLedTest.ino), install
Adafruit NeoPixel, choose ESP32S3 Dev Module / 16 MB flash / OPI PSRAM / USB CDC On
Boot Disabled, and upload to the WCH UART port. Baud is 115200.

## Commands

| Send | Action (always the S1 stick) |
|---|---|
| r / g / b / w | Solid red / green / blue / white |
| t | Walk its eight pixels, one every 400 ms |
| a | Red → green → blue cycle, two seconds per colour |
| 1–8, n, p | Green on S1 only; no station switching |
| 0 or x | Off; stop animations or diagnostic mode |
| h or ? | Help and status |
| + | GPIO45 steady HIGH for 30 seconds, then off (multimeter test) |
| - | GPIO45 steady LOW for 30 seconds, then off (multimeter test) |

After each powered-off station swap, reconnect power and observe the startup
RGB cycle. A good stick lights without needing any serial command.
Numbers are not station addresses anymore: every number lights S1 green.

## Minimum wiring — no other stations needed

Source: [Hub v0.4 spec](../../../../PCB%20design/hub/WIRING-PLAN.md), sections
2, 3.1 and 5; [station spec](../../../../PCB%20design/station/STATION-SPEC.md),
sections 2, 4 and 5. Pin numbers below are connector pad numbers, **not** a guess
at left/right when looking into a cable. Plug-side views can be mirrored; use
board labels and an unpowered continuity test.

| Hub | Station board | LED stick |
|---|---|---|
| J10.1 = regulated 5V | J1.1 → J2.1 | 5V / + pad |
| J10.2 = GND | J1.2 → J2.2 | GND / − pad |
| J10.7 = S1 DIN | J1.7 → J2.3 | DIN / DI, at the input end |
| J10.8 = S1 DOUT | J1.8 → J2.4 | DOUT from final pixel; optional for S1-only |

**DOUT is not needed to light S1 alone.** Do not connect hub DIN to the stick's
DOUT. Data arrows point from the first pixel toward the last pixel. LED power
must be **5 V**, not the station connector's pin 3 (3V3).

## Ordered fault-finding checklist

### 1. Check power first (powered, DC volts)

Black probe on system GND. With the intended regulated 5 V supply switched on,
measure the following against their local ground:

- Hub J10 pin 1 vs pin 2: approximately **5 V**.
- Station J1 pin 1 vs pin 2: approximately **5 V**.
- Station J2 pin 1 vs pin 2: approximately **5 V**.
- Actual LED stick + vs − pads: approximately **5 V**, including while testing.
- **Hub** U2 (74HCT125) pin 14 vs pin 7: approximately **5 V**.

If one point loses 5 V, fix that supply/cable/solder joint before debugging data.
The S3 can boot from USB while the station rail is unpowered. USB-only operation
does not prove the PCB has LED power. Use the designed buck/supply path; do not
bridge the YD IN-OUT jumper (spec says leave open) or add a second uncoordinated
5 V feed. Never connect the battery/12 V directly to a station.

### 2. Check wiring continuity (ALL power removed, USB too)

- System GND → hub J10.2 → station J1.2 → station J2.2 → LED GND.
- Hub GPIO45 (U1B pad 15) → **hub U2 pin 2**: near-zero resistance.
- **Hub U2 pin 3** → hub J10.7: approximately **330 Ω** through R6;
  a continuity buzzer may NOT beep through this resistor.
- Hub J10.7 → station J1.7 → station J2.3 → LED DIN: near-zero resistance.
- Look for reversed cable order, loose crimps, cold joints, solder bridges,
  and a missing/open 330 Ω resistor. Do not bypass it randomly.

### 3. Check the buffer (powered, DC volts)

U2 here means the **hub 74HCT125**, not the station's IR-module socket.
Confirm the actual chip is fitted, its notch/pin 1 matches the PCB, and its
marking is **74HCT125** (or a verified compatible TTL-input buffer). A 74HC125
is not a guaranteed equivalent for 3.3 V input at a 5 V supply. A loose socket,
reversed IC or open ground can stop every LED.

- Pin 14 = approximately 5 V.
- Pin 7 = GND.
- Pin 1 = approximately 0 V: active-low output enable must be grounded.

Count DIP pins from the package notch/dot using the datasheet, not a guessed
orientation. Avoid shorting adjacent pins with meter probes.

### 4. Trace static data levels (powered, DC volts, revised sketch)

First confirm the buffer AND stick are powered at 5 V. Send **-**, measure;
then send **+**, measure within 30 seconds. Both modes automatically return OFF.
All readings are relative to common GND:

| Point | After - | After + |
|---|---:|---:|
| ESP32 GPIO45 / hub U2 pin 2 | ~0 V | ~3.3 V |
| Hub U2 pin 3 | ~0 V | ~5 V |
| Hub J10 pin 7 | ~0 V | ~5 V |
| Station J2 pin 3 / LED DIN | ~0 V | ~5 V |

These are **steady DC tests, NOT WS2812 data**: they do not make the LEDs light.
A high input with no high output points to U2 power, enable, identity, orientation,
or failure. High at U2.3 but not J10.7 points to R6/PCB routing. High at J10.7 but
not LED DIN points to the harness/station wiring. Never inject 5 V into GPIO45,
never bridge the buffer output to its input, and do not use resistance mode on
powered hardware. Do not run these tests into unpowered LEDs/buffer.

### 5. If power and static levels are good

Send **a** to resume valid 800 kHz pixel data. Try a short known-good cable and
known-good WS2812 stick, still at S1. A normal multimeter cannot validate 800 kHz
pulse timing; a scope/logic analyser is needed to check a waveform (ensure the
instrument input tolerates 5 V). Wrong RGB byte order swaps colours, but does
not normally make all RGB tests completely dark. DIN/DOUT reversal, a failed
first pixel, wrong LED type, or signal integrity can still explain no light.

Record voltages at J10.1, LED +/−, U2.14, U2.1, U2.2, U2.3 and LED DIN.
DC level checks narrow the fault but do not prove signal timing or LED health.
