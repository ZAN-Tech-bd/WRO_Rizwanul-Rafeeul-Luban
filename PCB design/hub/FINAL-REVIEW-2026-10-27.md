# WRO_HUB final layout review + regenerated GND — 2026-10-27

> **Historical review, superseded by [the 2026-10-28 edited-board release review](RELEASE-2026-10-28.md).**
> Corrections: J5's +5V pad is left and GND is right; a morphology filter is not proof of every
> minimum neck width or full-load current capacity. Current strict DRC retains one synchronization finding.

## What was done

From the user's **finished final layout** (77 components, 630 tracks, 280 vias, 74 nets):

- **New GND plane regenerated from live geometry** — 2,360 fill pieces (701 top / 1,659 bottom).
  Old fills were never restored; the plane was recomputed from the current board.
- **Left power area isolation increased as requested**: new fill keeps **40 mil (1.01 mm)** from all
  other nets left of x≈1000 mil, and **50 mil (1.27 mm)** from the isolated source rails
  BAT+/BAT−/DC+/DC−/V12 (normal area 12 mil ≈ 0.30 mm, JLC 1 oz limit 0.10 mm).
  Source-negative nets BAT−/DC− remain fully separated from GND except through the switch.
- Thermal reliefs on THT GND pads (60 mil spokes on J3/J4/J5 so the power return stays solderable);
  antenna keep-out clear on both layers; edge16mil; morphology filter discourages narrow slivers
  (not a guarantee of every local neck width or current capacity).
- **Three unattached GND stitching vias relocated** short distances into the connected plane
  (they were stranded in the widened keep-outs); all signal vias untouched.
- **J5 polarity legend corrected**: it read "− 5V FROM BUCK +" while the pads are +5V left / GND
  right; now "+ 5V FROM BUCK −". Net assignment was always correct.

## Verification (all against the saved full-project export, not the canvas)

| Check | Result |
|---|---|
| Strict EasyEDA DRC | **0 errors** (baseline was 266 GND connection errors; no new findings) |
| Schematic ERC | true |
| Export diff vs pre-write archive | only 2,360 new FILL records + 3 via coordinate changes + 1 legend text |
| Components / tracks / pads / nets / footprints / outline / schematic | byte-preserved |
| Fill vs other-net copper | 0 overlaps, 0 clearance violations (checked on saved polygons) |
| GND connectivity | one network; 0 disconnected GND pads/vias/fills |
| Gerber outline | single intended rectangle −675..4695 × 0..3975 mil |
| Model tests | 6/6 pass |

Backups: [`WRO_HUB_before_ground_write_20261027-1647.epro2`](WRO_HUB_before_ground_write_20261027-1647.epro2) →
[`WRO_HUB_after_ground_20261027-1649.epro2`](WRO_HUB_after_ground_20261027-1649.epro2).
Evidence, plan and checks: `../scripts/hub/data/final_review_20261027/`; tooling `final_ground.py`,
`66_final_ground.template.js`, `verify_final_ground.py`, `test_final_ground.py`.

## Remaining blockers — NOT ready to order

1. **Q1 pads (1,2,3): annular ring 0.15 mm** < JLC 2-layer 1 oz absolute minimum 0.18 mm
   (0.25 mm recommended). Enlarge the Q1 footprint pads.
2. **J5 (XT30) holes 1.8 mm** vs your measured pins 2.1/2.2 mm → terminals will not fit.
   Approved size was 2.5 mm hole / 3.5 mm pad.
3. **J1/J2 holes 4.6 mm, J4 4.7 mm** vs approved 4.0 mm (XT60 legs 3.7/3.8 mm) — larger than
   approved; confirm intent or resize (J4 pad 6.0 mm is fine).
4. **Silk below JLC guideline**: 64 station pin labels 32 mil high (< 40 mil), many 5 mil strokes
   (< 0.15 mm) — text may be unreadable or removed by the fab.
5. **J3 KCD4 switch**: slots/pads meet JLC plated-slot rules, but physical fit/continuity/
   break-before-make on a real switch is still unverified (1:1 print + real part).
6. **+3V3 routes enter the antenna overhang strip** (y ≈ 3810–3915, x 2250–2680) on the bottom —
   deliberate or not, confirm against Wi-Fi performance expectations.
7. **Thermal/current qualification**: 5 mm source tracks and 2.5/3 mm 5 V distribution are
   geometrically fine; an ≈8 A worst-case load has not been drop/thermal-qualified.
8. J3's schematic device still binds the old VH-6 footprint (Import Changes would revert J3), and
   J3's BOM still says B6P-VH — fix before any BOM export.

DRC/ERC clean ≠ orderable: items 1–3 are hard fabrication/assembly blockers.
No order was placed; review is my own plus an independent read-only audit.
