# Hub PCB decisions

## H-009 — Resolve only confirmed J3 metadata differences; preserve raster artwork (2026-10-28)

The user added a top-silkscreen raster image and asked to fix the Netlist Error. Inspecting the
complete GUI comparison established25 J3 property updates, no electrical/footprint updates. A full
backup preceded the user-applied ECO, with track-net updates disabled. Saved comparison shows only
one J3 COMPONENT.attrs record changed; the original image object/resources, six slots, copper,
placements, pad nets and2331fills are preserved. Fresh strict DRC[] and ERCtrue/detailed[].

Ruling: a reviewed property-only ECO is appropriate here; previous blanket "never import" warnings
must not be mistaken for a prohibition on safely reconciling confirmed metadata. Cost if wrong:
any physical ECO difference requires stopping, not applying. Complete before/after checks passed.

A separate Image-only top-silk export produced an empty standard GTO. Ruling: preserve the raster
image and disclose that ordinary Gerber omits it; do not silently vectorize or promise that it prints.
Cost if wrong: converting or choosing color-silkscreen needs a deliberate artwork/fabrication review.
Current files and caveats: [synchronization fix](SYNC-FIX-2026-10-28.md).

> **Current state: [2026-10-28 release review](RELEASE-2026-10-28.md).** The user made a later R5/routing
> edit, and GND has been recomputed. Early "latest state" notes below are historical checkpoints.

## H-008 — Repour from the user's final edits; separate physical checks from release approval (2026-10-28)

- Preserved the user's R5 pose(1605,1305,90°), SD_SCK/SERVO_IN/+5V routing and via changes. Added only
  2,331 GND fills; all86documents compared, all633track bodies preserved. Two tracks changed IDs without
  changing body payloads; the guard stopped, complete equality was established, then only IDs were refreshed.
- Fresh ERC true; current schematic/PCB membership agrees for all256 named pins. Physical DRC0,
  **synchronization finding1**, not "strict DRC0". No rule disabled and no ECO applied.
- Ruling: do not infer the ECO cause solely from old pad-net equality or J3's override. Current pin-net
  checks prove the electrical membership match, not the reason for the synchronization warning.
  Cost if wrong: unresolved metadata/ECO debt needs manual review; the package remains a disclosed
  prototype fabrication-review package rather than unconditional production release.
- Ruling: retain the user's geometry and existing conservative GND clearances, rather than replay
  historical placements or pours. Cost if wrong: current physical part/enclosure fit still needs review.
- Inspected actual11-fileGerber package, closed136.398×100.965mmoutline, all548rounddrills+6platedslots,
  Q1aperture and77-part rawBOM. New files live in [the dated fabrication folder](fabrication/20261028-after-user-edits/README.md).
- Ruling: retain raw BOM aliases as evidence; disclose Q1's SS8050 alias versus approved2N2222A and J3's
  legacy footprint name instead of inventing catalog data. Cost if wrong: do not use this raw BOM for PCBA.
- Real-switch fit, support/contact action, Q1 pinout, module underside clearance and full-load thermal
  qualification remain hardware gates. No order/payment was placed.
- Lessons: [shared PCB review guidance](../PCB-REVIEW-LESSONS.md). Latest review supersedes prior claims
  that all blockers were resolved or that the Netlist Error was conclusively only a J3 footprint alias.

> **Latest state, 2026-10-26 (retry 2): KCD4-203-B footprint INSTALLED and verified in the saved project.** J3 is bound
> to project footprint `ad35aed4d7610fd2_0cbe9290d583d889` (six plated 7.8 × 1.8 mm slots, 3 × 9 mm oval pads, the
> same six nets). Board widened left to 132.6 × 99.1 mm; only J3, J4, J5, LED3, the two left holes and their labels
> moved. See "Retry 2 as built" at the end. Scripts 48 / 50 stay disabled; use 52–54. Physical fit still unverified.

## H-001 — KCD4-203-B direct mount (2026-10-26, user)

Replace the J3 VH-6 harness with the pictured six-terminal DPDT centre-off rocker directly on the PCB. Preserve all
six power nets and the historical J3 pin numbers. The authoritative geometry is WIRING-PLAN §2.4, not generic KCD4
dimensions: 10.5 mm contact pitch, 11 mm pole spacing, 34.5 × 25.5 mm flange. User supplied blade dimensions
6.3 ±0.10 × 0.8 ±0.03 mm and stated that no physical component is available.

Ruling: use 7.8 × 1.8 mm plated rounded-end slots with 9.0 × 3.0 mm oblong pads — the proposed 6.6 × 1.1 mm slots
do not enclose sharp rectangular blade corners — cost if wrong: hand assembly may be awkward or the physical
switch may still differ, so actual-part fit and fabricator slot-tolerance confirmation remain mandatory before ordering.

Ruling: retain the live user's layout rather than regenerate the old floor plan; reserve space for the entire flange
and extend the left outline locally if necessary — cost if wrong: increased fabrication bounding box and enclosure
space; the result must be visually reviewed before routing or ordering.

Execution record: task branch pcb/hub-direct-rocker-20261026. Existing PCB files are untracked in the repository, so
a separate worktree would omit the actual design; the live editor is protected instead by a verified full project
archive, WRO_HUB_before_direct_rocker_20261026.epro2 (604962 bytes; five entries, integrity check passed), plus
read-only before/after snapshots. No other project may be edited. No commit or order is authorized by this task.

Status: dimensions confirmed; five geometry regressions passed after an initial failing run. Generated native footprint
source and the guarded library writer are in scripts/hub (rocker_model.py, test_rocker_model.py, 47_rocker_library.*).
The finished-slot model contains a 6.4 × 0.83 mm rectangular blade at all ±0.20 mm offsets with the hole reduced to
7.7 × 1.7 mm. Slot axes follow the drawing's end view; all six pin-to-net assignments are unchanged.

2026-10-26 execution blocker: EasyEDA's top-level document reports visibilityState = hidden. Restoring/activating its
native WRO_HUB window and temporarily making it topmost did not change that report; the native window reports visible
and not minimized. Every attempted library writer aborted at its FIRST visibility guard, before any library or board
mutation. Do not remove that guard or assume the footprint was placed. User must make the editor visibly active before
continuing; hidden editors have frozen commit timers in this workspace. The temporary topmost setting is restored.

API finding for resume: getProjectLibraryUuid() returns the literal alias "project"; getState_Footprint() reports the
full project UUID instead. The generated writer accepts the documented project-library alias, but actual local-library
creation/update has NOT yet run. A project-footprint source read using the full UUID timed out. The personal-library
UUID is undefined in this workspace. Library source may need to be read using the "project" alias.

Remaining: run guarded library creation; verify six physical slot shapes read back; create a matching schematic device
without disturbing existing wires; replace the PCB component; give the large flange a collision-free location and
record any local outline change; strict ERC/DRC and before/after comparison; export the resulting project. The current
board and schematic still contain the original J3 harness connector.

2026-10-26 resume: user made the editor visible; visibility guard now passes. Project-library create returned undefined;
project footprint-source read timed out with both the full project UUID and the "project" alias; openInEditor also
failed. None changed the design. Ruling: use document-local pad overrides, as already used for the XT holes, rather
than unsupported library writes — cost if wrong: a later schematic import may restore the carrier's original geometry,
so the guarded switch override must be rerun and reverified after every import. Supply the generated native custom
footprint source for migration to a real custom library. Retain the base library binding as a carrier, NOT a claim
that B6P-VH is the physical part. Both schematic and PCB names/MPN/BOM supplier fields must identify the KCD4-203-B.

Placement ruling: switch pattern rotated 90 degrees at (−720, 1450) mil; flange 25.5 mm along board X and 34.5 mm
along Y. Add an outline tab from x −1300 to −300 mil, y 650 to 2250 mil. The original board region (x −300..4600,
y 0..3900), all four mounting holes, all other placements and all copper stay unchanged. New fabrication bounding
box is 149.86 × 99.06 mm. Cost: larger bounding-box price and an unverified enclosure/support arrangement.

2026-10-26 13:22 UTC: the component-pad position override is ALSO unsupported. The initial component move/MPN update
worked but the first pad edit was rejected ("object parameters incorrect, cannot apply to canvas"); no slot was made.
49_restore_rocker_partial restored J3's exact 11 original records only, then saved. Readback: position (−10,1446),
MPN B6P-VH, supplier C20088586, six pads. Other records were preserved. 48 is now deliberately disabled to prevent
another partial edit. The native custom footprint source and seven passing geometry/placement tests remain valid,
but the live design still has the original connector and ORIGINAL outline. Do not claim the extension was applied.

Resume requires the user to open J3's footprint for editing via the GUI. Once that FOOTPRINT document is active,
the documented sys_FileManager.setDocumentSource API can be used in the correct document context, followed by pad
readback and saving/applying the edited footprint. This avoids unavailable cloud-library APIs and unsupported
component-pad position overrides. Confirm the footprint document identity before writing; never write footprint
source into PCB1. The intended extension must only be applied once a real switch footprint is successfully installed.

Correction (13:30 UTC): reference inspection found a concrete parameter bug: the pad-shape enum member OBLONG has
the actual serialized/API value "OVAL", not "OBLONG". A new regression assertion failed on the emitted shape and
the generator was corrected. Therefore the earlier canvas rejection does NOT prove that component-pad positions
are unsupported. Script 48 remains disabled because the user has now opened the proper project-local footprint
editor; use that safer complete-footprint replacement path. The temporary footprint-edit document reports type 4,
UUID ad35aed4d7610fd2_0cbe9290d583d889, with a native PCB source header and six free pads e21..e26. Preserve these
pad IDs by pin number so applying the edited footprint retains every existing pad-to-net mapping.

13:36 UTC: the project-local footprint editor now contains the complete switch geometry, verified by native source
readback: six original pad IDs preserved; exact confirmed centres; plated SLOT 7.8 × 1.8 mm, relativeAngle 90°,
OVAL copper 3.0 × 9.0 mm; flange 34.5 × 25.5 mm; pin labels 1–6; no old connector fills. The initial native-hole
serialization was corrected by a failing regression: native SLOT width means LENGTH and height means DIAMETER,
not X/Y dimensions. Incorrect width < height had been normalized into an oversized round-looking hole; all six
are now verified correct. setDocumentSource returned false in this temporary footprint context despite applying
the change; exact readback, not that ambiguous return value, is the success criterion. 50 now enforces that check.

Next GUI gate: user applies/saves the local footprint edit back to PCB1. Do not assume the board component has changed
until its six pads read back with the new geometry and the same six power nets. Then place the switch, apply the
tested local outline extension, update BOM/schematic description, verify unaffected records and run ERC/DRC/export.

Final recovery entry (14:03 UTC): the user screenshots show NO "Apply only selected components" control in this
project-library editing context; that instruction was inappropriate. PCB1's J3 had zero pads after the local save.
The editor save API returned true, but independent full-project export proved the saved temporary footprint
ad35aed4d7610fd2_0cbe9290d583d889 contained no PAD/POLY/STRING records. Thus canvas geometry readback was NOT a valid
integration check. Changing the native source header from PCB to FOOTPRINT returned true but did not restore pads.
Do not repeat these attempts. WRO_HUB_switch_edit_checkpoint_20261026.epro2 (611680 bytes, archive integrity valid)
is diagnostic evidence only, NOT a release. The original untouched ae8f5762b637161e footprint still has all six pads.

49 restored the COMPLETE original 11 J3 records, including pad-net rows deleted by the invalid footprint update.
Readback after saving: x −10, y 1446 mil, B6P-VH, C20088586, six pads. Original outline and other live records were
preserved. The failed edit tab must be closed and the remaining numerical source/preview labelled provisional.
Scripts 48 and 50 are disabled to prevent another misleading canvas-only result. No successful switch installation,
physical fit, DRC-clean release or fabrication-ready file is claimed. No more GUI clicking is required from the user
for these failed attempts. Future work needs a separately verified library/import path and a real saved-project
round-trip test before touching the board again.

Recovery verification (14:06 UTC): closed the stale footprint-edit tab successfully and left PCB1 active. Fresh
20b inventory comparison against the pre-change snapshot found ZERO differences in all 84 component positions,
rotations, bboxes and pad/net maps. J3 again has BAT+, V12, DC+, BAT−, GND, DC− on the original six round pads.
The seven numerical geometry tests pass. This verifies recovery, NOT switch installation. Source writer 50 is now
disabled in both template and generated script. The diagnostic checkpoint may contain an unused empty footprint;
it must never be used for fabrication. The separate pre-change archive remains the reference backup.

Pre-flight for retry 2 (2026-10-26, read-only, nothing written to the design): bridge healthy, one window, WRO_HUB /
PCB1 active and visible. Fresh full-project export `WRO_HUB_before_rocker_v2_20261026.epro2` (611686 bytes, zip test
clean) via new `scripts/hub/51_export_project.js` + `export_project.py`; `inspect_export.py` reads what EasyEDA
serialized. Live 20b inventory = recovered snapshot (84 parts, 0 differences); seven geometry tests pass. Export shows
J3 original (ae8f5762b637161e, six round pads, PAD_NET e21–e26 = DC−, GND, BAT−, DC+, V12, BAT+) plus one orphan empty
footprint `ad35aed4d7610fd2_0cbe9290d583d889` (unused). Structure: PCB1's source holds no footprints; J3's `Footprint`
ATTR names a project footprint document and PAD_NET rows key on that footprint's pad ids, so a real switch needs a
project footprint with six slot PADs and J3 rebound to it. Pad-overrides on component pads are canvas-only (below).

FINDING (affects the XT parts, not only J3): the XT hole overrides of `44_xt_pads.js` are NOT in the saved project.
Live J1 hole = 4.6 mm, J4 4.7 mm, J5 1.8 mm (library defaults); the archive has no override records and footprint
90dc5a3cf82dd629 still holds 4.6 mm holes. The session-log claim "J1 / J2 4.0 / 5.5 mm …" is not true of the current
board. Treat component-pad `toAsync()…done()` edits as non-persistent until an export proves otherwise.

Native slot schema confirmed from existing footprints in the archive: `hole {holeType:'SLOT', width = overall length,
height = diameter}`, pad `{padType:'OVAL'}`, `relativeAngle` rotates the hole within the pad. API form:
`['SLOT', diameter, length]` (TPCB_PrimitivePadHole), pad `['OVAL', w, h]` (enum OBLONG = 'OVAL').

Retry 2 as built (2026-10-26, user-approved method; plan = the five steps in the chat, recorded here):
1. User opened J3 → Edit Footprint (doc type 4, `ad35aed4d7610fd2_0cbe9290d583d889`). `52_rocker_fp_editor.js` changed
   the six existing pads with `pcb_PrimitivePad.modify` (ids e21–e26 kept, so PAD_NET rows keep their nets): positions
   per §2.4, pad `['OVAL', 3 mm, 9 mm]`, hole `['SLOT', 1.8 mm, 7.8 mm]`, holeRotation 90, plated. Deleted the old VH-6
   silk / body / pin fills; drew the 34.5 × 25.5 mm flange (silk), body (layer 48), 36.5 × 27.5 mm courtyard (layer 13,
   document) and a pin-1 "1". Native-source readback before save: SLOT width 7.8 (length) / height 1.8, relativeAngle
   90, OVAL 3 × 9. Re-run: 6 pads OK, nothing redrawn. User saved the tab.
2. Export `WRO_HUB_after_rocker_fp_save_20261026.epro2`: J3 → that footprint, 6 PADs with the slot data, PAD_NET
   1..6 = BAT+, V12, DC+, BAT−, GND, DC−, 84 components, no empty footprint left (the orphan was overwritten).
   **Ruling:** in-editor primitive edits persist; source replacement (`setDocumentSource`) did not — the difference
   that made retry 1 fail.
3. User: "the PCB might need a bit more width; adjust only the overlapping things". New `live_space.py` (live
   inventory + exported outline / holes; the old `layout_plan.py` no longer matches the user's layout) found J3 over
   J4, J5, U1A, U6 and off the left edge. Ruling, option A (keeps the switch orientation, least widening): left edge
   −300 → −620 mil (124.5 → **132.6 mm**, height 99.1 mm unchanged); J3 (−10, 1446) → (−60, 1495), rot −90 kept;
   J4 −195 mil in y; J5 −125 in y; LED3 +450 in x (J5 would have hit it); left M3 holes to the new corners
   (−450, 160) and (−450, 3730) — this also clears J1 from the old (170, 3730) hole. The superseded (−720, 1450) +
   outline-tab plan is dropped (a rectangular board is simpler for the enclosure). Rejected: rotating J3 by 90° (only
   J3 moves, but 141.5 mm wide and a turned rocker); keeping J4/J5 and moving J1/J2 up (XT60 metal closer to the
   DevKit antenna, already ≈ 12 mm < 15 mm). `53_rocker_local_moves.js` (from-position checks, idempotent): 4 parts,
   2 holes, outline replaced; re-run all OK.
4. `54_rocker_silk.js`: free labels follow their parts (J4 "TO BUCK 12V" / "+" / "−", J5 "− 5V FROM BUCK +", LED3 "5V");
   old VH-6 "1" deleted; "SWITCH J3" and the J3 designator placed between the switch's pad rows. `24_check_silk.js` now
   reads the real outline: 0 edge, 0 overlaps; 1 hit = "MUSEUM HUB PCB v0.4" on a pad at (3817, 595), not near these
   moves (user layout; fix with the next silk pass).
5. Final export `WRO_HUB_after_rocker_silk_20261026.epro2`: 84 components, J3 6 slot pads + nets as above, outline
   −620..4600 × 0..3900; 20b inventory vs before: only J3, J4, J5, LED3 differ; `live_space.py` 0 problems; 7 model
   tests pass (`live_plan` = as built).

Still open (not done by this step):
- **Import Changes reverts J3** to the VH-6 footprint: the schematic device still names ae8f5762b637161e. Before any
  import, point the schematic J3 at the new footprint (GUI) or re-check J3 by export afterwards.
- J3's BOM fields are still B6P-VH / C20088586; the spec requires KCD4-203-B (hand-soldered, no LCSC part).
- `23_silkscreen.js` holds the old label positions and deletes + redraws all free silk: do not re-run until updated.
- XT holes (finding above): J1 / J2 4.6, J4 4.7, J5 1.8 mm on the board, not the user's 4.0 / 2.5 mm.
- Physical gate of §2.4 (real switch, 1:1 print, slot tolerance with the fab) unchanged.

## H-007 — Fabrication-blocker fixes in the isolated repair project (2026-10-27)

**Context:** the `lib_Device.modify` attempt (now disabled, script 68) corrupted all 28 DEVICE metadata
dictionaries of the original `WRO_HUB`. Work continued in `WRO_HUB_REPAIR_ISOLATED_20261027` (new identity
imported via API; full metadata equivalence proven by reverse UUID mapping of all 10,450+ records).

**Decisions (with cost if wrong):**
- *Ruling:* treat the isolated EDA project + verified `.epro2` checkpoints as the isolation boundary instead of
  a git worktree — the design lives in EasyEDA files that git does not track. Cost: concurrent editor edits
  could race; mitigated by source-locked guards in every writer (all abort on mismatch).
- *Ruling:* fix Q1/J5/J1/J2/J4 at the project-footprint level (openInEditor + PrimitivePad.modify), never
  component-pad overrides (canvas-only) and never source replacement. Verified byte-level in saved exports.
- *Ruling:* J3 stays an instance-attribute override. The offline custom-device package
  (`switch_metadata_package`) passed all invariants but its import into a fresh project failed twice (importer
  returned no project, left a 0-byte file) — both artifacts are diagnostic only. Catalogue supplier/mechanical
  fields were cleared on the instance instead; a future "Device Standardization" click would still revert J3
  and must not be used. Cost: BOM tools reading the library device still see a connector title; the exported
  BOM/attributes come from the corrected instance fields.
- *Ruling:* the antenna fix targets the module's actual radiating extension (pin-1 row + 1.91 mm body offset,
  y ≥ 3885.2 mil), not the deliberately larger GND-plane keep-out (y ≥ 3635). Only the two +3V3 segments that
  entered it were rerouted (full 0.8 mm width, original endpoints); under-body signals that stay below the
  module edge were left alone. Cost: RF performance still needs hardware validation.
- *Ruling:* four copper pairs at exactly 5.9843 mil (0.152 mm) are the pre-existing user routing at the 6 mil
  design rule; JLC 1 oz absolute minimum is 0.10 mm, so they stand (documented, not "fixed").
- Silk: 161 objects enlarged to ≥40 mil/6 mil; 11 bounded relocations using measured bboxes (pads ≥6 mil,
  edge 16 mil, text–text ≥6 mil). Ground: regenerated 2,364 fills from the corrected native geometry with the
  H-006 clearance scheme; verified 0 overlaps / 0 clearance violations / 0 disconnected GND, strict DRC 0.

**Open before ordering:** live Gerber+BOM capture (needs one API-Gateway reconnect; script 81 is ready),
real-switch slot fit + 1:1 print, ≈8 A thermal qualification. No commit, no order.

## H-002 — Remove SB1–SB7 bypasses; preserve the LED hop chain (2026-10-26)

**Source:** user's explicit subtraction task and follow-up authorization to remove the same schematic symbols
and only their exclusive wire stubs when PCB-only removal exposed a schematic/PCB mismatch.

**Decision:** retire seven C492401 2-pin / 2.54 mm bypass footprints and their designators. Keep R61–R67 (220 Ω),
J10–J17 and every surviving net/pad/placement/footprint unchanged. No onboard empty-station bypass remains.
Do not move or reroute anything, edit rails/outline, repour, replay historical builders, or Import Changes.

**As built:** full backup `WRO_HUB_before_sb_removal_20261026-2307.epro2` → snapshot-locked `56_remove_sb.js`:
PCB 84 → 77, seven components + 21 owned attributes + 14 pad-net rows removed. No bypass note existed and no
routed board copper/vias/pours existed: zero free-silk/copper deletions. PCB-only DRC exposed one Netlist Error;
user approved `58_remove_sb_sch.js`: seven symbols and 14 private 30-unit named wire stubs removed, preserving
all other symbol/connection bytes and all 74 named nets. Do not suppress DRC or import to hide a mismatch.

**Verification:** final full project `WRO_HUB_after_sb_removal_reconciled_20261026-2321.epro2` passes 13 acceptance
checks; all 77 survivors and every footprint document unchanged, all board geometry unchanged. Strict DRC:
270 → 256 existing Connection Error findings (exactly the 14 SB pad findings removed), zero new findings,
Netlist Error gone; schematic ERC true. Seven hub and seven station model tests pass. Separate author
self-review only, not an independent engineering review. [Detailed evidence](SB-REMOVAL-2026-10-26.md).

**Limitations:** board remains unrouted; no production-readiness claim. Historical builders still include SBs
and must not be replayed. J3's schematic footprint binding/BOM caveat and the actual XT hole sizes from H-001
remain unchanged. No commits, pushes or fabrication orders were performed.

## H-003 — Place only unplaced station-group parts for routing (2026-10-27)

**Source:** user requested good placement for only the currently unplaced S1–S8 and supporting resistors/caps,
then explicitly delegated placement judgment with no cable-position constraints; everything else must stay.

**Scope:** J10–J17, R31–R37, C3–C9, R41–R44, R51–R54, R61–R67 = 37 off-board parts. Freeze all other 40 parts,
including R4/R6/R13 and servo parts the user moved while planning; no schematic/copper/outline/hole/net/free-text edits.

**Agent's layout choice:** two four-port rows of vertical XH-8s: S1→S4 left-to-right at y3150, S5→S8 right-to-left
at y1850, forming a short serpentine LED chain. Hops directly beneath rows, zone pull-ups in column gaps,
zone series parts near the MCU, and all seven MQ caps low under the DevKit with signal pads <275 mil from ADC pins.
This arrangement fits the existing right-side space without moving the MCU, enlarging the board or touching routing.

**As built:** snapshot equality guard first aborted before writes on concurrent unrelated edits. Refreshed full
backup `WRO_HUB_before_station_place_guard_20261027-0148.epro2`; eight model checks pass. `59_place_station.js`
changed only those 37 poses. Full export `WRO_HUB_after_station_place_20261027-0148.epro2` proves only allowed
COMPONENT positions/rotations and 91 owned attribute transforms changed. All 40 other parts, pad nets, footprint
documents, 50 routed copper segments, free text, holes, outline and all schematic/library records unchanged.
DRC unchanged at 25 Physical Error + 234 Connection Error, zero new findings. [Report](STATION-PLACEMENT-2026-10-27.md).

**Deliberately not done:** no routing, no relocation of any unrelated off-board part, no free station-silk cleanup,
no J3/XT correction, no fabrication approval. The existing free station labels remain off-board by the user's strict
scope; do not replay old builders. Author self-review only; user visual inspection/fabrication review still applies.

## H-004 — Complete station silk and R4/R6/R13 placement (2026-10-27)

**Source:** user: "you left behind the silkscreens and R13,R4,R6 place them properly as well".
This explicitly extends the prior part-only scope; do not treat unrelated parts/routes as authorized.

**As built:** R4 (2310,1805), rot90 by U2's LED input; R6 (2145,1805), rot90 with LED_BUF pad about
122 mil from U2.3; R13 (2680,1905), rot90 with ESTOP_IO pad about 145 mil from U1B.18. The user rotated
U2 180° and moved C1 during planning; the first guard aborted before any write. Refreshed the baseline,
preserved both edits, and replanned against current pins. No copper was dragged or altered.

Moved exactly 73 existing free strings: eight station/zone names, 64 pin labels, old off-board board title.
Pin text is horizontal 32 mil / 6 mil stroke beside actual port pins; original content/IDs preserved.
Station names above/below the two rows; S5's 5V label offset 20 mil to clear the R64 turn pad. Three resistor
designators placed horizontally adjacent to their bodies. No blanket silk delete/recreate.

**Verification:** before `WRO_HUB_before_station_cleanup_guard_20261027-0209.epro2` → after
`WRO_HUB_after_station_cleanup_20261027-0209.epro2`; only those three component poses, seven owned attribute
transforms/styles and 73 authorized string transforms/styles changed. All other 74 components, 62 routed
copper segments, all nets/footprints/holes/outline, unrelated silk and every other project document unchanged.
Actual bounding boxes: all 76 adjusted text objects clear pads/edges/other text. No new DRC findings;
25 Physical Error + 230 Connection Error unchanged. Six new + 22 existing model tests pass. Author self-review,
not an independent fabrication review. [Evidence](STATION-CLEANUP-2026-10-27.md).

## H-005 — Finish the routing on the live board, keep the user's copper (2026-10-27)

**Source:** user: "make a copy first, preserve that copy as it is as backup, and finish the tracing on other copy".
**Backup (untouched, set read-only):** `hub/WRO_HUB_pre_route_20261027.eprj2` (SQLite snapshot, integrity ok) and
`hub/WRO_HUB_pre_route_20261027.epro2` (saved-state export). Routing was done in the open project WRO_HUB.

**Placement:** R5, R7–R10 were still off-board (x ≈ −1700). Free-space search on the live board (part outlines + 30
mil, the user's tracks + 45 mil, holes) → `62_place_servo_res.js`: R7–R10 vertical under the DevKit between U1A's
pin row and R6 (x 1660 / 1780 / 1900 / 2020, y 1840, rot 90, SERVO_BUF pad toward U2.6); R5 horizontal below U2 / C1
(1890, 932.5). A first spot (strip between J3 and U1A) sat on the user's I2C tracks and was replaced before any
routing was written. `63_servo_designators.js` moved R6's and U1A's designators off the new parts.

**Routing (`scripts/hub/route/`):** the robot router engine on a live-board model — `20e_live_pads.js` dumps every
pad's real shape / hole / net, the outline and the M3 pads; model frame = EasyEDA + 675 mil in x. The user's 62
tracks are fixed copper: obstacles for other nets, and each copper island with its pads is one item to join.
Classes: +5V trunk 2.5 mm (J5 buck → stations, servos, spare outputs; EasyEDA caps 1 oz tracks at 2.54 mm), small
5 V loads 0.8 mm, +3V3 0.8 mm (0.3 mm only between socket pins), BUZ_N 0.8 mm, signals 0.35 mm. PathFinder: 7
passes, 0 conflicts. `finish.py` trims / stitches / checks on the merged (user + new) copper. Add-only writer
`40_route_add.js` (never deletes; covered stubs skipped, so re-runs make 0).
**Fixes found on the way:** EasyEDA dropped the trunk's 85 mil stub into J5.2 (shorter than its 2.5 mm width) → +5V
split in two; replaced by a 102 mil stub. U1B.1 (DevKit GND in the fill-free antenna strip) joined to a stitching
via by a 0.4 mm stub. One orphan stitching via deleted (`43_delete_orphan_via.js`) and one 23 × 82 mil bottom-fill
sliver beside U1B.11/12 excluded (`pour_fill.EXCLUDE`): EasyEDA called both disconnected. `route_check`'s hole-to-hole
test made slot-aware (J3's 7.8 mm slots were checked as round holes: 2 false hits).
**Result:** new copper 692 segments, 291 vias (60 routing incl. parallel power vias, 231 GND stitching); the user's
tracks unchanged (dump compared before every write). GND fills both layers (thermal reliefs; 1.0 mm spokes on J3 / J4
/ J5 GND): 1843 pieces, read back = plan, 0 overlaps, min gap 8.8 mil, every GND pad connected. `check_eda_copper`:
0 clearance / edge / hole / antenna / JLC hole-rule issues, every net one island. DRC rule cap 2.54 → 5.08 mm
(`28_drc_track_max.js`, the user's 3 mm power tracks were the 25 "Physical Error"s). **Strict DRC: passed, 0 errors**
(before: 25 physical + 230 unrouted). Backups after: `WRO_HUB_routed_20261027.eprj2` / `.epro2` (291 VIA records).
**Open, not touched:** 7 older silk hits (XT +/− marks, "SWITCH J3", the DevKit label, GND / J26), the XT hole sizes
(H-001: still library drills), the J3 footprint binding before any Import Changes. No fabrication approval.
