# WRO_HUB SB1–SB7 removal — verified 2026-10-26

## Result

The requested subtraction is saved in WRO_HUB PCB1. The user separately authorized removing the matching
schematic symbols and their private wire stubs after PCB-only deletion exposed a netlist mismatch.

| Check | Result |
|---|---|
| PCB footprints | **84 → 77**, exactly SB1–SB7 removed, none added |
| Remaining 77 components | Component, attribute and pad-net payload bytes identical to backup |
| Footprint library documents | All unchanged, including J3's custom plated-slot footprint |
| R61–R67 | Present, original 220 Ω devices and S1_DOUT→S2_DIN … S7_DOUT→S8_DIN mappings unchanged |
| J10–J17 | Present; all pad nets, including LED pins 7/8, unchanged |
| Named nets | **74 → 74**, all 75 PCB NET records unchanged (includes empty-net entry) |
| Rails, holes, outline, other silk | All serialized geometry/attribute payload bytes unchanged |
| SB silkscreen designators | Seven removed with their components; none remain |
| Free bypass note | None existed; no unrelated text deleted (USB is not SB) |
| Routed copper / SB stubs | None existed; **zero copper deletions**; zero routes/vias/pours created |
| Schematic electrical components | **84 → 77** (title frame excluded) |
| Schematic wires | **270 → 256**, only the 14 verified private SB stubs removed |
| Schematic surviving connectivity | Every other symbol, wire, attribute, and pin membership unchanged |
| Strict PCB DRC | **270 → 256** existing unconnected-pad findings; no new findings |
| Schematic/PCB mismatch | Gone after authorized schematic subtraction; no Import Changes |
| Schematic ERC | **true** before and after |
| Acceptance tests | **13/13 pass** against full project exports and live snapshots |
| Existing PCB Python model tests | **14/14 pass** (7 hub + 7 station) |

**The PCB is still unrouted.** This verifies the subtraction and absence of new violations, not fabrication readiness.

## Exact serialized changes

PCB1 removed records: 7 COMPONENT, 21 owned ATTR, 14 PAD_NET. Schematic P1 removed records: 7 COMPONENT,
14 WIRE, their 14 LINE segments and 280 owned ATTR records. No non-allocator record was added or modified
in either document. No other project document's record payload changed. Native primitive-ID allocator
ELE_PLACEHOLDER counters changed/reordered as bookkeeping; PCB counter delta is exactly COMPONENT/7 and ATTR/21.
Record history tickets and document timestamps are not physical-design payloads; surviving payloads are
compared as exact serialized bytes, not rounded geometry.

The 14 schematic stubs were each proven to be a 30-unit horizontal segment touching exactly one SB pin,
no other symbol pin, and no other wire, including intersections/overlaps. Their named nets remain anchored
by the station connectors and hop resistors. No main-chain connection was deleted or renamed.

## Saved project archives

- **Pre-edit backup:** [WRO_HUB_before_sb_removal_20261026-2307.epro2](WRO_HUB_before_sb_removal_20261026-2307.epro2),
  610668 bytes; SHA-256 6b517d9de79d27c82140084361cca20f3dcbdb804cf3ea3f90e8bb60409b8b24.
- **PCB-only checkpoint:** [WRO_HUB_after_sb_removal_20261026-2313.epro2](WRO_HUB_after_sb_removal_20261026-2313.epro2),
  diagnostic intermediate with the now-resolved schematic mismatch; do not use as the final handoff.
- **Final reconciled project:** [WRO_HUB_after_sb_removal_reconciled_20261026-2321.epro2](WRO_HUB_after_sb_removal_reconciled_20261026-2321.epro2),
  606352 bytes; SHA-256 38bd83269fbca71946f8dc51909eae28686a7d38a7e68efdb4d40dbf9693e003.

All full project archives pass ZIP integrity checks. The active editor was left on WRO_HUB PCB1.

## Evidence and reproduction

Evidence directory: `../scripts/hub/data/sb_removal_20261026/`:

- `before_live.json`, `after_live.json` (intermediate), `final_live.json`: PCB geometries/pads/nets/silk/strict DRC.
- `before_sch_live_api_names.json`: untouched original schematic snapshot; the local net-name API returned empty.
- `before_sch_live.json`: same recorded geometry/wires/pin memberships, with net names derived from the original
  pin memberships; `after_sch_live.json`: final schematic snapshot/ERC. Both have 74 real named nets.
- `schematic_stub_manifest.json`, `deletion.json`, `schematic_deletion.json`: exact removal allowlists/results.
- `acceptance_red.txt`, `schematic_red.txt`: expected failures before the corresponding writes.
- `acceptance_final.txt`: all 13 final acceptance checks pass; `pcb_python_suite.txt`: model regression output.
- `summary.json`: hashes/counts/raw record deltas; `progress.md`: chronological ledger and decisions.

Run from the repository root:

```text
python "PCB design/scripts/hub/test_sb_removal.py" "PCB design/hub/WRO_HUB_before_sb_removal_20261026-2307.epro2" "PCB design/hub/WRO_HUB_after_sb_removal_reconciled_20261026-2321.epro2" "PCB design/scripts/hub/data/sb_removal_20261026/before_live.json" "PCB design/scripts/hub/data/sb_removal_20261026/final_live.json" "PCB design/scripts/hub/data/sb_removal_20261026/before_sch_live.json" "PCB design/scripts/hub/data/sb_removal_20261026/after_sch_live.json"
```

The acceptance tests intentionally skip under generic discovery: they require explicit before/after artifacts.
They were run directly and passed without skips. Generic discovery separately passed all 14 existing model tests.

## Review and remaining cautions

Separate **author self-review**, not an independent reviewer: checked the user workflow and follow-up scope,
exact removal allowlists, survivor-byte comparison, footprint document preservation, net memberships,
DRC leaf identities (ignoring only display/index fields), archive integrity and RED→GREEN evidence.
No unresolved defect was found in the subtraction. No new deferred minor findings.

No unrelated board issue was repaired: the existing J3 schematic footprint/BOM binding and XT hole-size
caveats in H-001 remain. Historical schematic/placement/silk builders still contain SBs and must not be
replayed; Import Changes can still revert J3. A separate engineering/physical/fabrication review remains
necessary before ordering. No commit, push, merge or order was performed.
