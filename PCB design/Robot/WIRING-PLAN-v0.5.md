# Museum ROBOT Carrier — Wiring Spec v0.5 (YD-ESP32-S3 N16R8 · 2 × LM386 on a battery rail · RC522 / DFPlayer / L298N / CAM / Mega ports · 100 % through-hole) — SUPERSEDED by WIRING-PLAN.md v0.6

> **Status: SUPERSEDED (2026-10-25) by [`WIRING-PLAN.md`](WIRING-PLAN.md) v0.6 — motor rail straight from the battery, D-029.**
> Kept for history; do **not** build from this file.
> Supersedes v0.4 → [`WIRING-PLAN-v0.4.md`](WIRING-PLAN-v0.4.md), v0.3 → [`WIRING-PLAN-v0.3.md`](WIRING-PLAN-v0.3.md), v0.2 →
> [`WIRING-PLAN-v0.2.md`](WIRING-PLAN-v0.2.md), v0.1 (DevKit V1 + Nano) → [`WIRING-PLAN-v0.1.md`](WIRING-PLAN-v0.1.md).
> Plan of record: `C:\Users\tahfi\.claude\plans\pasted-content-id-5437-task-curious-stardust.md` (v0.5 part-count review;
> v0.4 plan: `how-the-mega-needs-vivid-sundae.md`).
> Decisions: [`DECISIONS.md`](DECISIONS.md) (D-001…). Handoff: [`README.md`](README.md). Process: `pcb-pipeline` skill (`PLAYBOOK.md`).
> Inputs in this folder: [`YD-ESP32-S3 N16R8_footprint.png`](YD-ESP32-S3%20N16R8_footprint.png), [`lm386_amplifier.jpg`](lm386_amplifier.jpg),
> [`robot_wiring diagram.png`](robot_wiring%20diagram.png) (legacy Cirkit build).
> **Retired designators (v0.5): C1, C2, D3, R1, TP1–TP6.** New in v0.5: J15.
>
> **v0.5 changes (2026-10-25, user-approved part-count review; board was placed, not routed):**
> - **D3 1N5819 removed:** J9.1 (Mega 5 V) is now `+5V` directly; the Mega gets the full 5.1 V. Rule: unplug J9 before
>   any USB cable touches the Mega (D-022).
> - **R1 1.1 Ω 1 W removed:** D2's cathode feeds `VAMP_BAT` directly; the diodes + C9 remain the ripple filter (D-023).
> - **C1 1000 µF + C2 100 nF at J4 removed:** the buck has its own capacitors; the loads keep C3 / C5 and their 100 nF (D-024).
> - **Second obstacle port J15** (XH-3, same as J11): two FC-51 modules, each on its own cable, wired-OR on the PCB into
>   the existing `OBST` → GPIO2 input (D-025).
> - **TP1–TP6 deleted:** rails are probed at JP1 / J14 / D1 (§3.9) (D-026).
> - 3V3 stays on the DevKit's own LDO (CJ6107A33GW per the YD V1.4 schematic), budget in §1.3 (D-027). Battery divider
>   re-checked: unchanged.
> - Result: **66 parts, 51 nets** (`MEGA_5V` and `AMP_R` merge away). Board stays 95.0 × 97.5 mm; besides J15 only R14 / R15
>   moved (55 mil down, so J10's pin names fit where the old I²C legend was, D-028).
>
> **User decisions 2026-10-13 / 2026-10-15:**
> - **3S LiPo.** **Two of the team's LM386 modules + two 3 W 4 Ω speakers**, fed from the battery through a small voltage drop
>   (≈ 11 V at full charge). The heat trade-off was shown (TI datasheet: into 4 Ω the LM386 swing flattens above ~9 V) and the
>   user kept the battery rail (D-006); **JP1** lets the amps fall back to the proven 5 V setup without rework.
> - **LM386 modules:** the user re-solders each module's header pins-down; the module plugs flat into a 1×4 socket (D-007).
> - **RC522 on a cable** under the chassis (D-009). **DFPlayer** wired like the proven `esp32-rfid-audio-player` build (D-008).
> - **Mega port XH-4** (5 V + optional TX/RX data link); the I²C port gets series protection instead of a different plug (D-010).
> - **Buck feeds on XT30** (D-005); the user owns both bucks and guarantees the 5 V output.
> - Ports for a **front obstacle sensor**, an **I²C port like the hub's**, and **two buttons** (MUTE + spare) (D-011).
> - **Every part through-hole, never SMD** (D-013). Board fits 100 × 100 mm, smaller if possible, **top side up** (D-014).
>
> **User decisions 2026-10-16 (floor-plan review, §7):**
> - Board **95.0 × 97.5 mm** with the floor plan in §7 (D-019).
> - Code upload and console over the DevKit's **left USB-C port** ("USB", native USB/OTG); the right "COM" port is blocked
>   by C3 / J9 once the DevKit is seated (D-020).
> - **No zip-tie holes**; the DFPlayer's micro-SD card is swapped by lifting the module out of its socket (D-021).

---

## 0 · What this board is

A hand-soldered carrier for **one YD-ESP32-S3 N16R8 DevKit** that runs the whole guide robot: L298N motors, RC522 RFID,
DFPlayer narration through two LM386 amplifier modules, the ESP32-CAM photo trigger, the Mega face-display link, the web
console and the ESP-NOW emergency/mute link. **No logic ICs, no SMD parts:** every part is through-hole and hand-soldered;
the board is ordered bare (no JLC assembly). Modules plug in (DevKit, DFPlayer, LM386 ×2); everything off-board connects
through a keyed connector (XT60/XT30 power, JST-VH switch, JST-XH module cables).

| Decision | Choice | Why |
|---|---|---|
| MCU | YD-ESP32-S3 N16R8 in 2 × 1×22 sockets, rows 25.40 mm (D-001) | same DevKit as the hub; replaces WROOM DevKit + Nano |
| Motors | external L298N on XH-8, LEDC PWM from the S3, 10 k ENA/ENB pull-downs (D-002, D-003) | motor current never crosses the board; bridge off while the S3 boots |
| Power | 3S LiPo → XT60 → rocker (VH-2) → XT30 feeds to two off-board bucks; 5 V back on XT30 (D-004, D-005) | keyed genders: a LiPo can't reach the 5 V rail |
| Audio | DFPlayer (socket) DAC_L/DAC_R → 2 × LM386 module (sockets) on `VAMP` ≈ 11 V; JP1 → 5 V fallback (D-006–D-008, D-023) | user's modules + speakers; louder than 5 V; diode + C9 filter; no rework fallback |
| RFID | RC522 on an XH-7 cable, RST wired, 47 Ω on SPI (D-009) | reader sits 1–3 cm above the floor cards |
| Display | Mega XH-4 = +5V direct + optional UART (D-010, D-022) | face screens can follow the tour; Mega gets the full 5.1 V; **unplug J9 before Mega USB** |
| Expansion | 2 × obstacle XH-3 (J11 + J15, wired-OR), I²C XH-4, MUTE / spare buttons, battery sense, EXP pads (D-011, D-025) | BUILD items 3 and 5, no respin later |
| Protection | none beyond keyed connectors (hub v0.3 precedent) | beginner build; unplug the LiPo when not in use |

### Reference dimensions (caliper before layout)

| Board | Size | Header geometry |
|---|---|---|
| YD-ESP32-S3 N16R8 | 27.94 × 57.15 mm | 2 × 22, 2.54 mm; rows **25.40 mm**; pin 1 at 1.91 mm from the top edge; antenna overhangs the top edge by 6.24 mm |
| DFPlayer Mini | ≈ 20 × 20 mm | 2 × 8, 2.54 mm; rows 18.00 mm (LCSC footprint) vs 17.78 mm (0.7″): **caliper** |
| LM386 module (LC Technology) | **40.9 × 13.9 mm** (photo) | 1 × 4 header `VCC IN GND GND` at one short end; screw terminal `OUT GND` at the other; LM386M-1 (SOIC); no holes |
| RC522 module | ≈ 60 × 40 mm | 1 × 8: SDA SCK MOSI MISO IRQ GND RST 3.3V (on the cable) |
| L298N module | ≈ 43 × 43 mm | 1 × 6 ENA IN1 IN2 IN3 IN4 ENB + screws 12V / GND / 5V |
| ESP32-CAM | 27 × 40 mm | 5V, GND, IO12, IO13 … on one side (link = IO13) |

---

## 1 · Power path

```
3S LiPo ──XT60 female lead──► J1 (XT60PW-M: 1 = −, 2 = +)
   BAT+ ─► J5 VH-2 pin 1 ─► rocker COM ; rocker "I" ─► J5 pin 2 = BAT_SW        BAT− ─► GND (switch breaks + only)

BAT_SW ─┬─► J2 XT30PW-F (1 = +, 2 = −) ──► motor buck (user; set to the motor rating) ──► L298N 12V/GND screws directly
        ├─► J3 XT30PW-F ──► fixed 5 V buck (user) ──► XT30 female lead ──► J4 XT30PW-M (1 = −, 2 = +) = +5V
        ├─► D1 1N4007 ─► D2 1N4007 ─► VAMP_BAT ─► JP1.1          (no series resistor since v0.5, D-023)
        │                                       JP1.2 = VAMP ─► C9 1000 µF 35 V + C10 100 nF ─► U3/U4 pin 1 (both LM386)
        │                         +5V ─► JP1.3   (cap on 1–2 = battery rail [default]; on 2–3 = 5 V)
        ├─► R24 100 k ─┬─► VBAT_SENSE ─► GPIO1 (ADC1, ÷6)    R25 20 k + C11 100 nF to GND
        └─► R26 2.2 k ─► LED1 red ─► GND                        (on whenever the switch is on)

+5V (J4; no capacitor at J4, the buck has its own, D-024) ─► DevKit 5Vin (its CJ6107 1 A LDO makes 3V3) ·
     DFPlayer VCC (C3 470 µF + C4) · CAM port J8 (C5 470 µF + C6) · L298N logic J6.1 · Mega J9.1 (direct, D-022) · JP1.3 ·
     R27 1 k ─► LED2 green
+3V3 (DevKit LDO) ─► RC522 port J7 (C7 10 µF + C8) · I²C J10 + pull-ups · obstacle J11 + J15 · button pull-ups · EXP J14
```

### 1.1 Settings and values
- **`VAMP` (JP1 = BAT):** 2 × 1N4007 (≈ 0.63 V each at idle, ≈ 0.8 V loud), no series resistor (R1 removed in v0.5, D-023):
  | Pack | Idle | Loud speech |
  |---|---|---|
  | 12.6 V (full) | ≈ 11.3 V | ≈ 11.0 V |
  | 11.1 V (nominal) | ≈ 9.8 V | ≈ 9.5 V |
  | 9.9 V (empty) | ≈ 8.6 V | ≈ 8.3 V |
  LM386M-1: 4–12 V recommended, 15 V absolute max (TI SNAS545D §6.1/6.3) — inside the range in every case. The diodes also
  block a reversed supply and act as a hold-up valve (motor dips can't pull `VAMP` down).
- **Motor-ripple filter = the diodes' own resistance + C9** (≈ 2·nV_T/I: ≈ 3.5 Ω at 30 mA idle, ≈ 0.4 Ω at 0.25 A loud). With
  1000 µF (0.16 Ω at 1 kHz): ≈ −27 dB at 1 kHz idle, ≈ −9 dB in loud passages (R1 had added ≈ 2 dB / ≈ 10 dB). The LM386's
  PSRR (50 dB at 1 kHz with a pin-7 bypass) comes on top. **If whine is audible:** JP1 → 5V (the buck's loop rejects the
  motor ripple), or fit 2200 µF 25 V in C9's 13 mm footprint (≈ −34 dB idle / −16 dB loud, about the old filter).
- **Switch-on inrush into C9** without R1: ≈ 40 A peak for ≈ 0.25 ms (≈ 0.25 Ω path), I²t ≈ 0.2 A²s vs the 1N4007's 30 A /
  8.3 ms surge rating (≈ 3.7 A²s): ≈ 15× margin. The rocker sees a similar inrush from the bucks' input capacitors anyway.
- **Heat note (D-006):** into 4 Ω the LM386 output stops growing above ~9 V (≈ 3.7 Vpp, TI Fig. 6-3) while chip dissipation
  keeps rising (Fig. 6-7: ≈ 1.2 W at 12 V vs 0.73 W SOIC rating). Speech at moderate volume is the use case; firmware caps
  the volume at 24; **if a chip is too hot to touch after 10 min, move JP1 to 5 V** (the proven build's supply).
- **`VBAT_SENSE`:** 100 k / 20 k → ÷6: 12.6 V → 2.10 V, 9.0 V → 1.50 V (ADC1, 12 dB). Calibrate two points against a meter.
- **Mega feed (D-022):** J9.1 = `+5V` directly → the Mega and its TFT get the full 5.1–5.2 V. The Mega 2560 R3 joins its USB
  VBUS to its 5V pin through a 500 mA PTC + P-FET whenever VIN < ≈ 6.6 V, so with J9 plugged a laptop and the robot's buck
  would be tied together (buck → laptop ≈ 0.1–0.3 A with the robot on; laptop → whole +5V rail until the PTC trips with
  the robot off). **Unplug J9 before any USB cable touches the Mega** (flashing or serial monitor) and never power the
  Mega from its barrel jack while J9 is plugged.
- **XT polarity:** J1/J4 from the library pin names (`−`/`+`). J2/J3 (XT30PW-F C2913282, unnamed pins) = **pad 1 +, pad 2 −**,
  derived from the Amass drawing (chamfered end = −) and the mating geometry with XT30PW-M (D-015); **meter check at bring-up**.

### 1.2 Power states
| State | What runs |
|---|---|
| J1 unplugged / switch O | nothing |
| switch I, J3 + J4 plugged | 5 V logic, S3, DFPlayer, CAM, Mega, LEDs; amps on `VAMP` |
| J2 unplugged | motors dead, everything else alive (safe bench work) |
| J3/J4 unplugged | 5 V logic off; ENA/ENB pull-downs keep the bridge off; LED1 + amps' supply stay on |
| USB on the DevKit only | S3 + 3V3 loads; the YD 5Vin diode keeps USB out of the +5V rail |

### 1.3 Current budget and runtime
| Load | Typical | Worst case |
|---|---|---|
| ESP32-S3 + Wi-Fi AP + RC522 (3V3 via the DevKit LDO) | 0.3 A @ 5 V | 0.6 A |
| DFPlayer (DAC out) | 0.05 A | 0.1 A |
| ESP32-CAM | 0.15 A | 0.5 A |
| Mega 2560 + 3.2″ TFT | 0.25 A | 0.4 A |
| L298N logic | 0.04 A | 0.05 A |
| **5 V total** | **≈ 0.8 A** | **≈ 1.7 A** → fixed 5 V buck ≥ 3 A |
| 2 × LM386 on `VAMP` | 0.03 A | ≈ 0.5 A peaks |
| Motors (6 V buck) | ≈ 0.6 A | 2–3 A both stalled |
| **Battery side** | **≈ 1 A** | **≈ 3.5–4 A** (VH and one rocker pole are plenty) |

**3V3 rail (D-027):** the DevKit's own LDO (CJ6107A33GW on the YD V1.4 schematic: SOT-223, 1 A, 0.5 V dropout at 1 A; read
the marking on your board, an AMS1117 clone needs ≥ 4.8 V at 5Vin) carries S3 + Wi-Fi AP ≈ 0.13 A, RC522 field ≈ 60 mA
(100 mA max), 2 × FC-51 ≈ 35 mA, I²C module ≤ 20 mA → **≈ 0.25 A average, ≈ 0.55 A peak**. With ≈ 1.4 V across it
(5Vin − its input diode − 3.3 V) that is ≈ 0.35 W → ≈ +32 °C (θJA ≈ 90 °C/W), Tj ≈ 72 °C in a 40 °C robot body. No extra
regulator (two regulators must never share the DevKit's 3V3 pins).

2200 mAh 3S ≈ 1.5–2 h of touring; 5200 mAh ≈ 4 h.

### 1.4 Low-battery behaviour
Firmware reads `VBAT_SENSE`: warn at 10.8 V, stop the tour at 10.5 V (3.5 V/cell). Unplug the LiPo when not in use.

---

## 2 · Ports and harnesses

### 2.1 Port table
| Ref | Port | Board part (LCSC) | Pins 1 → n |
|---|---|---|---|
| J1 | Battery in | XT60PW-M (C98732) | 1 −, 2 + (3/4 tabs) |
| J2 | Motor-buck feed | XT30PW-F (C2913282) | 1 + (BAT_SW), 2 − (3/4 tabs) |
| J3 | 5 V-buck feed | XT30PW-F (C2913282) | 1 +, 2 − |
| J4 | 5 V ← buck | XT30PW-M (C53373776) | 1 −, 2 + (3/4 tabs) |
| J5 | Power switch | JST VH B2P-VH (C485345) | 1 BAT+ (rocker COM), 2 BAT_SW (rocker "I") |
| J6 | L298N | JST XH B8B-XH-A (C157972) | 5V, GND, ENA, IN1, IN2, IN3, IN4, ENB |
| J7 | RC522 (cable) | JST XH B7B-XH-A (C144398) | 3V3, GND, SS, SCK, MOSI, MISO, RST |
| J8 | ESP32-CAM | JST XH B3B-XH-A (C144394) | 5V, GND, CAM_TX |
| J9 | Mega face display | JST XH B4B-XH-A (C144395) | +5V (Mega 5V, no diode), GND, TX→RX1, RX←TX1 |
| J10 | I²C | JST XH B4B-XH-A (C144395) | 3V3, GND, SCL, SDA |
| J11 | Front obstacle sensor 1 (FC-51) | JST XH B3B-XH-A (C144394) | 3V3, GND, SIG (`OBST`) |
| J15 | Front obstacle sensor 2 (FC-51) | JST XH B3B-XH-A (C144394) | 3V3, GND, SIG (`OBST`, wired-OR with J11.3) |
| J12 / J13 | MUTE / spare panel button | JST XH B2B-XH-A (C378939) | SIG, GND |
| J14 | EXP pads (not fitted, not in BOM) | 1×4 header PZ254V-11-04P (C492403) | 3V3, GND, GPIO3, GPIO38 |
| U1A / U1B | YD-ESP32-S3 sockets | 1×22 female PM254V-11-22-H85 (C46595983) | §4 |
| U2A / U2B | DFPlayer socket | 1×8 female PM254V-11-08-H85 (C46595973) | §3.2 |
| U3 / U4 | LM386 module L / R | 1×4 female PM254V-11-04-H85 (C541851) | 1 VCC (VAMP), 2 IN, 3 GND, 4 GND |
| JP1 | Amp supply select | 1×3 header PZ254V-11-03P (C492402) + jumper cap | 1 VAMP_BAT, 2 VAMP, 3 +5V |
| SW1 / SW2 | MUTE / spare | 6×6 THT tact K2-1102DP-J4SW-04 (C136680) | pads 1 + 3 used (diagonal) |

### 2.2 Rules
1. **One family per job:** XT60 battery, XT30 buck feeds (female) and 5 V return (male), VH switch, XH every module cable.
   Genders make every wrong power mate impossible.
2. **XH pin 1 = supply, pin 2 = GND** (buttons: SIG, GND). Shared pin counts and why a swap is harmless:
   - XH-3 CAM (5 V) ↔ obstacle J11 / J15 (3V3): the CAM browns out, or the sensor runs at 5 V into a 1 k-protected S3
     output.
   - XH-4 Mega ↔ I²C: a Mega plug in J10 drives SDA through R17 1 k (≤ 1.4 mA into the clamp); an I²C board in J9 sees
     ≈ 5.1 V (fine for boards with a regulator) — silkscreen **"MEGA 5V"** vs **"I²C 3.3V"**.
3. **`VAMP` (≈ 11 V) never leaves the board** (U3/U4 sockets, JP1).
4. **Unplug J8 before flashing the ESP32-CAM.** The S3 flashes over USB-C with the robot on or off. **Unplug J9 before any
   USB cable touches the Mega** (flashing or serial monitor), and never power the Mega from its barrel jack while J9 is
   plugged: J9.1 is the robot's +5V with no diode (D-022).
5. **Speakers connect to the LM386 modules' own screw terminals** (`OUT`, `GND`, ground-referenced). DFPlayer SPK1/SPK2 are
   not connected on this board.
6. **LM386 modules lie flat**, header pins down into U3/U4, body over the silkscreen outline, screw terminals at the board edge,
   far end on foam. Turned round, a module would put `VAMP` on its GND pin: the outline shows the only orientation.

### 2.3 Harnesses (board end → far end)
| Cable | Board end | Far end |
|---|---|---|
| Battery | — | LiPo XT60 female lead → J1 (optional inline blade fuse, off-board) |
| Power switch | VHR-2N + 2 × SVH-21T | 6.3 mm spades on rocker **COM** (pin 1) and the tab that closes in **"I"** (pin 2); meter first |
| Motor buck | — | input lead **XT30 male** → J2; output hard-wired to the L298N 12V/GND screws, twisted |
| 5 V buck | — | input lead **XT30 male** → J3; output lead **XT30 female** → J4 |
| L298N | XHP-8 | 1 → `5V` screw, 2 → `GND` screw, 3–8 → 1×6 Dupont on ENA…ENB. **Remove the ENA, ENB and 5V_EN jumpers** |
| RC522 | XHP-7 | 1×8 Dupont: SDA ← 3, SCK ← 4, MOSI ← 5, MISO ← 6, IRQ empty, GND ← 2, RST ← 7, 3.3V ← 1 · ≤ 25 cm |
| ESP32-CAM | XHP-3 | 1×4 Dupont on 5V, GND, IO12 (empty), IO13: 1, 2, –, 3 |
| Mega | XHP-4 | built by the user from the back of the Mega's pins: 1 → 5 V, 2 → GND, 3 → RX1 (19), 4 → TX1 (18) |
| I²C | XHP-4 | module VCC, GND, SCL, SDA (check the module's own order) |
| Obstacle × 2 | XHP-3 each (J11, J15) | one cable per FC-51 module: VCC ← 1, GND ← 2, OUT ← 3 (module prints `OUT GND VCC`) |
| Buttons | XHP-2 | NO push button across the two wires |
| Speakers | — | LM386 screw terminal → speaker (optional inline 2-pin plug) |

### 2.4 Module pinouts used
- **DFPlayer Mini:** left column 1 VCC, 2 RX, 3 TX, 4 DAC_R, 5 DAC_L, 6 SPK1, 7 GND, 8 SPK2; right column (pin 16 opposite
  pin 1) 16 BUSY, 15 USB−, 14 USB+, 13 ADKEY2, 12 ADKEY1, 11 IO2, 10 GND, 9 IO1 (DFRobot DFR0299; legacy diagrams).
- **LM386 module:** `VCC IN GND GND` header, `OUT GND` terminal, LM386M-1, 10 k input pot, gain 200 with R1 = 0 Ω or 20 with
  R1 empty (protosupplies; LC Technology photo).
- **RC522:** `SDA SCK MOSI MISO IRQ GND RST 3.3V`. **L298N:** `ENA IN1 IN2 IN3 IN4 ENB`; ENA/ENB jumpers tie to +5 V only
  through the jumper (teardown). **ESP32-CAM:** IO13 = robot link RX (firmware/cam).

---

## 3 · On-board circuits

### 3.1 Audio amplifier rail and LM386 modules
`VAMP` per §1. U3 (left) IN ← `DAC_L`, U4 (right) IN ← `DAC_R` (the DFPlayer's DAC pins already carry series coupling
capacitors on the module). Keep DAC traces short over solid GND; U3/U4 GND pins return next to the DFPlayer GND pins.
Volume chain: firmware 0–30 (web console, Hub mute, MUTE button) → module pot → speaker. Recommended: check each module's
R1 (gain 200 if a 0 Ω is fitted; remove it for gain 20 = less hiss).

### 3.2 DFPlayer Mini (U2A = DF pins 1–8, U2B = DF pins 16…9) — UART1, as the proven build
| DF pin | Socket pin | Net | Connection |
|---|---|---|---|
| 1 VCC | U2A.1 | +5V | C3 470 µF + C4 100 nF at the socket |
| 2 RX | U2A.2 | `S3_TO_DF` | ← R7 1 k ← GPIO14 (UART1 TX) |
| 3 TX | U2A.3 | `DF_TO_S3` | → R8 1 k → GPIO17 (UART1 RX) — no divider: TX drove an ESP32 RX directly in the proven build |
| 4 DAC_R | U2A.4 | `DAC_R` | → U4.2 |
| 5 DAC_L | U2A.5 | `DAC_L` | → U3.2 |
| 6 SPK1, 8 SPK2 | U2A.6, U2A.8 | — | not connected |
| 7 GND, 10 GND | U2A.7, U2B.7 | GND | |
| 16 BUSY | U2B.1 | `DF_BUSY` | → R9 1 k → GPIO8 (LOW while playing) |
| 15, 14, 13, 12, 11, 9 | U2B.2–6, U2B.8 | — | not connected |

### 3.3 RC522 port (J7)
SS GPIO10 → R4 47 Ω, SCK GPIO12 → R5 47 Ω, MOSI GPIO11 → R6 47 Ω, MISO → GPIO13 direct, RST GPIO9 direct; C7 10 µF + C8 100 nF
at J7. GPIO10–13 = the S3's default FSPI pins (U1A.15–19 side by side). MFRC522v2 runs SPI at 4 MHz; drop to 1 MHz in
firmware if a long cable misbehaves. 3.3 V only.

### 3.4 Motor port (J6)
ENA GPIO4 / ENB GPIO16 with **R2 / R3 10 k to GND** (bridge disabled while the S3 resets, flashes, crashes or is pulled out);
IN1 GPIO5, IN2 GPIO6, IN3 GPIO7, IN4 GPIO15 direct. ESP32-S3 datasheet v2.2: GPIO4–7 only glitch LOW (60 µs) at power-up,
GPIO15/16 don't glitch, none has a boot pull-up. L298N VIH 2.3 V ≤ S3 VOH 2.64 V. Channel A = left motor ("150"), B = right.

### 3.5 ESP32-CAM port (J8)
`CAM_TX`: GPIO18 (UART2 TX, 9600) → R10 1 k → J8.3 → CAM IO13; GPIO18's power-up HIGH glitch = UART idle. C5 470 µF + C6 at J8.

### 3.6 Mega port (J9)
J9.1 = `+5V` directly (no diode since v0.5, D-022 — unplug J9 before any USB cable touches the Mega). `MEGA_TX`: GPIO41 → R11 1 k → J9.3 → Mega RX1. `MEGA_RX`: J9.4 (Mega TX1, 5 V) →
R12 10 k → `MEGA_RX_IO` → GPIO42, R13 20 k to GND (5 V → 3.33 V). The display firmware listens at 9600 on Serial1.

### 3.7 I²C port (J10)
R14 / R15 4.7 k pull-ups to +3V3 on the connector side (`I2C_SCL`, `I2C_SDA`); **R16 / R17 1 k series** to GPIO39 / GPIO40
(`I2C_SCL_IO`, `I2C_SDA_IO`). Low level at the bus with the S3 driving: 3.3 × 1/5.7 = 0.58 V < 0.99 V (0.3 × VDD). 3.3 V only.

### 3.8 Inputs — obstacle and buttons (active LOW)
`OBST` (J11.3 + J15.3, wired-OR on the PCB, D-025): R18 10 k to +3V3, R19 1 k → `OBST_IO` GPIO2 (ADC1), C12 100 nF to
GND. FC-51 outputs are LM393 open-collector with their own 10 k pull-up, so either module pulling LOW = obstacle and the
two can never fight. With both plugged in: 10 k ∥ 10 k ∥ R18 = 3.3 k; the detecting LM393 sinks ≈ 4 mA (pull-ups + both
modules' indicator LEDs, which light together) → VOL ≤ 0.4 V (TI LM393: 0.4 V max at 4 mA) < VIL 0.825 V. Each module
draws ≈ 15–20 mA from +3V3; at 3.3 V their range is shorter than at 5 V, so trim each pot on the robot. `BTN1` (J12.1 + SW1 pad 1): R20 10 k
pull-up, R21 1 k → `BTN1_IO` GPIO21, C13 100 nF. `BTN2` (J13.1 + SW2 pad 1): R22 / R23 → `BTN2_IO` GPIO47, C14. SW1/SW2 pad 3 →
GND; pads 2/4 not connected (diagonal pads 1/3 are always on different contacts, whichever pairs are joined inside, D-016).

### 3.9 Indicators, probe points, spare pads
LED1 red (+ = `LED1_A` via R26 2.2 k from BAT_SW, ≈ 4.8 mA), LED2 green (+ = `LED2_A` via R27 1 k from +5V, ≈ 3 mA).
**No test points** (TP1–TP6 deleted in v0.5, D-026). Probe the rails here:
| Rail | Probe at |
|---|---|
| `BAT_SW` | D1 anode lead, or the "+" contact of J2 / J3 |
| `VAMP_BAT` / `VAMP` / `+5V` | JP1 pin 1 / pin 2 / pin 3 |
| `+3V3` / `GND` | J14 pad 1 / pad 2 |

J14 EXP pads: +3V3, GND, GPIO3, GPIO38.
The DevKit's RGB LED (GPIO48) stays free for firmware status colours.

---

## 4 · DevKit pin map (U1)

U1A = left header, U1B = right header, pin 1 at the antenna end (hub numbering).

| U1A | GPIO | Net | | U1B | GPIO | Net |
|---|---|---|---|---|---|---|
| 1 | 3V3 | +3V3 | | 1 | GND | GND |
| 2 | 3V3 | +3V3 | | 2 | 43 | NC (TX0 → CH343) |
| 3 | RST | NC | | 3 | 44 | NC (RX0 → CH343) |
| 4 | 4 | ENA | | 4 | 1 | VBAT_SENSE (ADC1) |
| 5 | 5 | IN1 | | 5 | 2 | OBST_IO (ADC1) |
| 6 | 6 | IN2 | | 6 | 42 | MEGA_RX_IO |
| 7 | 7 | IN3 | | 7 | 41 | MEGA_TX_IO |
| 8 | 15 | IN4 | | 8 | 40 | I2C_SDA_IO |
| 9 | 16 | ENB | | 9 | 39 | I2C_SCL_IO |
| 10 | 17 | DF_TO_S3_IO (UART1 RX) | | 10 | 38 | EXP_IO38 |
| 11 | 18 | CAM_TX_IO (UART2 TX) | | 11 | 37 | NC (octal PSRAM) |
| 12 | 8 | DF_BUSY_IO | | 12 | 36 | NC (octal PSRAM) |
| 13 | 3 | EXP_IO3 | | 13 | 35 | NC (octal PSRAM) |
| 14 | 46 | NC (strapping) | | 14 | 0 | NC (BOOT) |
| 15 | 9 | RFID_RST | | 15 | 45 | NC (strapping) |
| 16 | 10 | RFID_SS_IO | | 16 | 48 | NC (onboard RGB) |
| 17 | 11 | RFID_MOSI_IO | | 17 | 47 | BTN2_IO |
| 18 | 12 | RFID_SCK_IO | | 18 | 21 | BTN1_IO (MUTE) |
| 19 | 13 | RFID_MISO | | 19 | 20 | NC (USB D+) |
| 20 | 14 | S3_TO_DF_IO (UART1 TX) | | 20 | 19 | NC (USB D−) |
| 21 | 5V | +5V | | 21 | GND | GND |
| 22 | GND | GND | | 22 | GND | GND |

UARTs: UART1 = DFPlayer (14/17), UART2 = CAM (18, TX only), UART0 = Mega (41/42) with the console on the native-USB port.
Nothing drives a strapping pin (0/3/45/46) at reset; GPIO3 only reaches the unpopulated EXP pad.

---

## 5 · Connection list (schematic source of truth)

`Ref.pin` = symbol pin number (dumped / read from the library symbol files, `scripts/robot/09_symbol_fp_files.js`).
Resistors and 100 nF capacitors are non-polar (pin 1 = first net listed). Polar pins by **name**: diodes 1 = K, 2 = A;
LEDs 1 = +, 2 = −; electrolytics 1 = + (silk "+" beside pad 1), 2 = −.

**Power**
- `BAT+`: J1.2, J5.1.
- `BAT_SW`: J5.2, J2.1, J3.1, D1.2 (A), R24.1, R26.1.
- `AMP_D`: D1.1 (K), D2.2 (A). `VAMP_BAT`: D2.1 (K), JP1.1.
- `VAMP`: JP1.2, C9.1 (+), C10.1, U3.1, U4.1.
- `+5V`: J4.2, U1A.21, U2A.1, C3.1 (+), C4.1, J6.1, J8.1, C5.1 (+), C6.1, J9.1, JP1.3, R27.1.
- `+3V3`: U1A.1, U1A.2, J7.1, C7.1 (+), C8.1, J10.1, R14.1, R15.1, J11.1, J15.1, R18.1, R20.1, R22.1, J14.1.
- `GND`: J1.1, J2.2, J3.2, J4.1, U1A.22, U1B.1, U1B.21, U1B.22, U2A.7, U2B.7, U3.3, U3.4, U4.3, U4.4, J6.2, J7.2, J8.2,
  J9.2, J10.2, J11.2, J12.2, J13.2, J14.2, J15.2, SW1.3, SW2.3, C3.2, C4.2, C5.2, C6.2, C7.2, C8.2, C9.2, C10.2,
  C11.2, C12.2, C13.2, C14.2, R2.2, R3.2, R13.2, R25.2, LED1.2 (−), LED2.2 (−).
- `VBAT_SENSE`: R24.2, R25.1, C11.1, U1B.4. `LED1_A`: R26.2, LED1.1 (+). `LED2_A`: R27.2, LED2.1 (+).

**Motor (J6)**: `ENA` U1A.4, J6.3, R2.1 · `IN1` U1A.5, J6.4 · `IN2` U1A.6, J6.5 · `IN3` U1A.7, J6.6 · `IN4` U1A.8, J6.7 ·
`ENB` U1A.9, J6.8, R3.1.

**RFID (J7)**: `RFID_SS_IO` U1A.16, R4.1 · `RFID_SS` R4.2, J7.3 · `RFID_SCK_IO` U1A.18, R5.1 · `RFID_SCK` R5.2, J7.4 ·
`RFID_MOSI_IO` U1A.17, R6.1 · `RFID_MOSI` R6.2, J7.5 · `RFID_MISO` J7.6, U1A.19 · `RFID_RST` U1A.15, J7.7.

**DFPlayer + audio**: `S3_TO_DF_IO` U1A.20, R7.1 · `S3_TO_DF` R7.2, U2A.2 · `DF_TO_S3` U2A.3, R8.1 · `DF_TO_S3_IO` R8.2,
U1A.10 · `DF_BUSY` U2B.1, R9.1 · `DF_BUSY_IO` R9.2, U1A.12 · `DAC_R` U2A.4, U4.2 · `DAC_L` U2A.5, U3.2.

**CAM (J8)**: `CAM_TX_IO` U1A.11, R10.1 · `CAM_TX` R10.2, J8.3.

**Mega (J9)**: `MEGA_TX_IO` U1B.7, R11.1 · `MEGA_TX` R11.2, J9.3 · `MEGA_RX` J9.4, R12.1 · `MEGA_RX_IO` R12.2, R13.1, U1B.6.

**I²C (J10)**: `I2C_SCL` J10.3, R14.2, R16.1 · `I2C_SCL_IO` R16.2, U1B.9 · `I2C_SDA` J10.4, R15.2, R17.1 · `I2C_SDA_IO` R17.2, U1B.8.

**Inputs**: `OBST` J11.3, J15.3, R18.2, R19.1 · `OBST_IO` R19.2, C12.1, U1B.5 · `BTN1` J12.1, SW1.1, R20.2, R21.1 · `BTN1_IO` R21.2,
C13.1, U1B.18 · `BTN2` J13.1, SW2.1, R22.2, R23.1 · `BTN2_IO` R23.2, C14.1, U1B.17.

**Spare**: `EXP_IO3` U1A.13, J14.3 · `EXP_IO38` U1B.10, J14.4.

**No-connects (flag NC):** J1.3, J1.4, J2.3, J2.4, J3.3, J3.4, J4.3, J4.4 (mounting tabs) · U1A.3, U1A.14 · U1B.2, U1B.3,
U1B.11–U1B.16, U1B.19, U1B.20 · U2A.6, U2A.8 · U2B.2–U2B.6, U2B.8 · SW1.2, SW1.4, SW2.2, SW2.4.

51 nets, 66 parts (+ the A4 frame); 189 wired pins. v0.5: `MEGA_5V` merged into `+5V`, `AMP_R` into `VAMP_BAT`.

---

## 6 · Bill of materials (LCSC verified live 2026-10-16, `scripts/robot/07_parts_verify_v04.js`: 32 part types, **0 SMD**, 0 missing; v0.5 drops 4 of those types — 1N5819, 1.1 Ω 1 W, 1000 µF 16 V, 1-pin header — and J15 reuses J11's C144394: 28 types, all verified)

Hand-soldered, bare PCB order: LCSC numbers fix the **footprints**; buy locally where cheaper. All "Extended" classes are
irrelevant here (no JLC assembly).

| Qty | Refs | Part | LCSC | Footprint |
|---|---|---|---|---|
| 1 | J1 | XT60PW-M | C98732 | CONN-TH_XT60PW-M |
| 2 | J2, J3 | XT30PW-F | C2913282 | CONN-TH_XT30PW-F-1 |
| 1 | J4 | XT30PW-M | C53373776 | CONN-TH_XT30PW-M |
| 1 | J5 | B2P-VH + VHR-2N + 2 × SVH-21T | C485345 | CONN-TH_P3.96_B2P-VH |
| 1 | J6 | B8B-XH-A + XHP-8 | C157972 | CONN-TH_B8B-XH-A |
| 1 | J7 | B7B-XH-A + XHP-7 | C144398 | CONN-TH_B7B-XH-A |
| 3 | J8, J11, J15 | B3B-XH-A + XHP-3 | C144394 | CONN-TH_3P-P2.50 |
| 2 | J9, J10 | B4B-XH-A + XHP-4 | C144395 | CONN-TH_B4B-XH-A |
| 2 | J12, J13 | B2B-XH-A + XHP-2 | C378939 | CONN-TH_2P-P2.50 |
| 0 (1) | J14 | 1×4 header PZ254V-11-04P (pads only) | C492403 | HDR-TH_4P-P2.54-V-M |
| 2 | U1A, U1B | 1×22 female PM254V-11-22-H85 | C46595983 | HDR-TH_22P-P2.54-V-F |
| 2 | U2A, U2B | 1×8 female PM254V-11-08-H85 | C46595973 | HDR-TH_8P-P2.54-V-F |
| 2 | U3, U4 | 1×4 female PM254V-11-04-H85 | C541851 | HDR-TH_4P-P2.54-V-F |
| 1 | JP1 | 1×3 header PZ254V-11-03P + jumper cap | C492402 | HDR-TH_3P-P2.54-V-M |
| 2 | SW1, SW2 | 6×6 tact K2-1102DP-J4SW-04 | C136680 | KEY-TH_4P-L6.0-W6.0-P4.50-LS6.5 |
| 2 | D1, D2 | 1N4007 | C108355 | DO-41 |
| 2 | LED1, LED2 | 5 mm LED red, green (buy locally) | footprint C9900102778 | LED-TH_BD5.6-P2.54 |
| 3 | R4–R6 | 47 Ω MFR-25 | C3402507 | RES-TH 10.3 mm |
| 11 | R7–R11, R16, R17, R19, R21, R23, R27 | 1 kΩ MFR-25 | C1364484 | RES-TH 10.3 mm |
| 6 | R2, R3, R12, R18, R20, R22 | 10 kΩ MFR-25 | C1519255 | RES-TH 10.3 mm |
| 2 | R13, R25 | 20 kΩ MFR-25 | C1364283 | RES-TH 10.3 mm |
| 2 | R14, R15 | 4.7 kΩ MFR-25 | C1371435 | RES-TH 10.3 mm |
| 1 | R24 | 100 kΩ MFR-25 | C1364475 | RES-TH 10.3 mm |
| 1 | R26 | 2.2 kΩ MFR-25 | C3454390 | RES-TH 10.3 mm |
| 8 | C4, C6, C8, C10–C14 | 100 nF ceramic | C342794 | CAP-TH P5.00 |
| 2 | C3, C5 | 470 µF 16 V | C109066 | CAP-TH_BD8.0-P3.50 |
| 1 | C7 | 10 µF 50 V | C503219 | CAP-TH_BD5.0-P2.00 |
| 1 | C9 | 1000 µF 35 V (2200 µF 25 V fits, §1.1) | C2960416 | CAP-TH_BD13.0-P5.00 |
| 4 | H1–H4 | M3 holes (PCB only) | — | — |

Totals: 66 schematic parts = 26 R + 12 C + 2 D + 2 LED + 2 SW + JP1 + 15 J + 6 sockets; 1 of them is pads only
(J14, `AddIntoBom = false`). Off-board: LiPo, rocker, 2 bucks, L298N, RC522,
DFPlayer, 2 × LM386 module, 2 × speaker, ESP32-CAM, Mega + TFT, 2 × FC-51 obstacle sensor, panel buttons, XH/VH housings
and crimps.

---

## 7 · Board layout

- 2 layers, 1.6 mm FR-4, 1 oz, HASL. **95.0 × 97.5 mm** (3740 × 3840 mil, D-019; hard limit 100 × 100). **All parts on the top
  side**, which faces up on the robot; bottom layer = GND.
- **Antenna:** DevKit antenna end at a board edge (it overhangs); no pour, vias or power tracks under the antenna end + a 5 mm
  strip (attendance D-028); no XT connector, electrolytic, LM386 module or JP1 within 15 mm.
- **USB lane:** a 17 mm strip from the DevKit's **left** USB-C port ("USB", native, ≈ 7.7 mm from the DevKit's left edge) to
  the bottom edge holds only parts ≤ 5.5 mm (the plug passes ≈ 10 mm up). The right "COM" port is blocked by design (D-020).
- **Height classes** (the DevKit and DFPlayer sit ≈ 11 mm up on their sockets, the LM386 modules ≈ 11 mm at the socket end):
  under the LM386 modules only **≤ 3 mm** parts (0.25 W resistors, DO-41 diodes), never over the foam spots at their far
  ends; under the DevKit / DFPlayer and in the USB lane **≤ 5.5 mm** (FG28 ceramics 5.5 mm). J14, JP1 and the switches
  always stay reachable.
- **Power column** at the right edge, top → bottom J1 (battery), J2 (motor buck), J3 (5 V buck), J4 (5 V in), mating faces
  outward; J5 left of J1. `BAT_SW` J5 → J2 → J3 / D1 ≥ 3 mm in a corridor kept free of other pads; `+5V` spine ≥ 2 mm from J4
  under both LM386 modules to the DevKit; `VAMP` ≥ 1 mm; signals 0.3–0.4 mm.
- **Audio block:** DFPlayer 1 mm right of the DevKit (UART / BUSY pins face the S3), U4 / U3 below it, `VAMP` block (D1, D2,
  JP1, C9, C10) on its right; LM386 outlines 40.9 × 13.9 mm with the screw-terminal end at the bottom edge.
- **Floor plan** (`scripts/robot/layout_plan.py`, D-019): left edge J7 RC522 · C7 / C8 · J14 EXP · J6 L298N; top edge right of
  the DevKit (≥ 15 mm from the antenna) J11 obstacle 1 · J10 I²C with R14–R19 / C12 · **J15 obstacle 2 in the top-right
  corner** (3225, H − 105, between J10 and H4, above J1; v0.5); bottom-left SW1 / SW2 + J12 / J13, J9 Mega, C3;
  bottom-right J8 CAM + C5 / C6, LED1 / LED2; under the DevKit R2 / R3 / R7 / R10, the VBAT divider R24 / R25 / C11 and
  C13 / C14; under the DFPlayer R8 / R9 / C4 / C10; under the LM386 modules R11–R13, R20–R23, R26 / R27; RFID series
  resistors R4–R6 in the USB lane.
- **v0.5 (2026-10-25):** C1, C2, D3, R1 and TP1–TP6 leave their spots empty; J15 is the only added part; R14 / R15 moved
  55 mil down (J10's pin names now sit under its pads, D-028); every other part kept its v0.4 position, so the v0.4
  placement review still holds outside the top-right corner.
- **Holes:** H1–H4 M3 (3.2 mm plated, 6 mm pad, no net) 4 mm from the corners. No zip-tie holes (D-021).
- **Silkscreen** (drawn 2026-10-16, `silk_plan.py`; regenerated for v0.5, then enlarged to 0.8–1.1 mm plain-English labels with
  every passive's value inside its outline on 2026-10-25): every port named with its pin functions;
  "3.3V ONLY" on J7 / J10 / J11 / J15; "OBST 1" / "OBST 2" (J15's upright between J10 and J15); J10's pin names under its
  pads and "I2C 3.3V ONLY" upright between J11 and J10; "MEGA 5V"; "UNPLUG J9 FOR MEGA USB" upright in D3's old spot;
  "+"/"−" at J1–J4 contacts; JP1 "BAT | AMP | 5V"; LM386 outlines with VCC marks, FOAM spots and SPK ends; DFPlayer
  outline + pin-1 mark; DevKit outline with the USB port and antenna ends; "MOTOR BUCK (OUT=STOP)" at J2;
  "L298N: REMOVE ENA ENB 5V-EN JUMPERS" at J6; rail names at the LEDs; title "v0.5".
- Placement is planned offline (`layout_plan.py`), silkscreen with `silk_plan.py` (pcb-pipeline P5).

---

## 8 · Checks before ordering
1. Calipers: S3 rows (25.40 mm) and DFPlayer rows (18.00 vs 17.78 mm).
2. LM386 modules: header re-soldered pins-down; confirm `VCC IN GND GND` order against U3/U4.
3. XT30 female: with a real XT30PW-F, confirm the molded "+" is the contact the footprint calls pad 1 (D-015).
4. Rocker continuity (COM ↔ which tab in "I"). L298N header order.
5. Print 1:1 and lay every module, XT plug and XH plug on the paper (USB lane, harness reach, LM386 outlines, both FC-51
   cables to J11 / J15).
6. FC-51 modules (× 2): confirm the printed `OUT GND VCC` order before crimping their XH-3 cables.

## 9 · Bring-up
1. No DevKit, no modules. Bench supply 12.6 V / 0.5 A on J1. Switch O → nothing lit. Switch I → LED1; **meter J2/J3: pad
   marked + = 12.6 V vs GND** (and agrees with the molded "+"); `VAMP` at **JP1 pin 2** ≈ 11.3 V with JP1 = BAT (≤ 12 V),
   ≈ 5 V with JP1 = 5V; `VBAT_SENSE` ≈ 2.1 V. (Probe points: §3.9.)
2. Each buck alone first (motor 6.0 V ±0.2, 5 V 5.1–5.2 V, polarity), then into J2/J3/J4. LED2; 5.1–5.2 V on J6.1, J8.1 and
   J9.1 (J9.1 is +5V directly since v0.5).
3. DevKit in, flash the migrated firmware over the **left USB-C port** ("USB", native; D-020); 3.3 V on J7.1, J10.1,
   J11.1, J15.1, J14.1; web console up.
4. L298N on J6, wheels up: START / STOP. **DevKit out while driving → both channels drop; ENA/ENB < 0.8 V.**
5. RC522 on its cable (reads; RST toggle re-inits). DFPlayer (`v`, tracks, `DF_BUSY` LOW while playing).
6. LM386 modules (pots low), both speakers; 10 min at volume 24 → chips touchable, else JP1 → 5V.
7. CAM, Mega (`FACE`, `STATION 3 0`; **J9 unplugged whenever a USB cable is on the Mega**, D-022), I²C scan, obstacle
   (cover each FC-51 in turn → `OBST_IO` LOW and both module LEDs light; trim each pot for range at 3.3 V), MUTE, BTN2.
8. Full 10-min tour: no brown-outs, no whine in the speakers (if there is: JP1 → 5V, or 2200 µF 25 V in C9, D-023),
   battery reading within 0.1 V after calibration, and the DevKit's LDO (SOT-223 beside its USB ports) still touchable
   (D-027).

## 10 · Firmware notes (separate task)
- Platform: `esp32-s3-devkitc-1`, `qio_opi`, 16 MB, `-DBOARD_HAS_PSRAM`, `-DARDUINO_USB_CDC_ON_BOOT=1` (console on the native
  USB port → UART0 free for the Mega).
- **Upload and console both run over the native port** (left USB-C, "USB"; the "COM" port is blocked on the carrier, D-020).
  Keep the default USB mode ("Hardware CDC and JTAG") so the upload tool resets the S3 into download mode by itself. If a
  sketch ever hangs the native USB: hold BOOT, tap RST, release BOOT, then upload. OTA (as on the hub) works too. Leave the
  YD board's IN-OUT and USB-OTG solder jumpers open (hub rule): the 5Vin diode then keeps the laptop's USB and the
  carrier's +5V apart.
- Pins (old → new): RC522 SS/SCK/MOSI/MISO/RST 5/18/23/19/– → 10/12/11/13/9 · DFPlayer TX/RX/BUSY 17/16/– → 14/17/8 (UART1,
  `begin(serial, true, true)`) · CAM TX 14 (SoftwareSerial) → 18 (UART2) · Mega TX/RX –/– → 41/42 (UART0) · L298N ENA/IN1/IN2/
  IN3/IN4/ENB (Nano 10/8/7/11/12/5) → 4/5/6/7/15/16 · I²C SDA/SCL 40/39 · obstacle 2 · MUTE 21 · BTN2 47 · battery 1 (× 6).
- **Obstacle (v0.5, D-025):** one input for both FC-51s (wired-OR on the PCB): GPIO2, `INPUT` (external pull-up R18), LOW =
  obstacle, ≈ 30 ms debounce; report it with `FLAG_OBSTACLE_DETECTED` (already in `firmware/common/MuseumProtocol.h`).
  No pin changes from the v0.5 review. The robot firmware is still the WROOM sketch in `ESP32_RFID_MP3_RAIL/` (there is no
  `firmware/robot/` yet), so these are migration notes only.
- **Battery:** average ≈ 16 ADC1 samples (12 dB attenuation) and judge warn / stop with the motors idle or with
  hysteresis: 10.8 V and 10.5 V are only 50 mV apart at the pin, and ±1 % resistors give ≈ ±0.18 V at 10.5 V until the
  two-point calibration removes it.
- Motor subsystem absorbs the Nano (v0.2 §11.3): LEDC 1 kHz 8-bit, `brake()` first in `setup()`, ML/MR in NVS, UART layer deleted.
- New: BUSY as a second "narration finished" signal; MUTE button = local latch OR-ed with the Hub mute; **volume cap 24**;
  battery warn 10.8 V / stop 10.5 V; Mega `STATION` / `EMERGENCY` / `FACE`; obstacle pauses the rail; RC522 RST recovery.

## 11 · Sources
- Firmware: `ESP32_RFID_MP3_RAIL/ESP32_RFID_MP3.ino`, `NANO_LFR_RAIL/NANO_LFR_RAIL.ino`, `firmware/cam/include/config.h`,
  `firmware/display/README.md` · proven DFPlayer + LM386 build: github.com/ZAN-Tech-bd/esp32-rfid-audio-player.
- ESP32-S3 Series Datasheet v2.2: Table 2-1 (pin reset states), 2-2 (power-up glitches), 5-4 (DC characteristics).
- TI LM386 SNAS545D: §6.1 abs max (15 V, SOIC 0.73 W), §6.3 (4–12 V, ≥ 4 Ω), Fig. 6-3 (output swing vs supply), Fig. 6-7 (dissipation, 4 Ω).
- DFRobot DFR0299 wiki (3.2–5 V, 1 k on RX, SPK < 3 W) · LC Technology LM386 module (photo; protosupplies: LM386M-1, R1 gain).
- Amass XT30PW-F / XT60PW-F specifications (LCSC datasheets for C2913282 / C428722) · XT polarity convention (done.land, components101).
- Generic L298N teardown (newscrewdriver.com 2021-01-28) · ESP32-CAM brown-out (makerguides.com).
- LCSC: `scripts/robot/04…09_*.js` (live library lookups, 2026-10-12…16).
- v0.5 review: Arduino Mega 2560 Rev3 schematic (USB VBUS → MF-MSMF050 PTC → FDN340P, switched by an LMV358 on VIN/2) ·
  TI LM393 datasheet (open-collector output, VOL 0.4 V max at 4 mA) · YD-ESP32-S3 CoreBoard V1.4 schematic (VCC-GND,
  via mischianti.org: U3 CJ6107A33GW, 5Vin through D3) · JLCPCB C2829350 (CJ6107A33GW: SOT-223, 1 A, 0.5 V dropout).
