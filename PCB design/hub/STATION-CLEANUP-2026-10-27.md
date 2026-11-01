# H-004 — Station silk and remaining three resistors, 2026-10-27

## Completed scope

| Item | Saved result |
|---|---|
| R4 | (2310,1805) mil, 90°; near U2's LED input/ground circuit |
| R6 | (2145,1805) mil, 90°; LED_BUF pad about 122 mil (3.1 mm) from U2.3 |
| R13 | (2680,1905) mil, 90°; ESTOP_IO pad about 145 mil (3.7 mm) from U1B.18 |
| Station labels | Eight existing station/zone names moved to their current S1–S8 ports |
| Pin labels | All 64 existing labels, original identity/content and correct pin order; horizontal beside actual pins |
| Pin-text style | 32 mil font, 6 mil stroke, larger than the old 24 mil / 5 mil text |
| Resistor designators | R4/R6/R13 horizontal beside their bodies |
| Existing board title | Returned from off-board to the open lower-right area |

The user rotated U2 180° and moved C1 while planning. The first write guard rejected the stale baseline
and wrote nothing. Both edits were preserved in the refreshed checkpoint; the final resistor locations
follow the actual new pin locations. U2/C1 were not moved or rotated by this task.

## Hard verification

- Full project archive inspection: exactly three component poses, seven owned attribute adjustments,
  and 73 existing free-string geometry/style adjustments changed. No record additions/deletions.
- **All other 74 components** and every unrelated PCB record are unchanged, including all **62 routed
  copper segments**, pad-net mappings, net definitions, footprint documents, holes and outline.
- Every other project document (including schematic/library) is unchanged.
- Actual measured bounding boxes: **76 adjusted text objects**, **zero pad-clipping / edge / text-overlap hits**.
  S5's 5V label is offset 20 mil below the pin to clear R64's lower turn pad.
- Strict DRC unchanged: **25 Physical Error + 230 Connection Error**, **zero new findings**.
- Six cleanup regression tests and 22 existing model tests pass (**28 model tests** total). Historical
  explicit-archive SB acceptance tests intentionally skip generic discovery; this cleanup's full-export
  acceptance was run directly and passed. RED export check rejected the unedited board first.
- Archive integrity checked; hashes are in the evidence summary. No rerouting, imports, pours or fabrication orders.

## Archives and evidence

- [Before-write full backup](WRO_HUB_before_station_cleanup_guard_20261027-0209.epro2) — 496099 bytes.
- [Saved cleaned project](WRO_HUB_after_station_cleanup_20261027-0209.epro2) — 495870 bytes.
- Original pre-plan backup: `WRO_HUB_before_station_cleanup_20261027-0157.epro2` (before concurrent user edits).
- Evidence directory: `../scripts/hub/data/station_cleanup_20261027/` contains the plan, before/after live
  inventory/DRC, actual silk bounding boxes/pads, write result, exact export acceptance, regression output,
  archive hashes and chronological ledger. The original stale snapshots are retained with `initial_` prefixes.
- Model/test/writer/gate: `station_cleanup.py`, `test_station_cleanup.py`, `61_cleanup_station.js`,
  `verify_station_cleanup.py` in `../scripts/hub/`.

Separate author self-review of the scope, concurrent-edit guard, current pin mapping, raw export diff and
actual text clearances. No independent reviewer was used. Existing DRC/J3/XT/fabrication issues are not
repaired by this task; no production-readiness claim is made. All other placements remain the user's.
