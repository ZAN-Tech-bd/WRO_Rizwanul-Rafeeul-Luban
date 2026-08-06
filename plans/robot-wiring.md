# Guide Robot Wiring Plan — Legacy Direct-ESP Layout

> **Superseded for line following/RFID:** the active two-controller wiring is in [`robot-nano-esp-wiring.md`](robot-nano-esp-wiring.md). The Arduino Nano now owns the IR array, L298N, motors, and RC522; the ESP32 receives RFID events and retains later audio/LED/network duties. Do not combine the direct-ESP pin tables below with the active Nano layout.

## 1. Confirmed scope

Main controller: **DOIT-style ESP32 DevKit V1, ESP-WROOM-32, 30-pin**.

The main ESP32 controls:

- one five-channel line-sensor array with five 3.3 V digital outputs;
- two 6 V TT gearmotors through one L298N module;
- one RC522 RFID reader over SPI;
- one DFPlayer Mini;
- one LM386 amplifier module;
- one 4-ohm, 3 W speaker;
- one straight eight-pixel WS2812B board.

Separate assemblies:

- Arduino Mega + TFT shield: fixed display only, separately powered, no ESP32 communication;
- ESP32-CAM: existing camera wiring/firmware unchanged, separately powered from a suitable 5 V branch.

The source is a 3S LiPo, but this plan starts at the buck-converter inputs/outputs as requested. LiPo charging, low-voltage cutoff, C-rating, and pack protection remain outside this document.

## 2. Final main-ESP32 GPIO allocation

This map avoids the USB UART, flash pins, and new loads on reset-strapping pins.

### Phase 1 line-follower wiring — connect this first

The line-following firmware uses only the ESP32, five-channel IR array, and L298N rows below. Physical calibration on 2026-08-06 proved the installed chassis differs from the earlier Arduino bench convention: **L298N channel A drives the robot's left motor and channel B drives its right motor.** Firmware and this table now match the measured chassis wiring.

| From | To | Exact purpose |
|---|---|---|
| IR array `VCC` | ESP32 `3V3` | Sensor power; do not use 5 V for this confirmed 3.3 V array |
| IR array `GND` | common logic GND | Must join ESP32 and L298N GND |
| physical far-left sensor output | ESP32 GPIO36 / `VP` | `s[0]` |
| physical left sensor output | ESP32 GPIO39 / `VN` | `s[1]` |
| physical centre sensor output | ESP32 GPIO34 | `s[2]` |
| physical right sensor output | ESP32 GPIO35 | `s[3]` |
| physical far-right sensor output | ESP32 GPIO32 | `s[4]` |
| ESP32 GPIO25 | L298N `ENA` | left-motor 8-bit PWM; remove the ENA jumper |
| ESP32 GPIO26 | L298N `IN1` | left direction A |
| ESP32 GPIO27 | L298N `IN2` | left direction B |
| L298N `OUT1`, `OUT2` | robot's physical left TT motor | measured forward for positive command |
| ESP32 GPIO13 | L298N `ENB` | right-motor 8-bit PWM; remove the ENB jumper |
| ESP32 GPIO14 | L298N `IN3` | right direction A |
| ESP32 GPIO33 | L298N `IN4` | right direction B |
| L298N `OUT3`, `OUT4` | robot's physical right TT motor | measured forward for positive command |
| regulated motor-buck `+` | L298N `+12V` / `VS` motor terminal | start at 6.0 V; this label is the motor input, not a demand for 12 V |
| regulated motor-buck `-` | L298N `GND` | high-current return to star ground |
| regulated 5 V logic-buck `+` | L298N `5V` logic terminal | remove/disable `5V_EN` first; never parallel regulators |
| regulated 5 V logic-buck `+` | ESP32 `5V`/`VIN` | final robot power; use USB alone during first logic test |
| logic-buck `-`, ESP32 GND, L298N GND, IR GND | one common star-ground point | mandatory signal reference |

On sensor boards whose printed `OUT1`–`OUT5` order reverses when mounted, follow **physical far-left to far-right position**, not the silkscreen number. Mount the array so the five detectors are perpendicular to the line and the centre detector is on the chassis centreline.

The supplied Arduino code is active-low: after each `digitalRead()`, it computes `1 - reading`, so **LOW means line detected**. Verify that with a meter/serial sensor test over black and white. The ESP32 firmware keeps this default behind one named setting.

| Function | ESP32 GPIO | Direction/notes |
|---|---:|---|
| IR sensor 1, far left | **36** | input-only; external array output must define HIGH/LOW |
| IR sensor 2, left | **39** | input-only |
| IR sensor 3, centre | **34** | input-only |
| IR sensor 4, right | **35** | input-only |
| IR sensor 5, far right | **32** | digital input |
| L298N ENA, left PWM | **25** | LEDC PWM output; physical chassis calibration confirmed channel A is left |
| L298N IN1 | **26** | direction output |
| L298N IN2 | **27** | direction output |
| L298N ENB, right PWM | **13** | LEDC PWM output; physical chassis calibration confirmed channel B is right |
| L298N IN3 | **14** | direction output |
| L298N IN4 | **33** | direction output |
| RC522 SCK | **18** | VSPI clock |
| RC522 MISO | **19** | VSPI controller input |
| RC522 MOSI | **23** | VSPI controller output |
| RC522 SDA/SS | **21** | SPI chip select; `SDA` label is not I²C here |
| RC522 RST | **22** | reset output |
| DFPlayer TX -> ESP32 RX2 | **16** | protect/verify module TX level as described below |
| ESP32 TX2 -> DFPlayer RX | **17** | through 1 kΩ series resistor |
| Eight-pixel WS2812B data | **4** | through 74HCT125 + 330–470 Ω |
| USB serial | **1, 3** | reserved for flashing/logs |
| Intentionally unused | **2, 5, 12, 15** | reset-strapping pins; keep free |

All 21 required signal roles fit. GPIO34/35/36/39 do not have internal pull resistors, but the powered IR-array outputs actively drive them; confirm this with a meter. If the array outputs are open collector rather than push-pull, add external 10 kΩ pull-ups to 3.3 V.

The implemented firmware and its README mirror this allocation. Keep this plan, [`firmware/robot/README.md`](../firmware/robot/README.md:13), and [`firmware/robot/src/main.cpp`](../firmware/robot/src/main.cpp:6) synchronized whenever a pin changes.

## 3. Five-channel IR line array

```text
array VCC -> ESP32 3V3
array GND -> common logic GND
OUT1 -> GPIO36
OUT2 -> GPIO39
OUT3 -> GPIO34
OUT4 -> GPIO35
OUT5 -> GPIO32
100 nF + 10 µF across array VCC/GND near its connector
```

The user identified the array as a 3.3 V digital-output module. Before connecting it:

1. Power it at 3.3 V with ESP32 inputs disconnected.
2. Measure every output over black and white surfaces.
3. Confirm no output exceeds 3.3 V.
4. Record whether black produces LOW or HIGH; do not assume polarity.
5. Check whether each output reaches a stable HIGH without an ESP32 pull-up.

Map the physical left-to-right order in software exactly as wired. Keep these wires away from motor leads.

## 4. L298N and two 6 V TT motors

Remove the ENA and ENB jumper caps so the ESP32 can apply PWM.

```text
GPIO25 -> ENA
GPIO26 -> IN1
GPIO27 -> IN2
OUT1/OUT2 -> robot's physical left motor (L298N channel A)

GPIO13 -> ENB
GPIO14 -> IN3
GPIO33 -> IN4
OUT3/OUT4 -> robot's physical right motor (L298N channel B)

regulated motor buck + -> L298N motor-supply input
regulated motor buck - -> L298N GND
ESP32 logic GND        -> same common GND at distribution point
```

The firmware's positive left command drives `IN1=HIGH, IN2=LOW`; its positive right command drives `IN3=HIGH, IN4=LOW`. Physical wheels-up calibration confirmed both positive directions propel their respective wheel forward. Do not reorder the five sensors to compensate for motor wiring.

### Motor-rail voltage caveat

The L298N is an old bipolar driver and loses substantial voltage internally, often around 1.5–2.5 V depending on current. Feeding its motor input with exactly 6.0 V may deliver only roughly 3.5–4.5 V to the motors. That is safe but reduces speed and torque.

Safe bring-up approach:

1. Start with the motor buck at **6.0 V**.
2. Measure voltage directly across each running motor at full command.
3. If torque is inadequate, increase the buck gradually while ensuring the average motor-terminal voltage stays within the motor's 6 V rating and motor temperature remains acceptable.
4. Do not connect the raw fully charged 3S pack directly to 6 V motors through an L298N and assume PWM alone is sufficient protection.

Use a genuine adjustable buck rated for at least **5 A continuous** as a conservative module class, then validate it against measured two-motor startup/stall current. Cheap modules often advertise a peak rating that requires a heatsink and airflow.

### L298N onboard 5 V regulator/jumper

Do not use the L298N's onboard 5 V regulator to power the ESP32, DFPlayer, LEDs, camera, or Mega. Module labels and jumper behavior vary. Treat its `5V` terminal as logic supply only according to the exact board schematic:

- safest universal configuration: remove/disable the module's `5V_EN` regulator jumper and feed its logic `5V` from the regulated 5 V logic buck;
- never connect two active 5 V regulator outputs together;
- verify the module's exact `5V_EN` convention before wiring.

### Motor-noise suppression

- one 100 nF ceramic directly across each motor's two terminals;
- optionally one 47 nF ceramic from each motor terminal to the metal motor can if the can can be grounded safely;
- twist each motor pair;
- 1000–2200 µF electrolytic plus 100 nF ceramic at the L298N motor-supply input;
- keep motor current out of the ESP32/RC522/DFPlayer ground wiring;
- the standard L298N module normally includes flyback diodes; verify they are present before use.

## 5. RC522 RFID reader — 3.3 V only

```text
RC522 VCC     -> ESP32 3V3
RC522 GND     -> common logic GND
RC522 SCK     -> GPIO18
RC522 MISO    -> GPIO19
RC522 MOSI    -> GPIO23
RC522 SDA/SS  -> GPIO21
RC522 RST     -> GPIO22
RC522 IRQ     -> unconnected
100 nF + 10 µF near RC522 VCC/GND
```

Do not power the RC522 from 5 V. Keep the SPI harness short and away from the L298N and motor wires. The pin labelled `SDA` on common RC522 boards is SPI `SS/CS` in this configuration, not I²C SDA.

Use tags on top of or beside the track at a tested read distance; do not place them beneath conductive or thick structural material without testing.

## 6. DFPlayer Mini UART and audio path

### Power and UART

```text
regulated 5 V logic/audio -> DFPlayer VCC
common audio/logic GND     -> DFPlayer GND

ESP32 GPIO17 TX2 -> 1 kΩ -> DFPlayer RX
DFPlayer TX -> protection/measurement -> ESP32 GPIO16 RX2
```

DFPlayer UART modules/clones vary. Before connecting DFPlayer TX to GPIO16, measure its idle HIGH voltage while powered from 5 V:

- if it is at or below 3.3 V, connect through a 1 kΩ series resistor;
- if it rises toward 5 V, use a 10 kΩ upper / 20 kΩ lower divider before GPIO16;
- do not use a bidirectional BSS138 module for ordinary push-pull UART unless its exact behavior has been validated.

Place 470–1000 µF plus 100 nF across DFPlayer 5 V/GND near the module. Use the prepared FAT32 card and deterministic [`playMp3Folder()`](../tools/dfplayer_audio/FIRMWARE_COMMAND_MAP.md:36) mapping.

### Correct DFPlayer-to-LM386 signal

Use a line-level DAC output, never the bridged speaker pair:

```text
DFPlayer DAC_L (or DAC_R) -> LM386 module audio IN
DFPlayer GND              -> LM386 signal GND
regulated 5 V             -> LM386 VCC
LM386 OUT+ / OUT-         -> one 4-ohm, 3 W speaker according to module labels
```

Do **not** connect DFPlayer `SPK1` or `SPK2` to a ground-referenced LM386 input. Those pins form a bridged power output; grounding either side or treating it as a normal line signal can distort or damage the output stage.

An LM386 at 5 V cannot normally deliver 3 W into 4 ohms. The 3 W marking is the speaker's maximum rating, not the amplifier's output. This pairing is electrically acceptable at moderate volume but may be too quiet in a noisy venue. If the sound test fails, replace the LM386 with a 5 V Class-D amplifier designed for 4-ohm loads; do not solve low volume by overdriving the LM386 input.

Keep amplifier and speaker wiring physically separated from the RC522 antenna and IR sensor harness. Start with DFPlayer and LM386 volume controls low.

## 7. Eight-pixel WS2812B board

Use one channel of the locked **74HCT125 DIP-14**, powered at 5 V. Its TTL-compatible input accepts the ESP32's 3.3 V HIGH and its push-pull output produces the required 5 V waveform:

```text
74HCT125 pin 14 VCC -> regulated 5 V
74HCT125 pin 7 GND  -> common GND
100 nF ceramic      -> directly between pins 14 and 7

pin 1 /1OE -> GND
GPIO4 -> pin 2 1A
pin 3 1Y -> 330–470 Ω -> LED board DIN

pin 4  /2OE -> 5 V; pin 5  2A -> GND; pin 6  2Y -> unconnected
pin 10 /3OE -> 5 V; pin 9  3A -> GND; pin 8  3Y -> unconnected
pin 13 /4OE -> 5 V; pin 12 4A -> GND; pin 11 4Y -> unconnected

LED VCC -> regulated 5 V logic/LED rail
LED GND -> common GND
LED DOUT -> unconnected
```

Add 1000 µF across 5 V/GND near the LED board. The 330–470 ohm resistor belongs at the 74HCT125 output end of the data wire. One eight-pixel board can draw up to about 0.48 A at full-white worst case. Apply a firmware brightness cap but size the rail for the hardware maximum.

Do **not** substitute the pictured HW-209/BSS138 automatic bidirectional converter in the final robot. Its passive pull-up HIGH transition is intended primarily for open-drain buses and is not equivalent to the 74HCT125's strong push-pull output. It may work with very short bench jumpers, but it is not the locked competition wiring.

Do not use GPIO2, 5, 12, or 15 for this LED simply because examples do; the chosen GPIO4 avoids adding another reset-strapping load.

## 8. Buck-rail and ground architecture

Start at the protected 3S supply distribution point:

```text
3S source after master switch/protection
  +-- fuse -> adjustable 6 V / genuine >=5 A buck -> L298N motor branch
  +-- fuse -> regulated 5 V / genuine >=5 A buck -> ESP32, DFPlayer,
  |                                                  LM386, WS2812B,
  |                                                  L298N logic
  +-- fuse -> dedicated 5 V buck for ESP32-CAM
  +-- fuse -> dedicated 5 V buck for Mega + TFT

all buck input negatives and output grounds join the common distribution ground
```

### Why separate camera and Mega branches

The Mega/TFT and ESP32-CAM can create their own current steps and noise. Giving each a dedicated buck or separately fused branch prevents display/camera behavior from pulling down the robot-brain/audio rail. Their exact current ratings should follow their measured load; a conservative starting class is 5 V/2 A for the ESP32-CAM and 5 V/3 A for the Mega/TFT shield.

### Main 5 V logic/audio buck budget

Conservative allowance:

- ESP32 Wi-Fi peaks: roughly 0.5–0.7 A design allowance;
- eight WS2812B pixels: up to 0.48 A;
- DFPlayer: transient dependent on playback/load, reserve at least 0.5 A;
- LM386 module: reserve several hundred mA;
- L298N logic, AHCT buffer, margin: additional allowance.

Use a genuine **5 V/5 A buck** for this branch. The normal draw should be lower, but the margin reduces audio resets and ESP32 brownouts.

### Grounding

Use a star or short-bus distribution point:

- motor/L298N high-current return goes directly to the distribution point;
- logic buck return goes directly to the distribution point;
- ESP32, RC522, IR array, DFPlayer, LM386, and LED grounds fan out from the logic rail;
- do not daisy-chain motor return through DFPlayer/LM386/ESP32 ground;
- the common connection is required for L298N control signals.

Use approximately 18–20 AWG for buck/motor power and appropriately smaller signal wire. Connector, switch, fuse, and PCB current ratings must match the branch.

## 9. Robot bill of materials

### Power and protection

| Quantity | Item |
|---:|---|
| 1 | Adjustable 6 V buck, genuine at least 5 A continuous, for motors |
| 1 | Regulated 5 V buck, genuine at least 5 A continuous, for main ESP32/audio/LED logic |
| 1 | 5 V/2 A or better dedicated buck for ESP32-CAM |
| 1 | 5 V/3 A or better dedicated buck for Mega + TFT |
| 1 | Master power distribution/fuse block after the protected 3S source |
| 4 | Appropriately rated branch fuse holders/fuses, finalized after measured current |
| 1 | Master switch and emergency-accessible power disconnect rated for total current |

### Logic/interface

| Quantity | Item |
|---:|---|
| 1 | 74HCT125 DIP-14 plus optional DIP socket for WS2812B data |
| 1 | 330–470 Ω resistor for LED DIN |
| 2 | 1 kΩ resistors for DFPlayer UART series protection |
| 1 each | 10 kΩ and 20 kΩ resistors, fitted on DFPlayer TX only if measured HIGH exceeds 3.3 V |
| up to 5 | 10 kΩ IR-output pull-ups to 3.3 V only if outputs prove open collector |

### Capacitors/noise suppression

| Quantity | Item |
|---:|---|
| 2 | 100 nF ceramic, one directly across each motor |
| 1 | 1000–2200 µF electrolytic at L298N motor input |
| 1 | 470–1000 µF electrolytic at DFPlayer |
| 1 | 1000 µF electrolytic at WS2812B board |
| 2 | 10 µF for RC522 and IR array local supply |
| several | 100 nF ceramics at 74HCT125, RC522, IR array, and other logic modules |

Use electrolytics rated at least 10 V on 5/6 V outputs. Buck input capacitors must be rated safely above the fully charged 3S input voltage; use the converter manufacturer's recommended input capacitors rather than assuming 10 V parts are sufficient there.

## 10. Bring-up and acceptance sequence

1. With all loads disconnected, set and meter every buck output. Mark the 5 V and motor-rail modules physically to prevent swapping.
2. Connect the ESP32 only; verify normal boot and USB diagnostics.
3. Add the IR array and use the firmware's startup/serial sensor diagnostics to record five-channel polarity over black/white before wheels-down operation.
4. Add RC522 alone; perform repeated reads with motors off.
5. Add L298N with wheels raised; test each direction pin at low PWM, then both motors. Confirm EN jumpers are removed.
6. Add motor suppression capacitors and test RFID plus IR readings while motors switch direction rapidly.
7. Add DFPlayer without LM386; verify UART, SD track mapping, and supply stability.
8. Add LM386 through `DAC_L` or `DAC_R`, begin at low gain/volume, and listen for clipping/reset/noise.
9. Add the 74HCT125 and eight LEDs; run an index 0–7 walking-pixel test.
10. Power ESP32-CAM and Mega/TFT from their dedicated branches; verify no robot-brain reset.
11. Run motors, RFID scans, line sensing, audio, Wi-Fi/UDP, LEDs, camera, and display concurrently. Record minimum 5 V/6 V rail voltage and ESP32 reset reason.
12. Require at least three consecutive full route/station/audio runs before closing the wiring.

Suggested diagnostics:

```text
[pins] ir=36,39,34,35,32 motor=right(ena25,in1=26,in2=27) left(enb13,in3=14,in4=16)
[pins] rfid=18,19,23,21,22 df=rx33,tx17 led=4
[ir] bits=00100 black_is=1
[motor] left_pwm=... right_pwm=... rail_mv=...
[rfid] uid=... reads=... errors=...
[audio] track=... df_tx_idle_mv=... state=PLAYING
[power] logic_min_mv=... motor_min_mv=... reset_reason=...
```

## 11. Final verdict

The 30-pin ESP-WROOM-32 has enough safe GPIOs for all requested robot peripherals with no expander and no Mega communication link. The allocation keeps USB logging available and reserves reset-strapping pins.

The pictured 74HCT125 works and is locked for the robot WS2812B link. The HW-209 is not used for final LED data.

The two principal design caveats are physical rather than pin-count issues:

1. L298N voltage loss means a nominal 6 V input may under-drive 6 V TT motors; tune from a safe 6 V starting point using measured motor-terminal voltage and temperature.
2. An LM386 at 5 V will not deliver the speaker's 3 W rating. Use DFPlayer DAC output into the LM386; replace the amplifier with a suitable Class-D module if venue-volume testing fails.

No wiring plan can make an unknown buck's advertised peak current trustworthy. The measured full-load acceptance sequence is part of this architecture.
