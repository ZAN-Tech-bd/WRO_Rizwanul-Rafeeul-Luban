# WRO Robot — final release, 2026-10-28

**Upload set (bare PCB, no assembly): `WRO_ROBOT_GERBER_20261028_FINAL.zip`.**
Project snapshot: `WRO_ROBOT_PROJECT_20261028_FINAL.epro2` · Raw BOM: `WRO_ROBOT_RAW_BOM_20261028.csv`
Checksums: `SHA256SUMS.txt` · Package inspection: `MFG-INSPECTION.json`.

## What changed since the user's edited board (10:59 backup)

Every change is proven by exact native diffs across all 110 project documents
(`PCB design/scripts/robot/data/*_check_20261028-*.json`); the user's own edits are untouched.

1. **XT drills (D-031/D-036):** J1 XT60 drill 4.0 mm (pad 5.5 mm kept); J2/J3/J4 XT30 drills
   2.5 mm, pads 3.5 mm — persisted in the three project footprints and in PCB1.
2. **+3V3 antenna reroute (D-037):** the spine that ran at y=3850 under the module's on-board
   antenna overhang (outline change made the overhang on-board) now dips to y=3819.348 at the
   0.8 mm class width and rises past x=1540. Actual radiating projection: **zero copper**.
3. **Orphaned GND stitching via removed** (its stub was lost in the user's rework; it sat in the
   pour-excluded strip, unreachable): 157 → 156 vias.
4. **GND regenerated from the final geometry:** 4,612 static pieces (2,178 top / 2,434 bottom),
   11 mil clearance, 16 mil edge, 8 mil antenna margins, 40/14 mil thermal spokes on the XT GND
   pads. Saved export equals the validated plan piece-for-piece; 0 foreign overlaps, 0 clearance
   violations (min gap 10.898 mil), 0 floating pieces, full GND connectivity.
5. **Silk fixes:** 11 user-edit artifacts resolved (labels off pads/texts, SW1 designator back
   on-board). `24_check_silk`: 187 texts, **0 hits**.

## Verification at release time

- **Strict DRC: passed, 0 findings in every category** (re-run inside the export script).
- ERC: the schematic's native records are byte-identical through every diff since the last
  ERC-true state (v0.6, D-032); PCB `PAD_NET` records are byte-identical as well — pin
  membership agreement carries over transitively.
- Gerber/drill inspection (actual CAM output): J1 4.0 mm ×2, XT30 2.5 mm ×6, mounting 3.2 mm ×4,
  plated flag present, 156 via hits in the via subset, one closed 92.329 × 98.425 mm outline,
  pours present on both copper layers, both silks present.

## Still open (honest gates — not fab blockers, not resolved by this release)

- **Hardware/fit qualification:** XT plug fit on a 1:1 print, switch contact action, full-load
  voltage-drop/thermal checks — physical gates, separate from this geometry review.
- **RF qualification:** the antenna projection is clear of copper (better than the pre-review
  state), but Wi-Fi range is only proven by a live test on the assembled robot.
- Silkscreen image layers, color silk, assembly: not applicable to this bare-PCB order.

## Suggested JLCPCB order settings (Robot)

| Setting | Value | Why |
|---|---|---|
| Layers | 2 | as designed |
| Dimensions | 92.3 × 98.4 mm | from the actual outline layer |
| Qty / turns | per your build plan; 1.6 mm FR-4, **1 oz**, HASL or lead-free HASL | 3 mm tracks and 4 mm holes are routine at 1 oz; no 2 oz need |
| Copper weight | 1 oz | D-032/D-033 rules were set for 1 oz; do not switch to 2 oz without re-checking all gaps (the 0.157 mm minimum gap passes 1 oz's 0.10 mm absolute, not 2 oz's ~0.16 mm) |
| Solder mask / silk | green / white (or preference) | standard; silk is 6 mil stroke, ≥40 mil text |
| Via process | tenting (default) | vias are plain signal/GND stitches; no via-in-pad |
| PCB assembly | **No** | 100 % through-hole, hand-soldered (D-013); BOM provided for your purchasing only |

Order only after your own visual review in EasyEDA and the 1:1 print check (spec §8.5).
No order has been placed and nothing here is an authorization to order.
