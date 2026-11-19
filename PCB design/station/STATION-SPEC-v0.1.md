# Museum Station Board — Spec v0.1 (7 copies, 100 % through-hole) — SUPERSEDED by STATION-SPEC.md v0.2

> ⚠️ **Status: SUPERSEDED (2026-10-25).** Kept for history; do **not** build from this file. Replaced by
> [`STATION-SPEC.md`](STATION-SPEC.md) v0.2 (D1 = 1N5819 DO-41, C1 removed, 3V3 outcome recorded).
>
> **Former status: DRAFT — 3 questions in §7 before the schematic.** Pairs with the hub spec
> [`../hub/WIRING-PLAN.md`](../hub/WIRING-PLAN.md) (v0.3). One station board sits at each of the 7 artifacts
> and connects to the hub with one 8-pin JST-XH cable.

## 1 · What sits on each station board

| Block | Part | Power | To the hub |
|---|---|---|---|
| Temperature / humidity | DHT11 module (VCC, GND, OUT), plugged into a female header | 3V3 | DHT |
| Proximity alarm | IR obstacle module (VCC, GND, OUT) + BC557 + active buzzer + 1 kΩ — the user's proven standalone circuit | 5V | PROX (through a Schottky diode) |
| Gas | MQ-5 module (VCC, GND, DO, AO) + 68 kΩ / 100 kΩ divider on AO | 5V | MQ (≤ 3.0 V) |
| LEDs | 8-pixel WS2812 stick on a JST-XH 4-pin (5V, GND, DIN, DOUT) | 5V | DIN in, DOUT back |

## 2 · Hub link — J1, JST XH 8-pin B8B-XH-A (C157972), cable wired 1:1 to the hub port

| Pin | Net | Used for |
|---|---|---|
| 1 | 5V | MQ-5 heater, IR module, buzzer, LED stick |
| 2 | GND | everything |
| 3 | 3V3 | DHT11 only |
| 4 | DHT | DHT11 OUT (the hub has the 10 k pull-up to 3.3 V) |
| 5 | MQ | divider node (AO → 68 k → node → 100 k → GND) |
| 6 | PROX | Schottky anode; cathode on the IR module OUT |
| 7 | DIN | LED stick DIN (from the hub) |
| 8 | DOUT | LED stick DOUT (back to the hub, which passes it to the next station) |

## 3 · Circuits

**Proximity alarm** (unchanged from the standalone build, plus one diode for the hub):
```
IR OUT ─┬─ 1 kΩ ─► BC557 base        BC557 emitter → 5V,  collector → buzzer + ,  buzzer − → GND
        └─◄|── BAT85 ── PROX (hub)    (cathode on IR OUT, anode toward the hub)
```
- No obstacle: IR OUT = 5 V → BC557 off, buzzer silent; the diode blocks, so the hub's pull-up holds PROX at 3.3 V.
- Obstacle: IR OUT = LOW → BC557 on, buzzer sounds; the diode pulls PROX to ≈ 0.5 V → the hub reads "detected".
- The hub never sees 5 V, and two stations can share one hub input (either one pulls it LOW).

**MQ-5**: AO → 68 kΩ → `MQ` node → 100 kΩ → GND. `MQ` goes to the hub (5 V × 100/168 = 2.98 V max).
The module's DO pin is not used.

**DHT11**: VCC = 3V3, OUT → `DHT`. Modules with an onboard pull-up are fine (it pulls to 3.3 V).

**LED stick**: 5V, GND, DIN ← hub pin 7, DOUT → hub pin 8. Optional 100 µF / 10 V across 5V–GND next to the
connector (footprint only; fit it if the first pixel flickers).

## 4 · Connection list
- `5V`: J1.1, IR VCC, BC557 emitter, MQ VCC, J2.1 (LED 5V), C1 + (optional).
- `GND`: J1.2, DHT GND, IR GND, MQ GND, buzzer −, 100 k bottom, J2.2, C1 −.
- `3V3`: J1.3, DHT VCC.
- `DHT`: J1.4, DHT OUT.
- `MQ`: J1.5, 68 k (other end MQ AO), 100 k (other end GND).
- `PROX`: J1.6, D1 anode. `IR_OUT`: D1 cathode, IR OUT, 1 k (other end BC557 base).
- `BUZ+`: BC557 collector, buzzer +.
- `DIN`: J1.7, J2.3. `DOUT`: J1.8, J2.4.

## 5 · Bill of materials (per board; ×7)

| Qty | Ref | Part | LCSC (footprint) |
|---|---|---|---|
| 1 | J1 | JST XH 8-pin B8B-XH-A(LF)(SN) | C157972 |
| 1 | J2 | JST XH 4-pin B4B-XH-A(LF)(SN) | C144395 |
| 1 | U1 | 1×3 female header (DHT11 module) | C541850 |
| 1 | U2 | 1×3 female header (IR module) | C541850 |
| 1 | U3 | 1×4 female header (MQ-5 module) | C541851 |
| 1 | Q1 | BC557B, TO-92 (check E-B-C on your part) | C2093 |
| 1 | D1 | BAT85 Schottky, DO-34 | C549292 |
| 1 | BZ1 | active buzzer 5 V, 12 mm, 7.6 mm pitch | footprint C9900016920 |
| 1 | R1 | 1 kΩ ¼ W | C1364484 |
| 1 | R2 | 68 kΩ ¼ W | any ¼ W axial (same footprint) |
| 1 | R3 | 100 kΩ ¼ W | C1364475 |
| 0 (1) | C1 | 100 µF 10 V radial | optional |
| 2 | H1–H2 | M3 holes | — |

## 6 · Layout (indicative)
≈ 60 × 45 mm, 2 layers. J1 and J2 on one edge (cables), the IR module on the opposite edge facing outward so it
can see visitors approaching the artifact, the MQ-5 module away from the buzzer. Silkscreen every header with its
pin names as printed on the module.

## 7 · Questions before the schematic
1. The printed pin order (left to right) on your DHT11, IR and MQ-5 modules — the female headers must match.
2. Should the IR module and DHT11 plug straight into the board, or sit on a short cable so they can be aimed?
3. Keep the local buzzer at every station (default: yes)?
