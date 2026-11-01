# Museum ROBOT Carrier — Wiring Spec v0.2 (single YD-ESP32-S3 N16R8, hand-soldered through-hole) — SUPERSEDED by WIRING-PLAN.md v0.3

> **Status: SUPERSEDED (2026-10-13).** Kept for history; do **not** build from this file. Replaced by [`WIRING-PLAN.md`](WIRING-PLAN.md) v0.3
> (two LM386 modules on a filtered battery rail, RC522 on a cable, fixed DFPlayer TX level, Mega data link, new sensor/I²C/button ports).
> Supersedes v0.1 (ESP32 DevKit V1 + Arduino Nano dual-controller) → kept in [`WIRING-PLAN-v0.1.md`](WIRING-PLAN-v0.1.md).
> Companion plan: the hub carrier, [`../hub/WIRING-PLAN.md`](../hub/WIRING-PLAN.md) — same DevKit, same design language (modules and bucks stay
> off-board, every off-board wire ends in a connector, 100 % through-hole, bare PCB order, no JLC assembly). One spare YD-ESP32-S3 can now rescue either board.
> Firmware today: [`ESP32_RFID_MP3_RAIL/ESP32_RFID_MP3.ino`](../../ESP32_RFID_MP3_RAIL/ESP32_RFID_MP3.ino) (WROOM + Nano). §11 is the migration task that makes it match this board.

---

## 0 · What this board is

A hand-soldered carrier for **one YD-ESP32-S3 N16R8 DevKit** that runs the whole robot: RFID, audio, camera
trigger, web console, ESP-NOW emergency link — and now the motors too. The Arduino Nano and its UART link
are **gone** (user decision, see box). The board carries **zero logic chips**: every level shift is either
unnecessary (RC522, CAM trigger are already 3.3 V) or a resistor (DFPlayer UART).

| Decision | Choice | Why |
|---|---|---|
| MCU | **YD-ESP32-S3 N16R8** (2 × 22 pins, 2 × USB-C), socketed in 2 × 1×22 female headers | same DevKit as the hub; 26+ safe GPIOs ≥ the 13 the robot needs; no second controller, no inter-MCU divider, no flash-disconnect jumpers |
| Motor drive | **External L298N red module** on an XH-8 cable, driven directly by the S3's LEDC PWM | a 2 V-drop bipolar driver belongs off-board with its own screw terminals (same call as the hub's external buck) |
| Power select | One **KCD4-203-B** panel rocker (same part family as the hub), **both poles in parallel** = DPST | one battery, one switch; paralleled poles double the contact rating for the ≈12 A worst case |
| Protection | **None** (no fuses, MOSFETs or power diodes — hub v0.3 precedent). Unplug the LiPo when not in use | beginner-solderable; the rocker upstream of everything is the master cut-off |
| 6 V motor rail | Adjustable **6 V / ≥5 A buck** fed on **XT60**, output wired **directly** to the L298N VS/GND screws | motor current never crosses the carrier |
| 5 V logic rail | Fixed **5 V / ≥5 A buck** fed on **XT60**, output returns on **XT30** — a different size, so a LiPo can never be plugged into the 5 V rail | hub gender scheme verbatim |
| Audio | **DFPlayer Mini in a 2×8 socket** + **LM386 amplifier module lying flat on the PCB** (its male headers plug down into carrier sockets), speaker on XH-2 | module stays removable; DAC path as in the legacy working build — never the bridged SPK pins into an amp |
| Mega (face display) | **2-pin power port (5 V + GND)** only — the serial link stays a firmware task | powers the Mega/TFT from the robot rail; §10 covers the future TX-only link |
| Indication | Two LEDs (battery switched, 5 V present) — **no WS2812, no 74HCT125** | user decision: the robot carries no LED board; the hub owns lighting |

> **Why the Nano is gone.** It was never a pin-count fix: even the legacy single-ESP32 plan concluded the
> 30-pin WROOM had "enough safe GPIOs … no expander" ([`plans/robot-wiring.md`](../../plans/robot-wiring.md) §11). The Nano was a leftover
> from the line-follower era; in the shipped rail build it only toggled 6 L298N pins on a 9600-baud
> START/STOP. The S3 absorbs that job with hardware LEDC PWM, deletes the whole UART protocol layer, and
> the two 10 k pull-downs on ENA/ENB (§3.3) give the boot-time motor-off guarantee the Nano used to provide.

### Reference dimensions (measure again with calipers before layout)

| Board | Size | Header geometry |
|---|---|---|
| YD-ESP32-S3 N16R8 | 27.94 × 57.15 mm | 2 × 22 pins, 2.54 mm; rows **25.40 mm** apart; pin 1 at 1.91 mm from the top edge; antenna overhangs the top edge by 6.24 mm — drawing: [`../hub/YD-ESP32-S3 N16R8_footprint.png`](../hub/YD-ESP32-S3%20N16R8_footprint.png) |
| DFPlayer Mini | ≈ 20.3 × 20.3 mm | 2 × 8 pins, 2.54 mm; rows **18.0 mm** apart (LCSC footprint `COMM-TH_L20.3-W20.3-P2.54-LS18.00`, C9900285937) |
| RC522 module | ≈ 60 × 40 mm | 1 × 8 pins, 2.54 mm (usually right-angle male) — plugs straight into a 1 × 8 female header |
| LM386 amplifier module | ≈ 21 × 16 mm (typical) | input header 1 × 3 (VCC / IN / GND per module silk — **verify, §8**), output = 2 screw terminals or 1 × 2; module lies **flat, male headers underneath** |
| L298N module (red) | ≈ 43 × 43 mm | 1 × 6 control header (ENA IN1 IN2 IN3 IN4 ENB) + VS/GND/5V screw terminals + 2 motor terminals |
| KCD4-203-B rocker | panel mount | 2 rows × 3 spade tabs (6.3 mm), rows 11 mm apart, tabs 10.5 mm apart (drawing: [`../hub/Switch_KCD4-203-B-dimension.jpg`](../hub/Switch_KCD4-203-B-dimension.jpg)) |

---

## 1 · Power path

```
3S LiPo ──XT60 female lead──► J1 BAT IN (XT60PW-M, board male)
   BAT+ ─► J5 VH-4 harness ─► panel rocker KCD4 (BOTH poles in parallel) ─► J5 back to board
   BAT− ─► GND directly (single source — no source-isolation needed, unlike the hub)

VBAT_SW ─┬─► J2 XT60PW-F (female) ──► adjustable 6 V / ≥5 A buck IN
         │        buck OUT 6 V + GND ──► L298N module VS / GND screw terminals (dedicated pair, off-board)
         └─► J3 XT60PW-F (female) ──► fixed 5 V / ≥5 A buck IN
                  buck OUT 5 V ──► XT30 female lead ──► J4 XT30PW-M (board male) = +5V rail

+5V rail feeds: DevKit 5Vin (its 1 A CJ6107 LDO makes 3V3), DFPlayer VCC, LM386 module VCC,
                ESP32-CAM port, L298N 5V logic pin, Mega power port (J9)
Red LED + 2.2 kΩ : VBAT_SW → GND   lights whenever the switch is on (battery present)
Green LED + 1 kΩ : +5V → GND       lights when the logic buck output is present
```

- The switch breaks only the **+** leg; both poles in parallel share the load. Only throw "I" of each pole
  is wired, so `I` = on, `O` = off, `II` = off.
- **XT gender follows the hub convention:** battery lead (XT60 female) fits only J1; both buck input leads
  (XT60 male) fit only J2/J3; the buck 5 V output lead (XT30 female) fits only J4. Wrong plugging is
  physically impossible.
- The S3 gets +5 V straight into its 5Vin pin. The YD board's own input diode stops USB from back-feeding
  the rail — **keep the YD board's IN-OUT solder jumper open** (hub rule).
- +3V3 for RC522 and the optional IR array comes from the DevKit's LDO: ≈ 30–80 mA — fine.
- **Star ground:** battery −, both buck IN −, the XT30 return − and every module ground meet on the
  carrier's GND bus. The L298N's *motor* return runs buck-OUT− → L298N GND screw as its own thick pair;
  only the XH-8 signal ground ties the module logic to the carrier.

### Current budget

| 5 V load | Typical | Worst case |
|---|---|---|
| ESP32-S3 + Wi-Fi AP + web console + RFID | 0.3 A | 0.7 A |
| DFPlayer + speaker path | 0.3 A | 0.7 A (3 W peaks) |
| LM386 amplifier module | 0.05 A idle | 0.3 A (loud) |
| Arduino Mega + 3.2″ TFT (J9) | 0.25 A | 0.5 A |
| ESP32-CAM (idle, flash LED off) | 0.09 A | 0.3 A (Wi-Fi TX) |
| L298N logic | 0.05 A | 0.1 A |
| **Total 5 V** | **≈ 1.0 A** | **≈ 2.6 A** |

A 5 V/5 A buck has huge margin. The 6 V motor rail sees two TT motors: ≈ 0.6 A running, ≈ 2–3 A both
stalled — an adjustable ≥5 A buck covers it, minus the L298N's 1.5–2.5 V internal drop (tune from 6.0 V up
while measuring motor-terminal voltage). Battery worst case ≈ 5 A total → a 2200 mAh 3S pack runs ≈ 40 min
of driving.

---

## 2 · Ports and harnesses

### 2.1 Port table

| Ref | Port | Board connector (LCSC) | Pins 1 → n |
|---|---|---|---|
| J1 | Battery in | XT60PW-M, male, right-angle (C98732) | +, − |
| J2 | 6 V motor-buck feed | XT60PW-F, female (C428722) | +, − |
| J3 | 5 V logic-buck feed | XT60PW-F, female (C428722) | +, − |
| J4 | 5 V ← buck | XT30PW-M, male (C53373776) | +, − |
| J5 | Rocker switch harness | JST VH 4-pin B4P-VH (C485344) | BAT+ (pole-1 COM), BAT_SW (pole-1 throw), BAT+ (pole-2 COM), BAT_SW (pole-2 throw) |
| J6 | L298N module | JST XH 8-pin B8B-XH-A(LF)(SN) (C157972) | 5V, GND, ENA, IN1, IN2, IN3, IN4, ENB |
| J7 | Speaker | JST XH 2-pin B2B-XH-A-BK (C378939) | OUT+, OUT− (from the LM386 module) |
| J8 | ESP32-CAM | JST XH 3-pin B3B-XH-A (C144394) | 5V, GND, CAM_SIG (S3 UART2 TX → cam GPIO13) |
| J9 | **Mega 5 V power** ("Molex-type" 2-pin) | JST XH 2-pin (C378939) | 5V, GND |
| U3 | RC522 module | 1×8 female header PM254V-11-08-H85 (C46595973) | module silk order: SDA, SCK, MOSI, MISO, IRQ, GND, RST, 3V3 |
| U5, U6 | DFPlayer Mini | 2 × 1×8 female header (C46595973), rows **18.0 mm** apart | drop-in socket for the module |
| U8 | LM386 amp module (**flat-mounted**) | 1×3 female (C541850) + 1×2 female (C541849), placed on the module's own header grid | VCC, IN, GND + OUT+, OUT− |
| U1 | YD-ESP32-S3 socket | 2 × 1×22 female header PM254V-11-22-H85 (C46595983), rows 25.40 mm | §4 |

Wire gauges: battery + buck feeds 16 AWG · XT30 return 16 AWG · L298N harness 22 AWG (signals only) ·
J7/J8/J9 and all other XH 22 AWG (XH max 22 AWG).

### 2.2 Rules

1. **One family per job:** XT60 = battery and buck feeds, XT30 = 5 V return, VH = front panel, XH = signals
   and module power. Families never cross-mate; an XT60 plug physically cannot reach J4.
2. **XH pin order: supply on pin 1, GND on pin 2, signals after.** Any XH plug in any same-size XH port
   can never see reversed polarity. J9 is deliberately a 2-pin power-only port — no signal pin to mix up.
3. **Unplug while flashing:** J8 (CAM) out when flashing the ESP32-CAM over its CAM-MB USB; the S3 flashes
   over either of its own USB-C ports with the carrier powered or not (nothing shares the console UART).
4. **DFPlayer socket orientation:** the module's silkscreened pin 1 (VCC) faces the same corner as U5/U6's
   pin-1 marks; verify with the real module before ordering (§8).
5. **LM386 module lies flat, headers underneath** — never mount it vertical: the volume pot and caps end up
   sideways and the module leverages its own solder joints. If its tallest part exceeds the 8.5 mm socket
   height, fit 11.5 mm sockets or M3 nylon standoffs in the H5/H6 holes under its corners (§7, §10).

---

## 3 · On-board circuits

### 3.1 Audio path — DFPlayer socket + LM386 module + speaker

```
S3 GPIO14 (UART1 TX) ──► 1 kΩ ──► U5.2 DF RX
U6.3 DF TX ── SB1 (default CLOSED) ──┬── R1 10 k ──┬──► S3 GPIO17 (UART1 RX)
                                    R2 20 k       │
                                    └── GND ──────┘
DFPlayer DAC_R (U5.4) ──► U8 IN   (line-level path)
U8 VCC → +5V · U8 GND → GND
U8 OUT+ / OUT− ──► J7 speaker
SPK1 (U6.8) and SPK2 (U6.6): NOT CONNECTED on this carrier
```

- The 10 k/20 k divider (≈3.33 V) guards the S3's RX against DFPlayer clones with 5 V TTL; **measure DF TX
  idle voltage first** (§8) — at 3.3 V the divider is harmless. SB1 exists so a true-5 V module can be
  reworked without cutting the PCB.
- **DAC out, never SPK out, into the LM386** (legacy plan rule): SPK1/SPK2 are a bridged power pair —
  grounding either side through a ground-referenced amp input distorts or damages the output stage. The
  two socket positions simply have no nets.
- 470 µF + 100 nF across DFPlayer VCC/GND at the socket (audio transient reservoir); 100 µF at U8 VCC if
  the module lacks its own bulk cap.
- An LM386 at 5 V will not reach a 3 W speaker's rating — acceptable at museum volume; the class-D upgrade
  path is a module swap, nothing on this board changes.

### 3.2 RC522 (U3) — straight 3.3 V pass-through

- 1:1 with the module's silk: SDA→GPIO21, SCK→GPIO12, MOSI→GPIO11, MISO→GPIO13, IRQ → not connected (the
  MFRC522v2 driver doesn't use it), GND, RST → **not connected** (the driver boots the chip without a reset
  line, same as the flashed sketch), 3V3.
- 100 nF + 10 µF right at U3's power pins. Keep the header ≤ 15 mm of trace from the DevKit.

### 3.3 Motor driver signals (L298N on J6)

| J6 pin | Net | S3 GPIO | Conditioning |
|---|---|---|---|
| 1 | 5V | — | feeds the module's logic with its 5V_EN jumper **removed** |
| 2 | GND | — | signal ground reference |
| 3 | ENA | 4 | LEDC ch 0 PWM + **10 k pull-down to GND** |
| 4 | IN1 | 5 | direct |
| 5 | IN2 | 6 | direct |
| 6 | IN3 | 7 | direct |
| 7 | IN4 | 15 | direct |
| 8 | ENB | 16 | LEDC ch 1 PWM + **10 k pull-down to GND** |

- The two pull-downs are the **boot-safe brake**: while the S3 resets or reflashes, ENA/ENB sit LOW → both
  bridge outputs disabled → wheels cannot move. This replaces the guarantee the Nano's boot-brake used to
  give. They are also why the motors stay off through a brown-out reboot.
- LEDC frequency: start at **1 kHz** (AVR-like, L298N-friendly; the bipolar bridge dislikes tens of kHz).
  The ML=150 / MR=210 trims were calibrated at the Nano's ~490 Hz — retune from the web console (§11).
- Motor power never touches this board: the 6 V buck's output pair goes to the module's VS/GND screws
  directly (§1). Remove the module's ENA/ENB jumper caps — the S3 owns the PWM.

### 3.4 IR array port (optional — the rail build ships sensor-free)

- 1×7 female header U7: 3V3, GND, FL→GPIO1, L→GPIO2, C→GPIO8, R→GPIO9, FR→GPIO10 — all **ADC1** (GPIO1–10),
  which keeps working with Wi-Fi on, and none of them is a strapping pin.
- The S3 has **no input-only pins** and every GPIO has real internal pull-ups — the classic board's
  GPIO34–39 problem disappears. Use `INPUT_PULLUP` in firmware; fit the five DNP 10 k footprints only if
  the array turns out open-collector (measure over black/white first).
- 3V3-only port, silkscreened as such.

### 3.5 Other

- **Source LEDs**: red LED + 2.2 kΩ from VBAT_SW, green LED + 1 kΩ from +5V (§1). No transistors needed.
- **Capacitors**: 470 µF at the DFPlayer socket (fitted) · 100 nF at U3 + U5/U6 (fitted) · 10 µF at U3 ·
  nothing at J4 — the buck has its own output caps.
- **Test points**: VBAT, VBAT_SW, +5V, +3V3, GND ×2 (bring-up without a scope).

---

## 4 · DevKit pin map (U1)

U1A = left 22-pin header, U1B = right 22-pin header, pin 1 at the antenna end (matches the hub's numbering
of the same board).

| U1A pin | GPIO | Net | | U1B pin | GPIO | Net |
|---|---|---|---|---|---|---|
| 1 | 3V3 | +3V3 | | 1 | GND | GND |
| 2 | 3V3 | +3V3 | | 2 | 43 | — (USB-UART) |
| 3 | RST | — | | 3 | 44 | — (USB-UART) |
| 4 | 4 | ENA → J6.3 | | 4 | 1 | IR_FL (optional U7) |
| 5 | 5 | IN1 → J6.4 | | 5 | 2 | IR_L (optional U7) |
| 6 | 6 | IN2 → J6.5 | | 6 | 42 | — (spare) |
| 7 | 7 | IN3 → J6.6 | | 7 | 41 | — (spare) |
| 8 | 15 | IN4 → J6.7 | | 8 | 40 | — (spare) |
| 9 | 16 | ENB → J6.8 | | 9 | 39 | — (spare) |
| 10 | 17 | DF_TX_IN ← DF TX (UART1 RX) | | 10 | 38 | — (spare) |
| 11 | 18 | CAM_TX → J8.3 (UART2 TX) | | 11 | 37 | — (octal PSRAM) |
| 12 | 8 | IR_C (optional U7) | | 12 | 36 | — (octal PSRAM) |
| 13 | 3 | — (strapping, unused) | | 13 | 35 | — (octal PSRAM) |
| 14 | 46 | — (strapping, unused) | | 14 | 0 | — (BOOT) |
| 15 | 9 | IR_R (optional U7) | | 15 | 45 | — (strapping, unused) |
| 16 | 10 | IR_FR (optional U7) | | 16 | 48 | — (spare; onboard RGB) |
| 17 | 11 | SPI_MOSI → U3 | | 17 | 47 | — (spare) |
| 18 | 12 | SPI_SCK → U3 | | 18 | 21 | SPI_SS → U3 |
| 19 | 13 | SPI_MISO ← U3 | | 19 | 20 | — (USB D+) |
| 20 | 14 | DF_TX_OUT → 1 k → DF RX | | 20 | 19 | — (USB D−) |
| 21 | 5V | +5V | | 21 | GND | GND |
| 22 | GND | GND | | 22 | GND | GND |

Strapping summary: GPIO0/3/45/46 carry nothing on this carrier, so boot stays clean. S3 facts (ADC2 vs
Wi-Fi, no input-only pins, 5 V-in diode, antenna keep-out) are documented once in the hub plan §3 and all
apply here unchanged. Spares 38–42, 47, 48 remain for the future Mega serial link / expansions.

---

## 5 · Connection list (schematic source of truth)

**Power**
- `BAT+`: J1.1, J5.1, J5.3.
- `BAT_SW`: J5.2, J5.4, J2.1, J3.1, R3 (2.2 k) → LED1 red anode (cathode → GND), TP1.
- `GND`: J1.2, J2.2, J3.2, J4.2, J5 (no GND — the rocker breaks + only), U1A.22, U1B.1/21/22, U3 GND,
  U5.7/U6.7 (DF GND), U8 GND, J6.2, J7.2, J8.2, J9.2, R2, both LED cathodes, both ENA/ENB pull-downs, TP6.
- `+5V`: J4.1, U1A.21, U5.1/U6.1 (DF VCC), U8 VCC, J6.1, J8.1, J9.1, R4 (1 k) → LED2 green anode
  (cathode → GND), TP2.
- `+3V3`: U1A.1, U1A.2, U3.8, U7.1 (optional), DNP pull-up row, TP3.

**RC522**: `SPI_SS` U1B.18 → U3.1 · `SPI_SCK` U1A.18 → U3.2 · `SPI_MOSI` U1A.17 → U3.3 ·
`SPI_MISO` U3.4 → U1A.19 · C 100 nF + 10 µF at U3.

**DFPlayer + audio**: `DF_TX_OUT` U1A.20 → 1 k → U5.2 · `DF_TX_IN` U6.3 → SB1 → divider (10 k/20 k) →
U1A.10 · 470 µF + 100 nF at VCC · `DAC_R` U5.4 → U8 IN · U8 OUT+ → J7.1, U8 OUT− → J7.2 ·
`SPK1`/`SPK2` socket pins: no net.

**Motor**: `ENA` U1A.4 → J6.3 (+10 k → GND) · `IN1` U1A.5 → J6.4 · `IN2` U1A.6 → J6.5 · `IN3` U1A.7 →
J6.6 · `IN4` U1A.8 → J6.7 · `ENB` U1A.9 → J6.8 (+10 k → GND).

**CAM trigger**: `CAM_TX` U1A.11 → J8.3 (3.3 V both ends, direct).

**IR (optional)**: U7.3 → U1B.4 · U7.4 → U1B.5 · U7.5 → U1A.12 · U7.6 → U1A.15 · U7.7 → U1A.16;
five DNP 10 k to +3V3.

---

## 6 · Bill of materials (hand-soldered; LCSC numbers verified live via the EasyEDA bridge, 2026-10-12)

| Qty | Refs | Part | LCSC | Notes |
|---|---|---|---|---|
| 1 | J1 | XT60PW-M (male, right-angle) | C98732 | |
| 2 | J2, J3 | XT60PW-F (female, right-angle) | C428722 | |
| 1 | J4 | XT30PW-M (male, right-angle) | C53373776 | |
| 1 | J5 | JST VH 4-pin B4P-VH | C485344 | + VHR-4N housing + 4 × SVH-21T crimps |
| 1 | J6 | JST XH 8-pin B8B-XH-A(LF)(SN) | C157972 | + XHP-8 housing + crimps |
| 2 | J7, J9 | JST XH 2-pin B2B-XH-A-BK(LF)(SN) | C378939 | speaker + Mega 5 V |
| 1 | J8 | JST XH 3-pin B3B-XH-A(LF)(SN) | C144394 | CAM port |
| 2 | U1 | 1×22 female header PM254V-11-22-H85 | C46595983 | rows 25.40 mm |
| 3 | U3, U5, U6 | 1×8 female header PM254V-11-08-H85 | C46595973 | U5/U6 rows 18.0 mm apart |
| 1 | U7 | 1×7 female header PM254V-11-07-H85 | C2832270 | optional IR port |
| 1 | U8-a | 1×3 female header PM254V-11-03-H85 | C541850 | LM386 module input — place on the module's grid |
| 1 | U8-b | 1×2 female header PM254V-11-02-H85 | C541849 | LM386 module output — see §8/§10 first |
| 1 | R1 | 10 kΩ ¼ W axial | C1519255 | DF TX divider upper |
| 1 | R2 | 20 kΩ ¼ W axial | C1364283 | DF TX divider lower |
| 2 | R5, R6 | 10 kΩ ¼ W axial | C1519255 | ENA/ENB boot-safe pull-downs |
| 1 | — | 2.2 kΩ ¼ W axial | C3454390 | red LED |
| 1 | — | 1 kΩ ¼ W axial | C1364484 | green LED (a second 1 k for DF RX series, same reel) |
| 5 | — | 10 kΩ ¼ W axial (DNP) | C1519255 | IR pull-ups, fit only if needed |
| 3 | — | 100 nF ceramic, 5 mm pitch | C342794 | U3, U5/U6 |
| 1 | — | 10 µF 16 V radial | — | RC522 reservoir — buy locally |
| 1 | — | 470 µF 16 V radial | C109066 | DFPlayer socket |
| 2 | LED1, LED2 | 5 mm LED red, green | footprint C9900102778 | buy locally |
| — | TP1–TP6, SB1 | test points / solder jumper | — | no parts |
| 4 + 2 | H1–H4, H5–H6 | M3 holes (3.2 mm) + LM386 standoffs | — | H5/H6 only if the module needs them (§10) |

No ICs, no sockets for ICs, no transistors — the only semiconductors on this board are two LEDs. All
resistors Yageo MFR-25 (1 %, ¼ W) footprints, 10.3 mm pitch — same series as the hub BOM, so one resistor
order covers both boards. Search scripts: `PCB design/scripts/robot/01…03_*.js` (read-only LCSC queries,
2026-10-12). The search API returns no stock figures — re-check at order time.

---

## 7 · Board layout (indicative)

```
 ←────────────────────────────────── ~120 mm ─────────────────────────────────→
 ┌H1──────────── antenna overhang (no copper) ──────┬─────────────────────H2┐
 │ U1 YD-ESP32-S3 (2×22, rows 25.40 mm)             │ J6 XH-8  L298N        │
 │ USB-C lane kept clear ▼                          │ U3 RC522 1×8 vertical │
 ├──────────────────────────────────────────────────┤ U7 IR 1×7 (optional)  │
 │ POWER ZONE            │ J5 VH-4 (rocker)         │ pull-up row (DNP)     │
 │ J1 XT60 BAT IN        │ LED1 red / LED2 green    ├───────────────────────┤
 │ J2 XT60 → motor buck  │ TP1–TP6                  │ U5/U6 DFPLAYER 2×8    │
 │ J3 XT60 → logic buck  ├──────────────────────────┤ @18.0 mm + 470 µF     │
 │ J4 XT30 5V IN         │                          │ U8 LM386 module FLAT  │
 │                       │                          │ J7 speaker · J8 CAM   │
 │                       │                          │ J9 MEGA 5V            │
 └H3─────────────────────┴──────────────────────────┴──────────────H4──────┘
```

- 2 layers, 1.6 mm FR-4, 1 oz copper, HASL. **Target ≈ 120 × 100 mm.**
- DevKit antenna flush with the top edge (module overhangs), copper keep-out on both layers, no XT60 or
  electrolytic within 15 mm — the robot's SoftAP serves the field console.
- The 5 V spine runs J4 → DevKit → audio/CAM/L298N/J9 as a ≥ 3 mm pour on both layers; bottom layer solid
  GND. Motor signals fan out short and direct (U1A.4–9 → J6 on one straight run).
- DFPlayer socket near a board edge so the microSD swaps in place; **U8 LM386 flat next to it** with J7 at
  the board edge — speaker wiring stays away from the RC522 antenna area.
- Only low parts under the DevKit (female headers give ≈ 8.5 mm); the LM386 module itself sits on top of
  its own sockets.
- Silkscreen: every port named with its partner ("L298N module", "CAM GPIO13", "MEGA 5V"); U3 pin-1 corner
  marked SDA; U5 pin-1 marked VCC; J7 marked "amp OUT — from U8"; SB1 marked "DF TX divider bypass".
- Keep a clear lane in front of both USB-C ports.

---

## 8 · Checks before ordering

1. **Caliper the S3 rows** (expect 25.40 mm — hub drawing) and the DFPlayer rows (expect 18.0 mm per the
   LCSC footprint); the real modules are the truth, the footprint numbers are the bet.
2. **LM386 module**: put the real module on paper — mark its input-header pin order (VCC/IN/GND varies by
   clone), its output terminal type (screw vs header), the pin grid for U8-a/U8-b, and its tallest
   component. If the pot is taller than 8.5 mm, order 11.5 mm sockets or plan H5/H6 standoffs.
3. **L298N module**: confirm the 6-pin control header order (ENA IN1 IN2 IN3 IN4 ENB) and its 5V_EN jumper
   convention against the real module photo.
4. **DFPlayer TX idle voltage**: powered at 5 V with nothing on TX — record next to SB1 (divider stays
   either way; the number tells you whether SB1 ever needs opening).
5. **Rocker continuity**: which spade tab joins the middle tab in position I (hub §2.3 gotcha) — check with
   a meter before crimping the VH-4 harness.
6. Print the PCB at 1:1 and drop the DevKit, DFPlayer, RC522 and LM386 module on the paper; check the USB
   lanes and the harness reaches.

---

## 9 · Bring-up

1. No DevKit, no modules. Bench supply (3S-equivalent, 0.5 A limit) on J1: switch off → nothing lit;
   switch on → red LED, VBAT_SW on J2/J3.
2. Check each buck **alone** first: 6 V ±0.2 on the motor buck (adjust), 5.0–5.2 V on the logic buck,
   correct polarity — *then* plug their leads into J2/J3/J4. Green LED, +5 V on J6.1, J7-none, J8.1, J9.1.
3. Insert the DevKit: flash the migrated S3 firmware over USB; web console `ESP32-MP3-RAIL` comes up;
   motor panel shows local ML/MR.
4. L298N on J6, wheels up: `START` → both motors forward; `STOP` → hard brake. **Pull the DevKit out while
   driving: both motor channels must drop out** (ENA/ENB pull-downs) — this is the Nano-replacement test.
5. Port by port: RC522 (card reads + UID log) → DFPlayer (`v` reads the module volume; walk tracks) →
   LM386 + speaker (volume pot low first) → CAM (SHOOT 1 only with the Hub up) → J9 Mega powers its face.
6. Full tour 10 min (rail + RFID + audio + Wi-Fi + camera): zero brown-outs; touch-check the bucks, L298N
   and DFPlayer; confirm the ESP-NOW emergency link against the hub (channel 1).

---

## 10 · Open items — answer before the schematic

1. **LM386 module**: photo + caliper of the exact module (input pin order, output terminal type, pin grid,
   tallest part) → fixes U8-a/U8-b placement and the socket height decision.
2. **Buck modules**: which two exact units? The 5 V one must be *fixed*-output (hub rule: a trim-pot knock
   = 12 V on the ESP32).
3. **Battery**: 3S pack capacity/chemistry, XT60 lead present?
4. **Speaker**: model and wire length (4 Ω 3 W assumed for the budget).
5. **L298N module**: photo (header order + 5V_EN jumper + onboard flybacks present?).
6. **DFPlayer**: measure TX idle voltage on the team's module (§8.4).
7. **Mega serial link**: J9 powers it today. Future TX-only link on a spare GPIO (38–42/47/48 are free;
   all three hardware UARTs are taken — UART0 console, UART1 DFPlayer, UART2 CAM) → needs a small firmware
   task; decide after the tour runs.
8. **IR array**: is line-following coming back? The rail build is deliberately sensor-free; if yes, source
   the 3.3 V 5-channel module and fit U7.
9. **Chassis**: carrier mounting vs the L298N module (harness length); can the DFPlayer microSD be reached
   without demounting?

---

## 11 · Firmware migration (separate task — ESP32_RFID_MP3 → S3, Nano deleted)

### 11.1 Platform / build

- `platformio.ini`: `board = esp32-s3-devkitc-1`, `board_build.arduino.memory_type = qio_opi`,
  `board_build.flash_size` / `board_upload.flash_size = 16MB` + a 16 MB partition table,
  `-DBOARD_HAS_PSRAM`; set the USB-CDC-on-boot flag to match the port used for the serial monitor.
  (Identical recipe to the hub's S3 migration, hub plan §11.)

### 11.2 Pin map — old (WROOM + Nano) → new (S3 only)

| Function | Old pin | New pin |
|---|---|---|
| RC522 SS / SCK / MOSI / MISO | 5 / 18 / 23 / 19 | **21 / 12 / 11 / 13** (FSPI defaults) |
| DFPlayer S3 TX → DF RX | 17 | **14** (UART1 TX, via the on-board 1 k) |
| DFPlayer DF TX → S3 RX | 16 | **17** (UART1 RX, via the on-board divider) |
| CAM trigger TX | 14 (SoftwareSerial) | **18** (hardware UART2 TX — SoftwareSerial deleted) |
| L298N ENA / IN1 / IN2 | Nano 10 / 8 / 7 | **4 / 5 / 6** |
| L298N IN3 / IN4 / ENB | Nano 11 / 12 / 5 | **7 / 15 / 16** |
| WS2812 data | 4 | **removed** (no LED board) |
| Nano link RX / TX | 25 / 26 | **deleted** |
| IR array (optional) | 36 / 39 / 34 / 35 / 32 | **1 / 2 / 8 / 9 / 10** (ADC1) |

### 11.3 Motor subsystem absorbs the Nano

- Port `driveChannels()` / `brake()` from `NANO_LFR_RAIL.ino` unchanged in behaviour: brake = all four IN
  HIGH + EN duty 255; drive = IN1/IN3 HIGH, IN2/IN4 LOW + EN duty = ML/MR.
- PWM: two LEDC channels at **1 kHz**, 8-bit resolution (S3 LEDC replaces AVR `analogWrite`; the bipolar
  L298N dislikes high frequencies). The saved ML=150 / MR=210 were tuned at ~490 Hz — **retune from the web
  console** after first wheels-up test.
- Persistence: replace the Nano's EEPROM `TUNE_MAGIC` block with `Preferences` (NVS): keys `ml`, `mr`.
- Delete the whole UART layer: `sendNano()`, `pollNano()`, `handleNanoLine()`, `nanoTuneConfirmed`,
  `motorCommandPending` and the `READY/STATE/TUNE` parsing. `railRunning` becomes a local flag set by
  `driveChannels()`/`brake()`. `NANO_BAUD` gone. The web `/api/motors` handlers apply values **directly**
  (no `SET`/`GET` round-trip) and `/api/motors/save` writes NVS.
- Boot order: `setup()` configures ENA/ENB LEDC at duty **0** and calls `brake()` **first**, before Wi-Fi
  or RFID — the same "boot is stopped until the START card" invariant the Nano enforced. The on-board
  ENA/ENB pull-downs cover the window before `setup()` runs.
- Delete `NANO_LFR_RAIL/` from the build once the merged robot runs a full tour.

### 11.4 Everything else

- ESP-NOW + SoftAP: unchanged — SSID `ESP32-MP3-RAIL`, channel 1, `static_assert` against
  `museum::kEspNowChannel` keeps working. The hub (S3) and cam (WROOM) peers are unaffected; channel is a
  radio-level property.
- `MuseumProtocol.h` / `EspNowCompat.h` symlinks into `firmware/common/` stay byte-for-byte.
- Optional IR array: five `INPUT_PULLUP` pins — the S3 has internal pulls on every GPIO (no input-only
  pins), so no external resistor unless the array is open-collector.

---

## 12 · Deliberately NOT on this board

| Item | Why |
|---|---|
| L298N chip/module circuitry | the red module with its screw terminals, flybacks and heatsink stays external; the carrier only speaks to it over an XH-8 |
| Both buck converters | hub decision — a ≥5 A switching converter doesn't belong on a hand-routed 2-layer board |
| Arduino Nano | user decision — see §0; its boot-brake job moved to two pull-downs, its tuning job to NVS |
| WS2812 + 74HCT125 buffer | user decision — the robot carries no LED board; the hub owns lighting |
| Fuses / reverse-polarity MOSFETs / TVS | hub v0.3 precedent; the rocker + unplug-the-LiPo rule covers the demo use case |
| Emergency button / buzzer | the Hub owns both; the robot learns about emergencies over ESP-NOW — no wiring |
| Mega serial link | J9 powers the Mega today; the TX-only signal link is a spare-GPIO + firmware task (§10.7) |

---

## 13 · Sources

- Robot brain firmware (pins, DFPlayer, CAM trigger, ESP-NOW, web console): [`ESP32_RFID_MP3_RAIL/ESP32_RFID_MP3.ino`](../../ESP32_RFID_MP3_RAIL/ESP32_RFID_MP3.ino) + [`README.md`](../../ESP32_RFID_MP3_RAIL/README.md)
- Deleted rail-driver firmware (driveChannels/brake/tuning being absorbed): [`NANO_LFR_RAIL/NANO_LFR_RAIL.ino`](../../NANO_LFR_RAIL/NANO_LFR_RAIL.ino)
- Camera-side link facts (RX on GPIO13): [`firmware/cam/include/config.h`](../../firmware/cam/include/config.h)
- Pin-conflict proof that kept the Mega link off the WROOM board (now moot on the S3): `findings.md` (2026-08-11 audit)
- DAC→LM386 audio rule and LM386 power reality: [`plans/robot-wiring.md`](../../plans/robot-wiring.md) §6 · power/ground architecture §8
- YD-ESP32-S3 N16R8 facts (rows 25.40 mm, strapping pins, ADC1 rule, LDO, qio_opi recipe): [`../hub/WIRING-PLAN.md`](../hub/WIRING-PLAN.md) §3/§10/§11 + [`../hub/YD-ESP32-S3 N16R8_footprint.png`](../hub/YD-ESP32-S3%20N16R8_footprint.png)
- DFPlayer Mini pin/SPK facts: DFRobot DFPLAYER MINI manual (manualslib.com/manual/1731781) — DAC pins, bridged SPK pair ≤ 3 W
- LCSC part numbers: live `lib_Device.search` via the EasyEDA bridge, 2026-10-12 (`PCB design/scripts/robot/01…03_*.js`)
- KCD4-203-B rocker drawing: [`../hub/Switch_KCD4-203-B-dimension.jpg`](../hub/Switch_KCD4-203-B-dimension.jpg)