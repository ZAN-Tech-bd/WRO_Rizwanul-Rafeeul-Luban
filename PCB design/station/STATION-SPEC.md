# Museum Station Board — Spec v0.4 (7 copies, 100 % through-hole, hand-soldered, bare PCB)

> **Status: APPROVED inputs (2026-10-25) — source of truth for the schematic and PCB** of EasyEDA project **`station_pcb`**
> (`PCB design/station/station_pcb.eprj2`). Pairs with the hub spec [`../hub/WIRING-PLAN.md`](../hub/WIRING-PLAN.md) v0.4:
> one station board sits at each of the 7 artifacts and connects to its hub port (J10–J16) with one 8-pin JST-XH cable
> wired 1:1. Supersedes v0.3 → [`STATION-SPEC-v0.3.md`](STATION-SPEC-v0.3.md), v0.2 → [`STATION-SPEC-v0.2.md`](STATION-SPEC-v0.2.md) and
> v0.1 → [`STATION-SPEC-v0.1.md`](STATION-SPEC-v0.1.md). **Retired designators (v0.4): U1, U3.**
> Process: `pcb-pipeline` skill. Scripts: `PCB design/scripts/station/`.
>
> **v0.4 changes (2026-10-25, user):**
> - **Only the IR module sits on the board, standing upright** in its 1×3 socket U2 (pins `OUT GND VCC`).
> - **DHT11 and MQ-5 come on JST cables:** U1 → **J3** JST XH 3-pin (1 3V3, 2 GND, 3 DOUT), U3 → **J4** JST XH 4-pin
>   (1 5V, 2 GND, 3 AO, 4 DO not connected). XH pin 1 = supply, pin 2 = GND (house rule); the cable's far ends follow
>   the modules' printed orders (DHT11 `VCC DOUT GND`, MQ-5 `VCC GND DO AO`). Nets unchanged (12 nets, 34 pins).
> - The board shrinks to what is left (§7).
>
> **Manual layout revision (2026-10-26):** the user replaced the placement and routing after the original §7 build.
> The original placement/routing scripts and §7 coordinates are historical, not instructions to overwrite the editor.
> Preserve the user's current tracks, vias, pads and component positions. Regenerate **bottom-only GND copper** from a
> fresh live geometry snapshot, with THT thermal reliefs, ≥10 mil other-net clearance and ≥12 mil board-edge clearance.
> Recheck the read-back copper independently of EasyEDA DRC before calling this revision ready. Review results below
> will supersede the earlier build's verification claims; fabrication approval still requires the §8 physical checks.
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
| Modules | IR obstacle module upright in a female header; DHT11 + MQ-5 on JST XH cables (J3, J4) (v0.4) | sensors go where the artifact needs them; swap without soldering |
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
| J3 | DHT11 cable (v0.4) | JST XH 3-pin B3B-XH-A (C144394) | 3V3, GND, DHT (module DOUT) |
| J4 | MQ-5 cable (v0.4) | JST XH 4-pin B4B-XH-A (C144395) | 5V, GND, AO, DO (not connected) |
| U2 | IR obstacle module socket — module stands **upright** | 1×3 female (C541850) | OUT, GND, VCC |

- **XH pin 1 = supply, pin 2 = GND** (house rule). J1 matches the hub's station port pin for pin (hub §2.1).
- **U2 follows the IR module's printed order**: plug it in upright with its labels matching the silkscreen.
- **Sensor cables (XH housing → module Dupont):** J3 1 → DHT VCC, 2 → GND, 3 → DOUT; J4 1 → MQ VCC, 2 → GND, 3 → AO,
  4 → DO (or leave that wire out). Keep each ≤ 1 m; put the DHT11 away from the MQ-5's warm heater.
- Plug-swap check: J3 (XH-3) is unique; J4 and J2 are both XH-4 with 5V / GND on pins 1 / 2, so a swapped plug never
  reverses power (the signals meet only through R2 68 k or the hub's series resistors). Silkscreen names both clearly.
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

- `5V`: J1.1, J2.1, U2.3 (IR VCC), J4.1 (MQ VCC), Q1.E.
- `GND`: J1.2, J2.2, J3.2 (DHT GND), U2.2 (IR GND), J4.2 (MQ GND), BZ1.2 (−), R3.2.
- `3V3`: J1.3, J3.1 (DHT VCC).
- `DHT`: J1.4, J3.3 (DOUT).
- `MQ`: J1.5, R2.2, R3.1. `MQ_AO`: J4.3 (AO), R2.1.
- `PROX`: J1.6, D1.2 (A). `IR_OUT`: U2.1 (OUT), D1.1 (K), R1.1.
- `Q_BASE`: R1.2, Q1.B. `BUZ_P`: Q1.C, BZ1.1 (+).
- `DIN`: J1.7, J2.3. `DOUT`: J1.8, J2.4.
- No-connect: J4.4 (MQ DO).

12 nets, 34 wired pins, 11 parts (+ 2 M3 holes on the PCB).

## 6 · Bill of materials (per board; ×7) — LCSC verified live 2026-10-25 (`02_parts_verify.js`, gate PASS, 0 SMD)

Hand-soldered, bare PCB order: LCSC numbers fix the footprints; buy locally where cheaper.

| Qty | Ref | Part | LCSC | Footprint |
|---|---|---|---|---|
| 1 | J1 | JST XH 8-pin B8B-XH-A(LF)(SN) + XHP-8 | C157972 | CONN-TH_B8B-XH-A-LF-SN |
| 1 | J2 | JST XH 4-pin B4B-XH-A(LF)(SN) + XHP-4 | C144395 | CONN-TH_B4B-XH-A-LF-SN |
| 1 | J3 | JST XH 3-pin B3B-XH-A(LF)(SN) + XHP-3 | C144394 | CONN-TH_3P-P2.50 |
| 1 | J4 | JST XH 4-pin B4B-XH-A(LF)(SN) + XHP-4 | C144395 | CONN-TH_B4B-XH-A-LF-SN |
| 1 | U2 | 1×3 female header 2.54 mm PM254V-11-03-H85 | C541850 | HDR-TH_3P-P2.54-V-F |
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

## 7 · Layout history (original v0.4 placement; superseded by the user's manual layout below)
- **50.8 × 35.6 mm** (2000 × 1400 mil), 2 layers, 1.6 mm. **H1 bottom-left, H2 top-right** (M3, 3.2 mm plated, 6 mm pad).
- **Bottom edge:** J1 hub cable, J2 LED stick (pin 1 = 5V on the right). **Top edge:** J3 DHT11 cable, J4 MQ-5 cable and the
  IR socket U2; the IR module stands upright in U2 with a ≈ 15 × 8.5 mm clear zone around it (no other part).
- **Middle:** R2 / R3 (MQ divider) left; D1 / R1 / Q1 (alarm) centre; BZ1 right.
- **Silkscreen** (plain English, 0.6–0.8 mm): "HUB CABLE (1:1)", "LED STICK", "DHT11 CABLE", "MQ-5 CABLE" with pin names;
  "IR SENSOR (UPRIGHT)" with OUT / GND / VCC and its zone outline; "BUZZER 5V ACTIVE" + "+"; "C B E" at Q1; values inside
  R1–R3 / D1; title "WRO MUSEUM STATION v0.4 2026-10". J1 / J2 designators beside their cable names.
- **Checks:** `layout_plan.py` 0 problems; `21` 11 / 11 placed first run (old 70 × 50 outline + H2 replaced; re-run alreadyOk);
  in-EDA 0 overlaps / off-board; `silk_plan.py` 0 problems; `24` 0 hits; DRC only Connection Error (34 pins / 12 nets).

### 7.1 · Routing (2026-10-26, `scripts/station/route_plan.py` → `40_route.js`; GND `pour_fill.py` → `42_fill.js`)
- **Basic 2-layer, 0 vias** (user: "bottom layer ground pour, no need too much via"). Bottom = one GND fill with thermal
  reliefs on the 7 GND pads; GND has no tracks.
- **Top:**
  - LED data (DOUT, DIN) and PROX run in three 0.3 mm lanes under J1's pins;
  - 3V3 (0.4 mm) and DHT run up the left edge to J3;
  - MQ_AO runs from J4 to R2;
  - the alarm nets (IR_OUT, Q_BASE, BUZ_P) stay in the middle;
  - 5V (0.6 mm) runs from J2.1 up the right edge to U2 and Q1, and along the top edge to J4.
- **Bottom tracks**, needed because J1's pin order crosses: 5V J1.1 → J2.1 (0.8 mm, along the bottom edge) and MQ J1.5 → R2.2.
- **Checks:**
  - clearance ≥ 12 mil (0.3 mm), edge ≥ 12 mil;
  - JLC hole rules OK;
  - every net is one copper island;
  - fill gaps 10–11 mil, 0 overlaps;
  - EasyEDA DRC **0 errors**.

### 7.2 · Current manual layout and ground review (2026-10-26)

- **Actual outline:** 50.8 × 31.877 mm (2000 × 1255 mil). H1 (157, 157), H2 (1850, 1100) mil; 3.2004 mm plated holes,
  5.9944 mm no-net copper rings. Eleven components, 35 component pads (34 wired, J4.4 intentionally NC).
- The user repositioned the parts and routed **59 straight tracks plus one DIN via**. All 5V and 3V3 segments are
  **1.0 mm**. The live snapshots, not the original §7/7.1 coordinates or generators, describe this placement.
- **Fresh bottom-only GND** generated from absolute live pads, actual tracks/via, holes and outline; 209 hole-free fill
  primitives, thermal gap 10 mil / spoke 15 mil, other-net gap ≥10 mil, outline inset 12 mil. One GND network joins all
  seven pads; no floating copper islands detected. This is **static copper**, not a self-updating native pour.
- **Verified:** strict DRC zero errors; all 11 non-GND nets connected; net assignments match §5; independent copper,
  hole and fill checks clean. Before/after comparisons confirm the user's routing, via, pads, placement and outline
  were not modified. Seven live-model regression tests pass.
- **Open review items:** J1 overlaps the HUB CABLE (1:1) text; add a board name/revision marking; confirm actual IR
  module/connector fit and BC557 pin order. No final Gerber/drill fabrication audit has been performed for this layout.
- **Evidence and backups:** [review](REVIEW-2026-10-26.md),
  [before GND](station_pcb_manual_before_gnd_20261026.eprj2),
  [reviewed layout](station_pcb_manual_gnd_reviewed_20261026.eprj2).
- **Change control:** never restore the old fill polygons after rewiring, and do not run the historical placement or
  routing generators. After any copper/pad/outline edit, take a fresh snapshot and regenerate/check the static fill.

## 8 · Checks before ordering
1. IR module printed order against U2's silkscreen (OUT GND VCC); crimp the J3 / J4 sensor cables per §2 (module side
   by its printed labels).
2. Print 1:1 and lay the three modules on it (the IR module's LEDs must clear the board edge).
3. BC557 of the batch bought: C-B-E order (flat face toward you, left → right) against the footprint's pads 1-2-3.

## 9 · Bring-up (per station, on the hub)
1. No modules: hub cable in → 5 V on J2.1 and J4.1, 3.3 V on J3.1, nothing warm.
2. DHT11 in → the hub's `/scan` page shows a DHT reading on this station's pin.
3. MQ-5 in → the `MQ` pin ≤ 3.0 V; the reading moves when gas (a lighter, unlit) is near.
4. IR module in → hand in front: the buzzer beeps and the hub's zone reads "detected"; hand away: silent, zone clear.
5. LED stick on J2 → pixels light in chain order.

## 11 · Sources
Hub spec v0.4 (port pin-out, PROX pull-up, MQ capacitor) · robot D-025 (LM393 open collector, VOL) · LCSC library via
`scripts/station/02_parts_verify.js` (2026-10-25).
