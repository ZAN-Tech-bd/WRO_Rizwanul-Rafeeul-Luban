# Museum ROBOT Carrier — Wiring Spec v0.1 (two-controller rail build) — SUPERSEDED by WIRING-PLAN.md v0.2

> **Status: SUPERSEDED (2026-10-12).** Kept for history; do **not** build from this file.
> Companion plan: the hub carrier, [`../hub/WIRING-PLAN.md`](../hub/WIRING-PLAN.md) (same design language: modules stay off the board, bucks stay
> off-board, every off-board wire ends in a connector, 100 % through-hole, bare PCB order — no JLC assembly).
> Firmware source of truth: [`ESP32_RFID_MP3_RAIL/ESP32_RFID_MP3.ino`](../../ESP32_RFID_MP3_RAIL/ESP32_RFID_MP3.ino) (brain) and
> [`NANO_LFR_RAIL/NANO_LFR_RAIL.ino`](../../NANO_LFR_RAIL/NANO_LFR_RAIL.ino) (rail drive). The board below mirrors those pins **exactly** — no firmware changes required.

---

## 0 · What this board is

A hand-soldered carrier that replaces the robot's loose jumper-wiring with one PCB. It **sockets both
controllers** (the ESP32 DevKit V1 brain and the Arduino Nano rail driver), conditions every inter-board
link, and gives every module (RC522, DFPlayer Mini, ESP32-CAM, speaker, LED board, L298N module) its own
connector. The hot and bulky parts — the L298N motor module and both buck converters — stay **off-board**,
exactly like the hub keeps its 10 A buck off-board.

| Decision | Choice | Why |
|---|---|---|
| Brain | **DOIT-style ESP32 DevKit V1, 30-pin** (ESP32-WROOM-32), socketed in 2 × 15 female headers | the flashed firmware's pins are locked to this board; replaceable, no RF/USB design on our PCB |
| Rail driver | **Arduino Nano** (ATmega328P, 30-pin DIP layout), socketed in 2 × 15 female headers | the shipped `NANO_LFR_RAIL` sketch owns the L298N; a socket lets either board be reflashed on the bench |
| Motor driver | **External L298N red module** on an XH-8 cable | a 2 V-drop bipolar driver belongs off-board with its own screw terminals; same reasoning as the hub's external buck |
| Power select | One **KCD4-203-B** panel rocker (same part family as the hub), **both poles in parallel** = DPST | one battery, one switch; paralleling the two poles doubles the contact rating for the ≈12 A worst case |
| Protection | **None** (no fuses, MOSFETs or power diodes — same user decision as hub v0.3). Unplug the LiPo when not in use | keeps the build beginner-solderable; the switch upstream of everything is the master cut-off |
| 6 V motor rail | Adjustable **6 V / ≥5 A buck** fed from the board on **XT60**, its output wired **directly** to the L298N module's VS/GND screws | motor current never crosses the carrier PCB |
| 5 V logic rail | Fixed **5 V / ≥5 A buck** fed on **XT60**, its output returns on **XT30** — a different size, so a battery pack can never be plugged into the 5 V rail | hub precedent; a LiPo and an XT30 physically don't mate |
| Level shifts | **74HCT125** DIP-14 (already owned) for the WS2812 data; resistor divider for the Nano→ESP32 UART | one logic chip, same as the hub build |
| Mega display | **Not connected** | GPIO16/17 are owned by the DFPlayer UART2; the documented Mega link would collide (§11) |

### Reference dimensions (measure again with calipers before layout)

| Board | Size | Header geometry |
|---|---|---|
| ESP32 DevKit V1 (DOIT 30-pin) | **51.45 × 28.33 mm** | 2 × 15 pins, 2.54 mm; row spacing **≈ 22.86 mm (0.9″)** on typical DOIT clones — clones vary, calipers before layout; USB at the antenna end |
| Arduino Nano | ≈ 45 × 18 mm | 2 × 15 pins, 2.54 mm; rows **15.24 mm (0.6″)** apart (standard DIP-30) |
| DFPlayer Mini | ≈ 20.3 × 20.3 mm | 2 × 8 pins, 2.54 mm; rows **18.0 mm** apart (LCSC footprint `COMM-TH_L20.3-W20.3-P2.54-LS18.00`, C9900285937) |
| RC522 module | ≈ 60 × 40 mm | 1 × 8 pins, 2.54 mm (usually right-angle male) — plugs straight into a 1 × 8 female header |
| L298N module (red) | ≈ 43 × 43 mm | 1 × 6 control header (ENA IN1 IN2 IN3 IN4 ENB) + VS/GND/5V screw terminals + 2 motor terminals |
| KCD4-203-B rocker | panel mount | 2 rows × 3 spade tabs (6.3 mm), rows 11 mm apart, tabs 10.5 mm apart (drawing in [`../hub/Switch_KCD4-203-B-dimension.jpg`](../hub/Switch_KCD4-203-B-dimension.jpg)) |

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

+5V rail feeds: DevKit VIN (its AMS1117 makes 3V3), Nano 5V pin, DFPlayer VCC,
                74HCT125 VCC, ESP32-CAM port, WS2812 LED port, L298N 5V logic pin
Red LED + 2.2 kΩ : VBAT_SW → GND   lights whenever the switch is on (battery present)
Green LED + 1 kΩ : +5V → GND       lights when the logic buck output is present
```

- The switch breaks only the **+** leg; both poles in parallel share the load. `I` and `II` positions both
  close this harness (only throw "I" of each pole is wired), so the centre `O` is the only OFF.
- **XT gender follows the hub convention:** battery lead (XT60 female) fits only J1; both buck input leads
  (XT60 male) fit only J2/J3; the buck 5 V output lead (XT30 female) fits only J4. Wrong plugging is
  physically impossible.
- The ESP32 gets +5 V straight into its VIN pin. The DevKit's own regulator/diode arrangement stops USB from
  back-feeding the rail; the Nano gets +5 V on its **5V pin** (never its VIN).
- +3V3 for RC522 and the optional IR array comes from the DevKit's AMS1117: ≈ 30–80 mA of load — fine.
- **Star ground:** battery −, both buck IN −, the XT30 return − and every module ground all meet on the
  carrier's GND bus. The L298N's *motor* return runs buck-OUT− → L298N GND screw as its own thick pair;
  only the XH-8 signal ground ties the module logic to the carrier.

### Current budget

| 5 V load | Typical | Worst case |
|---|---|---|
| ESP32-WROOM-32 (DevKit V1), Wi-Fi AP + web console | 0.25 A | 0.5 A |
| DFPlayer + 8 Ω speaker at full volume | 0.3 A | 0.7 A (3 W peaks) |
| 8 × WS2812B LED board | 0.06 A (dim) | 0.48 A (full white) |
| ESP32-CAM (idle, flash LED off) | 0.09 A | 0.3 A (Wi-Fi TX) |
| Nano + 74HCT125 + L298N logic | 0.06 A | 0.1 A |
| **Total 5 V** | **≈ 0.8 A** | **≈ 2.1 A** |

A 5 V/5 A buck has huge margin (the old robot plan's same conclusion). The 6 V motor rail sees two TT
motors: ≈ 0.6 A running, ≈ 2–3 A with both stalled — an adjustable ≥5 A buck covers it, minus the L298N's
1.5–2.5 V internal drop (tune the buck from 6.0 V up while measuring motor-terminal voltage, as the old
plan §4 required). Battery worst case ≈ 5 A total → a 2200 mAh 3S pack runs ≈ 40 min of driving.

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
| J7 | Speaker | JST XH 2-pin B2B-XH-A-BK (C378939) | SPK+, SPK− (bridged pair — never ground either wire) |
| J8 | ESP32-CAM | JST XH 3-pin B3B-XH-A (C144394) | 5V, GND, CAM_SIG (robot TX → cam GPIO13) |
| J9 | WS2812 LED board | JST XH 3-pin (C144394) | 5V, GND, DIN (buffered) |
| U3 | RC522 module | 1×8 female header PM254V-11-08-H85 (C46595973) | module silk order: SDA, SCK, MOSI, MISO, IRQ, GND, RST, 3V3 |
| U5, U6 | DFPlayer Mini | 2 × 1×8 female header PM254V-11-08-H85 (C46595973), rows **18.0 mm** apart | drop-in socket for the module |
| U7 | IR array *(optional)* | 1×7 female header PM254V-11-07-H85 (C2832270) | 3V3, GND, FL, L, C, R, FR |
| JP1, JP2 | Nano-link disconnect | 1×2 pin header + jumper cap (buy locally) | see §3.2 |
| U1 | ESP32 DevKit V1 socket | 2 × 1×15 female header PM254V-11-15-H85 (C46595979) | §4 |
| U4 | Arduino Nano socket | 2 × 1×15 female header PM254V-11-15-H85 (C46595979), rows 15.24 mm | §4 |

Wire gauges: battery + buck feeds 16 AWG · XT30 return 16 AWG · L298N harness 22 AWG (signals only) ·
all other XH 22 AWG.

### 2.2 Rules

1. **One family per job:** XT60 = battery and buck feeds, XT30 = 5 V return, VH = front panel, XH = signals
   and module power. Families never cross-mate; an XT60 plug physically cannot reach J4.
2. **XH pin order: supply on pin 1, GND on pin 2, signals after.** Any XH plug in any same-size XH port
   can never see reversed polarity. J7/J8/J9 are all 3 A-rated XH — fine at their currents.
3. **JP1/JP2 open = Nano USB-safe.** The two Nano↔ESP32 wires are only live with both jumper caps fitted
   (§3.2) — this kills the old "pull two wires before every Nano reflash" tax noted in `BUILD_PLAN.md`.
4. **Unplug while flashing:** J8 (CAM) out when flashing the ESP32-CAM; JP1/JP2 open when flashing the
   Nano; the DevKit flashes over its own USB with the carrier powered or not.
5. **DFPlayer socket orientation:** the module's silkscreened pin 1 (VCC) faces the same corner as U5/U6's
   pin-1 marks; verify with the real module before ordering the PCB (§8).

---

## 3 · On-board circuits

### 3.1 74HCT125 (U2, DIP-14 in a socket C3170835) — WS2812 level shift, same as the hub build

| Pin | Name | Connection |
|---|---|---|
| 1 | /1OE | GND |
| 2 | 1A | GPIO4 (LED_DIN) + 10 k to GND |
| 3 | 1Y | 330 Ω → J9.3 (LED DIN) |
| 4 | /2OE | +5V |
| 5 | 2A | GND |
| 6 | 2Y | not connected |
| 7 | GND | GND |
| 8 | 3Y | not connected |
| 9 | 3A | GND |
| 10 | /3OE | +5V |
| 11 | 4Y | not connected |
| 12 | 4A | GND |
| 13 | /4OE | +5V |
| 14 | VCC | +5V, 100 nF directly across pins 14 and 7 |

The 10 k pull-down keeps the LED board dark while the ESP32 boots (GPIO4 is a strapping-adjacent pin and
floats during reset — do not omit it).

### 3.2 Nano ↔ ESP32 UART conditioning (9600 8N1)

```
Nano D1 (TX) ──► R1 10 k ──┬──► GPIO25 (ESP32 RX1)      (5 V → 3.3 V divider)
                           R2 20 k
                           └──► GND
GPIO26 (ESP32 TX1) ──► JP1 ──► Nano D0 (RX)              (3.3 V is a valid AVR HIGH)
Nano D1 (TX) ──(same node) ──► JP2 ──► divider above     (both jumpers in series with the TX line)
```

- The 10 k/20 k divider is the divider the working build already uses (10 k upper / 20 k lower ≈ 3.33 V).
- **JP1 breaks GPIO26 → D0; JP2 breaks D1 → divider.** Both caps removed ⇒ the Nano's UART pins are free
  for its USB bootloader while the carrier stays assembled. This is the flying-wire problem from
  `BUILD_PLAN.md` status note 3, solved with two 1×2 headers.

### 3.3 DFPlayer Mini conditioning (UART2)

```
GPIO17 (TX2) ──► 1 kΩ ──► U5.2 DF RX          (series resistor, as in the flashed build)
U6 DF TX ──┬── SB1 (solder jumper, default CLOSED) ──┬── R3 10 k ──┬──► GPIO16 (RX2)
           │                                         R4 20 k       │
           │                                         └── GND ──────┘
```

- Default path = the 10 k/20 k divider (safe for any clone). **Measure DF TX's idle voltage first**; if the
  module idles at 3.3 V the divider is harmless and can stay. Closing/cutting SB1 is the emergency bypass
  if a module is ever measured at true 5 V logic and the divider is suspected of edge slowing — it exists
  so the PCB never has to be cut.
- 470 µF + 100 nF across DFPlayer VCC/GND at the socket (audio transient reservoir).
- Speaker on J7 comes from the module's **SPK1/SPK2 bridged pair** (the flashed build's arrangement — not
  DAC→LM386; that idea died with the direct-ESP layout).

### 3.4 RC522 (U3) — straight 3.3 V pass-through

- All eight header pins are 1:1 with the module's silk: SDA→GPIO5, SCK→GPIO18, MOSI→GPIO23, MISO→GPIO19,
  IRQ → not connected (the MFRC522v2 driver doesn't use it), GND, RST → **not connected** (the driver boots
  the chip without a reset line, same as the flashed sketch), 3V3.
- 100 nF + 10 µF right at U3's power pins. Keep the header ≤ 15 mm of trace from the DevKit; the SPI runs
  at the driver's default speed and the module sits directly on the carrier.

### 3.5 IR array port (U7, optional — the rail build ships sensor-free)

- 3V3, GND, then FL→GPIO36, L→GPIO39, C→GPIO34, R→GPIO35, FR→GPIO32 (the exact input-only ADC1 pin set the
  old line-follower build used).
- Footprints for five 10 k pull-ups to 3V3, **not fitted**: GPIO34/35/36/39 have no internal pulls, so if
  the array's outputs turn out open-collector, the resistors are one solder job away. Measure the array
  over black/white first (old plan §3 procedure).
- Powering anything 5 V here is forbidden — the port is 3V3-only and silkscreened as such.

### 3.6 Other

- **Source LEDs**: red LED + 2.2 kΩ from VBAT_SW, green LED + 1 kΩ from +5V (§1). No transistors needed.
- **Capacitors**: 100 nF at U2 (fitted) · 470 µF at U5/U6 DFPlayer (fitted) · 470 µF at J9 LED port
  (footprint only, fit if the LED board shows colour noise) · 100 nF + 10 µF at U3 RC522 · nothing at J4 —
  the buck has its own output caps.
- **Test points**: VBAT, VBAT_SW, +5V, +3V3, GND ×2 (bring-up without a scope).

---

## 4 · Controller pin maps

### 4.1 ESP32 DevKit V1 (U1) — U1A = left header (antenna end, top→bottom), U1B = right header

| U1A pin | GPIO | Net | | U1B pin | GPIO | Net |
|---|---|---|---|---|---|---|
| 1 | EN | — | | 1 | 23 | SPI_MOSI → U3 |
| 2 | 36 (VP) | IR_FL (U7, optional) | | 2 | 22 | — (unused) |
| 3 | 39 (VN) | IR_L (U7, optional) | | 3 | 21 | — (unused) |
| 4 | 34 | IR_C (U7, optional) | | 4 | 19 | SPI_MISO ← U3 |
| 5 | 35 | IR_R (U7, optional) | | 5 | 18 | SPI_SCK → U3 |
| 6 | 32 | IR_FR (U7, optional) | | 6 | 5 | SPI_SS → U3 (strap pin — CS idles HIGH, safe) |
| 7 | 33 | — (unused) | | 7 | 17 | DF_TX → 1 k → DF RX |
| 8 | 25 | NANO_RX ← divider ← D1/JP2 | | 8 | 16 | DF_RX ← divider ← DF TX |
| 9 | 26 | NANO_TX → JP1 → D0 | | 9 | 4 | LED_DIN → U2.2 |
| 10 | 27 | — (unused) | | 10 | 2 | — (onboard LED, strap — leave free) |
| 11 | 14 | CAM_SIG → J8.3 | | 11 | 15 | — (strap — leave free) |
| 12 | 12 | — (strap: must stay LOW — nothing pulls it up) | | 12 | GND | GND |
| 13 | GND | GND | | 13 | 3 (RX0) | — (USB console) |
| 14 | 13 | — (unused) | | 14 | 1 (TX0) | — (USB console) |
| 15 | VIN | +5V | | 15 | 3V3 | +3V3 |

Strapping notes: GPIO5 as CS is fine (straps HIGH by default, which is CS-inactive); GPIO12/15/2 carry
nothing on this carrier, so boot stays clean.

### 4.2 Arduino Nano (U4) — U4A = top row (left→right, USB left), U4B = bottom row

| U4A pin | Name | Net | | U4B pin | Name | Net |
|---|---|---|---|---|---|---|
| 1 | VIN | — (never powered here) | | 1 | D1 (TX) | JP2 → divider → GPIO25 |
| 2 | GND | GND | | 2 | D0 (RX) | JP1 ← GPIO26 |
| 3 | RESET | — | | 3 | RESET | — |
| 4 | 5V | +5V | | 4 | GND | GND |
| 5 | A7 | — | | 5 | D2 | — |
| 6 | A6 | — | | 6 | D3 | — |
| 7 | A5 | — | | 7 | D4 | — |
| 8 | A4 | — | | 8 | D5 | ENB → J6.8 (channel B PWM, "210" side) |
| 9 | A3 | — | | 9 | D6 | — |
| 10 | A2 | — | | 10 | D7 | IN2 → J6.5 (channel A dir) |
| 11 | A1 | — | | 11 | D8 | IN1 → J6.4 (channel A dir) |
| 12 | A0 | — | | 12 | D9 | — |
| 13 | REF | — | | 13 | D10 | ENA → J6.3 (channel A PWM, "150" side) |
| 14 | 3V3 | — | | 14 | D11 | IN3 → J6.6 (channel B dir) |
| 15 | D13 | — (onboard LED) | | 15 | D12 | IN4 → J6.7 (channel B dir) |

This is byte-for-byte the flashed `NANO_LFR_RAIL` map (ENA=10, IN1=8, IN2=7, ENB=5, IN3=11, IN4=12;
channels A/B are the **physically calibrated** left/right motors — do not "fix" the crossover on the PCB).

---

## 5 · Connection list (schematic source of truth)

**Power**
- `BAT+`: J1.1, J5.1, J5.3.
- `BAT_SW`: J5.2, J5.4, J2.1, J3.1, R_LEDR (2.2 k) → LED1 red anode (cathode → GND), TP1.
- `GND`: J1.2, J2.2, J3.2, J4.2, U1A.13, U1B.12, U4A.2, U4B.4, U2.7, U3 GND, U5.7/U6.7 (DF GND),
  J6.2, J7/J8/J9 GND pins, R1–R4, both LED cathodes, TP6.
- `+5V`: J4.1, U1A.15 (VIN), U4A.4 (Nano 5V), U2.14, U2.10, U2.13, U5.1/U6.1 (DF VCC), J6.1, J8.1, J9.1,
  R_LEDG (1 k) → LED2 green anode (cathode → GND), TP2.
- `+3V3`: U1B.15, U3.8, U7.1, DNP pull-up row (§3.5), TP3.

**RC522**: `SPI_SS` U1B.6 → U3.1 · `SPI_SCK` U1B.5 → U3.2 · `SPI_MOSI` U1B.1 → U3.3 ·
`SPI_MISO` U3.4 → U1B.4 · C 100 nF + 10 µF at U3.

**DFPlayer**: `DF_TXOUT` U1B.7 → 1 k → U5.2 · `DF_TXIN` U6.3 → SB1 → divider (10 k/20 k) → U1B.8 ·
470 µF + 100 nF at VCC · `SPK1` U6.8 → J7.1, `SPK2` U6.6 → J7.2.

**Nano link**: `NANO_TX_RAW` U4B.1 → JP2 → R1 10 k → node `NANO_RX_3V3` → U1A.8; R2 20 k node → GND ·
`NANO_RX_RAW` U1A.9 → JP1 → U4B.2.

**L298N**: J6.3 ENA → U4B.13 · J6.4 IN1 → U4B.11 · J6.5 IN2 → U4B.10 · J6.6 IN3 → U4B.14 ·
J6.7 IN4 → U4B.15 · J6.8 ENB → U4B.8 · J6.1 +5V · J6.2 GND.

**LED chain**: `LED_DIN` U1B.9 → U2.2 (10 k to GND); U2.3 → 330 Ω → J9.3.

**CAM trigger**: `CAM_SIG` U1A.11 → J8.3 (3.3 V both ends, direct).

**IR (optional)**: U7.3 → U1A.2 · U7.4 → U1A.3 · U7.5 → U1A.4 · U7.6 → U1A.5 · U7.7 → U1A.6;
five DNP 10 k to +3V3.

---

## 6 · Bill of materials (hand-soldered; LCSC numbers verified live via the EasyEDA bridge, 2026-10-12)

| Qty | Refs | Part | LCSC | Notes |
|---|---|---|---|---|
| 1 | J1 | XT60PW-M (male, right-angle) | C98732 | |
| 2 | J2, J3 | XT60PW-F (female, right-angle) | C428722 | |
| 1 | J4 | XT30PW-M (male, right-angle) | C53373776 | |
| 1 | J5 | JST VH 4-pin B4P-VH | C485344 | + VHR-4N housing + 4 × SVH-21T crimps (rocker harness) |
| 1 | J6 | JST XH 8-pin B8B-XH-A(LF)(SN) | C157972 | + XHP-8 housing + crimps |
| 1 | J7 | JST XH 2-pin B2B-XH-A-BK(LF)(SN) | C378939 | speaker (or a 5.08 screw terminal — pick at schematic step) |
| 2 | J8, J9 | JST XH 3-pin B3B-XH-A(LF)(SN) | C144394 | CAM + LED board |
| 5 | U3, U5, U6, U7 + spare | 1×8 female header PM254V-11-08-H85 | C46595973 | U5/U6 rows 18.0 mm apart (DFPlayer) |
| 1 | U7 | 1×7 female header PM254V-11-07-H85 | C2832270 | optional IR port |
| 4 | U1 ×2, U4 ×2 | 1×15 female header PM254V-11-15-H85 | C46595979 | DevKit rows ≈22.86 mm · Nano rows 15.24 mm |
| 2 | JP1, JP2 | 1×2 male header + jumper caps | — | buy locally |
| 1 | U2 | SN74HCT125N (DIP-14) | C154102 | already owned |
| 1 | — | DIP-14 socket | C3170835 | or buy locally |
| 1 | R1 | 10 kΩ ¼ W axial | C1519255 | Nano divider upper |
| 1 | R2 | 20 kΩ ¼ W axial | C1364283 | Nano divider lower |
| 1 | R3 | 10 kΩ ¼ W axial | C1519255 | DF TX divider upper |
| 1 | R4 | 20 kΩ ¼ W axial | C1364283 | DF TX divider lower |
| 1 | R5 | 10 kΩ ¼ W axial | C1519255 | LED_DIN pull-down |
| 1 | — | 330 Ω ¼ W axial | C1369900 | LED data out |
| 1 | — | 2.2 kΩ ¼ W axial | C3454390 | red LED |
| 1 | — | 1 kΩ ¼ W axial | C1364484 | green LED |
| 5 | — | 10 kΩ ¼ W axial (DNP) | C1519255 | IR pull-ups, fit only if needed |
| 3 | — | 100 nF ceramic, 5 mm pitch | C342794 | U2, U3, DF socket |
| 1 | — | 10 µF 16 V radial | — | RC522 local reservoir — buy locally |
| 2 | — | 470 µF 16 V radial | C109066 | DF socket (fitted) + LED port (footprint only) |
| 2 | LED1, LED2 | 5 mm LED red, green | footprint C9900102778 | buy locally |
| — | TP1–TP6, SB1 | test points / solder jumper | — | no parts |
| 4 | H1–H4 | M3 holes (3.2 mm), corners | — | |

All resistors Yageo MFR-25 (1 %, ¼ W) footprints, 10.3 mm pitch — same series as the hub BOM, so one
resistor order covers both boards. Search scripts: `PCB design/scripts/robot/01…02_*.js` (read-only LCSC
queries, 2026-10-12). The search API returns no stock figures — re-check at order time.

---

## 7 · Board layout (indicative)

```
 ←────────────────────────────────── ~130 mm ──────────────────────────────────→
 ┌H1──────────── antenna overhang (no copper) ──────┬─────────────────────H2┐
 │ U1 DevKit V1 (2×15, rows ≈22.86 mm)              │ J6 XH-8  L298N        │
 │ USB lane kept clear ▼                            │ U3 RC522 1×8 vertical │
 ├──────────────────────────────────────────────────┤ U7 IR 1×7 (optional)  │
 │ POWER ZONE            │ J5 VH-4 (rocker)         │ pull-up row (DNP)     │
 │ J1 XT60 BAT IN        │ LED1 red / LED2 green    ├───────────────────────┤
 │ J2 XT60 → motor buck  │ TP1–TP6                  │ U5/U6 DFPLAYER 2×8    │
 │ J3 XT60 → logic buck  ├──────────────────────────┤ @18.0 mm + 470 µF     │
 │ J4 XT30 5V IN         │ U4 NANO (2×15, 15.24 mm) │ J7 speaker            │
 │                       │ JP1 JP2 disconnect caps  │ J8 CAM · J9 LED DIN   │
 └H3─────────────────────┴──────────────────────────┴──────────────H4──────┘
```

- 2 layers, 1.6 mm FR-4, 1 oz copper, HASL. **Target ≈ 130 × 100 mm** (JLC $-tier needs ≤ 212 × 212 —
  shrink to 100 × 100 only if every port still fits with a finger's width of soldering space).
- DevKit antenna flush with the top edge, copper keep-out under it on both layers, no XT60/electrolytic
  within 15 mm — the robot's SoftAP serves the field console at every event.
- The 5 V spine runs J4 → DevKit/Nano → DF/LED ports as a ≥ 3 mm pour on both layers; XH signal fan-outs
  stay short. Bottom layer solid GND.
- Nano socket between the controllers: JP1/JP2 sit at its left edge (D0/D1 side), caps reachable without
  lifting the board out of the chassis.
- DFPlayer socket near the board edge so the microSD card can be swapped in place; J7 next to it so the
  speaker lead doesn't cross the RC522 antenna area.
- Silkscreen: every port named with its partner ("L298N module", "CAM GPIO13", "LED DIN"); JP1/JP2 marked
  "OPEN = flashing"; U3 pin-1 corner marked SDA; U5 pin-1 marked VCC; J7 marked "SPK bridged — no GND".
- Keep a clear lane in front of both controllers' USB ports.

---

## 8 · Checks before ordering

1. **Caliper the DevKit rows** (expect ≈ 22.86 mm) and the DFPlayer rows (expect 18.0 mm, per the LCSC
   footprint); if the real DFPlayer module differs, move U5/U6 — the footprint numbers are the bet, the
   module is the truth.
2. **L298N module**: confirm the 6-pin control header order (ENA IN1 IN2 IN3 IN4 ENB) against the real
   module photo, and its 5V-regulator jumper convention before wiring J6.1's purpose (§2.1).
3. **DFPlayer TX idle voltage**: powered at 5 V with nothing on TX, expect ≈ 3.3 V on clones — the divider
   stays. Record the measurement next to SB1.
4. **Rocker continuity**: which spade tab joins the middle tab in position I (hub §2.3 gotcha) — check with
   a meter before crimping the VH-4 harness.
5. Print the PCB at 1:1 and drop the DevKit, Nano, DFPlayer and RC522 on the paper; check the USB lanes,
   the LED port reach and the CAM XH reach before ordering.

---

## 9 · Bring-up

1. No controllers, no modules. Bench supply (3S-equivalent, 0.5 A limit) on J1: switch off → nothing lit;
   switch on → red LED, VBAT_SW on J2/J3.
2. Check each buck **alone** first: 6 V ±0.2 on the motor buck (adjust), 5.0–5.2 V on the logic buck,
   correct polarity — *then* plug their leads into J2/J3/J4. Green LED, +5 V on every XH pin 1.
3. Insert the Nano alone: its USB flashes with JP1/JP2 open; boot banner `READY:STOPPED`; STOP/START from
   the serial monitor twitch nothing (L298N not connected yet).
4. Insert the DevKit: flash `ESP32_RFID_MP3.ino` over USB; web console `ESP32-MP3-RAIL` comes up;
   `nanoTuned` stays false — correct, the link jumpers are still open.
5. Fit JP1/JP2: the console now shows `TUNE ML=150 MR=210` from the Nano; `GET`/`SET`/`SAVE` work.
6. L298N module on J6: wheels-up, `START` → both motors forward at the calibrated pair; `STOP` → hard
   brake. Check the 6 V rail under load; tune per §1.
7. Port by port: RC522 (card reads + UID log) → DFPlayer (`v` reads the module volume; walk tracks 1–9) →
   speaker → LED board (0–7 pixel walk) → CAM (SHOOT 1 only with the Hub up, per the camera firmware's
   Wi-Fi-gated capture) → IR array only if U7 is fitted (five-channel black/white table first).
8. Full tour 10 min (rail + RFID + audio + LED + Wi-Fi): zero brown-outs; touch-check the bucks, L298N and
   DFPlayer for heat; confirm the ESP-NOW emergency link against the hub (channel 1).

---

## 10 · Open items — answer before the schematic

1. **Buck modules**: which two exact units (photos/links)? The 5 V one should be *fixed*-output — a trim
   pot knob knock putting 12 V on the ESP32 is the failure mode the hub plan already banned.
2. **Battery**: 3S pack capacity/chemistry, XT60 lead present?
3. **Speaker**: model and wire length (8 Ω 3 W assumed for the budget).
4. **L298N module**: photo of the exact module (header order + 5V jumper), and confirm its onboard flybacks.
5. **DFPlayer**: measure TX idle voltage (§8.3) on the team's module.
6. **IR array**: is line-following coming back? The rail build is deliberately sensor-free; if yes, source
   the 3.3 V 5-channel module and fit U7 + pull-ups after the black/white measurement.
7. **Chassis**: where does the carrier mount relative to the L298N module (harness length) and can the
   microSD on the DFPlayer be reached without demounting?

---

## 11 · Deliberately NOT on this board

| Item | Why |
|---|---|
| L298N chip/module circuitry | the red module with its screw terminals, flybacks and heatsink stays external; the carrier only speaks to it over an XH-8 |
| Both buck converters | same hub decision — a ≥5 A switching converter doesn't belong on a hand-routed 2-layer board |
| **Mega + TFT display link** | `firmware/display` documents GPIO17→Mega RX1, but **GPIO16/17 are owned by DFPlayer UART2** and no sender exists in the flashed robot — wiring it would kill the narration. Future options (not now): TX-only SoftwareSerial on a spare pin, or a second ESP32 UART — needs a firmware task first |
| Fuses / reverse-polarity MOSFETs / TVS | user decision carried over from hub v0.3; the rocker + unplug-the-LiPo rule covers the demo use case |
| Emergency button / buzzer | the Hub owns the emergency button and the siren; the robot learns about emergencies over ESP-NOW — no wiring |
| WS2812 level shift alternatives | the HW-209/BSS138 bidirectional board stays banned for LED data (old plan §7) — the 74HCT125 is the locked part |

---

## 12 · Sources

- Robot brain firmware (pins, DFPlayer, Nano link, CAM trigger, ESP-NOW): [`ESP32_RFID_MP3_RAIL/ESP32_RFID_MP3.ino`](../../ESP32_RFID_MP3_RAIL/ESP32_RFID_MP3.ino) and its [`README.md`](../../ESP32_RFID_MP3_RAIL/README.md)
- Rail driver firmware (L298N map, hard-brake behaviour, EEPROM tuning): [`NANO_LFR_RAIL/NANO_LFR_RAIL.ino`](../../NANO_LFR_RAIL/NANO_LFR_RAIL.ino)
- Camera-side link facts (RX on GPIO13, 1-bit SD keeps it free): [`firmware/cam/include/config.h`](../../firmware/cam/include/config.h)
- Architecture audit validating every pin claim and the GPIO16/17 display conflict: `findings.md` (2026-08-11 working-combo audit), `diagrams/02_robot_hardware_communication_topology.drawio`
- Power/ground architecture, L298N voltage-drop and buck sizing: [`plans/robot-wiring.md`](../../plans/robot-wiring.md) (§4, §8) · status divergence notes: `BUILD_PLAN.md` §"Status check"
- DOIT ESP32 DevKit V1 dimensions and 30-pin map: https://www.espboards.dev/esp32/esp32doit-devkit-v1/ (51.45 × 28.33 mm)
- DFPlayer Mini pin/SPK facts: DFRobot DFPLAYER MINI manual (manualslib.com/manual/1731781) — SPK pair is bridged, ≤ 3 W
- LCSC part numbers: live `lib_Device.search` via the EasyEDA bridge, 2026-10-12 (`PCB design/scripts/robot/01…02_*.js`)
- KCD4-203-B rocker drawing: [`../hub/Switch_KCD4-203-B-dimension.jpg`](../hub/Switch_KCD4-203-B-dimension.jpg)
