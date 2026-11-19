# Museum HUB — Power & Wiring Architecture (PLAN v0.1 — SUPERSEDED by WIRING-PLAN.md v0.2)

> Status: **PLANNING** — no final PCB until this plan is reviewed.
> Phase A = dummy pipeline-test PCB (tiny, cheap) · Phase B = this hub board.
> All firmware pin references: [`firmware/hub/include/config.h`](../../firmware/hub/include/config.h)

---

## 1 · Power architecture (the core requirement)

**Rule**: battery and DC jack both land on the hub PCB → one master switch → buck → 5 V.
**Rule**: plugging the DC jack must *physically disconnect* the battery (no backfeed into cells).

```
 [Battery pack]──XT60──┐
                       │            ┌── switched contact ──┐
                       ├────────────┘                      │
 [DC jack 12V]──barrel─┤  (jack's built-in SPDT switch     │
   5.5/2.1mm           │   pin OPENS the battery leg       │
                       │   the moment DC is inserted)      │
                       ▼                                   ▼
                  [OR-junction node A]◄────────────────────┘
                       │
              [S1: master power switch, SPST rocker ≥10A]
                       │
              [F1: blade fuse, 7.5–10A]  +  [D1: reverse-protection Schottky SS54]
                       │
              ════════ 12V RAIL ════════
                 │            │        │
              XT60 OUT#1   XT60 OUT#2   (future 12V loads: LED strips, dock)
                 │
           [U_buck: 12V→5V sync buck, ≥5A, LCSC "Basic" candidate: XL4015 class
            + 100µF in / 220µF out + Schottky if non-sync]
                 │
              ════════ 5V RAIL ════════
                 │            │
              XT30 OUT#1   XT30 OUT#2      → ESP32 VIN, MQ heaters, buzzer pull-up
                 │
           [LDO AMS1117-3.3 on DevKit → 3V3 for logic]  (on the DevKit module, not this board)
```

### Why the switched barrel jack (decision)
- Standard 5.5/2.1 mm DC jacks ship with a 3rd "switch" pin that breaks when the plug inserts.
- Battery leg routes **through** that switch contact → **guaranteed** battery isolation on DC
  insertion, zero semiconductors, zero quiescent draw, nothing to fail silently.
- Fallback (only if >8 A through the jack): P-MOSFET ideal-diode pair + priority comparator.

### Current budget (to confirm)
| Rail | Consumer | Est. |
|---|---|---|
| 5 V | 56× WS2812 @ full white, brightness-capped firmware | ≤3.4 A (typ. ≤1.5 A) |
| 5 V | ESP32 DevKit (WiFi peaks) | ≤0.5 A |
| 5 V | 4× MQ heater (each ~150 mA!) | ≤0.7 A |
| 5 V | buzzer, sensors, LED | ≤0.3 A |
| **5 V total** | | **≤5 A** → buck ≥5 A, 12 V side ≥3 A |
| 12 V | pass-through (external strips/dock) | per XT60 out, fused separately (F2/F3, 5 A each) |

---

## 2 · Connector map — every wire is a connector (JST/Molex/XT)

| # | From ⇄ To | Connector | Pins / net order |
|---|---|---|---|
| 1 | Battery pack ⇄ board | **XT60** (in) | +12V_BAT, GND |
| 2 | DC adapter ⇄ board | **DC barrel 5.5/2.1, switched** (in) | +12V_DC, GND, SW→battery leg |
| 3 | 12 V distribution (×2) | **XT60** (out) | +12V, GND — each behind its own blade fuse |
| 4 | 5 V to LED chain | **XT30** (out) | +5V, GND (the current carrier) |
| 5 | 5 V spare out | **XT30** (out) | +5V, GND |
| 6 | WS2812 DIN signal | **JST-XH 3-pin** | DIN(GPIO15), GND, (shield sense unused) |
| 7 | DHT11 ×4 | **JST-XH 3-pin** ×4 | 3V3, DATA(14/16/17/27), GND + on-board 10k pull-up per line |
| 8 | MQ sensor ×4 | **JST-XH 4-pin** ×4 | 5V, GND, A0→(divider→34/35/36/39), D0→spare digital |
| 9 | Buzzer | **JST-XH 2-pin** | SIG(GPIO4), GND |
| 10 | Emergency button module | **JST-XH 3-pin** | 3V3, OUT(GPIO21), GND |
| 11 | ESP32 DevKitC 38-pin | 2× female 19-pin 2.54 mm | board carriers it, no wires |
| 12 | microSD module (opt.) | **JST-XH 6-pin** (v0.1) | 3V3, CS(5), SCK(18), MOSI(23), MISO(19), GND |

MQ divider: A0 can swing to ~5 V → 20k/10k divider → ≤3.3 V into ADC1 pins (34/35/36/39, input-only — wired exactly as firmware expects).

WS2812 level shift: DIN is driven 3.3 V from GPIO15 → 74AHCT125 (or 330 Ω series for short chains) + 1000 µF bulk at the XT30.

---

## 3 · Rail separation rules (routing + wiring)
- 12 V, 5 V and 3V3 are **separate rails**; nothing 12 V ever touches a 3V3 net.
- GND is common — single star point at the buck output cap; one solid GND pour on both layers.
- LEDs/servo-class current never flows through JST-XH (3 A limit) — XH carries **signals only**;
  all power goes through XT60/XT30.
- Each XT60 12 V out gets its own fuse so a shorted strip can't kill the hub.

---

## 4 · Open items (fill before Phase B schematic)
1. Battery pack chemistry/cells? (assumed 3S–4S Li-ion, 9–16.8 V — plan is valid for 9–24 V inputs anyway)
2. DC adapter spec? (assumed 12 V / ≥5 A brick)
3. What consumes the 12 V XT60 outs today? (assumed: none yet — future LED strips / robot dock)
4. MQ model(s) (MQ-135? MQ-2?) → sets heater current + divider values
5. Buck part final pick (LCSC Basic search in Phase B: XL4015 / MP1584-class, ≥5 A, SOIC/THT)

---

## 5 · Phase A — DUMMY pipeline-test PCB (build FIRST, this is the current task)
Tiny board whose only job is to prove every pipeline stage end-to-end:
- 2× red 0603 LED + 2× 1 kΩ 0603, 1× JST-XH 3-pin (5V/GND/SIG), 1× screw terminal in
- 2× mounting holes, 20 × 15 mm outline
- Exercises: LCSC part search (`lib_Device.getByLcscIds/search`) → schematic place/wire
  (`sch_Document.importChanges`) → ERC → PCB → DRC → BOM/CPL/Gerber export → order flow
- Target: **≤ $2 total**, can even skip PCBA on the real order
