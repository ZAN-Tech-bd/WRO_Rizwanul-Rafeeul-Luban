# LED / 74HCT125 point-to-point checklist

## Safety and identification

- **Disconnect battery, DC supply, USB, and any externally powered module.**
  Never use continuity/resistance mode on a powered board.
- These are measurements, **not instructions to short pins with a wire**.
- Touch the probes together first; note the lead resistance/beep sound.
  A direct copper/wire connection should read close to that resistance.
- For initial hub-only tests, disconnect the station cable. Reconnect it,
  with all power still removed, for the end-to-end cable tests.
- In-circuit resistors may read differently because of parallel paths and
  semiconductor junctions. To settle an ambiguous reading, isolate the path;
  do not declare a short from a single transient beep.
- Here **U2 is the HUB 74HCT125**, not the station board's U2 IR socket.
- Pin numbers refer to PCB pads. Looking at the board underside or into a
  mating cable mirrors the view. Use pin numbers, not assumed wire colours.
- C1 is a **100 nF ceramic**, not polarised. C1 pad 1 is the PCB's +5V side;
  C1 pad 2 is its ground side. Find the ground leg by continuity to hub J10.2.
- Resistors are also non-polarised. Their PCB pad numbers do not mean a fixed
  left/right orientation. Probe both ends to identify the end named below.
- If comfortable, removing the socketed MCU/module and socketed U2 can simplify
  bare-board resistance tests. Photograph orientation first; never force or
  attempt to pull a soldered component. Refit only with power disconnected.

### U2 numbering: viewed from the TOP, notch facing up

```text
          notch
        +---U---+
    1 --|       |-- 14
    2 --|       |-- 13
    3 --|       |-- 12
    4 --|       |-- 11
    5 --|       |-- 10
    6 --|       |--  9
    7 --|       |--  8
        +-------+
```

## A. Hub ground connections — every row should beep

Use **hub J10 pin 2** as a convenient ground reference.

| Check | Probe A | Probe B | Expected |
|---|---|---|---|
| A1 | Hub J10 pin 2 | C1 ground leg (pad 2) | Beep / near zero |
| A2 | U2 pin 7 | C1 ground leg | Beep / near zero |
| A3 | U2 pin 1 | U2 pin 7 | Beep / near zero |
| A4 | U2 pin 4 | U2 pin 7 | Beep / near zero |
| A5 | U2 pin 9 | C1 ground leg | Beep / near zero |
| A6 | U2 pin 12 | C1 ground leg | Beep / near zero |
| A7 | R4 ground end (pad 2) | C1 ground leg | Beep / near zero |
| A8 | Hub J5 pin 1 (5V-return connector GND) | C1 ground leg | Beep / near zero |
| A9 | ESP32 GND / U1B pin 1 | C1 ground leg | Beep / near zero |
| A10 | U1A pin 22 / U1B pins 21, 22, one at a time | C1 ground leg | Beep / near zero |

U2 pins **1, 4, 7, 9, 12** are deliberately on the SAME ground net.
Pins 1 and 4 are the active-low enables for the two used buffer channels.

## B. Hub +5V connections — every row should beep

| Check | Probe A | Probe B | Expected |
|---|---|---|---|
| B1 | Hub J5 pin 2 (regulated 5V return) | C1 +5V leg (pad 1) | Beep / near zero |
| B2 | U2 pin 14 | C1 +5V leg | Beep / near zero |
| B3 | U2 pin 10 | U2 pin 14 | Beep / near zero |
| B4 | U2 pin 13 | U2 pin 14 | Beep / near zero |
| B5 | Hub J10 pin 1 | U2 pin 14 | Beep / near zero |
| B6 | U1A pin 21 (DevKit 5V header) | C1 +5V leg | Beep / near zero |

U2 pins **10, 13, 14** are deliberately on the SAME +5V net.
Do NOT mistake J5 pin 1 for positive: on this saved design **J5.1=GND,
J5.2=+5V**. This is the buck's regulated output connection, NOT battery input.

## C. ESP32-to-buffer input — direct paths should beep

| Check | Probe A | Probe B | Expected |
|---|---|---|---|
| C1 | ESP32 GPIO45 / U1B pad 15 | U2 pin 2 | Beep / near zero |
| C2 | U2 pin 2 | R4 signal end (pad 1) | Beep / near zero |
| C3 | Across the two ends of R4 | — | About 10 kΩ; see in-circuit caveat |

The three points GPIO45, U2.2, and R4.1 are the SAME data-input net.
R4's other end goes to ground. GPIO45 is a GPIO label, **not connector pin 45**.
U1B is the 22-pin row containing GPIO45; pad 1 is at the antenna end.

## D. Buffer output to hub S1 — important resistor distinction

| Check | Probe A | Probe B | Expected |
|---|---|---|---|
| D1 | U2 pin 3 | R6 buffer end (pad 1) | Beep / near zero |
| D2 | R6 station end (pad 2) | Hub J10 pin 7 (S1 DIN) | Beep / near zero |
| D3 | Across R6's two ends | — | About 330 Ω |
| D4 | U2 pin 3 | Hub J10 pin 7 | About 330 Ω, NOT zero |

If resistor pad numbers are not visible, test each end from U2.3. One should
be near zero and the other about 330 Ω. From J10.7 the readings reverse.
A continuity meter may not beep at 330 Ω; that is not an open circuit.
U2.2 and U2.3 are an IC input/output pair, **not a copper short**.

## E. S1 cable and station PCB — power OFF, cable connected

"Hub J10" is the hub's S1 port. "Station J1" is the station's 8-pin hub port.
"Station J2" is its 4-pin LED port. Do not confuse these connector names.

| Check | Probe A | Probe B | Expected |
|---|---|---|---|
| E1 | Hub J10 pin 1 | Station J1 pin 1 | Beep / near zero |
| E2 | Hub J10 pin 2 | Station J1 pin 2 | Beep / near zero |
| E3 | Hub J10 pin 7 | Station J1 pin 7 | Beep / near zero |
| E4 | Hub J10 pin 8 | Station J1 pin 8 | Beep / near zero, return only |
| E5 | Station J1 pin 1 | Station J2 pin 1 | Beep / near zero |
| E6 | Station J1 pin 2 | Station J2 pin 2 | Beep / near zero |
| E7 | Station J1 pin 7 | Station J2 pin 3 | Beep / near zero |
| E8 | Station J1 pin 8 | Station J2 pin 4 | Beep / near zero, return only |

The intended hub-to-station cable is straight through, pin n to pin n. For
LED operation on S1 alone, pins 1, 2, and 7 are the essential three connections.

## F. Station-to-stick harness and end-to-end checks

Keep everything unpowered. Connect the actual LED cable/stick.

| Check | Probe A | Probe B | Expected |
|---|---|---|---|
| F1 | Station J2 pin 1 | LED stick actual +5V pad | Beep / near zero |
| F2 | Station J2 pin 2 | LED stick actual GND pad | Beep / near zero |
| F3 | Station J2 pin 3 | LED stick actual DI / DIN pad | Beep / near zero |
| F4 | Station J2 pin 4 | Last pixel's actual DO / DOUT pad | Beep / near zero, optional for S1 |
| F5 | Hub U2 pin 14 | Actual LED +5V pad | Beep / near zero |
| F6 | Hub U2 pin 7 | Actual LED GND pad | Beep / near zero |
| F7 | Hub J10 pin 7 | Actual LED DIN pad | Beep / near zero |
| F8 | Hub U2 pin 3 | Actual LED DIN pad | About 330 Ω |

**Do not require a beep between LED DIN and DOUT.** They are separated by the
WS2812 ICs, not a wire. The LED arrows run DIN -> pixels -> DOUT.
For this S1-only test, return-wire checks E4/E8/F4 can be skipped.

## G. Remaining U2 pins — not needed for S1, listed for completeness

| U2 pin | Point to check | Expected |
|---|---|---|
| 5 | R5 signal end (pad 1), and U1A pad 14 (GPIO46) | Beep / same SERVO_IN net |
| 6 | R7/R8/R9/R10 buffer ends (each pad 1) | Beep / same SERVO_BUF net |
| 8 | No external connection | Intentionally unrouted output |
| 11 | No external connection | Intentionally unrouted output |

R5 ground end (pad 2) -> C1 ground leg should beep. Across R5 is nominally
10 kΩ. Servo paths do not need a connected servo or servo test for the LED test.
No claim of infinite resistance is made for unused IC outputs while the IC is
installed: silicon can produce diode/resistance readings.

## H. Optional later-station data chain — skip for S1-only

Each row contains two direct connections plus a **220 Ω** resistor between them.

| From hub connector | Resistor pad 1 | Resistor pad 2 | To hub connector |
|---|---|---|---|
| J10.8 (S1 DOUT) | R61.1 | R61.2 | J11.7 (S2 DIN) |
| J11.8 (S2 DOUT) | R62.1 | R62.2 | J12.7 (S3 DIN) |
| J12.8 (S3 DOUT) | R63.1 | R63.2 | J13.7 (S4 DIN) |
| J13.8 (S4 DOUT) | R64.1 | R64.2 | J14.7 (S5 DIN) |
| J14.8 (S5 DOUT) | R65.1 | R65.2 | J15.7 (S6 DIN) |
| J15.8 (S6 DOUT) | R66.1 | R66.2 | J16.7 (S7 DIN) |
| J16.8 (S7 DOUT) | R67.1 | R67.2 | J17.7 (S8 DIN) |

Connector-to-near-resistor-pad: beep. Across each resistor / across the whole
row: about 220 Ω, not zero. J17.8 is the final return/end and is not routed onward.
All hub J10–J17 pin 1s share +5V; all their pin 2s share GND.
No SB1–SB7 bypasses remain in the current design.

## I. Check for unintended shorts — all power still removed

These pairs must NOT be a persistent near-zero-ohm copper short:

- C1's two legs / U2.14 to U2.7: +5V vs GND.
- U2.2 to U2.3: LED input vs buffered output.
- U2.2 to U2.7: input has a 10 kΩ pull-down, not a copper short.
- Hub J10.7 (DIN) to J10.1 (+5V), or to J10.2 (GND).
- Station J2.3 (DIN) to J2.1 (+5V), or to J2.2 (GND).
- Station J2.3 (DIN) to J2.4 (DOUT): no designed wire bridge.

Supply capacitors and semiconductor paths can cause momentary beeps/readings;
wait and reverse the probes. A stable near-zero result should be isolated by
unplugging cables and, if safe, removing socketed modules. Do not replace parts
solely because a continuity tester beeped briefly.

## Result interpretation

If A–F pass, power/ground/data continuity for S1 is present. That does **not**
prove LED polarity, correct voltage under load, high-speed pulse quality,
buffer function or a healthy first pixel. Recheck powered voltages separately
and use the RGB animation / known-good stick after completing unpowered tests.

Record failures by check ID, e.g. **D1 open**, **D3 33 kΩ**, **F2 open**.

## Sources verified

Pad/net assignments were read directly from both hub saved releases:

- [Hub after-user-edits project](../../../../PCB%20design/hub/fabrication/20261028-after-user-edits/WRO_HUB_PROJECT_20261028.epro2)
- [Hub sync-fixed project](../../../../PCB%20design/hub/fabrication/20261028-sync-fixed/WRO_HUB_PROJECT_20261028_SYNC_FIXED.epro2)
- [Station final project](../../../../PCB%20design/station/fabrication/20261028-final/STATION_PROJECT_20261028_FINAL.epro2)

The relevant pad/net assignments agree. These are design expectations; the
physical manufactured/assembled board must still be measured.
