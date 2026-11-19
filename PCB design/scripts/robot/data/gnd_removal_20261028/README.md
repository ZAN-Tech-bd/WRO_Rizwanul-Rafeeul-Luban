# Robot GND removal for the user's editing interval — 2026-10-28

## Current state

The user requested GND removal from the connected **WRO_Robot**, not Hub. Active projectUUID:
`7c9ddded879e0beabea718339ea291c0ca140c2c8d71e5fdf6645641934d5864`, PCB1UUID`643ce99542452c8f`.

- **1,922 static GND Fill primitives removed:** 325top / 1,597bottom, only copper layers1/2.
- PCB saved; live readback reports zero remaining selected GND fills.
- Full saved-project comparison covers **110 documents**. Every surviving native body is byte-identical;
  all66components,374coppertracks,175vias, pads/pad nets, footprint/library documents, schematic and
  silkscreen preserved.331fill-only allocator records were removed as bookkeeping, not geometry.
- This is a manual-edit state, **not a fabrication release**. GND unconnected findings are expected.

## Backups and evidence

- [Full pre-deletion archive](../../../../Robot/WRO_Robot_before_gnd_removal_20261028-0248.epro2)
- [Saved editing archive without GND](../../../../Robot/WRO_Robot_editing_no_gnd_20261028-0250.epro2)
- [Exact saved native FILL bodies](native_gnd_fills.json): IDs, polygon paths, net/layer and settings.
- [Live API polygon/settings dump](live_gnd_fills.json): the exact deletion guard source.
- [Saved-project verification](verification.json): strict preservation results and record counts.

## Tooling

- [52_backup_gnd_fills.js](../../52_backup_gnd_fills.js) is read-only and requires the exact Robot
  project and PCB IDs. The writer refuses a hidden editor or changed polygon/count/settings.
- [53_remove_gnd_fills.js](../../53_remove_gnd_fills.js) is generated from that snapshot and deletes
  only the selected fill IDs; never use it on another layout or board.
- [gnd_edit.py](../../gnd_edit.py) exports and validates the result. Its regression test first failed
  for the missing verifier, then passed, rejecting unrelated copper/net/other-fill changes.

## Repour after the user edits

Wait for the user to request it. Take a fresh complete backup and native/live snapshot. Verify their
actual current layout/nets/outline/antenna constraints, then regenerate GND from current geometry.
Do **not** automatically restore these old polygons or replay historic Robot placement/route scripts.
Check physical DRC, source isolation, all-network connectivity and saved copper/pad/hole clearances;
recheck outstanding drill/fit issues separately before generating manufacturing files. Earlier
Gerber/route checkpoints must not be assumed current after a manual edit.
