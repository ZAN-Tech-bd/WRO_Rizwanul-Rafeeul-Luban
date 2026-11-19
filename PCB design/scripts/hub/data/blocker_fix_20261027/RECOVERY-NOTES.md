# Blocker-fix execution ledger — plan: hub/WIRING-PLAN.md fabrication-blocker correction — COMPLETE (order files issued)

## CURRENT — J3 Netlist Error fixed; raster-artwork export caveat (2026-10-28 02:25 local)

- User added a top-silkscreen image and reported the remaining Netlist Error. The actual GUI ECO
  comparison was read in full: exactly25UpdateProperty rows for J3, no footprint/electrical changes.
  Cause is now confirmed: PCB metadata retained old VH-6 descriptions, dimensions and supplier values
  while schematic metadata was already corrected. This supersedes the previous "cause unresolved" hold.
- Full image-inclusive backup before any ECO: WRO_HUB_REPAIRED_before_sync_fix_with_image_20261028-0222.epro2.
  User applied only the reviewed metadata properties, with "Also update track's net" unchecked, saved.
- Fresh strictPCB DRC[] and ERCtrue/detailed[]. Saved native diff: exactly one PCBJ3COMPONENT.attrs record,
  with25changed values matching the inspected list and schematic. All other native records byte-identical,
  including custom slot geometry/net maps, positions, copper,2331fills, image OBJ and embedded BLOB data.
  Afterbackup: WRO_HUB_REPAIRED_after_sync_fix_with_image_20261028-0225.epro2.
- Refreshed standard Gerber/project/rawBOM/report/checksums in hub/fabrication/20261028-sync-fixed/.
  Copper/drill/outline/mask output preserved semantically; exporter renumbers apertures and emits unused
  definitions, so a RED→GREEN regression verifies comparison of actual drawing operations, not raw IDs.
- Separate finding: Image-only topSilk ordinary Gerber export has no drawable geometry. The user's image
  remains in the project but is omitted from standard monochrome Gerber. Do not promise it prints or
  silently convert it; artwork conversion/color-silkscreen production is a separate decision.
- Ruling: apply only an inspected, backed-up property-only ECO. Cost if wrong: unreviewed physical ECOs
  can still change custom footprints; the complete saved before/after preservation gate passed here.
- Ruling: preserve the raster source unchanged and disclose the ordinary-Gerber omission. Cost if wrong:
  ordering expecting this artwork would miss it; a deliberate conversion/export workflow is required.
- Details: [SYNC-FIX-2026-10-28.md](../../../../hub/SYNC-FIX-2026-10-28.md). No rule suppression,
  unsafe library API, artwork conversion, order or payment performed.
- Verification complete: strictDRC[],ERCtrue/detailed[]; all unchanged records and physical CAM operations
  compared. Effective-Gerber aperture comparison regression RED→GREEN; final Hub suite55tests,
  42passed13historical explicit-fixture skips. Standard Gerber and source project packaged with
  image-omission warning, report and checksums; do not promise the unconverted raster image prints.

## CURRENT — repoured after the user's later edits (2026-10-28)

- Authoritative disposition: [Hub release review](../../../../hub/RELEASE-2026-10-28.md).
  Working copy stays WRO_HUB_REPAIR_ISOLATED_20261027, not either failed custom-device import.
- User moved R5 to1605,1305,90° and rerouted SD_SCK/SERVO_IN/+5V. Pre-pour backup:
  WRO_HUB_REPAIRED_user_edited_20261027-2358.epro2. Restored GND from this geometry, not oldpolygons:
  **2331pieces(682top/1649bottom)**. All saved ground checks pass. Final project:
  WRO_HUB_REPAIRED_repoured_20261028-0009.epro2; matching project/Gerber/BOM/audit/checksums in
  PCB design/hub/fabrication/20261028-after-user-edits/.
- During the resumed fill write, two +5Vlines acquired new IDs but retained identical bodypayloads.
  Source guard aborted before further writes. All633trackbodies and every othernon-fillrecord across
  all86documents were verified equal before refreshing IDs. No trace/via/component movement by the repour.
- Fresh ERCtrue/detailed[]; all256 current named pin-net memberships match across74nets. PhysicalDRC0;
  **one synchronization finding remains**. Earlier "strictDRC0" and "the warning is proven only J3's
  override" claims are superseded. No ECO applied, no rule disabled, no unsafe library writes.
- Red/green preservation regression catches real trace/net edits while allowing pure re-identification;
  red/green Excellon regression verifies actual metric tool/hit/slot parsing. Current package inspected:
  548rounddrills+6platedslots matched to nativegeometry, outlineclosed136.398×100.965mm,77parts in rawBOM.
- Ruling: publish a dated **fabrication-review** package with explicit remaining sync/hardwaregates,
  not an unconditional production-release claim. Cost if wrong: manual ECO/physical fit/thermal
  review is still required before relying on the design as a qualified product.
- Ruling: do not rewrite rawBOMsupplierfields to hide catalog aliases. J3actualslotgeometryverified;
  Q1librarySS8050label versus approveduser2N2222A must be disclosed and actualpinoutchecked.
  Cost if wrong: rawBOMis not approvedPCBAinput.
- Self-review (no fresh reviewer dispatched): package geometry/net/preservation gates inspected separately;
  unresolved synchronization/physical qualification disclosed. No commit, merge, payment or order.
- Final review update: independent read-only reviewer01a17ea0-72af-7127-b53a-53f4f6f2d3c8 confirmed the
  present package (including actual slot orientation and Q1 flashes on both copper faces) and found two
  Important checker gaps. Fixed slot orientation/bijection and fill-only island rejection; both regression
  failures observed first, then GREEN, current artifacts re-audited without changing board/ZIP geometry.
- Final: minor (deferred): narrow document-header save-field allowlist; actual headers independently checked.
- Final: minor (deferred): automate per-Q1-pad/both-layer aperture use; current actual flashes checked.
- Final: fixed slot-orientation/bijection and floating-fill-only island gaps, both RED→GREEN;
  full Hub suite 54 tests:41passed,13explicit-fixture archive cases skipped. Current fabrication artifacts
  re-audited; their Gerber ZIP bytes and SHA-256 unchanged. No design mutations during the fix pass.
- New model tests run in temporary pour-output directories, not over the active user's ground plan.
  Older order files/checkpoints and editing states below are history, superseded by the current review.

## CURRENT EDITING STATE — GND removed for user edits (2026-10-27 23:23 local)

- User requested the pour be removed again for a slight manual modification; they will request the re-pour.
- 78_clear_repair_ground.js (rebuilt from the r5_ground release checkpoint) deleted exactly the 2,362
  checkpointed GND fills; components/tracks/vias untouched. Saved + export-verified:
  `WRO_HUB_REPAIRED_user_editing_no_gnd_20261027-2323.epro2` (640,900 bytes — fill data gone).
- Poured release kept for reference: `WRO_HUB_REPAIRED_r5_ground_20261027-2255.epro2`.
- On the re-pour request: fresh live guard snapshot (71), regenerate the plan from the edited archive
  (repaired_hub.regenerate_ground), 78 clear (if fills exist) + 79 add batches, re-verify
  (0 overlaps / clearance / disconnected), re-export, and REISSUE the Gerber/BOM — the current
  `exports/WRO_HUB_ORDER_*_20261027.*` files match the poured release and go stale after any board edit.
- First delete attempt hit its own stale checkpoint ("Fill count changed", 2364 vs live 2362) and aborted
  before any write — the source-locked guard doing its job. Rebuilt from the current release, then clean.

## R5 fix + ORDER FILES (2026-10-27 22:56 local)

- User found R5 under the flat microSD module. Planned offline (plan_r5_move.py) after measuring the
  module strip (U6 socket at x585 + 46×24 mm flat module → keep-out x500–2460, y30–1040) and the dense
  U2/C1/R4-R10 corridor. Chosen home: **R5 (1890,1335) rot0** — 402 mil straight down, clear of the module
  and every foreign pad (pads ≥6 mil verified), body over via stitching only (insulated body on mask is fine).
- Saved in WRO_HUB_REPAIRED_r5_moved_20261027-2234.epro2 by 82_move_r5.js: component move, designator
  centered on the body, and deletion of the old SERVO_IN stub 39b1d74c0d7108f7. The new SERVO_IN pad
  lands directly on net track 918299703502a0e0 (same-net overlap = connection; proven by shape intersection).
  Export diff: exactly COMPONENT R5 + 2 R5 ATTRs + 1 allocator; stub removed; nothing else.
- GND regenerated against the moved geometry: 2,362 fills (713 top / 1649 bottom), verified 0 overlaps /
  0 clearance violations / 0 disconnected GND / edge 16 mil / antenna 7.91 mil. Saved in
  WRO_HUB_REPAIRED_r5_ground_20261027-2255.epro2.
- ORDER FILES for the user (JLC upload, bare PCB): **`PCB design/exports/WRO_HUB_ORDER_GERBER_20261027.zip`**
  (11 files; outline −675..4695 × 0..3975 mil; PTH tools include the approved 4.0 mm XT60 / 2.5 mm XT30
  drills and 6 plated-slot commands for J3; via drill separate) and
  **`PCB design/exports/WRO_HUB_ORDER_BOM_20261027.csv`** (J3 row = KCD4-203-B, User-supplied, no C20088586).
- **Ruling — the single remaining DRC "Netlist Error" (rule "Import Changes")**: the PCB pad-net multiset
  (270 rows) is byte-identical to the pre-repair clean checkpoint and the whole repair chain touched no
  electrical records, so the netlist CONTENT is unchanged; the error is the J3 instance-vs-device footprint
  override surfaced after the schematic re-save. Do NOT click Import Changes (it would revert J3 to the
  catalog VH-6 footprint). Gerber/drill/BOM are generated from PCB geometry only and are unaffected.
- Still hardware-gated before ordering is a *product* decision: KCD4 slot fit on a 1:1 print + real switch,
  and ≈8 A worst-case thermal qualification. The Gerber zip itself meets the fabrication rules checked.

## Prior final state — blocker fixes complete (2026-10-27 21:41 local)

Release checkpoint: `PCB design/hub/WRO_HUB_REPAIRED_catalogue_cleared_20261027-2040.epro2`
(project `WRO_HUB_REPAIR_ISOLATED_20261027`, PCB `8efe5b895e7646ea`).

- J3 catalogue fix complete: 25 J3-owned schematic attribute records corrected (values only plus
  editor zIndex renumbering). Fresh `sch_Drc.check()` = true, detailed check = []; no wire/pin/net
  edits, no device-library writes. The stale B6P-VH supplier/datasheet/mechanical fields are gone;
  J3 identifies as user-supplied KCD4-203-B with the custom slot footprint (`de6d9dbe09d94d61`).
- Full-chain allowlist (`final_allowlist.py`) vs the switch-instance checkpoint base: exactly
  25 J3 schematic ATTRs + 167 PCB silk style/position records + 3 RF line records
  (2 modified endpoints + 1 new segment) + GND fill replacement (2,360 removed / 2,364 added /
  1 rewritten) + 1 ELE_PLACEHOLDER allocator. Zero illegal changes.
- Independent geometry gates on the saved release: copper–copper ≥ 5.9843 mil (0.152 mm, JLC 1 oz
  0.10 mm met; four pre-existing pairs at exactly the 6 mil design rule), copper–edge ≥ 35.9 mil,
  0 drill-spacing violations (JLC 0.2/0.45 mm), 0 hole-to-foreign-copper violations
  (JLC 0.2/0.28 mm). Silk 187/187 ≥ 40 mil height / 6 mil stroke. Antenna projection copper-free.
  GND plane: 0 overlaps / 0 clearance violations / 0 disconnected GND; strict DRC = [].
- Live Gerber/BOM export script ready (`81_export_repair_manufacturing.js`, read-only); the bridge
  restarted mid-run, so the final export still needs one GUI reconnect, then `manufacturing_export.json`.
- Physical-fit and full-load hardware gates remain open (see hub/FINAL-REVIEW-2026-10-27.md):
  KCD4 slot/blade fit on a real switch + 1:1 print, and ≈8 A worst-case drop/thermal qualification.
  Board-level fixes do not replace them.

## Continuation history (2026-10-27 20:14 local)

- Recovery COMPLETE: isolated project ea32443fed8f1fd5989b3f8520da07a81d65c35e38ca7f144b04f7dc31730e1f,
  PCB 8efe5b895e7646ea. All 28 DEVICE metadata dictionaries were independently verified restored.
- Verified checkpoint: WRO_HUB_REPAIRED_switch_instance_fixed_20261027-1914.epro2.
  J3 instance overrides persisted; no DEVICE records changed. ERC has 0 fatal/0 error/1 supplier warning.
- Original project remains metadata-damaged. Never use it as the current repair target.
- Ruling: use the verified isolated EDA project and full archive checkpoints as isolation, not a new git
  worktree — the actual PCB inputs are untracked and live in the editor; a worktree omits them.
  Cost if wrong: concurrent editor changes require an abort and fresh snapshot. No commits authorized.
- Pre-flight interfaces: silk and RF corrections feed the subsequent ground regeneration; all depend
  on corrected native footprint geometry. Final verification must use the new project IDs and parser.
- Ruling: preserve the existing approved spec/plan and this ledger, rather than introduce another active
  plan — this is continuation, not a redesign. Cost if wrong: missing assumptions must be added here.
- Resumption found the old project active. Opening the isolated UUID alone left Start Page (local
  project not registered in that editor instance); opening its existing local file started a second
  window. Bridge then stopped; restarted server. No PCB writes have occurred during this resumption.
- Silk style regression observed RED: missing safe style planner. Implemented visible-only min40/6;
  GREEN and live application pending. Text placement, RF route, GND and final DFM gates remain pending.
- Task silk: style and placement regressions RED to GREEN, 3/3. Saved 161 text styles and 11 small
  translations in WRO_HUB_REPAIRED_silk_positioned_20261027-2019.epro2. All 187 visible text/designators
  >=40mil/6mil, measured bounding boxes clear pads by >=6mil, board edge16mil and other text6mil.
  Export proves only 98 STRING and 69 ATTR bodies changed; all device/footprint/copper/placement records preserved.
- Ruling: distinguish the conservative GND-plane keep-out (starts y3635) from the actual radiating
  module extension (starts y3885.19685 = pin1 y3810 + drawing's 1.91mm). Only two +3V3 segments enter
  the latter. Keep all other under-body tracks, full 0.8mm width and original route endpoints.
  Cost if wrong: RF performance still requires hardware testing; carrier edge remains under the module.
- RF test observed RED (missing planner). Proposed path2250,3810 ->2305,3865 ->2630,3865 ->2680,3915.
  Independent geometry: closest foreign copper7.756mil (U1B.1); antenna gap4.449mil. GND-plane keep-out
  remains larger and untouched. Two endpoint modifications +one new segment, no via/component changes.
- RF complete: 1/1 regression GREEN; saved two scoped endpoint edits and one new segment in
  WRO_HUB_REPAIRED_rf_fixed_20261027-2022.epro2. Saved geometry has no copper in actual radiating extension.
- Ground complete: 2364 fills (704top/1660bottom) regenerated against corrected current native geometry.
  WRO_HUB_REPAIRED_ground_fixed_20261027-2028.epro2 verified0overlap/0clearance/0disconnectedGND;
  strict DRC[]; diff vsRFcheckpoint onlyFILL replacement and one ELE_PLACEHOLDER allocator update.
  No additional vias moved or resized. Evidence saved_ground_verified.json.
- J3 offline custom-device package test RED to GREEN (1/1) proves a targeted native META/attribute patch.
  However importing WRO_HUB_FIXED_20261027.epro2 failed twice (undefined project result, zero-byte local file).
  It is DIAGNOSTIC ONLY, NOT a delivery/release. Do not use the finalized/zero-byte local file.
  Import failure also broke getCurrentProjectInfo() in that editor instance; reopening by UUID did not fix it.
  Closed only after all tabs verified saved, then reopened the verified existing recovery local file.
- Ruling: stop the optional custom-library package path. Keep the working instance footprint override and
  correct all false catalogue fields via documented schematic instance attributes only. Do not directly
  edit SQLite/history internals, and do not retry the unsafe device-library API.
  Cost if wrong: future Device Standardization may still replace the custom override; prohibit that action
  and validate the saved footprint binding and exported assembly BOM instead.

## Historical recovery investigation (superseded by verified isolated recovery above)

User authorized correction of the prior final-review blockers. A fresh baseline was exported as
PCB design/hub/WRO_HUB_before_blocker_fixes_20261027-1836.epro2.

## Verified completed work

- Actual project-footprint editing via LIB_Footprint.openInEditor + PCB_PrimitivePad.modify +
  PCB_Document.save DOES persist (verified in a full export).
- Q1 keeps 0.75 mm holes and original IDs/centres; pad width 1.115 mm, height1.6 mm.
  Nominal ring about0.1825 mm meets absolute JLC1oz0.18 mm, not preferred0.25; padgap0.155 mm meets6mil.
- J1/J2 drills4.0 mm, pads5.5 mm; J4 drills4.0 mm, pads6.0 mm.
- J5 drills2.5 mm, pads3.5 mm. All PCB component/track/pad-net records unchanged; DRC empty.
- Known-good checkpoint: PCB design/hub/WRO_HUB_after_footprint_fixes_20261027-1844.epro2.
  Its DEVICE metadata remains byte-identical to the pre-task backup.

## Failed path: do not repeat

- LIB_Device.modify returned an opaque error for the local project device update.
- Export then showed ALL 28 DEVICE META attributes reduced to an incorrect Footprint entry.
  Geometry, PCB components/tracks/pad nets and schematic were not altered by that call.
- Correct project-library token from getProjectLibraryUuid is 'project', not project UUID.
  A one-device recovery attempt with that token ALSO failed; no further library writes are authorized.
- Reopening the same project did NOT clear the bad metadata. User's first reopen attempt still
  exported the damaged DEVICE metadata. Do not claim recovery until full metadata comparison passes.
- 68_sync_switch_binding.js is disabled with an unconditional throw. Do not replay it.

## Recovery artifact

- PCB design/hub/WRO_HUB_RECOVERY_20261027.epro2 is a distinct-name recovery archive.
- Its native EPRU design is byte-identical to the known-good after-footprint checkpoint.
  Only project2.json title was changed to WRO_HUB_RECOVERY_20261027 to avoid reopening the same identity.
- ZIP integrity verified; SHA256 cd825d604602476fd9565ecbd567f227410bdb52fb804084241edd46b083feea.
- Import as a NEW local project, preferably after closing/restarting EasyEDA. Then reconnect, export,
  and compare all DEVICE documents, corrected footprints, PCB records to the known-good checkpoint.

Remaining work AFTER recovery: J3 instance-only footprint/BOM overrides (no device-library API),
silkscreen >=40mil/6mil with pad/text clearance, scoped antenna-route fix, GND regeneration if needed,
final ERC/DRC and Gerber/drill inspection. Physical switch fit and full-load thermal checks need real hardware.
