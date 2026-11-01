# Station PCB — final release, 2026-10-28

**Upload set (bare PCB, no assembly): `STATION_GERBER_20261028_FINAL.zip`.**
Project snapshot: `STATION_PROJECT_20261028_FINAL.epro2` · Raw BOM: `STATION_RAW_BOM_20261028.csv`
Checksums: `SHA256SUMS.txt` · CAM inspection: `MFG-INSPECTION.json`.

## What changed since the 2026-10-26 review (both review findings closed)

1. **Silk overlap fixed:** the J1 designator moved from (1125, 180) to (1500, 125) — clear of the
   "HUB CABLE (1:1)" label and all pads. Exact native diff: only that ATTR changed.
2. **Identifier corrected:** the board string read "Station PCB V0.3"; this manual layout is
   spec revision v0.4, so the string now reads "Station PCB V0.4" (position/style untouched).

Copper is untouched: 59 hand-routed track segments, 1 DIN via, 209 GND fill pieces — the exact
reviewed state (verified against the 2026-10-26 post-GND snapshot counts).

## Verification at release time

- **Strict EasyEDA DRC: passed, 0 findings in every category** (re-run inside the export script).
- Gerber/drill inspection of the actual CAM package: outline one closed 50.8 × 31.877 mm
  rectangle; PTH drill = 35 component holes (0.8/0.9/1.0/1.02 mm) + 1 via (0.305 mm) + 2 × 3.2 mm
  mounting holes = 38 hits; both copper layers present (top 74 draw ops = the routed tracks,
  bottom 2,887 ops = the 209-piece GND plane); both silkscreen layers exported.
- Carried over from the [2026-10-26 review](../../REVIEW-2026-10-26.md): pad nets match the spec
  (12 nets, 34 wired pads, J4.4 intentionally NC); every non-GND net one connected network;
  GND one network incl. read-back fills; no clearance/hole/edge violations; 5V/3V3 1.0 mm
  throughout; annular rings ≥ ~9.85 mil.

## Still open — hardware gates, not fab blockers

- Print **1:1** and check the upright IR module fit, cable access, connector orientation and
  screw heads.
- Confirm the **purchased BC557's actual C/B/E order** against the board's C/B/E silk, and the
  **active buzzer's polarity** before soldering.
- Bring-up: continuity/short test before power, verify 5V/3V3 rails, then sensors/buzzer/LED chain
  (spec §bring-up). 1.0 mm power tracks are not a thermal test.

## Suggested JLCPCB order settings (Station)

2 layers · 50.8 × 31.877 mm · 1.6 mm FR-4 · **1 oz** · HASL · green/white · tented vias ·
**no PCB assembly** (through-hole, hand-soldered). Same 1 oz caution as the Robot: do not switch
to 2 oz without re-checking the fill clearances. Order only after your own visual review in
EasyEDA and the 1:1 print check. No order has been placed.
