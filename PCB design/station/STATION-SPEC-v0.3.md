# Museum Station Board — Spec v0.3 (7 copies, 100 % through-hole, hand-soldered, bare PCB) — SUPERSEDED by STATION-SPEC.md v0.4

> ⚠️ **Status: SUPERSEDED (2026-10-25).** Kept for history; do **not** build from this file. Replaced by
> [`STATION-SPEC.md`](STATION-SPEC.md) v0.4 (DHT11 and MQ-5 on JST cables, IR module upright, smaller board).

> **Status: APPROVED inputs (2026-10-25) — source of truth for the schematic and PCB** of EasyEDA project **`station_pcb`**
> (`PCB design/station/station_pcb.eprj2`). Pairs with the hub spec [`../hub/WIRING-PLAN.md`](../hub/WIRING-PLAN.md) v0.4:
> one station board sits at each of the 7 artifacts and connects to its hub port (J10–J16) with one 8-pin JST-XH cable
> wired 1:1. Supersedes v0.2 → [`STATION-SPEC-v0.2.md`](STATION-SPEC-v0.2.md) and v0.1 → [`STATION-SPEC-v0.1.md`](STATION-SPEC-v0.1.md).
> Process: `pcb-pipeline` skill. Scripts: `PCB design/scripts/station/`.
>
> **v0.3 changes (2026-10-25, user answers to the open questions):**
> - Module pin orders, as printed left → right: **DHT11 `VCC DOUT GND`**, **IR obstacle module `OUT GND VCC`**, **MQ-5
>   `VCC GND DO AO`**. Each socket follows its module's order exactly.
> - The DHT11, IR and MQ-5 modules **plug straight into female headers on the board** (no cables); every station keeps its
>   **local buzzer** (answers of 2026-10-12, re-confirmed).
> - **2 × M3 holes**, ≈ 60 × 45 mm. LED stick on **one XH-4 cable** (5V, GND, DIN, DOUT) wired to both ends of the stick.
> - Designators, net list by pin and the verified BOM added (`scripts/station/02_parts_verify.js`: 10 part types, all THT).

## 0 · What this board is

| Decision | Choice | Why |
|---|---|---|
| Logic | none — no MCU, no IC | the hub reads everything over the cable |
| Hub link | J1 JST XH 8-pin, 1:1 cable | one plug per station; same pin order as the hub port |
| Modules | DHT11, IR obstacle, MQ-5 in female headers | swap a dead sensor without soldering |
| Proximity | IR module + BC557 + active buzzer (the user's standalone alarm) + 1N5819 to the hub | local beep at the artifact; the hub only sees "pulled LOW" |
| Gas | 68 k / 100 k divider on MQ-5 AO | ≤ 3.0 V on the cable for the hub's 3.3 V ADC |
| LEDs | 8-pixel WS2812 stick on J2 (XH-4) | data runs hub → station → hub (chain) |
| Power | 5 V and 3.3 V from the hub cable, no regulator | the only 3.3 V load is the DHT11 (≈ 2.5 mA) |

## 1 · Power

| Load | Rail | Current |
|---|---|---|
| MQ-5 heater | 5V | ≈ 150 mA (continuous) |
| IR module | 5V | ≈ 20 mA |
| Active buzzer (only while something is near) | 5V | ≈ 30 mA |
| WS2812 stick at the firmware cap (brightness 60) | 5V | ≤ ≈ 110 mA (480 mA at full white) |
| DHT11 | 3V3 | ≤ 2.5 mA |
| **Total per station** | | **≈ 0.3 A typical, ≤ 0.7 A worst** — JST XH 3 A per pin, 22 AWG for 5V / GND (hub §1) |

No capacitor on the board (v0.2: the stick has its own per-pixel 100 nF; if the first pixel ever flickers, solder
100–470 µF across the stick's 5V/GND pads).

## 2 · Ports and sockets

| Ref | What | Part (LCSC) | Pins 1 → n |
|---|---|---|---|
| J1 | Hub link | JST XH 8-pin B8B-XH-A (C157972) | 5V, GND, 3V3, DHT, MQ, PROX, DIN, DOUT |
| J2 | LED stick (one cable to both ends of the stick) | JST XH 4-pin B4B-XH-A (C144395) | 5V, GND, DIN, DOUT |
| U1 | DHT11 module socket | 1×3 female 2.54 mm PM254V-11-03-H85 (C541850) | VCC, DOUT, GND |
| U2 | IR obstacle module socket | 1×3 female (C541850) | OUT, GND, VCC |
| U3 | MQ-5 module socket | 1×4 female PM254V-11-04-H85 (C541851) | VCC, GND, DO (not connected), AO |

- **XH pin 1 = supply, pin 2 = GND** (house rule). J1 matches the hub's station port pin for pin (hub §2.1).
- **Sockets follow each module's printed order**: plug the module in with its labels matching the silkscreen.
- LED-stick harness: J2 pins 1/2/3 → the stick's **input** end 5V/GND/DIN; J2 pin 4 → the stick's **output** end DOUT
  (its 5V/GND there stay free).

## 3 · Circuits

**Proximity alarm** (the user's proven standalone circuit, plus one diode for the hub):
```
IR OUT ─┬─ R1 1 kΩ ─► Q1 BC557 base        Q1 emitter → 5V,  collector → BZ1 + ,  BZ1 − → GND
        └─◄|── D1 1N5819 ── PROX (J1.6)   (cathode on IR OUT, anode toward the hub)
```
- No obstacle: IR OUT = 5 V (module pull-up) → Q1 off, buzzer silent; D1 blocks (1.7 V reverse) and the hub's 10 k pull-up
  holds PROX at 3.3 V.
- Obstacle: IR OUT LOW → Q1 on (base current ≈ 4 mA), buzzer sounds; D1 pulls PROX to LM393 VOL (0.25–0.4 V) + VF
  (≈ 0.15–0.2 V) ≈ 0.4–0.6 V < the S3's VIL 0.825 V → the hub reads "detected".
- The hub never sees 5 V, and two stations can share one hub zone input (either one pulls it LOW).
- 1N5819 leakage (a few µA at 25 °C) flows into PROX while idle; the hub's 10 k pull-up keeps that to ≈ +0.1 V (hub §3.3).

**MQ-5**: AO → R2 68 kΩ → `MQ` → R3 100 kΩ → GND. `MQ` goes to the hub: 5 V × 100 / 168 = 2.98 V max. DO is not used.

**DHT11**: VCC = 3V3, DOUT → `DHT`. The module must carry its own pull-up (3-pin modules have 4.7–10 kΩ; the hub has
none since hub v0.4). Meter DOUT↔VCC before fitting; if it reads open, tack a 10 k across the module's VCC and DOUT pins.

**LED stick**: J2 5V, GND, DIN ← hub pin 7, DOUT → hub pin 8 (the hub passes it on to the next station through its 220 Ω).

## 5 · Connection list (schematic source of truth)

Polarity pins by **name** (checked on the placed symbols): D1 1N5819 1 = K, 2 = A; BZ1 1 = +, 2 = −; Q1 by its E/B/C pin names.

- `5V`: J1.1, J2.1, U2.3 (IR VCC), U3.1 (MQ VCC), Q1.E.
- `GND`: J1.2, J2.2, U1.3 (DHT GND), U2.2 (IR GND), U3.2 (MQ GND), BZ1.2 (−), R3.2.
- `3V3`: J1.3, U1.1 (DHT VCC).
- `DHT`: J1.4, U1.2 (DOUT).
- `MQ`: J1.5, R2.2, R3.1. `MQ_AO`: U3.4 (AO), R2.1.
- `PROX`: J1.6, D1.2 (A). `IR_OUT`: U2.1 (OUT), D1.1 (K), R1.1.
- `Q_BASE`: R1.2, Q1.B. `BUZ_P`: Q1.C, BZ1.1 (+).
- `DIN`: J1.7, J2.3. `DOUT`: J1.8, J2.4.
- No-connect: U3.3 (MQ DO).

12 nets, 34 wired pins, 11 parts (+ 2 M3 holes on the PCB).

## 6 · Bill of materials (per board; ×7) — LCSC verified live 2026-10-25 (`02_parts_verify.js`, gate PASS, 0 SMD)

Hand-soldered, bare PCB order: LCSC numbers fix the footprints; buy locally where cheaper.

| Qty | Ref | Part | LCSC | Footprint |
|---|---|---|---|---|
| 1 | J1 | JST XH 8-pin B8B-XH-A(LF)(SN) + XHP-8 | C157972 | CONN-TH_B8B-XH-A-LF-SN |
| 1 | J2 | JST XH 4-pin B4B-XH-A(LF)(SN) + XHP-4 | C144395 | CONN-TH_B4B-XH-A-LF-SN |
| 2 | U1, U2 | 1×3 female header 2.54 mm PM254V-11-03-H85 | C541850 | HDR-TH_3P-P2.54-V-F |
| 1 | U3 | 1×4 female header 2.54 mm PM254V-11-04-H85 | C541851 | HDR-TH_4P-P2.54-V-F |
| 1 | Q1 | BC557B PNP, TO-92; footprint pads 1-2-3 = C-B-E (check your part's datasheet) | C2093 | TO-92-3_L4.8-W3.7-P2.54-L |
| 1 | D1 | 1N5819 Schottky | C526214 | DO-41 (P8.70) |
| 1 | BZ1 | active buzzer 5 V, 12 mm, 7.6 mm pitch (buy locally) | footprint C9900016920 | BUZ-TH_BD12.0-P7.60 |
| 1 | R1 | 1 kΩ ¼ W | C1364484 | RES-TH P10.30 |
| 1 | R2 | 68 kΩ ¼ W (any; footprint of C385541) | C385541 | RES-TH P10.30 |
| 1 | R3 | 100 kΩ ¼ W | C1364475 | RES-TH P10.30 |
| 2 | H1, H2 | M3 holes (PCB only) | — | — |

Off-board per station: DHT11 module, IR obstacle module, MQ-5 module, 8-pixel WS2812 stick, the hub cable (XH-8, 1:1)
and the stick cable (XH-4).

**Schematic (2026-10-25):** `station_pcb` P1.Schematic1 drawn by `scripts/station/11…15` — 11 parts in 3 blocks, 34 stubs,
12 nets, ERC true (panel 0 / 0 / 0), `sch_audit.py` 0 differences. Symbol pins: Q1 1 = C, 2 = B, 3 = E; BZ1 1 = +; D1 1 = K, 2 = A.

## 7 · Layout (placed 2026-10-25, `scripts/station/layout_plan.py` → `21_place_pcb.js`)
- **70.0 × 50.0 mm** (2756 × 1969 mil; the v0.2 draft's ≈ 60 × 45 had no room for the three modules + 2 M3 holes), 2 layers,
  1.6 mm. **H1 bottom-left, H2 top-right** (M3, 3.2 mm plated, 6 mm pad, no net).
- **Modules (TYPICAL sizes, confirm at the review):** each lies flat on its header (≈ 11 mm above the board), header at one
  short end, body toward the top edge. DHT11 ≈ 28 × 13 mm (U1 at the left), IR module ≈ 32 × 14 mm (U2, middle; its LED end
  overhangs the top edge by ≈ 4 mm so it sees visitors), MQ-5 ≈ 32 × 22 mm (U3, right; ≈ 15 mm from the DHT11 so its warm
  heater doesn't bias the DHT reading). If a module's pins point sideways instead, a right-angle 1-row female header fits
  the same holes.
- **Bottom edge (cables):** J1 hub cable, J2 LED stick (both pin 1 = 5V on the right). **Right column:** proximity alarm
  (BZ1, Q1, R1, D1). R2 / R3 (MQ divider) sit under the MQ-5 module (low parts only under modules).
- **Silkscreen** (plain English like the robot's, 0.66–1.0 mm): "HUB CABLE (1:1)" / "LED STICK" + pin names; each
  socket's pin names in the module's printed order; module outlines with "DHT11 / TEMP + HUMIDITY", "IR SENSOR / LEDS FACE
  OUT", "MQ-5 / GAS SENSOR"; "PROXIMITY ALARM", "BUZZER 5V ACTIVE" + "+", "C B E" at Q1; values inside R1–R3 / D1; title
  "WRO MUSEUM STATION v0.3 2026-10".
- **Checks:** `layout_plan.py` 0 problems; `21` 11 / 11 placed first run (re-run alreadyOk); in-EDA 0 overlaps / off-board;
  `silk_plan.py` 0 problems; `24` 0 hits; DRC only Connection Error (34 pins / 12 nets).

## 8 · Checks before ordering
1. Each module's printed pin order against the silkscreen (U1 VCC DOUT GND, U2 OUT GND VCC, U3 VCC GND DO AO).
2. Print 1:1 and lay the three modules on it (the IR module's LEDs must clear the board edge).
3. BC557 of the batch bought: C-B-E order (flat face toward you, left → right) against the footprint's pads 1-2-3.

## 9 · Bring-up (per station, on the hub)
1. No modules: hub cable in → 5 V on J2.1, 3.3 V on U1.1, nothing warm.
2. DHT11 in → the hub's `/scan` page shows a DHT reading on this station's pin.
3. MQ-5 in → the `MQ` pin ≤ 3.0 V; the reading moves when gas (a lighter, unlit) is near.
4. IR module in → hand in front: the buzzer beeps and the hub's zone reads "detected"; hand away: silent, zone clear.
5. LED stick on J2 → pixels light in chain order.

## 11 · Sources
Hub spec v0.4 (port pin-out, PROX pull-up, MQ capacitor) · robot D-025 (LM393 open collector, VOL) · LCSC library via
`scripts/station/02_parts_verify.js` (2026-10-25).
