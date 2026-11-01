# SB1–SB7 subtraction ledger — authoritative workflow: user's task, sections 1–5

- Preflight: mandatory project/spec/handoff/API documents read. Bridge connected, one window.
- Current branch: pcb/hub-direct-rocker-20261026; no commits/pushes requested or performed.
- Context verified: WRO_HUB, PCB1 ad35aed4d7610fd2 (document type 3).
- Fresh saved full-project backup: hub/WRO_HUB_before_sb_removal_20261026-2307.epro2,
  610668 bytes, zip integrity checked, 84 PCB components.
- Targets identified by exact SB1–SB7 designators; common two-pin footprint 2ca2a36c91af7b3f,
  device 6fb29c9794f3d22c, supplier C492401, 100 mil / 2.54 mm pad pitch.
- Ruling: no bypass note exists in the current PCB export; do not create/delete unrelated silk.
  The substring SB in USB is not an SB designator. Cost if wrong: an obsolete bypass label could remain;
  both raw-export and live-text checks must verify absence.
- Ruling: no routed board copper, vias or pours exist. Legacy LINE records are layers 3/11;
  POLY records are layers 3/11, and four free PADs are mounting holes. Delete no copper.
  Cost if wrong: an SB stub could remain; all exported geometry and the live copper count are checked.
- Ruling: adapt software TDD to the physical subtraction: acceptance checks first reject the unchanged
  84-component export, then pass only on the persisted 77-component export. No new feature/branch plan
  is created; the user's explicit workflow remains authoritative.
- Schematic untouched by scope. Import Changes can restore SBs and revert J3; no import is performed.
- No disabled rocker, placement, whole-silk, route, outline or pour script is executed.
- Baseline live snapshot saved: before_live.json; all seven SBs and 14 former pad locations recorded.
- Baseline strict DRC: 270 leaf findings, all Connection Error; no routed copper, 74 named nets.
- Acceptance RED run: 10 checks, 3 expected failures (84 != 77, seven owned-record sets still present,
  SB silk still present); 6 preservation checks pass, live check not yet supplied.
- PCB subtraction saved: 7 components, 21 owned attributes, 14 pad-net rows removed; 77 survivors unchanged.
- First hard export: WRO_HUB_after_sb_removal_20261026-2313.epro2 (624458 bytes, archive integrity OK).
- Hard checks: component/footprint/net/geometry/silk/live-inventory checks pass. Two initial gates failed:
  allocator ELE_PLACEHOLDER counter serialization changed, and DRC added one schematic/PCB Netlist Error.
- Investigation: placeholder delta is allocator bookkeeping (deleted COMPONENT 7 / ATTR 21), not added geometry.
  Existing 256 Connection Error findings remain; exactly the 14 SB pad findings disappeared.
- Scope extension authorized by user: "Also remove only SB1–SB7 from the schematic and their exclusive
  wire stubs, preserving every other symbol, net and connection; then repeat export and DRC verification."
- Ruling: do not suppress DRC or Import Changes. Reconcile by the explicitly authorized schematic
  subtraction; retain the PCB-only export checkpoint. Cost if wrong: unrelated schematic connectivity
  could be lost, so require an exact-stub manifest and full-record/net-membership diffs.
- Schematic preflight: 85 symbols including frame (84 electrical), 270 wires; all 14 candidate SB
  stubs are 30-unit horizontal wires, each touches exactly one SB pin and no other pin/wire.
- Expanded RED run: 13 checks, 3 expected failures (schematic SB records still present, live 85 != 78,
  and PCB/schematic mismatch). PCB physical invariants and stub exclusivity all pass.
- Ruling: compare primitive-ID allocator placeholders as multisets, not document-order IDs. PCB delta
  is exactly COMPONENT/7 and ATTR/21 counters; physical records remain exact-byte compared.
  Cost if wrong: an unexpected nonphysical bookkeeping change could be tolerated, not altered geometry.
- Schematic deletion succeeded and saved: exactly 7 symbols and 14 allowlisted wires, 78 symbols with
  frame / 256 wires remain; no PCB writes.
- Instrumentation finding: both SCH_Net.getCurrentProjectAllNets and getAllNetsName return empty in
  this local project. The snapshot already records all named wires and attached pins independently.
- Ruling: retain the raw API-empty baseline and derive schematic net names from original pinNets,
  matching the established 10_check.js approach. Cost if wrong: a net membership could be misreported;
  exact wire/attribute bytes plus every surviving pin membership are separately compared.
- Final reconciled full export saved: WRO_HUB_after_sb_removal_reconciled_20261026-2321.epro2,
  606352 bytes, ZIP integrity OK; hashes/counts/deltas in summary.json.
- Task complete evidence: 13/13 acceptance checks pass, no skips. PCB 77 unchanged survivors / 74 named nets;
  schematic 77 electrical symbols / 256 wires / 74 named nets, ERC true. DRC 270 → 256 existing Connection
  Error findings, exactly 14 SB pads removed, no new finding and no Netlist Error.
- Broader Python suite: hub 7/7 model tests pass and station 7/7 pass; acceptance's 13 tests intentionally
  skip under generic discovery and pass when directly supplied the required artifacts.
- Final review: self-review (no independent model-selectable reviewer available). Exact archive/payload
  diffs, allowlisted schematic stubs, no-import/no-reroute constraints and all requirements checked.
- Final: Ruling: existing unrouted connections, J3 schematic binding/BOM, XT holes and legacy-builder
  replay risks remain outside the subtraction; preserve and warn rather than modify. Cost if wrong:
  future replay/import could restore SBs or revert J3; this revision is explicitly not fabrication-ready.
- No newly deferred minors. No commit/push/merge/order. Editor left on WRO_HUB PCB1.
