# Museum ROBOT Carrier — Wiring Spec v0.3 (single YD-ESP32-S3 N16R8, hand-soldered through-hole) — SUPERSEDED by WIRING-PLAN.md v0.4

> ⚠️ **Status: SUPERSEDED (2026-10-16).** Kept for history; do **not** build from this file. Replaced by
> [`WIRING-PLAN.md`](WIRING-PLAN.md) v0.4 (XT30 buck feeds, Mega XH-4 + I²C protection, JP1 amp-supply jumper,
> DFPlayer per the proven build, designators, 100 % through-hole gate). Decisions: [`DECISIONS.md`](DECISIONS.md).
> Supersedes v0.2 → [`WIRING-PLAN-v0.2.md`](WIRING-PLAN-v0.2.md) and v0.1 (DevKit V1 + Nano) → [`WIRING-PLAN-v0.1.md`](WIRING-PLAN-v0.1.md).
> Companion plan: the hub carrier, [`../hub/WIRING-PLAN.md`](../hub/WIRING-PLAN.md) — same DevKit, same design language (bucks stay off-board,
> every off-board wire ends in a connector, 100 % through-hole, bare PCB order, no JLC assembly).
> Firmware today: [`ESP32_RFID_MP3_RAIL/ESP32_RFID_MP3.ino`](../../ESP32_RFID_MP3_RAIL/ESP32_RFID_MP3.ino) (WROOM + Nano). §11 is the migration task.

User decisions taken for v0.3 (2026-10-13): **3S LiPo**; **two of the team's own LM386 modules + two speakers, powered from the
battery through a small voltage-drop stage** (≈ 11 V at full charge); **RC522 on a cable under the chassis**; ports for a **front
obstacle sensor**, an **I²C port like the hub's**, and **two push buttons** (one = audio MUTE while the tour keeps running, one spare).

---

## 0 · What this board is

A hand-soldered carrier for **one YD-ESP32-S3 N16R8 DevKit** that runs the whole robot: motors (L298N), RFID, audio, camera
trigger, face display link, web console and the ESP-NOW emergency/mute link. The Arduino Nano is gone (v0.2 decision). The board
carries **no logic ICs**: every level shift is a resistor or not needed.

Everything that is not on the board connects through a **keyed, latching or polarised connector** — XT60/XT30 for power, JST-VH
for the power switch, JST-XH for every module cable. The LM386 modules, DFPlayer and DevKit plug into sockets.

### 0.1 · What changed from v0.2, and what you get

| # | Change | Benefit |
|---|---|---|
| 1 | **Two LM386 modules on a new `VAMP` rail**: switched battery → 2 × 1N4007 → 1.1 Ω → 1000 µF. ≈ 11.3 V idle at a full 12.6 V pack, ≈ 9.8 V at 11.1 V nominal. DAC_L → left amp, DAC_R → right amp | Keeps the LM386M-1 inside its 4–12 V rating. Roughly 3–4× the output power of a 5 V supply. The diodes block reverse polarity, the RC filter cuts motor-PWM whine (~20 dB at 1 kHz), and audio peaks never sag the ESP's 5 V rail |
| 2 | **RC522 on an XH-7 cable** (was a 1×8 socket on the board); RST wired to a GPIO; 47 Ω on SS/SCK/MOSI | Reader sits 1–3 cm above the floor cards wherever the chassis needs it. Firmware can hard-reset a hung reader. Cleaner SPI edges on the cable |
| 3 | **DFPlayer TX input fixed**: 1 k series + a 2.2 k pull-down footprint left empty (was a 10 k / 20 k divider) | v0.2 turned a 3.3 V DFPlayer TX into 2.2 V, below the S3's guaranteed HIGH of 2.475 V (Espressif datasheet Table 5-4). That can cause random "DFPlayer not responding" faults. Now it is correct for 3.3 V modules, and a 5 V clone needs only the empty resistor fitted |
| 4 | **DFPlayer BUSY → GPIO8** (new) | Hardware "narration finished" signal. The tour no longer depends only on the UART "play finished" message, which can be missed |
| 5 | **Mega port carries data + power (XH-5)** with a 1N5819 in its 5 V line (was 2-pin power only) | The face/station/emergency screens your display firmware already has can follow the robot. One cable. Flashing the Mega over USB can't back-feed the robot, or the robot the laptop. The old 2-pin port was identical to the speaker port, so a swapped plug could put 5 V DC on a speaker |
| 6 | **Front obstacle port** XH-3 (3V3, GND, SIG) on an ADC pin | BUILD item 5 "stops for people" |
| 7 | **I²C port** XH-4 (3V3, GND, SCL, SDA), 4.7 k pull-ups — same as the hub | OLED, VL53L0X distance sensor (better people detection), current monitor or IO expander later, no respin |
| 8 | **Two buttons**: on-board 6 × 6 tactile + XH-2 panel port in parallel each. BTN1 = MUTE, BTN2 = spare | Silence the robot by hand while the tour keeps running (same latch as the Hub mute). Spare for language/start later |
| 9 | **Battery monitor**: switched battery → 100 k / 20 k → GPIO1 | Live pack voltage on the web console. Firmware can warn or stop before the LiPo is over-discharged |
| 10 | **Bulk capacitors**: 1000 µF at the 5 V input, 470 µF at the CAM port (plus the 470 µF at the DFPlayer) | Fewer brown-out resets when Wi-Fi, camera and audio peak together. 470 µF at the ESP32-CAM is the standard fix for its brown-outs |
| 11 | **Switch harness VH-2 on one rocker pole** (was VH-4, both poles in parallel) | The battery-side current is ≤ ≈ 4 A, because the bucks convert down (v0.2's "≈ 12 A" was output-side current). 2 crimps instead of 4 |
| 12 | **Every port type has its own pin count** (XH-2 buttons, XH-3 CAM/obstacle, XH-4 I²C, XH-5 Mega, XH-7 RFID, XH-8 L298N); `VAMP` (≈ 11 V) never leaves the board | A plug can't go into a port where it does damage. The two XH-3 ports were checked: swapping them is harmless |
| 13 | **Board target ≤ 100 × 100 mm**, zip-tie anchor holes at every cable exit, 2 spare GPIOs on pads | Cheapest JLC size tier. Connectors don't get pulled out by vibration. Room to grow |

Kept from v0.2: S3 socket at 25.40 mm rows · XT60/XT30 power scheme and genders · both bucks and the L298N off-board ·
motor pins GPIO 4/5/6/7/15/16 with 10 k pull-downs on ENA/ENB · CAM on XH-3 · two status LEDs · no WS2812, no Nano, no logic chips.

### Reference dimensions (measure again with calipers before layout)

| Board | Size | Header geometry |
|---|---|---|
| YD-ESP32-S3 N16R8 | 27.94 × 57.15 mm | 2 × 22 pins, 2.54 mm; rows **25.40 mm** apart; pin 1 at 1.91 mm from the top edge; antenna overhangs the top edge by 6.24 mm — [`YD-ESP32-S3 N16R8_footprint.png`](YD-ESP32-S3%20N16R8_footprint.png) |
| DFPlayer Mini | ≈ 20 × 20 mm | 2 × 8 pins, 2.54 mm; rows **18.00 mm** per the LCSC footprint (C9900285937) — a 0.7″ breadboard fit would mean 17.78 mm: **caliper it** |
| LM386 module (×2) | 41 × 14 mm | 1 × 4 header `GND GND IN VCC` (order read from your Cirkit diagram) + 2-way screw terminal `OUT GND`; chip LM386M-1 (4–12 V) |
| RC522 module | ≈ 60 × 40 mm | 1 × 8 pins: SDA, SCK, MOSI, MISO, IRQ, GND, RST, 3.3V — on a cable now |
| L298N module (red) | ≈ 43 × 43 mm | 1 × 6 control header ENA IN1 IN2 IN3 IN4 ENB + screw terminals 12V / GND / 5V |
| ESP32-CAM | 27 × 40 mm | 5V, GND, IO12, IO13 … on one side (robot link = IO13) |
| KCD4-203-B rocker | panel mount | 2 rows × 3 spade tabs (6.3 mm) — [`../hub/Switch_KCD4-203-B-dimension.jpg`](../hub/Switch_KCD4-203-B-dimension.jpg) |

---

## 1 · Power path

```
3S LiPo ──XT60 female lead──► J1 BAT IN (XT60PW-M)
   BAT+ ─► J5 VH-2 ─► rocker (one pole: COM ↔ "I") ─► J5 back ─► BAT_SW
   BAT− ─► GND (single source; the switch breaks + only)

BAT_SW ─┬─► J2 XT60PW-F ──► adjustable motor buck (set to the motor rating, 6.0 V for TT motors)
        │                     buck OUT ──► L298N 12V/GND screws directly (motor current never crosses the board)
        ├─► J3 XT60PW-F ──► fixed 5 V buck ──► XT30 female lead ──► J4 XT30PW-M = +5V rail
        ├─► D1 1N4007 ─► D2 1N4007 ─► R 1.1 Ω 1 W ─► VAMP (1000 µF 25 V + 100 nF) ─► both LM386 module VCC pins
        ├─► 100 k ─┬─► GPIO1 (VBAT_SENSE, ÷6)   20 k + 100 nF to GND
        └─► 2.2 k ─► LED1 red ─► GND             (on whenever the switch is on)

+5V (1000 µF + 100 nF at J4) feeds: DevKit 5Vin (its 1 A LDO makes 3V3), DFPlayer VCC (470 µF), ESP32-CAM port (470 µF),
     L298N logic 5V, Mega port (through D3 1N5819), green LED2 + 1 k
+3V3 (from the DevKit LDO) feeds: RC522 port, I²C port, obstacle port, pull-ups
```

- **`VAMP` numbers** (2 × 1N4007 at ≈ 0.63 V each when idle, ≈ 0.8 V each when loud):

  | Pack | Idle | Speech, loud |
  |---|---|---|
  | 12.6 V (full) | ≈ 11.3 V | ≈ 10.8 V |
  | 11.1 V (nominal) | ≈ 9.8 V | ≈ 9.3 V |
  | 9.9 V (empty, 3.3 V/cell) | ≈ 8.6 V | ≈ 8.1 V |

  LM386M-1 operating range is 4–12 V (15 V absolute max) — every case is inside it. The 1000 µF sits behind the diodes, so
  when the motors pull the pack down for a moment the diodes turn off and the capacitor keeps the amps steady. With the
  1.1 Ω it forms a low-pass of ≈ 80 Hz (diode resistance included), which attenuates 1 kHz motor-PWM ripple by ≈ 20 dB. The
  footprint takes a 13 mm can, so 2200 µF fits if whine is still audible.
- **XT genders follow the hub/LiPo convention**: battery lead (XT60 female) fits only J1; both buck input leads (XT60 male)
  fit only J2/J3; the 5 V buck output lead (XT30 female) fits only J4. A LiPo can never reach the 5 V rail.
- The S3 gets +5 V straight into 5Vin. The YD board's input diode stops USB back-feeding the rail — **keep the YD board's
  IN-OUT and USB-OTG solder jumpers open** (hub rule).
- **Ground**: battery −, both buck IN −, the XT30 return and every module ground meet on the board's GND pour. The motor return
  runs buck OUT− → L298N GND screw as its own thick pair; only the XH-8 GND ties the L298N logic to the board.
- Unplugging **J2** = motors dead, logic alive (safe bench testing). Unplugging **J3** = 5 V logic off; the motors stay
  disabled because the ENA/ENB pull-downs hold the bridge off.

### Current budget

| Load | Typical | Worst case |
|---|---|---|
| ESP32-S3 + Wi-Fi AP + console + RC522 (3V3 via the DevKit LDO) | 0.3 A @ 5 V | 0.6 A |
| DFPlayer (DAC out only) | 0.05 A @ 5 V | 0.1 A |
| ESP32-CAM | 0.15 A @ 5 V | 0.5 A (Wi-Fi TX + flash) |
| Mega 2560 + 3.2″ TFT | 0.25 A @ 5 V | 0.4 A |
| L298N logic | 0.04 A @ 5 V | 0.05 A |
| **5 V total** | **≈ 0.8 A** | **≈ 1.7 A** → a fixed 5 V / ≥ 3 A buck (5 A preferred) |
| 2 × LM386 on `VAMP` | 0.03 A idle | ≈ 0.5 A peaks |
| Motors on the 6 V buck | ≈ 0.6 A | 2–3 A both stalled |
| **Battery side** (bucks ≈ 85 %) | **≈ 1 A** | **≈ 3.5–4 A** |

A 2200 mAh 3S pack gives roughly 1.5–2 h of touring; a 5200 mAh pack ≈ 4 h.

---

## 2 · Ports and harnesses

### 2.1 Port table

| Ref | Port | Board connector (LCSC) | Pins 1 → n |
|---|---|---|---|
| J1 | Battery in | XT60PW-M, male, right-angle (C98732) | +, − |
| J2 | Motor-buck feed | XT60PW-F, female (C428722) | +, − |
| J3 | 5 V-buck feed | XT60PW-F, female (C428722) | +, − |
| J4 | 5 V ← buck | XT30PW-M, male (C53373776) | +, − |
| J5 | Power switch | JST VH 2-pin B2P-VH (C485345) | BAT+ (to rocker COM), BAT_SW (from rocker "I") |
| J6 | L298N module | JST XH 8-pin B8B-XH-A (C157972) | 5V, GND, ENA, IN1, IN2, IN3, IN4, ENB |
| J7 | RC522 reader | JST XH 7-pin B7B-XH-A (C144398) | 3V3, GND, SS, SCK, MOSI, MISO, RST |
| J8 | ESP32-CAM | JST XH 3-pin B3B-XH-A (C144394) | 5V, GND, CAM_TX |
| J9 | Mega face display | JST XH 5-pin B5B-XH-A (C157991) | 5V (via D3), GND, TX → Mega RX1, RX ← Mega TX1, NC |
| J10 | I²C | JST XH 4-pin B4B-XH-A (C144395) | 3V3, GND, SCL, SDA |
| J11 | Front obstacle sensor | JST XH 3-pin (C144394) | 3V3, GND, SIG |
| J12 | BTN1 MUTE (panel) | JST XH 2-pin B2B-XH-A (C378939) | SIG, GND |
| J13 | BTN2 spare (panel) | JST XH 2-pin (C378939) | SIG, GND |
| U1 | YD-ESP32-S3 socket | 2 × 1×22 female PM254V-11-22-H85 (C46595983), rows 25.40 mm | §4 |
| U2 | DFPlayer Mini socket | 2 × 1×8 female PM254V-11-08-H85 (C46595973), rows per caliper | §3.2 |
| U3, U4 | LM386 module L / R | 1×4 female PM254V-11-04-H85 (C541851) each | VCC (`VAMP`), IN, GND, GND — order matched to the real module (§8) |
| SW1, SW2 | MUTE / spare | 6 × 6 mm THT tactile (vertical, buy locally; or right-angle TS-1102S3CH C54123281 at the board edge) | parallel to J12 / J13 |
| EXP | spare GPIO pads | 1×4 2.54 mm pads, not fitted | 3V3, GND, GPIO3, GPIO38 |

Wire gauges: battery + buck leads 16 AWG · switch harness 18 AWG · every XH cable 22–24 AWG (XH max 22 AWG, 3 A per pin).

### 2.2 Rules

1. **One family per job:** XT60 = battery and buck feeds, XT30 = 5 V return, VH = power switch, XH = every module cable.
   Families never cross-mate.
2. **XH pin 1 = supply, pin 2 = GND, signals after** (button ports: SIG, GND). **Each port type has its own pin count**, so a
   cable only fits its own port type. The only shared count is XH-3 (CAM 5 V / obstacle 3V3); a swap there is harmless:
   the CAM browns out, or the sensor runs at 5 V into a 1 k-protected S3 output.
3. **`VAMP` (≈ 11 V) exists only on the board** (U3/U4 sockets, TP2) — never on a cable.
4. **Unplug J8 (CAM) before flashing the ESP32-CAM** on its CAM-MB. The S3 flashes over either USB-C with the robot on or off.
   The Mega can be flashed over USB with J9 plugged in (D3 blocks back-feed).
5. **Speakers connect to the LM386 modules' own screw terminals** (`OUT`, `GND`). The LM386 output is ground-referenced, so a
   grounded speaker wire is correct here. Never wire a speaker to DFPlayer SPK1/SPK2 on this board (those pins are left
   unconnected).
6. **LM386 modules lie flat, header pins down** into U3/U4, screw terminals at the board edge; the far end rests on a foam pad
   or a nylon spacer. Silkscreen outline shows the only correct orientation (a module turned round puts `VAMP` on its GND pin).

### 2.3 Harness pin-outs (board end → far end)

| Cable | Board end | Far end |
|---|---|---|
| Battery | — | stock LiPo XT60 female lead → J1 |
| Power switch | VHR-2N + 2 × SVH-21T | 6.3 mm female spades on rocker **COM** (pin 1) and the tab that closes in position **"I"** (pin 2) — check with a meter (§8) |
| Motor buck | — | input lead XT60 male → J2; output hard-wired to the L298N 12V/GND screws, twisted, 16–18 AWG |
| 5 V buck | — | input lead XT60 male → J3; output lead XT30 female → J4 |
| L298N | XHP-8 | pin 1 → `5V` screw, pin 2 → `GND` screw (shares it with the buck's thick wire), pins 3–8 → 1×6 Dupont on ENA IN1 IN2 IN3 IN4 ENB. **Remove the ENA, ENB and 5V_EN jumpers** |
| RC522 | XHP-7 | 1×8 Dupont on the module: SDA ← 3, SCK ← 4, MOSI ← 5, MISO ← 6, IRQ (empty), GND ← 2, RST ← 7, 3.3V ← 1. Keep ≤ 25 cm |
| ESP32-CAM | XHP-3 | 1×4 Dupont on `5V, GND, IO12, IO13`: 5V ← 1, GND ← 2, IO12 empty, IO13 ← 3 (one housing, can't shift by a pin) |
| Mega | XHP-5 | pin 1/2 → a free 5 V / GND pin pair on the Mega + shield stack (§8), pin 3 → **RX1 (pin 19)**, pin 4 → **TX1 (pin 18)**, pin 5 unused |
| I²C | XHP-4 | module VCC, GND, SCL, SDA — follow the module's silkscreen (many OLEDs are `GND VCC SCL SDA`) |
| Obstacle | XHP-3 | IR module VCC ← 1, GND ← 2, OUT ← 3 (FC-51-style modules print `OUT GND VCC` — the order is mirrored) |
| Buttons | XHP-2 | panel push button (normally open) across the two wires |
| Speakers | — | LM386 screw terminal → speaker; add an inline 2-pin plug at the speaker if it must come off the shell |

---

## 3 · On-board circuits

### 3.1 Audio amplifier rail (`VAMP`) and the two LM386 modules

```
BAT_SW ──►|── D1 1N4007 ──►|── D2 1N4007 ──[R 1.1 Ω 1 W]──┬── VAMP ──► U3 VCC (left amp), U4 VCC (right amp)
                                                          ├── C 1000 µF 25 V (13 mm footprint) ── GND
                                                          └── C 100 nF ── GND        TP2 = VAMP
DFPlayer DAC_L (U2 pin 5) ──► U3 IN        DFPlayer DAC_R (U2 pin 4) ──► U4 IN
U3/U4 GND pins ──► GND, close to the DFPlayer GND pins
```

- The DFPlayer's DAC pins already carry series coupling capacitors on the module, so they connect straight to the LM386 `IN`
  pins (no DC reaches the amps). Keep both traces short and over solid ground.
- **Volume chain**: firmware volume (0–30, web console, Hub mute, MUTE button) → module pot → speaker. Set each module pot once
  and use the console after that.
- **Optional, recommended tweak on each module**: desolder the 0 Ω resistor at **R1** → gain drops from 200 to 20. The DAC
  line level is already strong, so the pot works over its whole travel and hiss/whine drop by ≈ 20 dB. No board change.
- **Heat check**: at ≈ 11 V an LM386M-1 runs warm into **4 Ω** at high volume. Speech is fine; avoid long full-volume tones
  or music. If a chip is too hot to touch, turn its pot down. 8 Ω speakers are kinder (see §10.1).

### 3.2 DFPlayer Mini (U2) — UART1

| DFPlayer pin | Net | Connection |
|---|---|---|
| 1 VCC | +5V | 470 µF 16 V + 100 nF at the socket |
| 2 RX | `S3_TO_DF` | ← 1 k ← GPIO14 (UART1 TX) |
| 3 TX | `DF_TO_S3` | → 1 k → GPIO17 (UART1 RX); **2.2 k to GND footprint, not fitted** |
| 4 DAC_R | `DAC_R` | → U4 IN |
| 5 DAC_L | `DAC_L` | → U3 IN |
| 6 SPK1, 8 SPK2 | — | **not connected** (bridged speaker pair — not used with the external amps) |
| 7 GND, 10 GND | GND | |
| 16 BUSY | `DF_BUSY` | → 1 k → GPIO8; **2.2 k to GND footprint, not fitted** (LOW while playing) |
| 9 IO1, 11 IO2, 12/13 ADKEY, 14/15 USB | — | not connected |

- **Measure the DFPlayer's TX idle voltage** (powered at 5 V). If it is ≈ 3.3 V (DFRobot's stated TTL level) leave the two 2.2 k
  out. If it reads ≈ 5 V, fit both: 5 V × 2.2 / 3.2 = 3.4 V, inside the S3's limits.
- DFPlayer microSD: the module is socketed, so the card can be swapped by lifting the module if the slot ends up facing inward.

### 3.3 RC522 port (J7) — SPI, 3.3 V

| J7 pin | Net | S3 | Conditioning |
|---|---|---|---|
| 1 | +3V3 | — | 10 µF + 100 nF at the port |
| 2 | GND | — | |
| 3 | `RFID_SS` | GPIO10 | 47 Ω series |
| 4 | `RFID_SCK` | GPIO12 | 47 Ω series |
| 5 | `RFID_MOSI` | GPIO11 | 47 Ω series |
| 6 | `RFID_MISO` | GPIO13 | direct |
| 7 | `RFID_RST` | GPIO9 | direct (firmware can hard-reset a hung reader) |

- GPIO10/11/12/13 are the S3's **default FSPI pins** (Arduino `SPI.begin()` with no arguments uses them). They sit side by side
  on U1A pins 15–19, so the bus is one straight run.
- The MFRC522v2 driver runs SPI at 4 MHz. On a ≤ 25 cm cable that is fine with the 47 Ω resistors. If reads ever get flaky,
  firmware can drop it to 1 MHz (`MFRC522DriverSPI` takes `SPISettings`).
- 3.3 V only. The RC522 must never see 5 V; the port is silkscreened "3.3 V".

### 3.4 Motor driver port (J6)

| J6 pin | Net | S3 GPIO | Conditioning |
|---|---|---|---|
| 1 | +5V | — | L298N logic supply (its 5V_EN jumper removed) |
| 2 | GND | — | logic reference |
| 3 | `ENA` | 4 | LEDC PWM + **10 k to GND** |
| 4 | `IN1` | 5 | direct |
| 5 | `IN2` | 6 | direct |
| 6 | `IN3` | 7 | direct |
| 7 | `IN4` | 15 | direct |
| 8 | `ENB` | 16 | LEDC PWM + **10 k to GND** |

- **Boot-safe off**: while the S3 resets, flashes, crashes or is pulled out, the two pull-downs hold ENA/ENB LOW, so the bridge
  is disabled (wheels coast). On a generic L298N board the EN pins have no pull-up of their own once the jumpers are removed
  (they only tie to +5 V through the jumper), so 10 k is enough. Bring-up step 4 proves it.
- Pin choice checked against the S3 datasheet power-up table: GPIO 4/5/6/7 glitch only **LOW** (≈ 60 µs) and GPIO15/16 don't
  glitch; none of them has a boot pull-up. That is why ENA/ENB are not on GPIO18/19/20 (HIGH glitches) or GPIO0/39/43/44
  (pull-ups at reset).
- Channel A = physical left motor ("150" side), channel B = right ("210" side) — as calibrated on the chassis.
- L298N logic inputs need ≥ 2.3 V for HIGH; the S3 drives ≥ 2.64 V. No level shifting.

### 3.5 ESP32-CAM port (J8)

- `CAM_TX`: GPIO18 (UART2 TX, 9600 baud) → 1 k → J8.3 → CAM IO13. Both sides 3.3 V. GPIO18's power-up HIGH glitch is the
  UART's idle level, so the camera sees nothing.
- 470 µF 16 V + 100 nF across J8 5V/GND.

### 3.6 Mega display port (J9)

```
+5V ──►|── D3 1N5819 ── J9.1 (Mega 5 V)           ≈ 0.4 V drop → Mega at ≈ 4.7 V with a 5.1 V buck
GPIO41 (TX) ── 1 k ── J9.3 ──► Mega RX1 (pin 19)
Mega TX1 (pin 18) ──► J9.4 ── 10 k ──┬──► GPIO42 (RX)          5 V × 20/30 = 3.33 V
                                    20 k
                                     └── GND
```

- The display firmware ([`firmware/display`](../../firmware/display/README.md)) already listens on Serial1 at 9600 for
  `FACE`, `STATION n m`, `EMERGENCY`, `LANG n`, `MENU`, `BOOT`. The robot side is a firmware task (§11).
- A 3.3 V HIGH into the Mega's 5 V input has ≈ 0.2–0.3 V margin over its 3.0 V threshold (0.6 × VCC). Running the Mega at
  ≈ 4.7 V lowers that threshold to ≈ 2.8 V, so the margin grows.
- D3 stops a laptop on the Mega's USB from powering the whole robot when it is off (the Mega's 500 mA fuse would trip). It
  also stops the robot pushing current into the laptop. Set the 5 V buck to 5.1–5.2 V.

### 3.7 I²C port (J10)

- SCL = GPIO39, SDA = GPIO40, 4.7 k pull-up each to +3V3. 3.3 V only (silkscreen).

### 3.8 Inputs — obstacle sensor and buttons

```
J11.3 OBST ──┬── 10 k ── +3V3          J12.1 BTN1 ──┬── 10 k ── +3V3        (BTN2: J13 + SW2 → GPIO47, same network)
             └── 1 k ──┬──► GPIO2                    ├── SW1 ── GND
                    100 nF                           └── 1 k ──┬──► GPIO21
                       └── GND                              100 nF ── GND
```

- All three inputs are **active LOW** (an IR obstacle module pulls its OUT low on detection; a pressed button shorts to GND).
  The 1 k + 100 nF debounce and protect the pin on a long wire. GPIO2 is ADC-capable, so an analog distance sensor can use
  the same port later.
- The obstacle port supplies **3.3 V**. FC-51/HW-201 IR modules work at 3.3 V with somewhat shorter range. For longer range
  use a VL53L0X on the I²C port.

### 3.9 Battery monitor, LEDs, test points, spare pads

- `VBAT_SENSE`: BAT_SW → 100 k → node → 20 k → GND, 100 nF across the 20 k, node → GPIO1 (ADC1). 12.6 V → 2.10 V,
  9.0 V → 1.50 V. Firmware: `V_bat = V_adc × 6` (calibrate once against a meter). No drain when the switch is off.
- LED1 red + 2.2 k on BAT_SW (≈ 4.8 mA at 12.6 V). LED2 green + 1 k on +5V (≈ 3 mA). The DevKit's own LED shows 3V3; its RGB
  LED (GPIO48) is free for firmware status colours.
- Test holes: TP1 BAT_SW, TP2 VAMP, TP3 +5V, TP4 +3V3, TP5/TP6 GND.
- EXP pads (unpopulated 1×4): 3V3, GND, GPIO3, GPIO38. GPIO3 is a strapping pin only if a JTAG eFuse is burned (default:
  ignored), so it is safe as a spare.

---

## 4 · DevKit pin map (U1)

U1A = left 22-pin header, U1B = right header, pin 1 at the antenna end (same numbering as the hub).

| U1A pin | GPIO | Net | | U1B pin | GPIO | Net |
|---|---|---|---|---|---|---|
| 1 | 3V3 | +3V3 | | 1 | GND | GND |
| 2 | 3V3 | +3V3 | | 2 | 43 | — (TX0 → CH343 USB-UART) |
| 3 | RST | — | | 3 | 44 | — (RX0 → CH343 USB-UART) |
| 4 | 4 | ENA → J6.3 (10 k PD) | | 4 | 1 | VBAT_SENSE (ADC1) |
| 5 | 5 | IN1 → J6.4 | | 5 | 2 | OBST_IO ← J11.3 (ADC1) |
| 6 | 6 | IN2 → J6.5 | | 6 | 42 | MEGA_RX ← divider ← J9.4 |
| 7 | 7 | IN3 → J6.6 | | 7 | 41 | MEGA_TX → 1 k → J9.3 |
| 8 | 15 | IN4 → J6.7 | | 8 | 40 | I2C_SDA → J10.4 |
| 9 | 16 | ENB → J6.8 (10 k PD) | | 9 | 39 | I2C_SCL → J10.3 |
| 10 | 17 | DF_TO_S3 (UART1 RX) | | 10 | 38 | EXP spare |
| 11 | 18 | CAM_TX (UART2 TX) → J8.3 | | 11 | 37 | — (octal PSRAM) |
| 12 | 8 | DF_BUSY | | 12 | 36 | — (octal PSRAM) |
| 13 | 3 | EXP spare | | 13 | 35 | — (octal PSRAM) |
| 14 | 46 | — (strapping) | | 14 | 0 | — (BOOT) |
| 15 | 9 | RFID_RST → J7.7 | | 15 | 45 | — (strapping) |
| 16 | 10 | RFID_SS → J7.3 | | 16 | 48 | — (onboard RGB LED) |
| 17 | 11 | RFID_MOSI → J7.5 | | 17 | 47 | BTN2_IO ← SW2 / J13 |
| 18 | 12 | RFID_SCK → J7.4 | | 18 | 21 | BTN1_IO (MUTE) ← SW1 / J12 |
| 19 | 13 | RFID_MISO ← J7.6 | | 19 | 20 | — (USB D+) |
| 20 | 14 | S3_TO_DF (UART1 TX) | | 20 | 19 | — (USB D−) |
| 21 | 5V | +5V | | 21 | GND | GND |
| 22 | GND | GND | | 22 | GND | GND |

Why this map: the motor pins can't glitch HIGH or boot pulled-up (ESP32-S3 datasheet v2.2 Tables 2-1/2-2). Battery and
obstacle sit on ADC1 (ADC2 is unusable with Wi-Fi on). RFID uses the S3's default SPI pins. Nothing can pull a strapping pin
(0/3/45/46) at reset. The USB (19/20) and PSRAM (35–37) pins are untouched. **UARTs**: UART1 = DFPlayer, UART2 = CAM (TX
only), UART0 = Mega on GPIO41/42 with the console moved to the native-USB port (§11). 23 of 25 free GPIOs are used; GPIO3
and GPIO38 stay spare.

---

## 5 · Connection list (schematic source of truth)

XT symbols (same parts as the hub): XT60 male J1: 1 = −, 2 = +. XT60 female J2/J3: 3 = +, 4 = − (1/2 = mounting tabs).
XT30 male J4: 1 = −, 2 = + (3/4 = mounting tabs). Mounting tabs stay unconnected.

**Power**
- `BAT+`: J1.2, J5.1.
- `BAT_SW`: J5.2, J2.3, J3.3, D1 anode, R 100 k (VBAT top), R 2.2 k → LED1 anode, TP1.
- `AMP_D` (between the diodes): D1 cathode, D2 anode.
- `AMP_R`: D2 cathode, R 1.1 Ω.
- `VAMP`: R 1.1 Ω other end, C 1000 µF/25 V +, C 100 nF, U3.VCC, U4.VCC, TP2.
- `+5V`: J4.2, C 1000 µF/16 V +, C 100 nF, U1A.21, U2.1, C 470 µF + 100 nF (DF), J6.1, J8.1, C 470 µF + 100 nF (CAM),
  D3 anode, R 1 k → LED2 anode, TP3.
- `MEGA_5V`: D3 cathode, J9.1.
- `+3V3`: U1A.1, U1A.2, J7.1, C 10 µF + 100 nF (RFID), J10.1, J11.1, I²C 4.7 k ×2, 10 k pull-ups (OBST, BTN1, BTN2), EXP.1, TP4.
- `GND`: J1.1, J2.4, J3.4, J4.1, U1A.22, U1B.1/21/22, U2.7, U2.10, U3/U4 GND pins, J6.2, J7.2, J8.2, J9.2, J10.2, J11.2,
  J12.2, J13.2, SW1, SW2, every capacitor −, 10 k ENA/ENB pull-downs, 20 k (VBAT), 20 k (MEGA), both LED cathodes, both
  DNP 2.2 k, EXP.2, TP5, TP6. (J5 carries no GND.)

**Signals**
- Motor: `ENA` U1A.4 → J6.3, 10 k → GND · `IN1` U1A.5 → J6.4 · `IN2` U1A.6 → J6.5 · `IN3` U1A.7 → J6.6 · `IN4` U1A.8 → J6.7 ·
  `ENB` U1A.9 → J6.8, 10 k → GND.
- RFID: `RFID_SS` U1A.16 → 47 Ω → J7.3 · `RFID_SCK` U1A.18 → 47 Ω → J7.4 · `RFID_MOSI` U1A.17 → 47 Ω → J7.5 ·
  `RFID_MISO` J7.6 → U1A.19 · `RFID_RST` U1A.15 → J7.7.
- DFPlayer: `S3_TO_DF` U1A.20 → 1 k → U2.2 · `DF_TO_S3` U2.3 → 1 k → U1A.10, 2.2 k (DNP) U1A.10-side → GND ·
  `DF_BUSY` U2.16 → 1 k → U1A.12, 2.2 k (DNP) → GND · `DAC_L` U2.5 → U3.IN · `DAC_R` U2.4 → U4.IN · U2.6, U2.8, U2.9,
  U2.11–U2.15: no net.
- CAM: `CAM_TX` U1A.11 → 1 k → J8.3.
- Mega: `MEGA_TX` U1B.7 → 1 k → J9.3 · `MEGA_RX` J9.4 → 10 k → U1B.6, 20 k U1B.6-side → GND · J9.5 no net.
- I²C: `I2C_SCL` U1B.9 → J10.3 · `I2C_SDA` U1B.8 → J10.4 · 4.7 k each to +3V3.
- Obstacle: `OBST` J11.3, 10 k → +3V3, → 1 k → `OBST_IO` U1B.5, 100 nF `OBST_IO` → GND.
- Buttons: `BTN1` J12.1 + SW1 (other side GND), 10 k → +3V3, → 1 k → `BTN1_IO` U1B.18, 100 nF → GND ·
  `BTN2` J13.1 + SW2, 10 k → +3V3, → 1 k → `BTN2_IO` U1B.17, 100 nF → GND.
- Battery: `VBAT_SENSE` = 100 k / 20 k node → U1B.4, 100 nF → GND.
- Spare: EXP.3 → U1A.13 (GPIO3), EXP.4 → U1B.10 (GPIO38).
- No-connect pins (set No Connect in the schematic): U1A.3, U1A.14, U1B.2, U1B.3, U1B.11–U1B.16, U1B.19, U1B.20.

---

## 6 · Bill of materials (hand-soldered; LCSC numbers verified live via the EasyEDA bridge 2026-10-12/13)

| Qty | Refs | Part | LCSC | Notes |
|---|---|---|---|---|
| 1 | J1 | XT60PW-M (male, right-angle) | C98732 | |
| 2 | J2, J3 | XT60PW-F (female, right-angle) | C428722 | |
| 1 | J4 | XT30PW-M (male, right-angle) | C53373776 | |
| 1 | J5 | JST VH 2-pin B2P-VH | C485345 | + VHR-2N housing + 2 × SVH-21T |
| 1 | J6 | JST XH 8-pin B8B-XH-A(LF)(SN) | C157972 | + XHP-8 + crimps |
| 1 | J7 | JST XH 7-pin B7B-XH-A(LF)(SN) | C144398 | + XHP-7 |
| 1 | J9 | JST XH 5-pin B5B-XH-A(LF)(SN) | C157991 | + XHP-5 |
| 1 | J10 | JST XH 4-pin B4B-XH-A(LF)(SN) | C144395 | + XHP-4 |
| 2 | J8, J11 | JST XH 3-pin B3B-XH-A(LF)(SN) | C144394 | + XHP-3 |
| 2 | J12, J13 | JST XH 2-pin B2B-XH-A(LF)(SN) | C378939 | + XHP-2 |
| 2 | U1 | 1×22 female PM254V-11-22-H85 | C46595983 | rows 25.40 mm |
| 2 | U2 | 1×8 female PM254V-11-08-H85 | C46595973 | rows per caliper |
| 2 | U3, U4 | 1×4 female PM254V-11-04-H85 | C541851 | LM386 modules |
| 2 | SW1, SW2 | 6 × 6 mm THT tactile | — / C54123281 (right-angle) | buy locally |
| 2 | D1, D2 | 1N4007 (DO-41) | C108355 | `VAMP` drop |
| 1 | D3 | 1N5819 Schottky (DO-41) | C526214 | Mega feed |
| 1 | — | 1.1 Ω 1 W metal film (RSF100JB-73-1R1) | C1365792 | `VAMP` filter, 15.5 mm pitch |
| 6 | — | 10 kΩ ¼ W | C1519255 | ENA/ENB PD, OBST/BTN PU ×3, Mega divider top |
| 2 | — | 20 kΩ ¼ W | C1364283 | Mega divider, VBAT bottom |
| 9 | — | 1 kΩ ¼ W | C1364484 | DF ×3, CAM, Mega TX, OBST, BTN ×2, LED2 |
| 1 (+2 DNP) | — | 2.2 kΩ ¼ W | C3454390 | LED1 · DF TX / BUSY pull-downs only for a 5 V DFPlayer |
| 2 | — | 4.7 kΩ ¼ W | C1371435 | I²C |
| 3 | — | 47 Ω ¼ W | C3402507 | SPI |
| 1 | — | 100 kΩ ¼ W | C1364475 | VBAT top |
| 9 | — | 100 nF ceramic, 5 mm pitch | C342794 | J4, DF, RFID, CAM, VAMP, VBAT, OBST, BTN ×2 |
| 1 | — | 1000 µF 16 V radial, 10 mm | buy locally | 5 V input |
| 1 | — | 1000 µF 25 V radial (13 mm footprint) | buy locally | `VAMP` (2200 µF fits) |
| 2 | — | 470 µF 16 V radial, 8 mm | C109066 | DFPlayer, CAM |
| 1 | — | 10 µF 16 V radial | buy locally | RFID port |
| 2 | LED1, LED2 | 5 mm LED red, green | footprint C9900102778 | buy locally |
| — | TP1–TP6, EXP | test holes / pads | — | no parts |
| 4 | H1–H4 | M3 holes | — | + 4 × M3 standoffs |

All resistors are Yageo MFR-25 footprints (10.3 mm pitch) except the 1 W part — same series as the hub BOM, so one order
covers both boards. Search scripts: `PCB design/scripts/robot/01…06_*.js` (read-only). The search API returns no stock
figures — re-check at order time.

---

## 7 · Board layout (constraints + indicative floor plan)

```
 ←──────────────────────────────── ≤ 100 mm ───────────────────────────────→
 ┌H1 ─ antenna overhang ─┬────────────────────────────────────────────────H2┐
 │  (no copper, 15 mm    │ J6 L298N · J7 RC522 · J8 CAM · J9 MEGA   ← harness │
 │   clear of XT/caps)   │ ENA/ENB PD · SPI 47 Ω · CAM/Mega resistors  exits  │
 │ U1 YD-ESP32-S3        ├─────────────────────────┬──────────────────────────┤
 │ 2 × 22 sockets        │ U2 DFPlayer + 470 µF    │ J10 I²C · J11 OBSTACLE   │
 │ rows 25.40 mm         │                         │ SW1/J12 MUTE · SW2/J13   │
 │ low parts underneath  ├────────────┬────────────┤ LED1 LED2 · VBAT div     │
 │ USB-C ▼               │ U3 LM386 L │ U4 LM386 R ├──────────────────────────┤
 │ ┊ USB lane:           │ (flat)     │ (flat)     │ POWER: J1 BAT · J5 SW    │
 │ ┊ nothing > 5 mm      │ VAMP: D1 D2 R 1000 µF   │ J3 →5 V · J4 5 V in      │
 │ ┊                     │ speakers ▼ │ speakers ▼ │ J2 →motor · 1000 µF      │
 └H3─────────────────────┴────────────┴────────────┴────────────────────────H4┘
```

- 2 layers, 1.6 mm FR-4, 1 oz copper, HASL. **Target ≤ 100 × 100 mm** (cheapest JLC tier); grow to ≈ 120 × 100 only if
  routing gets cramped.
- **Antenna**: DevKit antenna overhangs a board edge; no copper under it on either layer; no XT connector, electrolytic or
  LM386 module within 15 mm. The robot's SoftAP serves the field console.
- **USB lane**: a ≈ 20 mm-wide strip from the DevKit's USB-C end to a board edge holds only parts lower than ≈ 5 mm, so a
  USB-C plug passes over them (the DevKit sits ≈ 11 mm high on its sockets).
- **Power column** on one edge, all XT mating faces outward, switch harness J5 and the 5 V input cap beside it. `BAT_SW` to
  J2/J3 as ≥ 3 mm pours on both layers; `+5V` spine ≥ 2 mm; `VAMP` ≥ 1 mm; signals 0.3–0.4 mm; bottom layer solid GND.
- **Audio block** (DFPlayer, U3/U4, `VAMP` filter) together and away from the antenna and the RC522 harness; DAC traces
  short; module screw terminals at a board edge.
- **Holes**: H1–H4 M3 (3.2 mm plated, 6 mm pad, no net) 4 mm in from each corner · **zip-tie anchors** (pairs of 2.5 mm NPTH
  holes, 5 mm apart) beside the power column, the L298N/RC522 exits, the CAM/Mega exits and the speaker exit · TP1–TP6
  (1.0 mm plated) · nothing else (modules are socketed).
- **Silkscreen**: every port named with its partner and pin functions ("L298N — remove ENA/ENB/5V_EN jumpers",
  "RC522 3.3 V", "CAM IO13", "MEGA RX1/TX1"); "3.3 V" on J7/J10/J11; LM386 outlines with "VCC" marks and "speaker → module
  terminal"; DFPlayer pin-1 (VCC) mark; "DNP: fit for 5 V DFPlayer" beside the two 2.2 k footprints; "J2 out = motors off".

---

## 8 · Checks before ordering

1. **Calipers**: the S3's row spacing (expect 25.40 mm) and the DFPlayer's (18.00 mm footprint vs 17.78 mm). The module is the
   truth.
2. **LM386 modules (photo of both sides + calipers)**: header pin order (expect `GND GND IN VCC` from one end); do the header
   pins point **down** from the component side? (If they point up, solder a fresh straight header from the top.) Overall
   length, any mounting holes, tallest part. This fixes U3/U4 position and the support under the far end.
3. **Speaker impedance** (4 Ω or 8 Ω) — sets the heat/volume note in §3.1.
4. **DFPlayer TX idle voltage** at 5 V: ≈ 3.3 V → leave the 2.2 k out; ≈ 5 V → fit them.
5. **Mega + TFT shield**: which 5 V and GND pins are still reachable with the shield on (power header pass-through, ICSP, or
   the end pins of the 2 × 18 header); confirm pins 18/19 are reachable.
6. **L298N**: header order ENA IN1 IN2 IN3 IN4 ENB; the 5V_EN jumper position.
7. **Rocker continuity**: which spade tab joins COM in position "I" (hub §2.3 gotcha).
8. Print the PCB 1:1 and lay the DevKit, DFPlayer, both LM386 modules and real XT60/XT30/XH plugs on it; check the USB lane
   and every harness reach.

---

## 9 · Bring-up

1. No DevKit, no modules. Bench supply 12.6 V, 0.5 A limit, on J1. Switch off → nothing lit. Switch on → red LED; BAT_SW on
   J2/J3; **TP2 `VAMP` ≈ 11.3 V** (must stay ≤ 12 V); `VBAT_SENSE` ≈ 2.1 V.
2. Each buck alone first: motor buck 6.0 V ±0.2 (adjust), 5 V buck 5.1–5.2 V, polarity right — then plug them into J2/J3/J4.
   Green LED; 5 V on J6.1, J8.1, J9.1 (≈ 4.7 V after D3); 3V3 pins dead until the DevKit is in.
3. Insert the DevKit, flash the migrated firmware over USB; 3.3 V on J7.1/J10.1/J11.1; web console comes up.
4. L298N on J6, wheels up: START → both forward; STOP → hard brake. **Pull the DevKit out while driving → both channels must
   drop out** (ENA/ENB pull-downs; with the DevKit out, ENA/ENB at J6 must read < 0.8 V).
5. RC522 on its cable: card reads + UID log. Toggle the RST pin from the serial console → reader re-initialises.
6. DFPlayer: `v` reads the module volume; play tracks; watch `DF_BUSY` go LOW while playing.
7. LM386 modules in (pots low): both speakers play; turn up while a track plays; touch-check the LM386 chips after 5 min.
8. CAM (SHOOT with the Hub up) → Mega (`FACE` / `STATION 3 0` from the robot) → I²C scan → obstacle input → MUTE button
   (silences, tour keeps running) → BTN2.
9. Full tour 10 min with motors + RFID + audio + Wi-Fi + camera: zero brown-outs; listen for motor whine in the speakers;
   battery reading matches a meter within ≈ 0.1 V after calibration.

---

## 10 · Open items — answer before the schematic

1. **Speakers**: impedance (4 Ω / 8 Ω) and wattage of each.
2. **LM386 module**: photo + calipers (§8.2).
3. **Board space** on the chassis: does ≤ 100 × 100 mm fit, and which edge faces where (sets which side the harnesses exit)?
4. **Mega power pins** with the shield on (§8.5).
5. **Motors**: TT 6 V as calibrated? (Only sets the motor-buck voltage — no board change.)
6. **Buck modules**: which two exact units? The 5 V one must be **fixed-output** (hub rule).
7. **DFPlayer TX voltage** (§8.4).

---

## 11 · Firmware migration (separate task — ESP32_RFID_MP3 → S3, Nano deleted)

### 11.1 Platform

`board = esp32-s3-devkitc-1`, `board_build.arduino.memory_type = qio_opi`, 16 MB flash + partition table,
`-DBOARD_HAS_PSRAM`, **`-DARDUINO_USB_CDC_ON_BOOT=1`** (console on the "USB" port, which frees UART0 for the Mega). Same
recipe as the hub.

### 11.2 Pin map — old (WROOM + Nano) → new (S3)

| Function | Old | New |
|---|---|---|
| RC522 SS / SCK / MOSI / MISO / RST | 5 / 18 / 23 / 19 / — | **10 / 12 / 11 / 13 / 9** |
| DFPlayer S3 TX → DF RX / DF TX → S3 RX / BUSY | 17 / 16 / — | **14 / 17 / 8** (UART1) |
| CAM trigger TX | 14 (SoftwareSerial) | **18** (UART2 TX) |
| Mega link TX / RX | — | **41 / 42** (UART0 remapped) |
| L298N ENA / IN1 / IN2 / IN3 / IN4 / ENB | Nano 10 / 8 / 7 / 11 / 12 / 5 | **4 / 5 / 6 / 7 / 15 / 16** |
| I²C SDA / SCL | — | **40 / 39** |
| Obstacle in / MUTE button / BTN2 | — | **2 / 21 / 47** (active LOW) |
| Battery sense | — | **1** (× 6) |
| Nano link RX / TX | 25 / 26 | deleted |

### 11.3 Motor subsystem absorbs the Nano (unchanged from v0.2)

`driveChannels()` / `brake()` ported 1:1; LEDC 1 kHz, 8-bit; `brake()` first in `setup()`; ML/MR in `Preferences` (NVS)
instead of EEPROM; the whole Nano UART layer deleted; retune ML/MR from the web console after the first wheels-up test.

### 11.4 New features the board enables

- **BUSY**: treat `DF_BUSY` rising (playback ended) as "narration finished" alongside the UART event.
- **MUTE button**: a local mute latch OR-ed with the Hub's `MSG_AUDIO` mute — volume 0, the tour unchanged (same rule as the
  Hub mute).
- **Battery**: show `V_bat` on the console; warn at 10.8 V, stop the tour at 10.5 V (3.5 V/cell).
- **Mega**: send `STATION n 0/1`, `EMERGENCY`, `FACE` on the matching tour events at 9600 baud.
- **Obstacle**: pause the rail while the sensor reads LOW, resume when clear (BUILD item 5).
- **RC522 RST**: if the version register stops answering, pulse RST and re-run `PCD_Init()`.

ESP-NOW, SoftAP channel 1, `MuseumProtocol.h` / `EspNowCompat.h` symlinks: unchanged.

---

## 12 · Deliberately NOT on this board

| Item | Why |
|---|---|
| L298N circuitry, both buck converters | off-board modules with their own terminals, heatsinks and flybacks |
| Arduino Nano | deleted in v0.2 — the S3 drives the L298N |
| WS2812 + 74HCT125 | the robot carries no LED board; the hub owns lighting |
| LM386 chips | the team's existing modules plug in instead |
| RC522, ESP32-CAM, Mega | off-board on their own cables (position matters for each) |
| IR line array | the rail build is sensor-free; EXP pads + I²C cover future sensors |
| Fuses / MOSFET protection | hub v0.3 precedent. An inline blade fuse on the LiPo lead is a cheap off-board option |
| Emergency button / siren | the Hub owns them; the robot gets the emergency over ESP-NOW |

---

## 13 · Sources

- Robot firmware: [`ESP32_RFID_MP3_RAIL/ESP32_RFID_MP3.ino`](../../ESP32_RFID_MP3_RAIL/ESP32_RFID_MP3.ino), [`NANO_LFR_RAIL/NANO_LFR_RAIL.ino`](../../NANO_LFR_RAIL/NANO_LFR_RAIL.ino) · camera link (IO13, 9600): [`firmware/cam/include/config.h`](../../firmware/cam/include/config.h) · display protocol: [`firmware/display/README.md`](../../firmware/display/README.md)
- User wiring diagram (2 × LM386 + 2 speakers, DAC_L/DAC_R): [`robot_wiring diagram.png`](robot_wiring%20diagram.png)
- ESP32-S3 Series Datasheet v2.2 (Espressif): Table 2-1 pin reset states, Table 2-2 power-up glitches, Table 5-4 DC characteristics (VIH = 0.75 × VDD, VOH = 0.8 × VDD, 45 kΩ pulls) — https://documentation.espressif.com/esp32-s3_datasheet_en.pdf
- DFPlayer Mini (DFRobot DFR0299 wiki): 3.2–5 V, 1 kΩ on RX, SPK < 3 W — https://wiki.dfrobot.com/dfplayer_mini_sku_dfr0299 · TX level discussion: https://www.dfrobot.com/forum/topic/321531 · DAC coupling caps on the module: https://dfrobot.com/forum/topic/28525
- LM386 module (LM386M-1, 4–12 V, 41 × 14 mm, R1 0 Ω → gain 20): https://protosupplies.com/product/lm386-audio-amplifier-module/
- Generic L298N board teardown (ENA/ENB jumpers to +5 V, IN pins direct, 5V_EN): https://newscrewdriver.com/2021/01/28/circuit-schematic-of-generic-l298n-driver-board/
- ESP32-CAM brown-out fix (470 µF at 5 V): https://www.makerguides.com/fix-brownout-of-esp32-cam/
- YD-ESP32-S3 facts (5Vin diode, IN-OUT/USB-OTG jumpers, CJ6107 LDO, rows): [`../hub/WIRING-PLAN.md`](../hub/WIRING-PLAN.md) §3 and hub v0.2 sources
- LCSC part numbers: live `lib_Device.search` via the EasyEDA bridge (`PCB design/scripts/robot/01…06_*.js`)
