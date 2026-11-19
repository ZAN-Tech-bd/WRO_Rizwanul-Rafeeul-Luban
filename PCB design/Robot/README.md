# Robot Carrier PCB — Agent Handoff

> **RELEASE READY — 2026-10-28, 16:2x:** the full review finished and the upload set is
> [fabrication/20261028-final/](fabrication/20261028-final/README.md): Gerber zip
> `WRO_ROBOT_GERBER_20261028_FINAL.zip`, project `WRO_ROBOT_PROJECT_20261028_FINAL.epro2`,
> raw BOM, MFG-INSPECTION.json, SHA256SUMS. Final state = user's edited layout + verified D-031
> XT drills + D-037 +3V3 antenna reroute + orphan via removed + **4,612-piece regenerated GND**
> (saved export == plan; 0 overlaps/violations/floating; full connectivity; strict DRC **passed,
> 0 findings in every category**; silk audit 0 hits; actual CAM drills 4.0×2 / 2.5×6 / 3.2×4 mm,
> 156 via hits, one closed 92.329×98.425 mm outline). Hardware fit/RF/thermal gates remain open
> (see the release README). Order settings inside. Suggested JLC: 2-layer 1.6 mm FR-4 1 oz, HASL,
> tented vias, no assembly. The review-in-progress note below is history.
> **CURRENT REVIEW IN PROGRESS — 2026-10-28:** the user completed manual edits and requested a
> full fabrication review, fresh GND and Gerber. Preserve the user-edited backup
> [10:59 archive](WRO_Robot_user_edited_20261028-1059.epro2); do not replay historical builders.
> A fresh [13:24 diagnostic export](WRO_Robot_association_check_20261028-1324.epro2) compared all110
> documents: no DEVICE metadata, PCB record body, schematic record body or binding changed.
> Only the three XT footprint documents changed (six pads' sub-0.001mm rounding/default fields and
> three empty NET records); document-header differences were client IDs only. No evidence establishes
> that the user's possible association click caused these changes. Drill targets are still NOT fixed.
> The original54writer used missing configuration fields; it is disabled. The replacement is tested
> offline (4Python +7Node tests), handles one already-open project footprint and requires export proof.
> Cancel any **Associated Device** dialog; do not rebind devices to bypass it. See D-036.
> **XT FIX PERSISTED at13:51:** [verified project](WRO_Robot_xt_verified_20261028-1351.epro2).
> J1 drills4.0mm / pad5.4991mm (existing pad retained); J2/J3/J4 drills2.5mm / pads3.5mm.
> Each footprint was exported/diffed separately: only intended PAD dimensions plus its save-header
> timestamp/version changed. Live PCB1 also reads the corrected sizes with original pin nets/positions.
> No device association or ECO was applied. Offline current-geometry audit: no shorts, minimum copper
> gap0.1569mm, zero hole-gap violations, all non-GND nets connected. RF/power review remains open.
> **No new pour or verified manufacturing package exists yet.** The editing-state note below is history.

> **CURRENT EDITING STATE — 2026-10-28:** at the user's request, Robot PCB1's **1,922 GND fills**
> (325 top / 1,597 bottom) are removed and saved so the user can edit. The full native comparison
> across110projectdocuments proves all other design bodies unchanged:66components,374coppertracks,
>175vias, pads/nets, silkscreen, footprints and schematic preserved. Only fill records and331fill-only
>allocator bookkeeping records were removed. This is not a fabrication-ready state; unrouted GND is expected.
> Before: [full backup](WRO_Robot_before_gnd_removal_20261028-0248.epro2).
> Editing snapshot: [pour removed](WRO_Robot_editing_no_gnd_20261028-0250.epro2).
> Exact native/live polygon backups and proof: [GND edit record](../scripts/robot/data/gnd_removal_20261028/README.md).
> **Wait for the user's repour request.** Then regenerate against their new layout; do not replay old
> fill polygons or historical placement/routing scripts. Earlier routing/DRC notes below are history.

> Any AI agent (or person) can read THIS file and continue the robot-board work without chat history.

| Document | Role |
|---|---|
| [`../../CLAUDE.md`](../../CLAUDE.md) | Project rules for agents (auto-loaded) |
| [`WIRING-PLAN.md`](WIRING-PLAN.md) | **Spec v0.6 = source of truth**: power path §1, ports + harnesses §2, circuits §3, pin map §4, **net list §5**, BOM §6, layout §7, checks §8, bring-up §9, firmware §10 |
| [`DECISIONS.md`](DECISIONS.md) | Decision log D-001…D-031: why every choice was made, what was rejected |
| [`WIRING-PLAN-v0.4.md`](WIRING-PLAN-v0.4.md) / [`-v0.3`](WIRING-PLAN-v0.3.md) / [`-v0.2`](WIRING-PLAN-v0.2.md) / [`-v0.1`](WIRING-PLAN-v0.1.md) | History only. Don't build from them |
| [`../README.md`](../README.md) | Shared PCB handoff for all WRO boards (pipeline, bridge, hub + station status) |
| `C:\Users\tahfi\.claude\skills\pcb-pipeline\` | **The standard pipeline**: `PLAYBOOK.md` (phases, gates, conventions, API lessons), `SPEC-TEMPLATE.md`, `templates/` |
| `C:\Users\tahfi\.claude\plans\pasted-content-id-5437-task-curious-stardust.md` | Approved plan v0.5 (2026-10-25 part-count review); v0.4 plan: `how-the-mega-needs-vivid-sundae.md` |
| Reference project | `D:\projects\attendance_project\PCB\` (same pipeline, fully documented, ordered) |

**Last updated:** 2026-10-27 · **Board:** WRO Robot Carrier v0.6 (66 parts, 51 nets) · **95.0 × 97.5 mm** (D-019) · 2 layers · **100 %
through-hole, hand-soldered, bare PCB** · EasyEDA project **WRO_Robot** (`WRO_Robot.eprj2`, page `P1.Schematic1`).

## 0 · How to resume (any new session)
1. Read `../../CLAUDE.md`, this file, the spec and `DECISIONS.md`. Load the `pcb-pipeline` skill (`PLAYBOOK.md`).
2. `curl -s http://127.0.0.1:49620/health` → `edaConnected: true`, `edaWindowCount: 1` (else start the bridge: `cd /d
   D:\projects\easyeda-api-skill && npm run server`, then EasyEDA → API Gateway → Reconnect).
3. Read-only check of the open project (must be **WRO_Robot**) and that the EasyEDA window is **visible**:
   `node "D:\projects\WRO_Rizwanul-Rafeeul-Luban\tools\easyeda-bridge\eda.mjs" exec "const p = await eda.dmt_Project.getCurrentProjectInfo(); return JSON.stringify({p: p && (p.friendlyName || p.name), vis: document.visibilityState});"`
4. Continue at **§4 Next steps**. Re-running finished scripts is safe (idempotent) but unnecessary.

## 1 · Status
| Phase | State | Result |
|---|---|---|
| P0 intake | ✅ | firmware, Cirkit diagram, LM386 photo, proven DFPlayer build, S3 / LM386 / DFPlayer / XT datasheets |
| P1 plan v0.4 | ✅ 2026-10-16 | approved (plan file above); D-001…D-017 |
| P2 spec v0.4 | ✅ 2026-10-16 | `WIRING-PLAN.md`: 75 parts, 53 nets (net map evaluated offline: 200 wired pins, 0 single-pin nets) |
| P3 parts + THT gate | ✅ 2026-10-16 | `07_parts_verify_v04.js`: 32 part types, **0 SMD**, 0 missing (PASS); polarity facts D-015…D-017 |
| P4 schematic | ✅ 2026-10-16 | 75 parts in 7 blocks on A4, 200 stubs, 32 No-Connect, 53 nets; `15_check` 0 shorts / bare / stacked / single-pin, **ERC true** (panel 0 / 0 / 0, 4 Info = U1A/U1B/U2A/U2B designator style, intentional); `sch_audit` **0 differences** |
| P4 review + import | ✅ 2026-10-16 | user imported the schematic into `PCB1`; `20_pcb_inventory`: 75 footprints, 232 pads, 200 pad nets = schematic, 0 mismatches |
| P5 floor plan | ✅ 2026-10-16 | user approved (D-019 size, D-020 native USB, D-021 no zip ties); `layout_plan.py` 0 problems |
| P5 placement + silk | ✅ 2026-10-16 | `21_place_pcb`: outline 3740 × 3840, H1–H4, **75/75 placed first run** (0 rotation fixes); `22`: 0 overlaps / edge / holes / zones / height, 42 polarity nets OK, DevKit rows 25.40 mm, DF rows 18.00 mm; silkscreen 79 labels + 14 lines + 75 designators, `24` 0 hits; `27_drc`: only 200 unrouted (53 nets) |
| P5 placement review | 🟡 open (user) | v0.4 placement unchanged except the v0.5 removals and J15 (top-right corner) |
| **Rev v0.5 (part-count review)** | ✅ 2026-10-25 → 🟡 **user review** | spec v0.5 + D-022…D-028; schematic (66 parts, 189 stubs, 51 nets, ERC true, `sch_audit` 0 differences); PCB import read back (66 fp), J15 placed, outline + silk recreated as polylines (EasyEDA 3.2.206), `22` 0 problems, `24` 0 hits, `27_drc` only unrouted (189 / 51 nets) |
| **Rev v0.6 (raw battery motor rail D-029, vertical XT D-030)** | ✅ 2026-10-27 | J1–J4 placed in the right column (`22` 0 problems); silk v0.6 (J1 +/−, J2 "MOTOR POWER", title) `24` 0 hits |
| **P5 routing + GND** | ✅ 2026-10-27 → 🟡 **user visual review** | D-032: PathFinder 0 conflicts; 394 segments, 175 vias (153 GND stitching); GND fills both layers 1922 pieces read back identical; `check_eda_copper` clean; strict DRC **0 errors** (D-033 track max 5.08 mm) |
| XT drill holes (D-031) | 🟡 **user GUI step** | D-034: canvas-only so far; saved file has library drills → `45_xt_fp_editor.js` in each XT part's footprint editor |
| P6 production files | ⬜ | bare PCB only (no assembly files) |
| P7 hand-solder + bring-up | ⬜ | spec §9 |
| P8 firmware | ⬜ separate task | spec §10 |

## 2 · Pipeline and runbook (this board)
```
AI agent ──► node D:\projects\WRO_Rizwanul-Rafeeul-Luban\tools\easyeda-bridge\eda.mjs exec -f <script.js>
         ──► bridge server D:\projects\easyeda-api-skill (npm run server, 127.0.0.1:49620)
         ──► Run API Gateway extension inside EasyEDA Pro (live eda.* API)
```
Scripts live in `PCB design/scripts/robot/` (outputs in `data/`). Order for the schematic:
1. `10_resolve.js` → (device ids pasted into `11_place.js`) → `11_place.js` until `remaining: 0` → `12_dump.js > data/sch_dump_v04.txt`
2. `python sch_pack.py data/sch_dump_v04.txt` → set `COLS` in `13_layout.js` → `13_layout.js` until `remaining: 0`
3. `14_wire.js` in lockstep until `remaining: 0` (check `polarity: []`, `unmapped: []`)
4. `15_check.js > data/sch_check_v04.json` → `python sch_audit.py data/sch_check_v04.json` (0 differences) → `15b_erc_panel.js`
5. User review, then **Design → Import Changes from Schematic → Apply** on the PCB (GUI).

## 3 · Scripts
| Script | Writes? | What it does |
|---|---|---|
| `01…06_*.js` | no | v0.1–v0.3 LCSC searches (history) |
| `07_parts_verify_v04.js` | no | **v0.4 BOM check + through-hole gate** (fails on any SMD footprint) |
| `08_parts_find_v04.js`, `08b_parts_find_v04.js` | no | gap searches, THT footprints only (XT30 female, header, tact, electrolytics, LED) |
| `09_symbol_fp_files.js` + `read_symbols.py` / `read_elibz.py` | no | library symbol + footprint files of the polarity parts → pin names, pad positions, silk "+" (D-015…D-017) |
| `10_resolve.js` | no | LCSC → library device uuids |
| `11_place.js` | **yes** | places the 75 parts with designators on a staging grid outside A4; TP1–TP6 / J14 `AddIntoBom = false` |
| `12_dump.js` | no | per-part bbox + pins (number, name, offset, rotation) |
| `sch_pack.py` | offline | tries column splits / block orders for A4 from the dump |
| `13_layout.js` | **yes** | packs the parts into 7 labelled blocks (A power · B DevKit · C motor · D RFID · E audio · F CAM + Mega · G inputs) |
| `14_wire.js` | **yes** | spec §5 net map → named 20-unit stubs, 60 per run, lockstep, polarity-name guard |
| `15_check.js` | **yes** (NC flags, save) | No-Connect flags, union-find shorts, bare / stacked / single-pin checks, net list, ERC |
| `15b_erc_panel.js` | no | detailed ERC lines from EasyEDA's DRC panel |
| `sch_audit.py` | offline | `15_check` net list vs the `14_wire` map, pin by pin |
| `20_pcb_inventory.js` | no | PCB footprints + pad nets + pad offsets → `data/pcb_inventory_import.json` (→ `data/pcb_inventory_v04_import.txt`) |
| `layout_plan.py` | offline | **floor plan = source of truth for placement**: checks (overlap, edge, holes, antenna strip, METAL 15 mm, USB lane, height classes under the modules, LM386 foam spots), PNG preview + ratsnest, `--js` writes `21_place_pcb.js` |
| `21_place_pcb.template.js` → `21_place_pcb.js` | **yes** | outline, M3 holes, placement with pad read-back (generated by `layout_plan.py --js`; idempotent) |
| `22_check_placement.js` | no | in-EDA check with real bboxes: overlaps, edge, holes, antenna / METAL 15 / USB lane / height zones, socket geometry, 42 polarity + supply pad nets |
| `20c_pad_geometry.js` | no (1 temp text off-board) | real pad sizes + font metrics → `data/pad_geometry.json` (+ `20_pcb_inventory` → `data/pcb_inventory_placed.json`) |
| `silk_plan.py` | offline | labels, outlines, designators (auto-placed) checked against pads +7, holes, texts, edge → `23_silkscreen.js` (incremental, time-boxed) + `25_designators.js` |
| `23_silkscreen.js` / `25_designators.js` | **yes** | free layer-3 texts/lines (keep matching, delete others, add missing) / designator attributes |
| `24_check_silk.js` | no | EasyEDA's own text boxes vs pads +6, holes, texts, edge |
| `27_drc.js` | no | strict DRC, verbose, per category (before routing only "Connection Error") |
| `16_relabel_v05.js` | **yes** | v0.5: J9.1 MEGA_5V → +5V, D2.1 AMP_R → VAMP_BAT (idempotent, read-back) |
| `17b_remove_v05.js` | **yes** | v0.5: removes C1, C2, D3, R1, TP1–TP6 with their stubs (idempotent) |
| `18_place_j15_v05.js` | **yes** | v0.5: creates J15, puts it into free space in / under block G and grows the G frame; never re-pack the wired sheet with `13_layout` |
| `28_drc_track_max.js` | **yes** (DRC rules) | D-033: raises the 1 oz max track width 2.54 → 5.08 mm (3 mm battery tracks) |
| `30_fp_library.js` → `fp_geometry.py` → `route_model.py` | no / offline | footprint files → exact pads per `PLAN`, cross-checked with EasyEDA (213 pads, 0 problems); `PAD_SHAPE` / `PAD_HOLE` = D-031 XT pads |
| `router.py` / `pathfinder.py` / `trim.py` / `gnd_stage.py` / `route_check.py` | offline | robot net classes (D-032), antenna escape rule, PathFinder → `data/routes.json`, stitching (7.6 mm grid), exact check |
| `gen_route_js.py` → `40_route.js` | **yes** | tracks + vias per net (idempotent; re-run = 51 skipped) |
| `29_dump_copper.js` → `check_eda_copper.py` | no / offline | copper actually in PCB1 → clearances, holes, connectivity |
| `pour_fill.py` → `42_fill.js` → `29b_dump_fills.js` → `check_fills.py --plan` | offline / **yes** / no | static GND fills both layers (XT GND spokes 1.0 mm), read-back gate |
| `44_xt_pads.js` | **yes** (canvas only) | D-031 pads on the canvas; **does not persist** (D-034) |
| `45_xt_fp_editor.js` | **yes** (open footprint editor) | D-034: persistent XT pads / holes; user opens Edit Footprint, runs it, saves |
| `51_export_project.js` | no | saved-state project export (persistence evidence) |
| `../backup_project.py` | offline | `python backup_project.py robot\|hub <tag>`: consistent SQLite snapshot of the open `.eprj2` + integrity check |

## 4 · Next steps
**Now (2026-10-27): routed, DRC 0 errors — waiting for the user.**
1. **User — visual routing review** in EasyEDA (`WRO_Robot` → `PCB1`; previews `scripts/robot/data/route_top_labeled.png`,
   `route_bot_labeled.png`, `fill_preview.png`). Static GND fills don't follow edits: after any copper change run the copper
   loop (`29` → `check_eda_copper.py` → `pour_fill.py` → `42_fill.js` ×2 → `29b` → `check_fills.py --plan` → `27_drc`).
2. **User + agent — XT drill holes (D-034):** for each of J1, J2, J3, J4: PCB1 → right-click → Edit Footprint → agent runs
   `45_xt_fp_editor.js` → user saves the tab. Then `51_export_project.js` and confirm 4.0 / 2.5 mm drills in the saved file.
3. **P6:** exports (`50…52`), `fab_files.py`, the independent review of the upload set, `ORDERING.md` (bare PCB only).

Older steps (kept for history):
0. **Rev v0.5 (2026-10-25): spec, schematic and placement done.**
   - User: review PCB1 in EasyEDA. The top-right corner is new (J15, J10 pin names, "I2C 3.3V ONLY" / "OBST 2 3.3V"
     upright); R14 / R15 moved 55 mil down; "UNPLUG J9 FOR MEGA USB" in D3's old spot; the outline is recreated
     (EasyEDA 3.2.206, D-028). Previews: `scripts/robot/data/layout_preview.png`, `silk_preview.png`.
   - Then the hub's v0.4 edits (switch EasyEDA to WRO_HUB: `scripts/hub/14_remove_v04.js`, same polyline fixes for
     its outline / silk).
1. **User — placement review in EasyEDA** (`WRO_Robot` → `PCB1`; previews `scripts/robot/data/layout_preview.png`,
   `silk_preview.png`). Check: each port where you want its cable; the left USB-C port reachable with the DevKit seated;
   the LM386 modules' far ends on the FOAM marks; JP1 reachable; labels readable. Print 1:1 and lay the real modules,
   XT plugs and XH housings on it if you can (spec §8.5). Tell me anything to move.
2. **Agent — routing (pcb-pipeline P5 step 9):** `30_fp_library` → `fp_geometry.py` / `route_model.py` (cross-check every
   pad centre) → `router.py` rules (BAT_SW ≥ 3 mm in its corridor, +5V spine ≥ 2 mm, VAMP ≥ 1 mm, signals 0.3–0.4 mm,
   antenna strip: no pour / vias / power) → `pathfinder.py` → `trim` / `gnd_stage` / `route_check` → `40_route.js` →
   GND fills (`pour_fill.py` → `42_fill.js` → `check_fills.py`) → `27_drc` 0 errors → user routing review.

## 5 · Lessons from this board
- LCSC keyword search returned **C431092 "XT30PW-M30.G.Y" (a male)** for an XT30 female query: always read the MPN and the
  footprint name back (`07` does). The female PCB socket is C2913282.
- Library parts can have pins named only "1…4" even for polarised connectors (XT30PW-F): derive polarity from the maker's
  drawing + mating geometry, record it as a decision, and add a meter check to bring-up (D-015).
- 4-pin tact switches with unknown internal pairing: wire the **diagonal** pads (D-016).
- With the EasyEDA window hidden, each library-file download took ~4 s: time-box and batch (`09` ran in three batches).
- Placing library parts takes ≈ 2.5 s each (7–10 per 18 s run): 75 parts = 10 runs of `11_place`.
- The `15_check` run that **sets** the No-Connect flags reported `erc: false`; the panel (`15b`) showed 0 / 0 / 0 and a second
  `15_check` returned `true`. Re-run `15_check` once after the NC flags are new before trusting the ERC result.
- A tact symbol's pin sides don't tell the internal pairs: the K2-1102DP symbol has 1/3 left and 2/4 right, but its bars
  join 1–2 and 3–4 (read the `.esym` POLY records).
- Raised modules: clearance = socket 8.5 mm + male-header spacer 2.5 mm ≈ 11 mm. Classifying parts by height (≤ 3 mm under
  the LM386 modules that rest on foam, ≤ 5.5 mm under the DevKit / DFPlayer, test points / jumpers / switches never under)
  let 23 resistors and caps (R1 partly) sit under the modules and made 95 × 97.5 mm possible.
- `24_check_silk` reported a designator on J1's mounting pad: the pad sits at −90° and JS `%` keeps the sign
  (`-90 % 180 = -90`), so the check never swapped the oval's w/h. Normalise angles first (fixed here and in the template).

## 6 · Session log
- **2026-10-27 (placement fix + routing + GND fills, D-032…D-034)**
  - User: route the robot PCB with ground pour and vias; place the misplaced parts. Backup `WRO_Robot_v0.6_pre_route.eprj2`.
  - Inventory: only J1–J4 off-board. `layout_plan.py` on the new inventory (`data/pcb_inventory_v06_now.txt`): the v0.5
    column spots fit the vertical XT with 0 problems → `21`: 4 placed, 62 already OK → `22` 0 problems.
  - `44_xt_pads`: the 8 pads had reverted (import) → set again; the saved export still shows library drills (D-034).
  - Silk v0.6: `20c` + inventory re-captured, `silk_plan.py` 0 problems → `23` (6 new / 6 removed) → `25` (4 moved) → `24` 0 hits.
  - Routing: templates copied, `30_fp_library` (28 footprints in 3 runs), `route_model` 213 pads 0 problems → PathFinder
    7 passes 0 conflicts → `trim` 0 → `gnd_stage` 153 stitching vias → `route_check` clean → `40_route` (568 objects;
    re-run 51 skipped) → `29` + `check_eda_copper` clean. U1B.1 had no fill (antenna strip): 0.4 mm stub to a stitching via.
  - Fills: `pour_fill` 1922 pieces, all GND pads connected → `42_fill` (the window dropped once mid-run, reconnected;
    re-runs converged, final run 1922 kept / 0 created) → `29b` + `check_fills --plan` 1922 identical, 0 overlaps, min 9.8 mil.
  - `27_drc`: 9 Physical Errors = 3 mm tracks over the 2.54 mm rule cap → `28_drc_track_max` (D-033) → **DRC passed, 0 errors**.
  - Saved + backups `WRO_Robot_v0.6_routed.eprj2` and `WRO_Robot_v0.6_routed_export.epro2` (175 VIA, 2132 FILL records).
- **2026-10-26 (vertical XT swap D-030 + drill holes D-031)**
  - User: XT60 hole 4.0 mm, XT30 hole 2.5 mm; vertical connectors on robot and hub. Spec v0.6 + D-031 written first.
  - Backup `WRO_Robot_v0.6_pre_xt_swap.eprj2` (integrity ok). `17_swap_v06`: J1 → XT60UPB-M, J2/J3 → XT30UPB-F, J4 →
    XT30UPB-M (8 stubs removed) → `14_wire` 8 made, 189 committed, `polarity: []` → `15_check` ×2: 51 nets, 189 pins,
    24 NC, 0 shorts / bare / stacked, ERC true → `sch_audit` 0 differences.
  - New `44_xt_pads.js` (J1 4.0 / 5.5 mm, J2–J4 2.5 / 3.5 mm). User legs: XT60 3.7 / 3.8 mm, XT30 2.2 / 2.1 mm.
  - User imported into PCB1: J1–J4 arrived off-board (66 parts). **User places J1–J4 by hand** (no `layout_plan` re-place).
    `44_xt_pads`: 8 pads changed, read back J1 4.00 / 5.50 mm, J2–J4 2.50 / 3.50 mm; re-run 8 skipped (saved).
    **Next:** user places J1–J4 → `20_pcb_inventory` → `layout_plan.py` sync →
    silk `20c` / `silk_plan` / `23` / `25` / `24` → `27_drc`.
  - Hub prepared offline: spec v0.4 rows (J1/J2 XT60UPB-M, J4 XT60UPB-F C9900007500 1 = +, J5 XT30UPB-M), `09_wire_nets`
    J4 1/2, `06_resolve` ids, new `hub/16_xt_vertical_files.js`, `hub/17_swap_xt.js`, `hub/44_xt_pads.js`.
- **2026-10-25 (rev v0.6: motor rail straight from the battery, D-029)**
  - User: **no motor buck** — the L298N 12V/GND screws get the raw switched battery on the existing J2 XT30 lead; the
    motors see pack − L298N bridge drop ≈ 7–10.5 V. Spec v0.5 archived as `WIRING-PLAN-v0.5.md` (banner); v0.6 written:
    power path §1, budget §1.3, §0 table, port/harness tables (J2 = "L298N power feed"), bring-up §9.2, §10 firmware
    recalibration note; D-029 logged.
  - **Board-identical** (J2 already carried `BAT_SW`/GND — 51 nets / 66 parts / placement untouched). **Pending before
    ordering:** regenerate two silk texts (J2 label → "L298N RAW BAT (OUT=STOP)", title → v0.6) via `silk_plan.py` →
    `23`/`25`/`24`, then `27_drc`.
- **2026-10-25 (rev v0.5: part-count review → spec, decisions, schematic)**
  - Review verdicts approved by the user, with two changes: both FC-51s plug into the PCB (new J15, wired-OR); hub
    Q1 = 2N2222A. Spec v0.5 (v0.4 archived with a banner) and D-022…D-027 written first.
  - Builders updated so a re-run can't undo v0.5: `11_place`, `13_layout` (with a "never re-run on the wired sheet"
    note), `14_wire`, `sch_pack.py`. The `14_wire` map evaluates to 51 nets / 189 pins / 0 single-pin nets.
  - Backups: `WRO_Robot_v0.4_pre_v0.5.eprj2` (before) and `WRO_Robot_v0.5_sch.eprj2` (after), both integrity ok, via
    the new `scripts/backup_project.py`.
  - Schematic writes, all gated (WRO_Robot, 1 window, visible):
    - `17b_remove_v05`: C1, C2, D3, R1, TP1–TP6 removed with their 14 stubs → 65 parts, 186 wires.
    - `16_relabel_v05`: D2.1 AMP_R → VAMP_BAT and J9.1 MEGA_5V → +5V, read back.
    - `18_place_j15_v05`: block G was packed full (largest gap 38 units vs J15's 75), so J15 went to (480, 180),
      straight below J11 in the strip under block G. The G frame grew from 280.5 to 351.5 units and its title now names
      both obstacle ports.
    - `14_wire`: 3 new stubs, 189 committed, remaining 0.
  - Gate: `15_check` ×2 → 0 bare / shorts / stacked / single-pin, 51 nets, ERC **true**; `15b` panel 0 / 0 / 0
    (4 Info: U1A/U1B/U2A/U2B designator style); `sch_audit.py` **0 differences** (51 / 51 nets, 189 / 189 pins).
  - User imported into PCB1. `20_pcb_inventory` → `data/pcb_inventory_v05_import.txt`: 66 footprints; the 10 removed
    parts gone, J15 arrived off-board, J9.1 = +5V, D2.1 = VAMP_BAT; every other pad net and position unchanged.
  - **EasyEDA had auto-updated to 3.2.206:** `pcb_PrimitiveLine` creates copper only (layers 3 / 11 throw), and the board
    outline + 14 silk lines were gone (0 lines; texts and parts intact). Off-board probes: polylines on layers 3 / 11
    work. `21` template (outline = one closed polyline), `22`, `24` and `silk_plan.py` switched to polylines (also in the
    pcb-pipeline templates; playbook §7 lesson). D-028.
  - `layout_plan.py` v0.5 (10 parts out, J15 at (3225, H − 105), R14 / R15 55 mil down): 0 problems → `21`: outline
    created, J15 + R14 / R15 placed, re-run 66 alreadyOk → `22`: 0 problems, polarity nets incl. J9.1 / J15 OK, DevKit rows
    25.40 mm, DF rows 18.001 mm.
  - Silk: `20c` + `20_pcb_inventory` re-captured (221 pads), `silk_plan.py` 0 problems (81 labels, 15 polylines, 66
    designators) → `23` (12 texts + 15 lines new, 10 stale removed; re-run 96 kept) → `25` (7 moved) → `24` 0 hits.
    `27_drc`: Connection Error only (189, 51 nets). Backup `WRO_Robot_v0.5_pcb.eprj2`.
  - **Silkscreen v0.5b (user: bigger, plain words):** every port says what plugs in (e.g. "BATTERY 3S LIPO", "TO 5V BUCK
    IN", "L298N MOTORS: REMOVE ENA ENB 5V JUMPERS", "MEGA DISPLAY - UNPLUG J9 FOR MEGA USB"), module areas say what goes
    there ("ESP32-S3 BOARD / PLUG IN HERE", "RIGHT / LEFT SPEAKER AMP, LM386 PINS DOWN", "DFPLAYER MP3 + SD CARD"), and
    all 40 resistors / capacitors / diodes carry their value inside their outline. Names 0.8–1.1 mm, pins 0.7–0.8 mm.
    `silk_plan.py` 0 problems (124 labels, 15 lines) → `23` (116 new, 73 replaced; re-run 139 kept) → `25` (6 moved) →
    `24` 190 texts, 0 hits; `27_drc` unchanged (Connection Error only).
- **2026-10-16 (placement + silkscreen in EasyEDA — waiting for the user's placement review)**
  - User: left USB-C = native USB/OTG, upload over it is fine (D-020); drop zip-tie holes; OK to lift the DFPlayer for its
    SD card (D-021); proceed. Spec §7 / header / §9 / §10 and D-019…D-021 written first.
  - `21_place_pcb`: outline 3740 × 3840, H1–H4, 75/75 placed on the first run, 0 rotation fixes; re-run 75 alreadyOk.
    C3 then nudged 40 mil up (room for J9's pin legend) → 1 part moved. Post-placement inventory = plan for all 75.
  - `22_check_placement`: 0 overlaps / off-board / near holes / antenna / METAL 15 / USB lane / under-module / foam; 42
    polarity + supply pad nets correct; DevKit rows 25.400 mm, pin 1 1.905 mm below the top edge; DF rows 18.001 mm, 1.016 mm
    from the DevKit; LM386 VCC on the right; XT fronts 0.3 / 0.3 / 0.3 / −0.1 mil past the edge.
  - Silkscreen: `20c` (232 pads, font metrics) → `silk_plan.py` 0 problems → `23` 79 texts + 14 lines (re-run kept 93) →
    `25` 75 designators → `24` 0 hits after fixing its −90° pad bug. `27_drc`: only Connection Error 200 (53 nets).
- **2026-10-16 (PCB import read back, floor plan drafted — awaiting review)**
  - Bridge restarted after a PC reboot (`npm run server`, EasyEDA → API Gateway → Reconnect). `20_pcb_inventory`: 75
    footprints, 232 pads, 200 with nets, 0 mismatches against the schematic (footprint names read "?" in this build).
  - `layout_plan.py` (pcb-pipeline template + robot rules): height classes FLAT ≤ 3 mm (0.25 W resistors, DO-41) under
    the LM386 modules, LOW ≤ 5.5 mm (+ R1, FG28 ceramics) under the DevKit / DFPlayer and in the 17 mm USB lane; test
    points, J14, JP1 and the switches never under a module; METAL 15 mm from the antenna includes JP1.
  - 95 × 90 mm did not fit (top-edge ports + button cluster); 95 × 95 left C3 / LEDs homeless; **95.0 × 97.5 mm** fits
    with 0 problems. Groups: left edge J6 / J7 / J14; top edge J11 / J10 (3.3 V ports only); bottom-left buttons + Mega
    + C3; bottom-right CAM + LEDs + TPs; VAMP block right of the DFPlayer with a clear 3 mm BAT_SW corridor beside J2/J3;
    +5V spine under both LM386 modules; resistors under the DevKit / DFPlayer / LM386 modules.
- **2026-10-16 (schematic drawn, gate passed)**
  - Gates: WRO_Robot, 1 window, window visible. `11_place` 10 runs → 75/75, 0 errors. `12_dump` → `data/sch_dump_v04.txt`
    (pins confirmed: D 1 = K / 2 = A, LED 1 = + / 2 = −, J1/J4 1 = − / 2 = +, all headers 1…n).
  - Tact symbol re-read: bars join 1–2 and 3–4 → diagonal 1/3 wiring confirmed (D-018).
  - `sch_pack.py`: A4 fits, split 320/735 (columns B C D | E G | A F, slack 211) → `13_layout` 1 run: 75 moved, 7 frames.
  - `14_wire` 4 lockstep runs: 60 + 60 + 60 + 20 = 200 stubs, committed 200, `polarity: []`, `unmapped: []`, 0 errors.
  - `15_check`: 32 NC, 200 pins wired, 0 bare / shorts / stacked / single-pin, 53 nets; ERC false on the NC-setting run →
    `15b` panel 0 fatal / 0 error / 0 warn (4 Info: U1A/U1B/U2A/U2B designator style) → re-run ERC **true**, saved.
    `sch_audit.py`: map 53 / sheet 53 nets, 200 / 200 pins, **0 differences**. → waiting for the user's review + import.
- **2026-10-16 (plan v0.4 → spec, parts, scripts)**
  - Plan v0.4 approved after the user's answers (≈ 11 V LM386 rail kept + JP1 fallback, LM386 re-headered pins-down, Mega
    XH-4 + I²C 1 kΩ protection, XT30 buck feeds, 100 % THT, follow the attendance process).
  - Spec v0.4, `DECISIONS.md` D-001…D-017, this handoff and the root `CLAUDE.md` written; v0.3 archived.
  - Parts: `07` THT gate PASS (32 types, 0 SMD); polarity from library files (`09`); XT30PW-F polarity derived (D-015).
  - Scripts 10–15 + `sch_pack.py` / `sch_audit.py` / `15b` prepared from the pcb-pipeline templates and the attendance
    live versions (API-yield sleep, lockstep wiring, project + visibility gates). `10_resolve`: 32/32 devices.
  - Net map evaluated offline: 75 parts, 53 nets, 200 wired pins, 0 single-pin nets = spec §5.
