# Track Hub Wiring Plan — microSD, 56 Pixels, 4 Real Sensor Pairs

## 1. Locked scope and architecture

Target controller: **DOIT-style 30-pin ESP32 DevKit V1 with ESP-WROOM-32**, running the existing Hub application.

The final track build contains:

- the existing Hub microSD module, still enabled on its existing VSPI pins;
- seven straight WS2812B boards with eight addressable pixels each, chained as **56 pixels on one data GPIO**;
- four real DHT11 sensors at stations 1, 3, 5, and 7;
- four real MQ-135 modules, digital `DO` only, at stations 1, 3, 5, and 7;
- clearly marked simulated DHT/MQ values for stations 2, 4, and 6;
- four SG90 servos receiving one shared position command;
- one active buzzer and one emergency pushbutton;
- future explicit emergency-state transmission from Hub to robot over UDP.

The existing SD settings and wiring are retained unchanged. This is the selected middle-ground design: no GPIO expander, no SD removal, and no attempt to pretend that generated station values are independent physical measurements.

The architecture is electrically and pin-count feasible. Physical acceptance still requires voltage, polarity, current, noise, and full-load tests because clone modules and long track wiring cannot be certified from labels alone.

## 2. ESP32 restrictions and preserved interfaces

- GPIO34, 35, 36, and 39 are input-only and have no internal pull resistors. They are assigned to externally driven MQ divider nodes.
- GPIO0, 2, 5, 12, and 15 are reset-strapping pins. GPIO5 remains SD chip-select and GPIO15 remains WS2812B data because these are established Hub assignments; neither external circuit may force an unsafe reset level.
- GPIO1 and GPIO3 remain free for USB flashing and serial diagnostics.
- GPIO6 through 11 belong to onboard flash and are unavailable.
- GPIO16 and GPIO17 are usable on this ESP-WROOM-32 board.
- ESP32 GPIOs are not 5 V tolerant. Every possibly 5 V logic output must be divided or shifted before reaching one.

Existing source assignments are [`NEOPIXEL_PIN`](../firmware/hub/include/config.h:8), [`BUZZER_PIN`](../firmware/hub/include/config.h:10), and the SD constants in [`config.h`](../firmware/hub/include/config.h:20). Keep [`HUB_SD_ENABLED`](../firmware/hub/include/config.h:20) true and retain GPIO18, 19, 23, and 5 exactly as currently configured.

## 3. Final GPIO map

| Function | ESP32 GPIO | Interface |
|---|---:|---|
| 56-pixel WS2812B chain | **15** | 74HCT125 channel, then 330–470 ohm to board 1 `DIN` |
| Active buzzer | **4** | transistor or logic-MOSFET driver; never power it directly from GPIO |
| microSD SCK | **18** | existing VSPI wiring, unchanged |
| microSD MISO | **19** | existing VSPI wiring, unchanged |
| microSD MOSI | **23** | existing VSPI wiring, unchanged |
| microSD CS | **5** | existing VSPI wiring, unchanged |
| Four SG90 shared PWM | **13** | direct 3.3 V command or a spare 74HCT125 channel; one 220 ohm resistor per servo |
| Real DHT11 station 1 | **14** | sensor powered at 3.3 V; DATA pulled up to 3.3 V |
| Real DHT11 station 3 | **16** | same |
| Real DHT11 station 5 | **17** | same |
| Real DHT11 station 7 | **27** | same |
| Real MQ-135 station 1 `DO` | **34** | 10 kΩ upper / 20 kΩ lower divider |
| Real MQ-135 station 3 `DO` | **35** | same |
| Real MQ-135 station 5 `DO` | **36** | same |
| Real MQ-135 station 7 `DO` | **39** | same |
| Emergency button | **21** | normally-open switch to GND; `INPUT_PULLUP` |
| Safe spare GPIOs | **22, 25, 26, 32, 33** | leave accessible for corrections or future additions |
| Preserve/avoid for new loads | **1, 2, 3, 12** | USB UART or strapping concerns |

This map retains SD and leaves five safe spare GPIOs. The LED chain consumes one GPIO, not seven.

## 4. microSD wiring — unchanged

Use the existing wiring:

```text
microSD SCK  -> GPIO18
microSD MISO -> GPIO19
microSD MOSI -> GPIO23
microSD CS   -> GPIO5
microSD GND  -> common GND
```

Supply voltage depends on the exact breakout:

- a common module with an onboard 3.3 V regulator and logic buffer can normally take 5 V at `VCC`;
- a bare socket breakout without regulation or level protection must receive 3.3 V only.

Inspect the board and verify its part markings before applying 5 V. The repository's existing module assumptions are documented in [`firmware/sdcard/README.md`](../firmware/sdcard/README.md:7). Keep SD signal wires short, add 100 nF near the module, and add 10–47 µF locally if card writes are unstable.

## 5. WS2812B chain and locked 74HCT125 interface

### Data is serial

```text
GPIO15 -> 74HCT125 -> 330–470 ohm -> board 1 DIN
board 1 DOUT -> board 2 DIN
board 2 DOUT -> board 3 DIN
board 3 DOUT -> board 4 DIN
board 4 DOUT -> board 5 DIN
board 5 DOUT -> board 6 DIN
board 6 DOUT -> board 7 DIN
board 7 DOUT -> unconnected
```

### Power is parallel

```text
5 V LED branch -> board 1 VCC, board 2 VCC, ... board 7 VCC
GND LED branch -> board 1 GND, board 2 GND, ... board 7 GND
```

Do not feed every board's current through board 1. Inject 5 V/GND at every board on a long track, or at minimum at the beginning, middle, and end after proving voltage under full-white load.

Logical ranges are:

| Station board | Pixel indexes |
|---|---|
| 1 | 0–7 |
| 2 | 8–15 |
| 3 | 16–23 |
| 4 | 24–31 |
| 5 | 32–39 |
| 6 | 40–47 |
| 7 | 48–55 |

The current constructor at [`Adafruit_NeoPixel pixels()`](../firmware/hub/src/main.cpp:33) allocates only `STATION_COUNT` pixels. Implementation must introduce eight pixels per station and a total of 56, then update [`updateLeds()`](../firmware/hub/src/main.cpp:608) to color station ranges rather than single pixels.

### 74HCT125 DIP-14 wiring

The pictured **74HCT125** is locked for the final design. HCT is intentional: when the IC is powered from 5 V, its TTL-compatible input recognizes the ESP32's 3.3 V HIGH and its push-pull output produces a strong 5 V WS2812B waveform.

For channel 1 of the standard DIP-14 pinout:

```text
74HCT125 pin 14 VCC -> regulated 5 V
74HCT125 pin 7 GND  -> common GND
100 nF ceramic      -> directly between pins 14 and 7

pin 1  /1OE -> GND
pin 2  1A   -> ESP32 GPIO15
pin 3  1Y   -> 330–470 ohm -> board 1 DIN
```

Unused channels must not float:

```text
pin 4  /2OE -> 5 V          pin 5  2A -> GND          pin 6  2Y -> unconnected
pin 10 /3OE -> 5 V          pin 9  3A -> GND          pin 8  3Y -> unconnected
pin 13 /4OE -> 5 V          pin 12 4A -> GND          pin 11 4Y -> unconnected
```

If channel 2 is intentionally used for the shared servo command, connect pin 4 `/2OE` to GND, pin 5 `2A` to GPIO13, and fan pin 6 `2Y` through four individual 220 ohm resistors. Then do not tie pin 5 to GND.

Place the 74HCT125 near the ESP32 or, preferably, near the first LED input if the 3.3 V side can remain short. Keep the first data run away from servo and MQ power wiring. The series resistor belongs at the driving end of the 5 V data wire.

### Why HW-209 is not used

The pictured HW-209-style automatic bidirectional converter uses BSS138 MOSFETs and passive pull-ups. It is useful for open-drain buses such as I2C. It is not equivalent to the 74HCT125's strong push-pull output: cable and input capacitance slow its resistor-produced HIGH transition. It may appear to work on a short bench jumper, but it is **not part of the final 800 kHz long-track LED design**.

## 6. Four real DHT11 sensors

Install real DHT11 modules at stations 1, 3, 5, and 7.

For each one:

```text
DHT VCC  -> ESP32 3V3 rail
DHT GND  -> common GND
DHT DATA -> assigned GPIO
4.7–10 kΩ DATA-to-3.3 V pull-up, only if not already fitted on the module
100 nF ceramic between VCC and GND near the sensor
```

Powering DHT11 from 3.3 V keeps its DATA HIGH safe for the ESP32. Do not add a second strong pull-up blindly to a three-pin module that already includes one. Sample sequentially and no faster than approximately once every two seconds per real sensor.

Long one-wire runs can be unreliable. Keep DATA beside a ground conductor, separate it from servo/LED power, and validate every installed cable. If a long run fails, shorten it or move the sensor/controller; do not conceal invalid readings with instant random replacement.

## 7. Four real MQ-135 digital inputs

Install real modules at stations 1, 3, 5, and 7. Power their heaters from the fused 5 V MQ branch; they are not 3.3 V loads.

For each `DO`:

```text
MQ DO ---- 10 kΩ ----+---- assigned ESP32 GPIO34/35/36/39
                      |
                     20 kΩ
                      |
                     GND
```

A 5 V HIGH becomes approximately 3.33 V. The 10 kΩ/20 kΩ pair is preferred over 348 kΩ/680 kΩ because it is less susceptible to leakage and coupled noise while drawing only about 0.17 mA.

Before connecting the ESP32:

1. Power one MQ module from 5 V with the ESP32 disconnected.
2. Measure `DO` in both states.
3. Measure the divider node and prove it remains at or below approximately 3.3–3.4 V.
4. Determine alarm polarity; many LM393 boards assert LOW on alarm, but clones vary.
5. Adjust the module potentiometer only after heater warm-up and real-world observation.

An optional 10–100 nF capacitor from the ESP32-side divider node to GND can reduce chatter. Add software debounce as well. A digital threshold output is an alert state, **not calibrated gas concentration or ppm**.

## 8. Simulated stations 2, 4, and 6

Keep the seven-row dashboard and station model, but explicitly track whether each value is measured or simulated.

Recommended mapping:

| Station | DHT/MQ source |
|---|---|
| 1 | real pair on GPIO14/GPIO34 |
| 2 | simulated from neighboring real stations 1 and 3 |
| 3 | real pair on GPIO16/GPIO35 |
| 4 | simulated from neighboring real stations 3 and 5 |
| 5 | real pair on GPIO17/GPIO36 |
| 6 | simulated from neighboring real stations 5 and 7 |
| 7 | real pair on GPIO27/GPIO39 |

Simulation policy:

- Temperature and humidity: begin with the bounded average/interpolation of the neighboring valid real readings; optionally add a very small, slowly changing bounded offset rather than independent random jumps.
- MQ digital state: do not invent ppm. Derive a synthetic alert conservatively from neighboring boolean states; a safe policy is alert if either neighbor is persistently alert, with the same debounce interval as real inputs.
- If one neighbor is invalid, use the other valid neighbor and mark quality degraded. If both are invalid, publish `unknown`, not a plausible fake value.
- Preserve a source flag such as `measured`, `simulated`, `degraded`, or `unknown` in logs and the future API. The dashboard may display simulated values, but technical documentation and judging explanations must identify them honestly.
- Clamp generated DHT values to realistic demonstrated bounds and change them gradually at the real sensor sampling cadence.

## 9. Four SG90 servos on one command

Use one resistor per signal branch:

```text
GPIO13 directly, or enabled 74HCT125 channel 2 output
  +-- 220 ohm -> servo 1 signal
  +-- 220 ohm -> servo 2 signal
  +-- 220 ohm -> servo 3 signal
  +-- 220 ohm -> servo 4 signal

5 V servo branch -> all four red wires
common GND       -> all four brown/black wires and ESP32 GND
```

One shared PWM waveform is valid because all four intentionally receive the same target. Mechanical position will still vary between units, so align horns/linkages physically. Never power servos from ESP32 3V3 or through a breadboard rail.

Direct 3.3 V normally drives SG90 signal inputs. Use the spare 74HCT125 channel when the fan-out cable is long or direct signaling proves marginal.

## 10. Buzzer driver

For a typical 5 V active buzzer:

```text
GPIO4 -> 1 kΩ -> base of 2N2222 or BC337
10 kΩ from base to GND
emitter -> GND
collector -> buzzer negative
buzzer positive -> fused 5 V logic/buzzer branch
```

Use a suitable logic-level N-MOSFET instead for a high-current buzzer. Add a reverse diode only for a magnetic/inductive buzzer; an ordinary self-driven piezo active buzzer normally does not need one. Verify type and current.

## 11. Emergency button and future UDP state

```text
GPIO21 ---- normally-open pushbutton ---- GND
```

Configure the pin as `INPUT_PULLUP`: released is HIGH and pressed is LOW. For a long cable, add an external 10 kΩ pull-up to 3.3 V and an optional 100 nF capacitor from GPIO21 to GND. Keep the cable away from servo power.

Required behavior:

- debounce for 30–50 ms;
- a short press latches emergency ON;
- release never clears it;
- an uninterrupted five-second hold while active clears it;
- releasing early cancels hold-to-clear;
- an authenticated/authorized admin-dashboard clear also turns it OFF;
- future UDP packets carry explicit active/clear state plus a changing sequence number;
- repeat state transitions and send a periodic active heartbeat because UDP delivery is not guaranteed.

Current [`handleEmergency()`](../firmware/hub/src/main.cpp:280) changes only Hub RAM. Physical-button handling and robot UDP signaling belong to the later implementation phase.

## 12. Power distribution

### Supply class

Use a regulated **5 V, 10 A** supply, or a genuine buck converter able to deliver at least 10 A continuously from the selected source. Do not use an AMS1117 board, MB102 breadboard supply, or small LM2596 module for the full track.

Conservative upper allocation:

- 56 WS2812B pixels: up to 3.36 A at full white;
- four SG90 servos: approximately 2.6 A combined startup/stall allowance, potentially more for clones;
- four MQ-135 heaters: approximately 0.6 A;
- ESP32/Wi-Fi, microSD write peaks, logic, buzzer, and margin: approximately 0.6–0.9 A;
- four DHT11 sensors: negligible relative to these loads.

The resulting conservative peak is approximately 7.2–7.5 A. A 10 A supply preserves appropriate margin. Limit LED brightness in firmware, for example near 25%, but do not rely on software as the only current protection.

### Branch topology

```text
regulated 5 V / 10 A
  +-- 2 A fuse -> ESP32 VIN/5V + SD + logic + buzzer
  +-- 4 A fuse -> four-servo branch
  +-- 1 A fuse -> four-MQ branch
  +-- 5 A fuse -> 56-pixel LED branch

all branch grounds return to one distribution/star point
```

Final fuse values must not exceed wire, connector, board, or module ratings. The labels above are starting points, not a claim that all branch maxima occur continuously.

Recommended wiring and capacitance:

- main 5 V/GND trunk: approximately 18 AWG;
- station branches: approximately 20–22 AWG, adjusted for actual length/current;
- servo junction: 2200 µF low-ESR electrolytic plus 100 nF ceramic;
- LED branch entrance: 1000 µF electrolytic;
- each remote eight-pixel board: 100–470 µF electrolytic on long runs;
- 74HCT125: 100 nF directly between pins 14 and 7;
- each DHT11: 100 nF locally;
- SD module: 100 nF plus optional 10–47 µF locally;
- optional ESP32 branch: 470–1000 µF near VIN/5V if transients remain visible.

All grounds are electrically common for signals, but high-current returns must meet at the power-distribution point instead of flowing through ESP32 pins or breadboard traces.

### USB caution

Do not let external 5 V and USB VBUS fight through an unknown clone-board power path. During development, disconnect high-current peripheral power while flashing, use a data cable with VBUS isolated when appropriate, or verify the exact DevKit power path first. Ground must remain common whenever external signals are attached.

## 13. Bill of materials

### Functional and power parts

| Quantity | Part |
|---:|---|
| 1 | regulated 5 V / 10 A supply, or genuine 5 V / at least 10 A buck |
| 1 | 74HCT125 DIP-14 quad buffer; obtain a DIP-14 socket if soldering to perfboard |
| 1 | 2N2222 or BC337 for buzzer, or suitable logic-level N-MOSFET |
| 1 | normally-open emergency pushbutton |
| 1 | fused screw-terminal/star power-distribution block |
| 1 each | appropriate fuse and holder for logic, servo, MQ, and LED branches |
| 4 | real DHT11 modules |
| 4 | real MQ-135 modules |
| 7 | eight-pixel WS2812B straight boards |
| 4 | SG90 servos |

### Resistors

| Quantity | Value/use |
|---:|---|
| 4 | 10 kΩ, 1%, MQ upper dividers |
| 4 | 20 kΩ, 1%, MQ lower dividers |
| up to 4 | 4.7–10 kΩ DHT DATA pull-ups, only where modules do not already include them |
| 4 | 220 Ω, one per SG90 signal |
| 1 | 330–470 Ω, first WS2812B `DIN` |
| 1 | 1 kΩ, buzzer transistor base |
| 1 | 10 kΩ, buzzer transistor base-to-GND |
| 1 | 10 kΩ, optional external emergency pull-up |

### Capacitors

| Quantity | Value/use |
|---:|---|
| 1 | 2200 µF, servo distribution |
| 1 | 1000 µF, LED branch entrance |
| up to 7 | 100–470 µF, remote LED-board power taps on long runs |
| 5 minimum | 100 nF ceramic: four DHT11 modules plus 74HCT125 |
| 1 | 100 nF at SD module; add 10–47 µF if needed |
| up to 4 | 10–100 nF, optional MQ divider-node filtering |
| 1 | 100 nF, optional emergency-button filtering |

Use electrolytics rated at least 10 V on the 5 V rail and observe polarity.

### Explicitly not required

- no HW-209 or generic BSS138/TXS/TXB converter for WS2812B data;
- no GPIO expander or 74HCT165 for the selected four-real-pair design;
- no data GPIO per LED board;
- no 5 V-to-3.3 V regulator for DHT11;
- no physical sensor at simulated stations 2, 4, and 6.

## 14. Implementation and validation sequence

1. Preserve existing SD constants and establish the final GPIO table in firmware without peripherals attached.
2. Build/flash and verify normal boot, USB logs, SD mount/read/write, and absence of strapping failures.
3. Assemble fused 5 V distribution and verify polarity plus 5.0 V at every unloaded branch.
4. Install the 74HCT125 with its 100 nF capacitor and one LED board; walk pixels 0–7.
5. Add all LED boards, allocate 56 pixels, and walk indexes 0–55 before station grouping.
6. Test full white briefly at the chosen brightness cap; record supply current and voltage at first/middle/final boards.
7. Add one real DHT11 and validate cable reliability, then add the other three and log independent valid/error status.
8. Characterize one MQ module's raw `DO`, divided voltage, polarity, warm-up, and threshold with ESP32 disconnected; then add all four protected inputs.
9. Implement measured/simulated quality flags and validate interpolation for stations 2, 4, and 6, including one-neighbor and both-neighbors-invalid cases.
10. Add one SG90 and then all four while monitoring 5 V and ESP32 reset reason.
11. Add buzzer driver and emergency button; test latch, early-release cancellation, five-second clear, and dashboard clear.
12. Run SD writes while LEDs, sensors, servos, and Wi-Fi are active; require no card errors, corrupt files, brownouts, or resets.
13. Run all subsystems concurrently for at least ten minutes and verify far-end voltage and component temperature.
14. Complete at least three consecutive emergency-state tests before implementing robot UDP behavior.

Suggested diagnostics:

```text
[pins] led=15 buzzer=4 servo=13 button=21 sd=18,19,23,5
[dht] station=1 pin=14 source=measured ok=1 t10=... h10=...
[dht] station=2 source=simulated neighbors=1,3 quality=ok t10=... h10=...
[mq] station=1 pin=34 source=measured raw=0 alert=1 divided_mv=...
[mq] station=2 source=simulated neighbors=1,3 alert=... quality=ok
[led] count=56 station=7 range=48..55
[button] short_press emergency=1 latched=1
[button] clear_hold_ms=5000 emergency=0
[sd] mounted=1 write_ok=1
[power] reset_reason=... brownout_count=0
```

## 15. Final verdict

The selected 30-pin ESP-WROOM-32 can operate the unchanged Hub microSD, 56 LED pixels, four real DHT11 modules, four protected MQ-135 digital channels, four shared-command SG90s, a buzzer, and the emergency switch simultaneously, while retaining five safe spare GPIOs.

The pictured **74HCT125 works and is locked** as the final WS2812B interface. The HW-209 may be kept for open-drain/slow experiments but is not used on either final LED data link. Stations 2, 4, and 6 remain software-generated and must always be represented honestly as simulated rather than independent physical measurements.
