# Museum Station Board — Spec v0.2 (7 copies, 100 % through-hole) — SUPERSEDED by STATION-SPEC.md v0.3

> ⚠️ **Status: SUPERSEDED (2026-10-25).** Kept for history; do **not** build from this file. Replaced by
> [`STATION-SPEC.md`](STATION-SPEC.md) v0.3 (module pin orders answered, designators, net list by pin, verified BOM).

> **Status: DRAFT — 3 questions in §7 before the schematic.** Pairs with the hub spec
> [`../hub/WIRING-PLAN.md`](../hub/WIRING-PLAN.md) (v0.4). One station board sits at each of the 7 artifacts
> and connects to the hub with one 8-pin JST-XH cable. Supersedes v0.1 → [`STATION-SPEC-v0.1.md`](STATION-SPEC-v0.1.md).
> Plan of record (v0.2): `C:\Users\tahfi\.claude\plans\pasted-content-id-5437-task-curious-stardust.md`.
>
> **v0.2 changes (2026-10-25, user-approved part-count review):**
> - **D1 BAT85 (DO-34) → 1N5819 (DO-41).** Lower forward voltage at the ≈ 0.3 mA loop current (≈ 0.15–0.2 V vs
>   ≈ 0.25–0.3 V), so a better LOW at the hub; a sturdier body to hand-solder; a common part locally. Its reverse leakage
>   is ≈ 100× the BAT85's: the hub keeps its 10 k pull-up + 1 k series per zone to absorb it (hub §3.3).
> - **C1 (optional 100 µF across the LED stick) removed.** At the firmware brightness cap the stick draws ≤ ≈ 0.11 A.
> - **3V3:** no regulator on the station. It gets 3.3 V from the hub's DevKit LDO over the cable, for the DHT11 only.
> - The hub has **no DHT pull-up** any more (hub v0.4): the DHT11 module's own pull-up holds the line.

## 1 · What sits on each station board

| Block | Part | Power | To the hub |
|---|---|---|---|
| Temperature / humidity | DHT11 module (VCC, GND, OUT), plugged into a female header | 3V3 (from the hub, ≈ 2.5 mA) | DHT |
| Proximity alarm | IR obstacle module (VCC, GND, OUT) + BC557 + active buzzer + 1 kΩ — the user's proven standalone circuit | 5V | PROX (through a Schottky diode) |
| Gas | MQ-5 module (VCC, GND, DO, AO) + 68 kΩ / 100 kΩ divider on AO | 5V | MQ (≤ 3.0 V) |
| LEDs | 8-pixel WS2812 stick on a JST-XH 4-pin (5V, GND, DIN, DOUT) | 5V | DIN in, DOUT back |

**3.3 V:** the station has no regulator. Pin 3 of the hub cable carries the hub DevKit's 3.3 V (its CJ6107A33GW LDO,
hub §1). The only 3.3 V load here is the DHT11 (≈ 0.5–2.5 mA while it measures), so 1.5 m of cable drops nothing
measurable.

## 2 · Hub link — J1, JST XH 8-pin B8B-XH-A (C157972), cable wired 1:1 to the hub port

| Pin | Net | Used for |
|---|---|---|
| 1 | 5V | MQ-5 heater, IR module, buzzer, LED stick |
| 2 | GND | everything |
| 3 | 3V3 | DHT11 only |
| 4 | DHT | DHT11 OUT (the module's own pull-up holds it HIGH; the hub has only a 1 k series resistor) |
| 5 | MQ | divider node (AO → 68 k → node → 100 k → GND) |
| 6 | PROX | Schottky anode; cathode on the IR module OUT |
| 7 | DIN | LED stick DIN (from the hub) |
| 8 | DOUT | LED stick DOUT (back to the hub, which passes it to the next station) |

## 3 · Circuits

**Proximity alarm** (unchanged from the standalone build, plus one diode for the hub):
```
IR OUT ─┬─ 1 kΩ ─► BC557 base        BC557 emitter → 5V,  collector → buzzer + ,  buzzer − → GND
        └─◄|── 1N5819 ── PROX (hub)  (cathode on IR OUT, anode toward the hub)
```
- No obstacle: IR OUT = 5 V → BC557 off, buzzer silent. The diode blocks (1.7 V reverse), so the hub's 10 k pull-up
  holds PROX at 3.3 V.
- Obstacle: IR OUT = LOW → BC557 on, buzzer sounds. The diode pulls PROX to LM393 VOL (0.25–0.4 V) + VF (≈ 0.15–0.2 V)
  ≈ 0.4–0.6 V, below the S3's VIL of 0.825 V, so the hub reads "detected".
- The hub never sees 5 V, and two stations can share one hub input (either one pulls it LOW).
- 1N5819 leakage (a few µA at 25 °C, ≈ 10× per 25 °C) flows from IR OUT into PROX in the idle state. The hub's 10 k
  pull-up keeps that to ≈ +0.1 V. Never rely on the S3's internal pull-up alone here (hub §3.3).

**MQ-5**: AO → 68 kΩ → `MQ` node → 100 kΩ → GND. `MQ` goes to the hub (5 V × 100/168 = 2.98 V max).
The module's DO pin is not used.

**DHT11**: VCC = 3V3, OUT → `DHT`. The module must carry its own pull-up (3-pin modules have 4.7–10 kΩ). Check DOUT↔VCC
with a meter before fitting; if it reads open, tack a 10 k across the module's VCC and DOUT pins.

**LED stick**: 5V, GND, DIN ← hub pin 7, DOUT → hub pin 8. No capacitor on the station (v0.2). At the firmware brightness
cap (`LED_BRIGHTNESS = 60`) a stick draws ≤ ≈ 0.11 A, so 1.5 m of 22 AWG (≈ 0.16 Ω) sags ≈ 0.02 V; the stick's own
per-pixel 100 nF capacitors cover the edges. At full brightness (0.48 A) it is ≈ 0.08 V. If the first pixel ever
flickers, solder 100–470 µF across the stick's 5V/GND pads.

## 4 · Connection list
- `5V`: J1.1, IR VCC, BC557 emitter, MQ VCC, J2.1 (LED 5V).
- `GND`: J1.2, DHT GND, IR GND, MQ GND, buzzer −, 100 k bottom, J2.2.
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
| 1 | D1 | 1N5819 Schottky, DO-41 | C526214 |
| 1 | BZ1 | active buzzer 5 V, 12 mm, 7.6 mm pitch | footprint C9900016920 |
| 1 | R1 | 1 kΩ ¼ W | C1364484 |
| 1 | R2 | 68 kΩ ¼ W | any ¼ W axial (same footprint) |
| 1 | R3 | 100 kΩ ¼ W | C1364475 |
| 2 | H1–H2 | M3 holes | — |

Retired designators (v0.2): C1.

## 6 · Layout (indicative)
≈ 60 × 45 mm, 2 layers. J1 and J2 on one edge (cables), the IR module on the opposite edge facing outward so it
can see visitors approaching the artifact, the MQ-5 module away from the buzzer. Silkscreen every header with its
pin names as printed on the module.

## 7 · Questions before the schematic
1. The printed pin order (left to right) on your DHT11, IR and MQ-5 modules — the female headers must match.
2. Should the IR module and DHT11 plug straight into the board, or sit on a short cable so they can be aimed?
3. Keep the local buzzer at every station (default: yes)?
