# Museum HUB Carrier — Wiring Spec v0.3 (ESP32-S3, beginner build, 100 % through-hole) — SUPERSEDED by WIRING-PLAN.md v0.4

> ⚠️ **Status: SUPERSEDED (2026-10-25).** Kept for history; do **not** build from this file. Replaced by
> [`WIRING-PLAN.md`](WIRING-PLAN.md) v0.4 (part-count review: DHT pull-ups R21–R27 and E-stop pull-down R14 removed,
> D1 fitted, Q1 = 2N2222A, 3V3 budget corrected).
>
> **Former status: APPROVED by the user (2026-10-12).** This was the source of truth for the schematic and PCB.
> Supersedes v0.2 (SMD / MOSFET power path) → [`WIRING-PLAN-v0.2.md`](WIRING-PLAN-v0.2.md) and v0.1 → [`WIRING-PLAN-v0.1.md`](WIRING-PLAN-v0.1.md).
> Firmware today ([`firmware/hub/include/config.h`](../../firmware/hub/include/config.h)) still uses the old ESP32-WROOM pins; §10 maps old → new.

---

## 0 · What this board is

A hand-soldered carrier for the **YD-ESP32-S3 N16R8** DevKit. Every part is through-hole, there is one logic
chip (the **74HCT125** the team already owns), and every off-board wire ends in a connector.
Bare PCB order only — no JLC assembly.

| Decision | Choice |
|---|---|
| Power source select | Manual 3-position DPDT rocker **KCD4-203-B**: I = battery, O = off, II = 12 V adapter |
| Protection | None (no fuses, MOSFETs or power diodes — user decision). Unplug the LiPo when not in use |
| 12 V | XT60 in (battery, adapter) and out (to the external 10 A buck) |
| 5 V | Back from the buck on **XT30** — a different size, so a LiPo can never be plugged into the 5 V rail |
| Stations | 8 ports, one **8-pin JST-XH** cable each to a station board: 5V, GND, 3V3, DHT, MQ, PROX, LED DIN, LED DOUT |
| Pin budget | 28 usable S3 pins → proximity is shared by pairs of stations; port 8 has no DHT/MQ |
| Servos | Up to 4, all on one shared signal |
| Other | Onboard buzzer, emergency-button port, I²C port, microSD module lying flat |

Reference drawings in this folder:
- `YD-ESP32-S3 N16R8_footprint.png` — rows **25.40 mm** apart, board 27.94 × 57.15 mm, pin 1 at 1.91 mm from the
  top edge, antenna overhangs the top edge by 6.24 mm (63.39 mm total).
- `Switch_KCD4-203-B-dimension.jpg` — 2 rows × 3 spade tabs (6.3 mm), rows 11 mm apart, tabs 10.5 mm apart; panel mount.
- `Micro-SD-Card-Adapter-Module-Dimensions.jpg` — 46 × 24 mm; pins CS, SCK, MOSI, MISO, VCC, GND; has its own
  AMS1117-3.3 + level shifter → **VCC = 5 V**, signals are 3.3 V-safe.

---

## 1 · Power path

```
3S LiPo ──XT60──► J1 BAT IN :  BAT+ ─► switch pole 1 "I"      BAT− ─► switch pole 2 "I"
12 V adapter ─panel jack─XT60─► J2 DC IN :  DC+ ─► pole 1 "II"    DC− ─► pole 2 "II"
          (rocker on the enclosure, wired through J3 VH-6; middle "O" = both poles open)
          pole 1 COM = V12        pole 2 COM = GND (system ground)
V12 / GND ─► J4 XT60 → buck 10 A IN ;  buck 5 V OUT ─► J5 XT30 ─► +5V rail → everything on the board
Red   LED + 2.2 kΩ : BAT+ → GND   lights only in position I  (battery selected and plugged in)
Blue  LED + 2.2 kΩ : DC+  → GND   lights only in position II
Green LED + 1 kΩ   : +5V  → GND   buck output present
```

- Both wires of the unused source are open, so the adapter can never charge or touch the battery.
- I ↔ II passes through O, so the two sources are never connected together. The hub restarts when you switch.
- **BAT− and DC− are separate nets on the PCB, not GND.** They become GND only through the switch.
- No transistor is needed for the source LEDs: the switch closes only the selected source's LED loop.
- The ESP gets +5V straight into its 5Vin pin. The YD board's own diode stops USB from back-feeding the rail —
  **keep the YD board's IN-OUT solder jumper open.**
- +3V3 (stations, I²C, E-stop) comes from the DevKit's 1 A regulator (CJ6107): ≈ 0.35 A typical, 0.7 A peak.

### Current budget
| 5 V load | Typical | Worst case |
|---|---|---|
| 7 × MQ-5 heaters + 7 IR modules / station buzzers | 1.2 A | 1.4 A |
| 56 × WS2812B (7 boards × 8) at firmware brightness 60/255 | ≤ 0.8 A | 3.4 A (full white) |
| 1–4 × SG90 | 0.1 A | 2.6 A (all stalled) |
| ESP32-S3 + Wi-Fi, SD module, buzzer, 74HCT125 | 0.5 A | 0.6 A |
| **Total** | **≈ 2.5–3 A** | **≈ 8 A** |

Per station port ≤ 0.7 A (MQ-5 heater + 8 LEDs + IR module + station buzzer) → JST-XH (3 A per pin) is fine;
use 22 AWG for 5V/GND.
12 V side worst case ≈ 4 A. A 2200 mAh 3S pack lasts roughly 1.5 h — the MQ heaters alone draw about 1 A.

---

## 2 · Ports and harnesses

### 2.1 Port table
| Ref | Port | Board connector (LCSC) | Pins 1 → n |
|---|---|---|---|
| J1 | Battery in | XT60PW-M, male (C98732) | +, − |
| J2 | DC adapter in | XT60PW-M, male (C98732) | +, − |
| J3 | Rocker switch harness | JST VH 6-pin B6P-VH (C20088586) | BAT+, V12, DC+, BAT−, GND, DC− |
| J4 | 12 V → buck | XT60PW-F, female (C428722) | +, − |
| J5 | 5 V ← buck | XT30PW-M, male (C53373776) | +, − |
| J10–J16 | Stations S1–S7 | JST XH 8-pin B8B-XH-A (C157972) | 5V, GND, 3V3, DHT, MQ, PROX, DIN, DOUT |
| J17 | Station S8 (spare, LED-chain end) | JST XH 8-pin (C157972) | 5V, GND, 3V3, –, –, PROX, DIN, DOUT |
| J18 | I²C (expansion port) | JST XH 4-pin B4B-XH-A (C144395) | 3V3, GND, SCL, SDA |
| J20 | Emergency button | JST XH 3-pin B3B-XH-A (C144394) | 3V3, GND, SIG |
| J21–J24 | Servo 1–4 (one shared signal) | JST XH 3-pin (C144394) | 5V, GND, SIG |
| SB1–SB7 | LED bypass, stations S1–S7 | 2-pin 2.54 mm header footprint (C492401), left empty | wire link or header + jumper cap, only if that port stays empty |
| U1 | YD-ESP32-S3 socket | 2 × 1×22 female PM254V-11-22-H85 (C46595983), rows 25.40 mm | §4 |
| U6 | microSD module (flat) | 1×6 female PM254V-11-06-H85 (C2832269) | CS, SCK, MOSI, MISO, VCC, GND |

### 2.2 Rules
- **Every XH port has its supply on pin 1 and GND on pin 2.** A plug in the wrong XH port can never reverse
  polarity; the worst case is 3.3 V ↔ 5 V. Pin counts also differ by job (8 / 4 / 3). Silkscreen "3.3 V" on J18 and J20.
- **XT genders follow the LiPo/ESC convention** (the live side has the sockets): battery and adapter leads
  (XT60 female) fit only J1/J2; the buck input lead (XT60 male) fits only J4; the buck 5 V lead (XT30 female)
  fits only J5.
- **Station boards** (7 copies, spec in [`../station/STATION-SPEC.md`](../station/STATION-SPEC.md)): MQ-5 on 5V with
  its 68 k / 100 k divider on the station, so the MQ signal on the cable is ≤ 3.0 V. DHT on **3V3** (as in the old
  working build). IR obstacle module on 5V with its local BC557 + buzzer alarm; its OUT reaches the hub through a
  Schottky diode, so the hub only ever sees "pulled LOW = detected" (3.3 V-safe, and two stations can share one
  input). LED stick on 5V.

### 2.3 Harness pin-outs
| Cable | Hub end | Other end |
|---|---|---|
| Station S1–S8 | XH-8 housing | XH-8 on the station board, wired 1:1 (pin n ↔ pin n) |
| Rocker switch | VH-6 housing | 6.3 mm female spade crimps on the KCD4 tabs (table below) |
| Battery | — | stock LiPo lead, XT60 female |
| DC adapter | — | panel 5.5 × 2.1 mm jack → 16–18 AWG → XT60 female |
| Buck | — | input lead XT60 male (to J4); output lead XT30 female (to J5); 16 AWG |
| Servo | XH-3: 5V, GND, SIG | servo plug: red, brown, orange |
| Emergency button | XH-3: 3V3, GND, SIG | module VCC, GND, OUT (HIGH = pressed), or a bare NO button between SIG and 3V3 |
| I²C | XH-4: 3V3, GND, SCL, SDA | module VCC, GND, SCL, SDA |

Rocker (KCD4-203-B) wiring — tabs seen from the back, 2 rows × 3:
| J3 pin | Net | Switch tab |
|---|---|---|
| 1 | BAT+ | pole 1, end tab "I" |
| 2 | V12 | pole 1, middle tab (COM) |
| 3 | DC+ | pole 1, end tab "II" |
| 4 | BAT− | pole 2, end tab "I" (same end as pin 1) |
| 5 | GND | pole 2, middle tab (COM) |
| 6 | DC− | pole 2, end tab "II" |

⚠️ On many rockers, pressing the "I" side connects the middle tab to the **opposite** end tab. Check with a
multimeter (continuity) before crimping; if it is reversed, swap the I and II wires on both poles.

---

## 3 · On-board circuits

### 3.1 74HCT125 (U2, DIP-14 in a socket) — same wiring as the old build, new GPIO numbers
| Pin | Name | Connection |
|---|---|---|
| 1 | /1OE | GND |
| 2 | 1A | GPIO45 (LED_DIN) + 10 k to GND |
| 3 | 1Y | 330 Ω → S1_DIN |
| 4 | /2OE | GND |
| 5 | 2A | GPIO46 (SERVO_IN) + 10 k to GND |
| 6 | 2Y | SERVO_BUF → 4 separate 220 Ω → J21–J24 SIG |
| 7 | GND | GND |
| 8 | 3Y | not connected |
| 9 | 3A | GND |
| 10 | /3OE | +5V |
| 11 | 4Y | not connected |
| 12 | 4A | GND |
| 13 | /4OE | +5V |
| 14 | VCC | +5V, with 100 nF directly across pins 14 and 7 |

The two 10 k pull-downs stop the LEDs flashing and the servos twitching while the S3 boots. They also hold the
strapping pins GPIO45/46 LOW at reset, which the S3 needs to boot.

### 3.2 LED chain through the hub
`S1_DOUT → 220 Ω → S2_DIN → … → S7_DOUT → 220 Ω → S8_DIN`; S8_DOUT ends at J17 pin 8 (chain end).
- LED power is parallel: each LED board gets 5 V from its own station port.
- Each hop runs out to a station and back over two cable lengths, driven by the station's own WS2812 →
  keep station cables ≤ about 1.5 m.
- SBk (solder pads between Sk_DIN and Sk_DOUT): only needed when a middle port stays empty while later ports are
  used — otherwise the LEDs after the empty port stay dark. Plug stations in order and you never touch them.

### 3.3 Proximity zones (shared inputs)
The PROX pins of S1+S2, S3+S4, S5+S6, S7+S8 are joined on the PCB into zones Z1–Z4 ("either station detects" = LOW).
Per zone: 10 k pull-up to +3V3 (an empty zone reads "nothing") + 1 k series → GPIO.

### 3.4 Per station S1–S7
- DHT: 10 k pull-up to +3V3 + 1 k series → GPIO.
- MQ-5: arrives already divided by the station (68 k / 100 k, AOUT 5 V → 3.0 V), so **no divider on the hub**: the
  line goes straight to its ADC1 pin with **100 nF to GND** at the pin. That source is ~40 kΩ and shares the cable
  with the 800 kHz LED data; the capacitor keeps the reading steady. Firmware: AOUT = V_adc × 168 / 100.

### 3.5 Other
- **Buzzer** (old build): GPIO47 → 1 k → NPN base (SS8050 or 2N2222, TO-92, E-B-C); 10 k base → GND; emitter → GND;
  collector → buzzer −; buzzer + → +5V. 12 mm passive 5 V buzzer on the board. 1N4148 across the buzzer
  (cathode to +5V) — optional, protects the transistor.
- **Emergency button**: SIG → 1 k → GPIO21; 10 k from GPIO21 to GND (HIGH = pressed, as the firmware expects).
- **I²C**: 4.7 k pull-ups on SDA and SCL to +3V3. Plugging an I/O-expander module in here is how to add pins later.
- **Source LEDs**: §1.

### 3.6 Capacitors
- **100 nF at the 74HCT125** (fitted): the chip switches at 800 kHz and pulls short current spikes; the capacitor
  supplies them locally. The old build had it too.
- **470 µF at the servo ports** (footprint only, not fitted): one SG90 needs nothing. If 3–4 servos starting
  together reset the ESP, solder it in.
- **No capacitor at J5**: the buck has its own.

---

## 4 · DevKit pin map (U1)

U1A = left 22-pin header, U1B = right 22-pin header, pin 1 at the antenna end.

| U1A pin | GPIO | Net | | U1B pin | GPIO | Net |
|---|---|---|---|---|---|---|
| 1 | 3V3 | +3V3 | | 1 | GND | GND |
| 2 | 3V3 | +3V3 | | 2 | 43 (TX) | — |
| 3 | RST | — | | 3 | 44 (RX) | — |
| 4 | 4 | MQ_S1_ADC | | 4 | 1 | MQ_S5_ADC |
| 5 | 5 | MQ_S2_ADC | | 5 | 2 | MQ_S6_ADC |
| 6 | 6 | MQ_S3_ADC | | 6 | 42 | PROX_Z2_IO |
| 7 | 7 | MQ_S4_ADC | | 7 | 41 | PROX_Z1_IO |
| 8 | 15 | DHT_S1_IO | | 8 | 40 | DHT_S7_IO |
| 9 | 16 | DHT_S2_IO | | 9 | 39 | DHT_S6_IO |
| 10 | 17 | DHT_S3_IO | | 10 | 38 | DHT_S5_IO |
| 11 | 18 | DHT_S4_IO | | 11 | 37 | — (octal PSRAM) |
| 12 | 8 | I2C_SDA | | 12 | 36 | — (octal PSRAM) |
| 13 | 3 | PROX_Z3_IO | | 13 | 35 | — (octal PSRAM) |
| 14 | 46 | SERVO_IN | | 14 | 0 | — (BOOT) |
| 15 | 9 | I2C_SCL | | 15 | 45 | LED_DIN |
| 16 | 10 | MQ_S7_ADC | | 16 | 48 | PROX_Z4_IO |
| 17 | 11 | SD_MOSI | | 17 | 47 | BUZ_IO |
| 18 | 12 | SD_SCK | | 18 | 21 | ESTOP_IO |
| 19 | 13 | SD_MISO | | 19 | 20 | — (USB D+) |
| 20 | 14 | SD_CS | | 20 | 19 | — (USB D−) |
| 21 | 5V | +5V | | 21 | GND | GND |
| 22 | GND | GND | | 22 | GND | GND |

Notes: GPIO3 may carry a 10 k pull-up from the YD board (harmless on a pulled-up input). GPIO48 is also the
onboard RGB LED's pin, so that LED is not used. All MQ inputs sit on ADC1 (GPIO1–10), which keeps working with Wi-Fi on.

---

## 5 · Connection list (schematic source of truth)

Net names are what the schematic must use. "Sk" = station k.

XT symbols number their power pins differently: XT60 male (J1, J2) 1 = −, 2 = +; XT60 female (J4) 3 = +, 4 = −
(pins 1/2 are mounting tabs); XT30 male (J5) 1 = −, 2 = + (pins 3/4 are mounting tabs). Mounting tabs stay unconnected.

**Power**
- `BAT+`: J1.2, J3.1, R1 (2.2 k) → LED1 red anode; LED1 cathode → GND.
- `BAT-`: J1.1, J3.4.
- `DC+`: J2.2, J3.3, R2 (2.2 k) → LED2 blue anode; LED2 cathode → GND.
- `DC-`: J2.1, J3.6.
- `V12`: J3.2, J4.3.
- `GND`: J3.5, J4.4, J5.1, U1A.22, U1B.1/21/22, U2.7, U6 GND, all XH GND pins.
- `+5V`: J5.2, U1A.21, U2.14, U2.10, U2.13, U6 VCC, J10–J17 pin 1, J21–J24 pin 1, BZ1 +, R3 (1 k) → LED3 green
  anode (cathode → GND), C2 + (servo 470 µF, not fitted).
- `+3V3`: U1A.1, U1A.2, J10–J17 pin 3, J18.1, J20.1, all 10 k pull-ups to 3V3, I²C 4.7 k pull-ups.
- C1 100 nF: U2.14 ↔ U2.7.
- Probe voltages at the connector pins (there are no separate test points).

**Stations S1–S7** (k = 1…7; Jk' = J10…J16)
- `DHT_Sk`: Jk'.4 → 10 k to +3V3 → 1 k → `DHT_Sk_IO` (U1 pin per §4).
- `MQ_Sk_ADC`: Jk'.5 → U1 pin directly; 100 nF from `MQ_Sk_ADC` to GND (C3…C9 for S1…S7).
- `PROX`: Jk'.6 → zone net (below).
- `Sk_DIN`: Jk'.7. `Sk_DOUT`: Jk'.8. SBk pads between Sk_DIN and Sk_DOUT.

**Station S8** (J17): pins 1/2/3 = +5V/GND/+3V3; pins 4 and 5 not connected; pin 6 → `PROX_Z4`;
pin 7 = `S8_DIN`; pin 8 = `S8_DOUT` → TP5.

**LED chain**
- `LED_DIN` (U1B.15, GPIO45) → U2.2; 10 k U2.2 → GND.
- U2.3 (1Y) → 330 Ω → `S1_DIN`.
- `S1_DOUT` → 220 Ω → `S2_DIN`; … `S7_DOUT` → 220 Ω → `S8_DIN`.

**Proximity zones**
- `PROX_Z1` = J10.6 + J11.6; `PROX_Z2` = J12.6 + J13.6; `PROX_Z3` = J14.6 + J15.6; `PROX_Z4` = J16.6 + J17.6.
- Each `PROX_Zn`: 10 k to +3V3 → 1 k → `PROX_Zn_IO` (U1 pin per §4).

**Servos**
- `SERVO_IN` (U1A.14, GPIO46) → U2.5; 10 k U2.5 → GND.
- U2.6 (2Y) = `SERVO_BUF` → 220 Ω → J21.3; → 220 Ω → J22.3; → 220 Ω → J23.3; → 220 Ω → J24.3.
- U2.1, U2.4 → GND; U2.9, U2.12 → GND; U2.8, U2.11 open.

**Buzzer**: `BUZ_IO` (U1B.17) → 1 k → Q1 base; 10 k Q1 base → GND; Q1 emitter → GND; Q1 collector → `BUZ_N`
→ BZ1 −; BZ1 + → +5V; D1 1N4148 (optional): anode `BUZ_N`, cathode +5V.

**Emergency button**: J20.3 `ESTOP` → 1 k → `ESTOP_IO` (U1B.18); 10 k `ESTOP_IO` → GND.

**I²C**: J18.3 = `I2C_SCL` (U1A.15), J18.4 = `I2C_SDA` (U1A.12); 4.7 k each to +3V3.

**microSD (U6)**: CS = `SD_CS` (U1A.20), SCK = `SD_SCK` (U1A.18), MOSI = `SD_MOSI` (U1A.17),
MISO = `SD_MISO` (U1A.19), VCC = +5V, GND = GND.

---

## 6 · Bill of materials (hand-soldered; LCSC numbers are for footprints — buy locally where cheaper)

| Qty | Refs | Part | LCSC | Notes |
|---|---|---|---|---|
| 2 | J1, J2 | XT60PW-M (male, right-angle) | C98732 | |
| 1 | J4 | XT60PW-F (female, right-angle) | C428722 | |
| 1 | J5 | XT30PW-M (male, right-angle) | C53373776 | |
| 1 | J3 | JST VH 6-pin B6P-VH | C20088586 | + housing VHR-6N + 6 × SVH-21T crimps |
| 8 | J10–J17 | JST XH 8-pin B8B-XH-A(LF)(SN) | C157972 | + XHP-8 housings + crimps |
| 1 | J18 | JST XH 4-pin B4B-XH-A(LF)(SN) | C144395 | |
| 5 | J20–J24 | JST XH 3-pin B3B-XH-A(LF)(SN) | C144394 | |
| 2 | U1 | 1×22 female header 2.54 mm, 8.5 mm | C46595983 | rows 25.40 mm apart |
| 1 | U6 | 1×6 female header 2.54 mm | C2832269 | SD module gets straight male pins |
| 1 | U2 | SN74HCT125N (DIP-14) | C154102 | already owned |
| 1 | — | DIP-14 IC socket | — | same footprint as U2; buy locally |
| 1 | Q1 | NPN TO-92 (SS8050 or 2N2222), E-B-C | C50229519 | check pinout of the part you buy |
| 1 | D1 | 1N4148 (DO-35) | C84412 | optional |
| 1 | BZ1 | 12 mm passive buzzer, 5 V, 7.6 mm pitch | footprint C9900016920 | buy locally |
| 1 each | LED1–LED3 | 5 mm LED red, blue, green | footprint C9900102778 | buy locally |
| 15 | — | 10 kΩ ¼ W axial | C1519255 | pull-ups/downs |
| 14 | — | 1 kΩ ¼ W axial | C1364484 | series, buzzer base, green LED |
| 11 | — | 220 Ω ¼ W axial | C3537012 | LED hops ×7, servos ×4 |
| 1 | — | 330 Ω ¼ W axial | C1369900 | LED data out |
| 2 | — | 2.2 kΩ ¼ W axial | C3454390 | red/blue LEDs |
| 2 | — | 4.7 kΩ ¼ W axial | C1371435 | I²C |
| 8 | C1, C3–C9 | 100 nF ceramic, 5 mm pitch | C342794 | C1 at the 74HCT125; C3–C9 on the MQ inputs |
| 0 (1) | C2 | 470 µF 16 V radial, 8 mm | C109066 | footprint only; fit if servos reset the ESP |
| 0 (7) | SB1–SB7 | 2-pin header 2.54 mm (footprint only) | C492401 | fit only for an empty port |
| 4 | H1–H4 | M3 holes at the corners (plated, no net) | — | + 4 × M3 standoffs; SD module far end on a foam pad / hot glue |

All resistors are Yageo MFR-25 (1 %, ¼ W) footprints, 10.3 mm pitch. Search scripts:
`PCB design/scripts/hub/01…05_*.js` (read-only LCSC library queries, 2026-10-12). The search API returns no stock
figures, so check availability when ordering.

---

## 7 · Board layout (placed 2026-10-12 by `scripts/hub/21_place_pcb.js`)

**132.1 × 99.1 mm (5200 × 3900 mil)**, 2 layers, 1.6 mm FR-4, 1 oz copper, HASL. Origin bottom-left.

```
┌──────────────── DevKit antenna overhangs the top edge ────────────────────┐
│ LED BAT · LED DC  │                 │ E-stop · I²C    │ S4 port │ S5 port │
│ ◄ XT60 battery    │                 │ buzzer · C2     │ S3      │ S6      │
│ ◄ XT60 DC jack    │  DevKit U1A/U1B │                 │ S2      │ S7      │
│   switch J3 (VH-6)│                 │ SD module flat  │ S1      │ S8      │
│ ◄ XT60 → buck     │                 │ over U2 + R4–R10│         │         │
│   servo 1–4       │  USB lane       │ + C1            │  cell = port + DHT pull-up/series,     │
│ ◄ XT30 ← buck     │  (low parts)    │                 │  MQ cap, LED hop, bypass (+ zone R)    │
│ LED 5V            │                 │ card slot ▼     │         │         │
└────────────────────────────────────────────────────────────────────────┘
```
- **Power column** (left edge): J1 battery, J2 DC, J4 → buck, J5 ← buck — all mating faces out of the left edge
  (verified from pad positions); J3 switch harness with its pin table in silkscreen; servo ports J21–J24 stacked
  beside them.
- **DevKit**: U1A (x 1050) and U1B (x 2050) rows exactly 1000 mil apart, pin 1 75 mil below the top edge, so the
  DevKit's top edge is flush with the board and the antenna overhangs it. Only R1/R2 under the DevKit; the USB lane
  below it is empty (low parts only).
- **Middle column**: J20 E-stop + J18 I²C (top), their resistors, buzzer group, C2; the SD header U6 with the module
  lying flat over U2 (74HCT125), C1 and R4–R10 (all lower than the 8.5 mm header), card slot at the bottom edge.
  **No SD support holes** (positions of the module's holes unknown, and parts sit under it) — fix its far end with a
  foam pad or a dab of hot glue.
- **Station block**: identical cells — S1→S4 up column A, S5→S8 down column B (= LED chain order); zones are
  neighbouring pairs Z1 S1+S2, Z2 S3+S4, Z3 S5+S6, Z4 S7+S8. Each cell: XH-8 port, DHT pull-up + series, MQ 100 nF,
  LED hop 220 Ω, SB bypass; odd cells also carry their zone's pull-up + series.
- **Holes**: 4 × M3 (3.2 mm plated, 6 mm pad, no net) at (170, 170), (5030, 170), (170, 3730), (5030, 3730) mil.
- **Silkscreen** (`23_silkscreen.js`, `25_designators.js`): every port's name and per-pin functions, station and zone
  labels, LED-chain order, bypass note, switch pin table, polarity marks, DevKit and SD outlines; checked clear of
  pads (`24_check_silk.js`, `26_check_designators.js`).
- Verified (`22_check_placement.js` + PCB DRC): no overlaps, nothing off the board, holes clear, DRC shows only the
  282 not-yet-routed pads of the 74 nets.

---

## 8 · Checks before ordering
1. Station proximity (IR module at 5 V + Schottky diode): with a 10 k pull-up to 3.3 V on the hub side, the line
   reads ≤ 0.5 V when an object is detected and 3.3 V otherwise; two stations in parallel still read correctly.
2. Rocker: which end tab joins the middle tab in position I (§2.3).
3. Print the PCB at 1:1 and place the real DevKit, SD module, XT60/XT30 plugs and an XH-8 plug on the paper.

## 9 · Bring-up
1. No DevKit, no stations. Battery or bench supply on J1 with the switch in O → nothing lit.
2. Position I → red LED; J4 shows battery voltage; DC pins dead. Position II with the adapter → blue LED; battery pins dead.
3. Check the buck alone: 5.0–5.2 V and correct polarity **before** plugging it into J5. Then green LED, 5 V on every port pin 1.
4. Insert the DevKit (USB flash works with the hub powered or not); 3.3 V on every port pin 3.
5. LED chain with S1 only, then add stations in order. Then each DHT and MQ, each proximity zone (either station of
   a pair triggers it), servos, buzzer, emergency button, SD card, I²C.

---

## 10 · Firmware migration (separate task)
| Function | Old WROOM pin | New S3 pin |
|---|---|---|
| DHT S1–S7 | 14, 16, 17, 27 (4 stations) | 15, 16, 17, 18, 38, 39, 40 |
| MQ-5 S1–S7 (AOUT ÷ 1.68 on the station) | 34, 35, 36, 39 (4 stations) | 4, 5, 6, 7, 1, 2, 10 |
| Proximity zones Z1–Z4 | — | 41, 42, 3, 48 |
| LED data | 15 | 45 |
| Servo | 13 | 46 |
| Buzzer | 4 | 47 |
| microSD SCK / MISO / MOSI / CS | 18 / 19 / 23 / 5 | 12 / 13 / 11 / 14 |
| Emergency button | 21 | 21 |
| I²C SDA / SCL | — | 8 / 9 |

Also needed: `board = esp32-s3-devkitc-1`, octal PSRAM (`qio_opi`), 16 MB flash; LED pixel order follows the
station chain; remove the old input-only-pin logic in `HubSensors` (the S3 has none).

## 11 · Carried over from the working WROOM build
DHT on 3V3 (now via the station cable) · the IR proximity alarm (now on each station board) · 74HCT125 pin-by-pin wiring incl. the 100 nF
and the tied-off spare channels · 330 Ω LED / 4 × 220 Ω servo resistors · 2N2222 buzzer driver · parallel LED power ·
microSD on 5 V · emergency button on GPIO21.
Not carried over: the second buck (one 10 A buck now) and the fuses.
