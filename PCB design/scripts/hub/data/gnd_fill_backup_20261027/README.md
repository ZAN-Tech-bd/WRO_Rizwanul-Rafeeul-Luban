# GND fills removed for user editing — 2026-10-27

Final state: WRO_HUB PCB1 saved with zero fill polygons. Removed exactly 1843 GND fills:
463 top copper (layer 1), 1380 bottom copper (layer 2). No other design record changed.
676 tracks, 274 pads and 77 components remain. No layer visibility was changed.

- [Full before backup](../../../../hub/WRO_HUB_before_gnd_fill_removal_20261027-1208.epro2).
- [Final editing export](../../../../hub/WRO_HUB_without_gnd_fills_final_20261027-1221.epro2).
- [Exact polygon dump](gnd_fills.json): original sources, IDs, nets, layers, fill mode, width and lock state.
- [Verification](verification.json) and [final acceptance](acceptance_final.txt): only 1843 FILL records
  and 30 fill allocator counters removed; all other PCB records/project documents unchanged.
- [Restore verification](restore_verification.json): tested recovery of all 1843 geometries/settings
  exactly in a full project export. EasyEDA assigns new IDs. Restored fills were then deleted again.
- Restore test completed in three resumable batches (720, 600, 523); final deletion removed all 1843.
- [Recovery script](../../64_restore_gnd_fills.js) is an old-geometry recovery tool, NOT a GND regeneration
  tool. **Do not replay it after component, route, pad or outline edits.**

Latest user direction: "just remove the gnd pours from both layer and let me edit first, then we will
repair the gnd pour again". Leave the editor alone now; repair GND from the updated layout later.
