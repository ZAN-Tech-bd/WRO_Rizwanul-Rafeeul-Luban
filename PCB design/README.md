# 🛠️ Museum Hub Carrier Board — PCB Agent Handoff

> **CURRENT — 2026-10-28, Robot AND Station released:** after the Robot release below, the
> Station's two silk findings were closed (J1 designator relocated, identifier corrected to
> "Station PCB V0.4"), copper verified untouched (59 tracks / 1 via / 209 fills), strict DRC
> passed, and the CAM package was exported and audited: upload set
> [station/fabrication/20261028-final/](station/fabrication/20261028-final/README.md)
> (`STATION_GERBER_20261028_FINAL.zip`). Hardware gates (1:1 print fit, BC557 C/B/E, buzzer
> polarity, bring-up) remain with the user. All three boards now have current, audited upload
> sets; none has been ordered.
> **CURRENT — 2026-10-28, Robot released:** the Robot review completed after the user's manual
> edits: D-031 XT drills persisted (4.0 / 2.5 mm), the +3V3 run rerouted out of the module's
> actual on-board antenna overhang, 4,612-piece GND regenerated and verified in the saved export,
> strict DRC passed with 0 findings, silk audit 0 hits, actual CAM package inspected. Upload set:
> [Robot/fabrication/20261028-final/](Robot/fabrication/20261028-final/README.md). Hardware
> fit/RF/thermal gates remain open there. Hub release below is unchanged.
> **CURRENT — 2026-10-28, J3 synchronization error FIXED:** the actual ECO contained only25 stale
> J3 descriptive properties. After a full image-inclusive backup and review, the user applied only
> those properties with track-net updates unchecked. **Fresh strict DRC[]; ERCtrue/detailed[].**
> Native comparison changed exactly one J3 metadata record; all physical and image data unchanged.
> See [confirmed cause and fix](hub/SYNC-FIX-2026-10-28.md) and
> [refreshed standard Gerber package](hub/fabrication/20261028-sync-fixed/README.md).
> **Separate warning:** the raster image is preserved in the editor project but omitted by ordinary
> monochrome Gerber export; do not order expecting it to print without a deliberate conversion or
> verified color-silkscreen workflow. The older synchronization hold below is resolved, not current.

> **CURRENT — 2026-10-28, after the user's final edits:** GND is restored from the edited geometry:
> **2,331 fills (682 top / 1,649 bottom)**. The user moved R5 to **(1605,1305), rotation90°** and
> rerouted SERVO_IN/SD_SCK/+5V; those edits were preserved. Fresh ERC passes; physical DRC has zero
> findings, but **one Netlist Error / Import Changes synchronization finding remains**. Current
> schematic and PCB agree on all256 named pin memberships; that does not establish the exact ECO cause.
> Use the [new release review](hub/RELEASE-2026-10-28.md) and
> [dated Gerber package](hub/fabrication/20261028-after-user-edits/README.md), not any earlier order files.
> Switch fit/contact action, Q1 pinout and full-load thermal qualification remain hardware gates.
> Reusable lessons: [PCB-REVIEW-LESSONS.md](PCB-REVIEW-LESSONS.md).

## Historical checkpoints below — superseded by the current review

> **SUPERSEDED ORDER FILES — 2026-10-27:** R5 moved out of the microSD module strip
> (1890,932.5 → 1890,1335, same orientation; old SERVO_IN stub deleted; pad re-taps the net track; GND
> regenerated, 2,362 fills, 0 violations). Upload for the bare-PCB order:
> [`exports/WRO_HUB_ORDER_GERBER_20261027.zip`](../exports/WRO_HUB_ORDER_GERBER_20261027.zip) +
> [`exports/WRO_HUB_ORDER_BOM_20261027.csv`](../exports/WRO_HUB_ORDER_BOM_20261027.csv).
> Final release: [`hub/WRO_HUB_REPAIRED_r5_ground_20261027-2255.epro2`](hub/WRO_HUB_REPAIRED_r5_ground_20261027-2255.epro2).
> Correction: EasyEDA reports one synchronization finding. The earlier attribution solely to J3's
> footprint override was not established. Never apply an ECO that restores J3's old VH-6 geometry.
> This checkpoint and its files have been superseded by the current edited-board review.

> **BLOCKERS FIXED IN ISOLATED REPAIR PROJECT — 2026-10-27 (H-007):** after the `lib_Device.modify`
> incident corrupted all 28 DEVICE metadata dictionaries in the original `WRO_HUB`, all work moved to the
> verified isolated copy **`WRO_HUB_REPAIR_ISOLATED_20261027`** (imported via API; project UUID
> `ea32443f…730e1f`, PCB `8efe5b895e7646ea`). Release checkpoint:
> [`hub/WRO_HUB_REPAIRED_catalogue_cleared_20261027-2040.epro2`](hub/WRO_HUB_REPAIRED_catalogue_cleared_20261027-2040.epro2).
> Done and export-verified there: Q1 ring ≈0.1825 mm (absolute JLC min met), J1/J2/J4 4.0 mm and J5 2.5/3.5 mm
> drills; J3 = KCD4-203-B instance override with all B6P-VH supplier/datasheet fields cleared (fresh ERC true,
> no warnings); 187/187 silk ≥40 mil/6 mil with 11 small label relocations; the two +3V3 segments rerouted out
> of the antenna projection (0.8 mm width and endpoints kept); GND regenerated (2,364 fills, 12/40/50 mil
> clearances, 0 violations, 0 disconnected, strict DRC 0). Full change allowlist vs the pre-fix checkpoint:
> only silk, 3 RF lines, fills and J3 metadata — nothing else. The original `WRO_HUB` project and
> `WRO_HUB_FIXED_20261027.epro2` / `finalized/` are **diagnostic only, do not use** (failed import).
> Still open: final live Gerber/BOM capture after one API-Gateway reconnect, plus hardware gates (real-switch
> slot fit on a 1:1 print; ≈8 A drop/thermal). [Ledger](scripts/hub/data/blocker_fix_20261027/RECOVERY-NOTES.md).

> **SUPERSEDED — FINAL LAYOUT REVIEWED + GND REGENERATED — 2026-10-27 (H-006):** from the user's finished layout,
> fresh GND fills were **regenerated from live geometry** (never restored): 2,360 fill pieces (701 top /
> 1,659 bottom), 12 mil normal clearance, **40 mil (1.0 mm) in the left power area**, **50 mil (1.27 mm)
> to BAT+/BAT−/DC+/DC−/V12**; thermal-relief spokes on THT GND pads; antenna keep-out clear. Three
> unused GND stitching vias relocated into connected plane; **J5 reversed polarity legend corrected**
> ("+ 5V FROM BUCK −"). **Strict DRC now 0 errors** (was 266 GND connection errors); schematic ERC true;
> all components/tracks/pads/nets/footprints/outline byte-preserved vs the pre-write archive
> (`WRO_HUB_before_ground_write_20261027-1647.epro2` → `WRO_HUB_after_ground_20261027-1649.epro2`).
> Gerber outline verified single intended rectangle. **NOT yet orderable — open blockers: Q1 pad annular
> ring 0.15 mm < JLC 0.18 mm abs. min; J5 holes 1.8 mm vs 2.1/2.2 mm XT30 pins; J1/J2/J4 holes 4.6/4.7 mm
> vs approved 4.0 mm; silk text below JLC 40 mil height / 0.15 mm stroke guideline; J3 switch physical
> fit unverified; +3V3 routes enter antenna strip.** [Full report](hub/FINAL-REVIEW-2026-10-27.md).

> **PREVIOUS EDITING STATE — 2026-10-27:** per user request, the hub's 1843 GND fill polygons
> (463 top / 1380 bottom) are removed and the project is saved so the user can edit. All 676 tracks,
> 274 pads, 77 components, nets and other design records are unchanged in the verified full export.
> **Do not restore the old polygons automatically after edits:** regenerate/repair GND against the new
> geometry. Original full backup: `hub/WRO_HUB_before_gnd_fill_removal_20261027-1208.epro2`;
> final editing snapshot: `hub/WRO_HUB_without_gnd_fills_final_20261027-1221.epro2`.
> Exact polygon backup and tested recovery tooling: `scripts/hub/data/gnd_fill_backup_20261027/`.

> **Purpose**: any AI agent (Zoo Code, Claude Code, Cursor, ...) can read THIS file and
> immediately continue the PCB work — no chat history needed. Everything about the
> pipeline, the board spec, the process, and the state is tracked here.

**Last updated**: 2026-10-27 (hub **routed**, H-005; robot routed, D-032)

> **Hub routed (H-005, 2026-10-27):** read-only backup first (`hub/WRO_HUB_pre_route_20261027.eprj2` / `.epro2`). R5,
> R7–R10 placed (were off-board); the rest routed around the user's 62 fixed tracks (`scripts/hub/route/`, add-only
> writer): 692 segments, 291 vias, GND fills both layers, strict DRC **0 errors**. Open: older silk hits, XT drill sizes,
> J3 binding before Import Changes. [H-005](hub/DECISIONS.md).

> **Latest hub follow-up (H-004, 2026-10-27):** R4/R6 are now near U2's current input/output pins and R13 is beside
> the MCU E-stop input. The eight station names, 64 pin labels and board title are back on the board; resistor
> designators are horizontal beside their parts. Actual readback: zero scoped pad/edge/text-overlap hits.
> Full export proves the other 74 components, all 62 existing copper segments, nets, outline, holes, unrelated
> silk and all other project documents unchanged. DRC unchanged (25 physical, 230 unconnected); 28 model tests pass.
> This supersedes the H-003 note about leaving station silk/R4/R6/R13 off-board. [H-004 report](hub/STATION-CLEANUP-2026-10-27.md).

> **Latest hub placement (2026-10-27, H-003):** user subsequently hand-placed/routed the rest of the board. Only the
> 37 off-board station parts were placed: S1–S4 across the upper row, S5–S8 back across the lower row, with short
> LED hops and MQ filters near ADC pins. All other 40 components, existing copper, free text, nets, holes, outline
> and schematic are unchanged vs the fresh guarded backup. DRC has **no new findings** (25 existing physical,
> 234 unconnected). Free station labels remain off-board; unrelated R4/R6/R13 and the later-offboard servo resistors
> were deliberately not moved. [Placement report / preview](hub/STATION-PLACEMENT-2026-10-27.md).

> **Hub SB removal verified (2026-10-26):** PCB1 is now **77 footprints** (84 − 7), with SB1–SB7 and their silk gone.
> User also authorized removing their schematic symbols and 14 exclusive named wire stubs. All 77 surviving PCB
> components, footprints, pad nets, holes, outline and other silk are byte-identical in the full project export.
> No bypass note or routed copper existed, so neither needed deletion. Strict DRC: 270 → 256 existing unrouted-pad
> findings, **zero new findings**; schematic mismatch cleared, ERC true. [Verification report](hub/SB-REMOVAL-2026-10-26.md).
> Do not replay historical schematic/placement/silk builders (they still contain SBs); do not Import Changes
> before resolving the existing J3 footprint binding warning below.

> **Hub direct-mount switch INSTALLED (2026-10-26, retry 2):** J3 is now the KCD4-203-B footprint (six plated
> 7.8 × 1.8 mm slots, nets unchanged), verified in a full project export, not only on the canvas. Board widened left
> to **132.6 × 99.1 mm**; only J3, J4, J5, LED3, the two left holes and their labels moved. Open items: Import Changes
> would revert J3 (rebind the schematic footprint first), J3 BOM still says B6P-VH, `23_silkscreen.js` is stale, and
> **the XT hole overrides are not on the board** (J1 / J2 4.6 mm, J5 1.8 mm). Details: [`hub/DECISIONS.md`](hub/DECISIONS.md)
> H-001 "Retry 2 as built", spec [`hub/WIRING-PLAN.md`](hub/WIRING-PLAN.md) §2.4.

**Boards**: Museum Hub Carrier **v0.4** (SB removal verified; 77 footprints) · **Robot Carrier v0.6** (100 % THT; 66 parts;
**routed 2026-10-27**, GND fills both layers, strict DRC 0 errors; XT drills pending D-034) · Station **v0.4 manual layout** (routed, 50.8 × 31.877 mm) · 2-layer · JLCPCB fab

> **Current station state:** the user replaced the original placement and routes. Fresh bottom GND was added and checked
> against the live geometry, preserving their 59 tracks, one via, all pads and component positions. Read the
> [manual-layout review](station/REVIEW-2026-10-26.md) before continuing. The earlier placement/routing scripts and their
> old fill geometry are historical: do **not** replay them onto this layout. Static fills must be regenerated after any
> copper/pad/outline change; the snapshot-locked writer refuses changed source geometry. Electrical checks passed;
> J1 text overlap, physical fit checks and the independent Gerber/fabrication gate remain.

> **Robot board → its own handoff: [`Robot/README.md`](Robot/README.md)** (status, scripts, next steps, session log), spec
> [`Robot/WIRING-PLAN.md`](Robot/WIRING-PLAN.md) v0.6, decisions [`Robot/DECISIONS.md`](Robot/DECISIONS.md). Process = the
> `pcb-pipeline` skill (same as `D:\projects\attendance_project`). Agent rules: [`../CLAUDE.md`](../CLAUDE.md).
> Review plan of record (2026-10-25): `C:\Users\tahfi\.claude\plans\pasted-content-id-5437-task-curious-stardust.md`.

---

## 1 · STATUS SNAPSHOT

| Item | State |
|---|---|
| EasyEDA Pro 3.2.149 (desktop, Half-offline mode) | ✅ installed & activated |
| Run API Gateway extension v1.0.5 | ✅ installed, enabled, *Allow interact with external* + *Show at header menu* ON |
| Bridge server (`D:\projects\easyeda-api-skill`, v1.1.44) | ✅ runs on `127.0.0.1:49620` |
| End-to-end AI → EasyEDA link | ✅ **verified live** (`edaConnected: true`, exec round-trip + API introspection OK) |
| Zoo Code helper | ✅ `tools/easyeda-bridge/eda.mjs` (workspace rule `.roo/rules/easyeda-bridge.md`) |
| Claude Code skill | ✅ junctioned at `C:\Users\tahfi\.claude\skills\easyeda-api-skill` |
| EasyEDA project | ✅ **"Museum Hub Dummy"** created by hand in half-offline mode (Board1/Schematic1/PCB1/Panel1). Local projects are INVISIBLE to `dmt_Project` APIs (cloud-only) — work document-scoped instead (§4 note) |
| Phase plan | **Phase A**: dummy pipeline-test PCB ✅ → **Phase B (NOW)**: hub board. Spec **v0.3 APPROVED 2026-10-12** in [`hub/WIRING-PLAN.md`](hub/WIRING-PLAN.md): ESP32-S3 N16R8, 100 % through-hole, one 74HCT125, DPDT rocker power select, 8 × XH-8 station ports. Parts selected (spec §6). **Hub schematic DRAWN 2026-10-12** in EasyEDA project **WRO_HUB** (`hub/WRO_HUB.eprj2`, P1.Schematic1) by `scripts/hub/07…10`: 90 parts, 283 named stubs, 75 nets — net list verified pin-by-pin against spec §5; after review fixes **ERC passes** (0 fatal / 0 errors / 0 warnings). **PCB1 placed 2026-10-12** (132 × 99 mm, outline, holes, silkscreen; DRC = only unrouted). **Awaiting placement review → then routing.** Station board: draft spec [`station/STATION-SPEC.md`](station/STATION-SPEC.md) |
| Dummy parts placed | ✅ **2026-10-11 — first AI placement worked**: J1 (C9900016709) + R1/R2 (C21190) + LED1/LED2 (C2286) placed on Schematic1 via `scripts/06_place_dummy.js`, saved. Project file lives at `PCB design/Museum Hub Dummy.eprj2` (versionable in repo!) |
| Dummy wired v2 | ✅ **Net labels alone do NOT bind** (lesson!). Fix: `sch_PrimitiveWire.create([x1,y1,x2,y2],'NET')` stub wires starting ON pins, net name = 2nd arg → nets formed (`scripts/16`) |
| Annotated | ✅ `scripts/08` — `comp.toSync().setState_Designator(...)` → J1/R1/R2/LED1/LED2; ERC run (`sch_Drc.check()` → `false`; ⚠️ correction 2026-10-12: `false` means NOT passed — see §2 conventions) |
| **PCB imported** | ✅ named wires → `pcb_Document.importChanges()` → `true` → **ECO dialog pops in GUI, human clicks Apply** → footprints on PCB1. (Earlier `false` = empty netlist, not an API bug) |
| **PCB finished** | ✅ `scripts/23` tidy layout + 800×600 mil outline on **layer 11** + netflag cleanup; `scripts/24` 8 traces (Top=1, 10 mil) → `pcb_Drc.check()` → `false` (⚠️ correction 2026-10-12: per the API docs `check()` returns *whether it passed*, so `false` = violations; the later strict review in `scripts/26` is what made the dummy clean) |
| **Routing rework** | ✅ `scripts/26` after **human review caught real shorts** (LED_A traces ran straight over the GND K-pads; naive DRC `check()` missed it!). Fix: LED_A nets jog over/under, **GND moved to Bottom layer (2)** with 2 vias (`pcb_PrimitiveVia.create(net,x,y,20,40)`) at LED K pads, J1.3 TH reaches bottom natively. Strict `pcb_Drc.check(true)` → clean. **LESSON: always strict DRC + human eyes** |
| Exports | ⏸ `getBomFile/getGerberFile/getPositionFile` exist but exact signatures unknown — naive calls open interactive dialogs and time out. **Next**: read `PCB_ManufactureData.md` (~line 1070 has the full getBomFile example), export to `PCB design/exports/` |
| **MULTI-WINDOW lesson** | ⚠️ **Multiple EasyEDA windows = the bridge talks to whichever is "active"** — an agent can draw in window A while you stare at empty window B. Diagnose: `GET /eda-windows` lists all; probe each via `POST /execute {code, windowId}` (`scripts/28`); select target via `POST /eda-windows/select` (`scripts/select_window.json`). **Keep exactly ONE EasyEDA window open when working with agents.** WRO_HUB (91 parts, J1–J24/U1A-U1B/U2/SB1-7 per WIRING-PLAN v0.3) lives in its own window — verified + zoomed + saved 2026-10-12. Bridge restarts reassign window IDs — re-probe after any restart |
| Schematic / PCB / exports / order | ⬜ pending |

---

## 2 · THE PIPELINE (what talks to what)

```
AI agent (this chat / Claude Code panel)
   │  node tools/easyeda-bridge/eda.mjs exec "…js…"
   ▼
Bridge server  (D:\projects\easyeda-api-skill, npm run server, port 49620)
   │  WebSocket ws://localhost:49620/eda
   ▼
Run API Gateway extension (inside EasyEDA Pro)  →  executes JS against live `eda.*` API
```

### Commands
```bash
# 0) bridge down? (e.g. after reboot) start it:
cd /d D:\projects\easyeda-api-skill && npm run server

# 1) health (expect edaConnected:true; if false → EasyEDA → menu API Gateway → Reconnect)
node tools/easyeda-bridge/eda.mjs health

# 2) run code inside EasyEDA Pro:
node tools/easyeda-bridge/eda.mjs exec "return JSON.stringify(Object.getOwnPropertyNames(eda))"
node tools/easyeda-bridge/eda.mjs exec -f path\to\script.js   # multi-line batches
```

### API conventions (learned live — don't relearn)
- **Explicit `return` is mandatory** (code runs inside a function body) — no return ⇒ `result: null`.
- **DRC/ERC return value**: `sch_Drc.check(strict, ui, verbose)` / `pcb_Drc.check()` return **`true` = passed**,
  `false` = violations. This build returns only a summary (`[{type:'warn', count:N}]`); pass `ui = true` to open
  the DRC panel in EasyEDA and read the details there.
- **Wiring by named stubs at scale** (learned on the hub, 2026-10-12):
  - point each stub along the **pin rotation** (0 = +x, 90 = +y, 180 = −x, 270 = −y). "Away from the part
    centre" ran stubs along long header columns over neighbouring pins → nets shorted → every later
    `create()` failed with `create failed!` until those wires were deleted;
  - `sch_PrimitiveWire.create()` returns **before** the canvas commits: cap a run at ~60 wires, wait ~2.5 s,
    don't `save()` between batches (the editor rejects creates while saving), retry a refused create after a pause;
  - a stored wire's `getState_Line()` may start at either end — check both endpoints when skipping done pins;
  - **never retry a refused `create()` in the same run**: it can still commit later, the retry stacks an identical
    wire, EasyEDA merges them into one wire holding its `Name` attribute twice → "multiple net names" ERC warning.
    Let the next run pick up genuinely missing pins. Find such wires via `sch_PrimitiveAttribute.getAll(wireId)`
    (count `Name` keys);
  - **pins of two parts on the same point** = ERC *fatal* ("endpoints of pins overlap and are not connected"). Moving
    parts in the GUI can cause it, and EasyEDA may silently join such pins with an unnamed wire. Check pin coordinates
    across parts after any re-arrangement; fix by nudging a part (`toAsync().setState_X/Y`) and redrawing its stubs
    (the API does not drag wires) — see `scripts/hub/13_fix_overlap.js`;
  - intentionally empty pins: `pin.toAsync().setState_NoConnected(true); await …done()` on the placed component's
    pin (clears the "pins floating" warning). The API's ERC result has no details and nothing reaches `sys_Log` —
    read the DRC panel in the GUI.
- Namespaces on the live `eda` object: `dmt_*` (Project/Schematic/Pcb/Board/EditorControl/…),
  `lib_*` (Symbol/Footprint/Device/3DModel/LibrariesList), `sch_*` (Document/Drc/Net/Netlist/Primitive*),
  `pcb_*` (Document/Drc/Layer/Net/Primitive*/Pour), `pnl_*`, `sys_*` (ToastMessage/I18n/HeaderMenu/…).
- Naming: class `SCH_Document` → instance `eda.sch_Document`; methods return **promises** (await/then).
- 30 s request timeout — keep each exec small; batch via `-f`.
- **READ BEFORE WRITING CALLS**: `D:\projects\easyeda-api-skill\SKILL.md`, `references\classes\*`
  (`EDA.md` lists every member), `guide\invoke-apis.md`. Never guess method names.

---

## 3 · BOARD SPEC v0.1 — Museum Hub Carrier (grounded in firmware)

> ⚠️ **Superseded** by [`hub/WIRING-PLAN.md`](hub/WIRING-PLAN.md) v0.3 (ESP32-S3 N16R8, all through-hole,
> new pin map, XT60/XT30 power with a DPDT rocker). Kept for history only. **Do not build from this section.**

Source of truth for pins: [`firmware/hub/include/config.h`](../firmware/hub/include/config.h).
Board = dumb carrier; the ESP32 DevKitC plugs in (Basic-part assembly, no RF cert issues).

### Interfaces the board must provide
| # | Subsystem | Firmware pin(s) | Connector on PCB | Notes |
|---|---|---|---|---|
| 1 | ESP32 DevKitC (38-pin) socket | — | 2× 19-pin female headers 2.54 mm | breadboard-friendly footprint |
| 2 | WS2812 LED chain (7 stations × 8 = 56 px) | GPIO15 | 3-pin screw/JST-XH (5V/GND/DIN) | 330 Ω series on DIN + 1000 µF bulk cap; 5 V rail sized ≥3 A |
| 3 | Passive buzzer | GPIO4 | 2-pin header | driven by PWM |
| 4 | Emergency button module | GPIO21 | 3-pin header (3V3/GND/IN) | module has own pull-down, HIGH = pressed |
| 5 | DHT11 sensors ×4 | GPIO14, 16, 17, 27 | 3-pin headers ×4 | 10 kΩ pull-up per data line |
| 6 | MQ gas sensors (analog) ×2–4 | GPIO34/35/36/39 (ADC1, input-only) | 4-pin headers ×4 | **A0 is 0–5 V → add 2:1 resistor divider** (e.g. 10k/20k) to protect 3.3 V ADC; D0 to spare digital pin |
| 7 | microSD (SPI) | GPIO5/18/19/23 | header for external module **or** onboard µSD slot | v0.1: header only (keep parts Basic) |
| 8 | 5 V power input | VIN/5V | DC barrel 5.5/2.1 + reverse-protection Schottky + 1000 µF | powers DevKit via VIN and the LED rail |
| 9 | Status LED + 330 Ω | GPIO2 | onboard 0603 LED | free on DevKit |
| 10 | Mounting | — | 4× ⌀3.2 holes | corners |

### Fabrication target (JLCPCB cheapest tier)
- 2 layers, 1.6 mm FR-4, 1 oz copper, HASL/ENIG, min track 6 mil, min via 0.3/0.4 mm
- Board size: aim ≤ 100×100 mm
- **Economic PCBA**: prefer *Basic* parts (free assembly); each Extended part ≈ +$3
- Order 5 pcs prototype

---

## 4 · EXECUTION PLAN (work top→bottom, tick as you go)

> **Live API notes (introspected 2026-10-11, see `scripts/03_introspect.js`):**
> `lib_Device.getByLcscIds/search/searchByProperties` (part selection) ·
> `sch_Document.importChanges/save/autoRouting/autoLayout` (schematic build) ·
> `sch_Net.getCurrentProjectAllNets/getAllNetsName` (verify wiring) ·
> `pcb_Document`/`pcb_Drc` for the board. `dmt_Project` list/create are **cloud-only** —
> in half-offline mode the agent drives whatever document is open in the editor.

- [ ] **0. Preflight** — `eda.mjs health` → `edaConnected: true` (else EasyEDA → API Gateway → Reconnect)
- [ ] **1. Create EasyEDA project** — ⚠️ **MANUAL GUI STEP (one-time)**: the `dmt_Project.createProject()`
      API returns `undefined` in client half-offline mode (verified live 2026-10-11; matches JLC's
      documented limitation "project creation API targets cloud/team projects").
      **Human**: EasyEDA Pro → File → New → Project → name `Museum Hub Carrier v0.1` → save locally
      (half-offline workspace) → a schematic opens automatically.
      **Agent then**: `dmt_Project.getAllProjectsUuid()` → find it → work inside via `dmt_EditorControl`.
      (Tried programmatically in `scripts/01_create_project.js` — kept for retry in online mode.)
- [ ] **2. Part selection** — search the **LCSC library** via `lib_*` APIs for every part;
      prefer in-stock Basic parts; record LCSC # per designator in table below
- [ ] **3. Schematic** — place symbols with deliberate coordinates (power left→right),
      net labels (5V, 3V3, GND, DHT_*, MQ_*, LED_DIN, BTN, SDA/SCL spare), wire subsystems
- [ ] **4. ERC** — `sch_Drc` until clean; screenshot/record result
- [ ] **5. PCB** — import netlist → board outline (from §3 size) → place footprints
      (connectors at edges, caps near loads) → route 5V/3V3 fat (≥1 mm LED rail), signals 0.25 mm
      → ground pour + vias → **DRC** (`pcb_Drc`) until clean
- [ ] **6. Export** — Fabrication: Gerber + **BOM (must contain LCSC Part # column)** + CPL
- [ ] **7. Human review** — user opens EasyEDA, reviews every subsystem, approves
- [ ] **8. Order** — EasyEDA "Order at JLCPCB" (one-click) or upload Gerber+BOM+CPL zip at jlcpcb.com;
      PCBA options: Economic, 5 pcs, HASL; confirm shipping to Bangladesh
- [ ] **9. Update this file** — tick boxes, fill parts table, log the session below

### Parts shortlist (fill during step 2)
| Ref | Part | LCSC # | Library | Stock |
|---|---|---|---|---|
| U1 socket | ESP32 DevKitC 38-pin headers ×2 | — | — | (user supplies DevKit) |
| J1 | DC barrel 5.5×2.1 | TBD | | |
| J2 | LED chain terminal | TBD | | |
| D1 | Schottky SS34 | TBD | | |
| C1 | 1000 µF/10 V electrolytic | TBD | | |
| R1 | 330 Ω 0603 | TBD | | |
| R2–R5 | 10 kΩ 0603 (DHT pull-ups) | TBD | | |
| R6–R9 | divider 10 kΩ 0603 | TBD | | |
| R10–R13 | divider 20 kΩ 0603 | TBD | | |
| LED1 | red 0603 + R | TBD | | |

---

## 5 · JLCPCB ORDER CHECKLIST (why files matter)
- **Gerber zip** → the physical board
- **BOM** → *which* component: needs **LCSC Part #** per line (that's how they know what reel to load)
- **CPL/Pick&Place** → *where*: Designator, MidX, MidY, Layer, Rotation
- Cross-check: every BOM designator exists in CPL, every LCSC # in stock at order time

## 6 · TROUBLESHOOTING
| Symptom | Fix |
|---|---|
| `edaConnected: false` | EasyEDA top menu → **API Gateway → Reconnect** (auto-retry gives up after max retries; only manual reconnect resumes) |
| `result: null` | add explicit `return` to the executed code |
| Bridge not found (ports 49620-29) | `cd /d D:\projects\easyeda-api-skill && npm run server` |
| After PC reboot | start bridge ↑ *then* Reconnect in EasyEDA |
| Reconnect fails repeatedly | disable system proxy/VPN or bypass `localhost;127.0.0.1` |
| WS drops during session | extension auto-rescans; if not → Reconnect again |
| `exec` times out (30 s) | **no document open** — editor sits on Start Page (`getSplitScreenTree()` shows `doctype:-1`). Double-click **Schematic1** in the project tree, then retry the script |
| Placement script hangs | same cause: `sch_*` APIs need the schematic page open & active |
| `pcb_Document.importChanges()` returns `false` | **beta-API gap (verified 2026-10-11)**: Board link confirmed good (Board1 → Schematic1 `6b3e09be3267388a` ↔ PCB1 `fda71305eb81c1ea`), flags AddIntoBom/AddIntoPcb already true, uuid passed explicitly — still false. **Workaround: GUI → PCB tab → Design → "Import Changes from Schematic" → Apply.** PCB then contains J1/R1/R2/LED1/LED2; agent re-inventories via `pcb_PrimitiveComponent` and continues automated |

## 7 · SESSION LOG
- **2026-10-27 (station silk + R4/R6/R13 follow-up, H-004)** — User explicitly authorized the previously excluded
  labels/resistors. Fresh before-write backup `hub/WRO_HUB_before_station_cleanup_guard_20261027-0209.epro2`
  preserves the user's concurrent U2 180° rotation and C1 movement (first write guard aborted with zero writes).
  `61_cleanup_station.js`: only R4, R6, R13; three designators; 73 existing strings (8 station names, 64 pin labels,
  board title). Pin labels enlarged to 32 mil / 6 mil stroke; matched by existing identities and verified group order.
  `verify_station_cleanup.py` + final full export `hub/WRO_HUB_after_station_cleanup_20261027-0209.epro2`: only
  authorized properties changed; all other 74 parts, 62 routed segments and other records/documents preserved.
  Actual 76 silk objects: zero pad, edge or text-overlap hits. DRC 25 Physical + 230 Connection unchanged.
  Six new cleanup tests plus 22 existing model tests pass. [Report](hub/STATION-CLEANUP-2026-10-27.md).
- **2026-10-27 (hub station-only placement, H-003)** — User: place only currently unplaced S1–S8 + supporting
  resistors/caps, leave everything else. Fresh inventory identified 37 targets: J10–J17, R31–R37, C3–C9,
  R41–R44, R51–R54, R61–R67; all other 40 parts frozen. `station_placement.py` uses live footprint geometry and
  ADC pin locations; eight tests verify scope, clearances, pad transforms, cap proximity and LED hop lengths.
  First write safely aborted before any move because BZ1/D1/servo resistors had changed meanwhile; preserved
  those edits and refreshed the backup (`hub/WRO_HUB_before_station_place_guard_20261027-0148.epro2`).
  `59_place_station.js` moved only the 37; full export `hub/WRO_HUB_after_station_place_20261027-0148.epro2` +
  `verify_station_placement.py`: only allowed poses/owned attributes changed; every other PCB record and project
  document unchanged. DRC unchanged: 25 Physical Error, 234 Connection Error. No routing, imports, outline,
  pour or free-silk edits. [Report](hub/STATION-PLACEMENT-2026-10-27.md). Free station silk needs a separate cleanup.
- **2026-10-26 (hub SB1–SB7 subtraction, H-002)** — Saved fresh full-project backup
  `hub/WRO_HUB_before_sb_removal_20261026-2307.epro2`; exact-designator, snapshot-locked `56_remove_sb.js` deleted
  seven C492401 footprints (84 → 77) and their owned designator attributes/pad-net records only. No bypass note
  existed; no copper, vias or pours existed, so no copper was deleted. PCB-only deletion exposed one Netlist Error;
  user explicitly authorized `58_remove_sb_sch.js` to remove the same seven symbols and 14 proven-exclusive 30-unit
  named wire stubs. No import, reroute, move, outline edit, repour or disabled rocker script was used.
  Final `hub/WRO_HUB_after_sb_removal_reconciled_20261026-2321.epro2`: **13/13 export/live acceptance tests pass**;
  77 unchanged survivors including R61–R67 and J10–J17, 74 unchanged named nets, all footprint documents unchanged,
  all other schematic symbols/connections unchanged. Strict DRC 270 → 256 Connection Error findings (only the
  14 SB pad findings removed), no Netlist Error, ERC true. Existing hub/station model tests 14/14 pass.
  [Report and evidence](hub/SB-REMOVAL-2026-10-26.md); schematic/placement/silk builders are historical and must not
  be replayed. Existing J3/XT mechanical and binding caveats are unchanged.
- **2026-10-26 (hub J3 direct-mount switch, retry 2)** — Backup `hub/WRO_HUB_before_rocker_v2_20261026.epro2` via the new
  read-only exporter `scripts/hub/51_export_project.js` + `export_project.py`; `inspect_export.py` reads what EasyEDA
  saved. User opened J3 → Edit Footprint; `52_rocker_fp_editor.js` changed the six pads with `pcb_PrimitivePad.modify`
  (ids kept → nets kept), user saved; export shows 6 plated SLOT 7.8 × 1.8 mm / OVAL 3 × 9 mm pads, nets BAT+, V12,
  DC+, BAT−, GND, DC−. User asked for "a bit more width, move only overlapping things": `live_space.py` (live board
  check) → `53_rocker_local_moves.js` (left edge −300 → −620 mil, 132.6 mm; J3, J4, J5, LED3, two holes) →
  `54_rocker_silk.js` (their labels). Final export: 84 parts, 0 overlaps; `24_check_silk` (now reads the real outline)
  0 edge / 0 overlaps, 1 old title-on-pad hit. Found on the way: the `44_xt_pads` hole sizes never reached the saved
  board. Backups `WRO_HUB_after_rocker_fp_save / _moves / _silk_20261026.epro2`.
- **2026-10-27 (robot routed, D-032…D-034)** — J1–J4 placed in the right column (v0.5 spots fit the vertical XT),
  silk v0.6, offline PathFinder routing (0 conflicts, 175 vias incl. 153 GND stitching), static GND fills on both layers
  (1922 pieces, read back identical), `check_eda_copper` clean, strict DRC 0 errors after raising the 2.54 mm max-track
  rule to 5.08 mm. The robot's `44_xt_pads` drills don't persist either (same as the hub): `45_xt_fp_editor.js` in each
  XT footprint editor is the user GUI step left. Details: `Robot/README.md` §6.
- **2026-10-26 (vertical XT connectors + drill holes, robot D-030 / D-031, hub spec v0.4)** — User: the team owns vertical
  XT parts; holes XT60 4.0 mm (legs 3.7 / 3.8 mm), XT30 2.5 mm (legs 2.2 / 2.1 mm); the user places the XT parts by hand.
  - Robot: backup `WRO_Robot_v0.6_pre_xt_swap`; `17_swap_v06` → `14_wire` → `15_check` (51 nets, ERC true) → `sch_audit`
    0 differences → user import (J1–J4 off-board) → `44_xt_pads` 8 pads (J1 4.0 / 5.5 mm, J2–J4 2.5 / 3.5 mm), re-run 0.
  - Hub: backup `WRO_HUB_v0.4_pre_xt_swap`; `17_swap_xt` (J1 / J2 XT60UPB-M, J4 XT60UPB-F C9900007500 1 = + / 2 = −,
    J5 XT30UPB-M) → `09_wire_nets` 8 made, 270 committed → `10_check` 74 nets, 270 pins, ERC true; vs v0.4 only
    J4.3 / J4.4 → J4.1 / J4.2 and 8 fewer bare pins (mounting tabs). User imported → `hub/44_xt_pads` 8 pads (J1 / J2
    4.0 / 5.5 mm, J4 4.0 / 6.0 mm, J5 2.5 / 3.5 mm).
  - Hub un-placed on request (backup `WRO_HUB_v0.4_xt_holes_pre_unplace`, `hub/46_unplace_all.js`: 84 parts below the
    board), then **re-placed shorter** on request ("too much empty space bottom-left" = USB lane + power column): new
    `hub/layout_plan.py` (inventory from `20b_pcb_inventory.js`; overlaps, edge, holes, LOW-only zones, METAL15; PNG) →
    **4600 × 3900 mil (116.8 × 99.1 mm, was 132.1)**, spec §7 rewritten. Backup `WRO_HUB_v0.4_unplaced_pre_replace` →
    `21b_place_pcb.js`: outline replaced, 2 holes moved, 84 placed by pad read-back, 0 rotation fixes; re-run 84
    alreadyOk; in-EDA bboxes 0 overlaps / 0 off-board. `hub/21_place_pcb.js` superseded. SD card end overhangs the bottom
    edge ≈ 5 mm (user to confirm).
  - Silkscreen v0.4b: `23_silkscreen.js` rewritten pad-relative (SD + DevKit outlines from U6 / U1 pads; it owns all
    layer-3 silk), `25_designators.js` (LEDs, U1, J18 / J20, BZ1, J21–J24, odd SBs; visibility + project gate added),
    `24_check_silk.js` (real pad radii, edge, text overlaps). Q1 40 mil down, odd-cell SB1/3/5/7 to the right of their
    cell (clear of the next "Sx Zy" label). Result: 116 texts, 7 lines, 84 designators, **0 hits / 0 edge / 0 overlaps**.
    **Next:** user placement + silk review → routing / DRC.
- **2026-10-26 (user's manual station redesign, fresh GND and review)** — User moved the components, shortened the board
  to 2000 × 1255 mil and routed 59 segments plus one DIN via. Seven GND connections were the only pre-pour DRC findings.
  - Full before/after project backups: [before GND](station/station_pcb_manual_before_gnd_20261026.eprj2),
    [with reviewed GND](station/station_pcb_manual_gnd_reviewed_20261026.eprj2); archive integrity verified.
  - [31_snapshot](scripts/station/31_snapshot.js), [manual_model](scripts/station/manual_model.py) and
    [32_review_manual](scripts/station/32_review_manual.py) adapt the existing fill/check engines to absolute live pads,
    the actual outline and moved mounting holes, without replaying old placement. Seven model regression tests pass.
  - Generated 209 bottom GND fill pieces with thermal reliefs (15 mil spokes, 10 mil gap); read back matches the plan,
    zero overlaps or clearances below 10 mil, board-edge clearance 12 mil. Ground forms one network with all seven pads.
  - All 11 other nets physically connected; pad-net map matches the spec; J4.4 remains intentionally NC. Independent
    copper/hole checks clean; 5V/3V3 widths 1 mm; strict EasyEDA DRC has no errors. Before/after snapshots prove user
    tracks, via, components, pads and outline unchanged.
  - Silkscreen check: no pad/hole/off-board hits, one J1 / HUB CABLE (1:1) text overlap. Team marking replaced the board
    title/revision; recommend adding an identifier. Labels were not changed. Physical module/BC557 checks and the
    independent fabrication-file review remain. Full evidence and limitations: [review](station/REVIEW-2026-10-26.md).
- **2026-10-26 (station v0.4 routing)** — User: "bottom layer ground pour, no need too much via, keep basic design".
  - **Hand-planned routing** (`scripts/station/route_plan.py`): the grid router (`templates/pcb/routing/`) is tied to
    the attendance board, and 11 THT parts / 12 nets don't need it. Waypoints per net, 45° corners, 0 vias. J1's pin
    order (fixed by the hub) forces crossings, so two nets run wholly on the bottom between THT pads: **5V J1.1 → J2.1**
    (0.8 mm, along the bottom edge under J2's pins) and **MQ J1.5 → R2.2**. Everything else is on top: LED data and
    PROX in three 0.3 mm lanes under J1's pins; 3V3 / DHT up the left edge to J3; 5V 0.6 mm from J2.1 up the right
    edge to U2 / Q1, then along the top edge to J4. GND has no tracks.
  - **Toolchain** copied from the template and set up for the station: `30_fp_library.js` → `fp_geometry.py` →
    `route_model.py` (35 pads cross-checked against EasyEDA: 0 problems), `route_check.py` (min gap 12 mil, the J1
    lanes), `gen_route_js.py` → `40_route.js` (80 segments; re-run skips all 11 nets), `29_dump_copper.js` →
    `check_eda_copper.py` (0 problems, every net one island).
  - **GND copper:** `pour_fill.py` (bottom only, thermal reliefs on the 7 GND pads, 0 unconnected) → `42_fill.js`
    (130 hole-free pieces; re-run keeps 130) → `29b_dump_fills.js` → `check_fills.py --plan`: 0 overlaps, gaps 10–11
    mil, edge 12 mil.
  - Fix on the way: the 6 mm mounting-hole rings were drawn with 4 segments per quarter, which left the fill at 8.7 mil.
    They are now drawn with 16 (playbook §7).
  - **`27_drc` passed, 0 errors** (nothing unrouted). Backups `station/station_pcb_v0.4_pre_route.eprj2`,
    `station_pcb_v0.4_routed.eprj2`.
  - **Next:** user visual review of PCB1 (preview `scripts/station/data/route_preview.png`, `fill_preview.png`), then
    the production gate (Gerber / BOM, independent review).
- **2026-10-26 (station v0.4 schematic)** — User: only the IR module on the board (upright); DHT11 and MQ-5 on JST cables.
  Spec v0.4 (v0.3 archived): U1 → J3 XH-3 (3V3, GND, DHT), U3 → J4 XH-4 (5V, GND, AO, DO NC); U1 / U3 retired.
  `17_swap_v04.js` (from the template, window-visible gate) swapped both; `14_wire` 6 new stubs (34 committed, re-run
  idempotent); `15_check` ERC **true**, panel 0 / 0 / 0; `sch_audit` **0 differences** (12 nets / 34 pins). Backups
  `station_pcb_v0.3_pre_v0.4.eprj2`, `station_pcb_v0.4_sch.eprj2`. **Next:** user Import Changes, then a smaller board.
- **2026-10-26 (station v0.4 PCB)** — Import read back (11 parts, J3 / J4 in, U1 / U3 gone). New floor plan **50.8 × 35.6 mm**
  (was 70 × 50): cables J1 / J2 bottom, J3 / J4 + upright IR socket U2 (clear zone) top, divider + alarm middle; `21` placed
  11 / 11, outline + H2 replaced; 0 overlaps; plain-English silkscreen (38 texts, 3 lines), `24` 0 hits; DRC only unrouted
  (34 / 12). Backup `station/station_pcb_v0.4_pcb.eprj2`. **Next:** user placement review, then routing.
- **2026-10-25 (station v0.3: spec + schematic)** — User opened the new EasyEDA project **`station_pcb`**
  (`station/station_pcb.eprj2`). The 2026-10-12 answers had been lost from the spec; re-asked and recorded:
  DHT11 `VCC DOUT GND`, IR `OUT GND VCC`, MQ-5 `VCC GND DO AO`, modules plug into the board, local buzzer kept, 2 × M3,
  one XH-4 cable to the LED stick. Spec **v0.3** (v0.2 archived): designators, net list by pin (12 nets / 34 pins), BOM.
  `scripts/station/02_parts_verify.js`: 10 types, **THT gate PASS** (68 k = C385541, same P10.30 footprint). Scripts 11–15 +
  `sch_audit.py` from the robot set: 11 parts in 3 blocks, 34 stubs, ERC **true** (0 / 0 / 0), audit **0 differences**.
  Backups `station/station_pcb_v0.3_empty.eprj2`, `station_pcb_v0.3_sch.eprj2`. **Next:** user review + Import Changes.
- **2026-10-25 (station v0.3 PCB)** — Import read back (11 footprints, pad nets = schematic). Floor plan with TYPICAL module
  sizes (user hasn't measured yet): board **70 × 50 mm** (the modules + 2 M3 holes didn't fit 60 × 45), modules DHT11 / IR /
  MQ-5 left → right over the top half, cables on the bottom edge, alarm column right. `21` placed 11 / 11 + outline polyline
  + H1 / H2; checks 0 problems; plain-English silkscreen (42 texts, 11 lines), `24` 0 hits; DRC only unrouted (34 / 12).
  Backup `station/station_pcb_v0.3_pcb.eprj2`. **Next:** user placement review (module sizes / mounting), then routing.
- **2026-10-25 (hub v0.4 schematic)** — WRO_HUB opened (EasyEDA 3.2.206: its PCB1 outline + silk lines are gone too, as on
  the robot; 90 parts and 121 texts intact). User added **two spare 5 V outputs J25 / J26** (JST XH 2-pin B2B-XH-A C378939,
  1 = +5V, 2 = GND): spec v0.4 updated first (84 footprints). Builders `07` / `09` / `10` updated (+ project / visibility
  gates). `14_remove_v04.js`: R14, R21–R27 removed with 16 stubs; `15_add_spare5v_v04.js`: J25 (280, 535) / J26 (280, 490),
  nearest free spots to J5 on the hand-arranged sheet; `09`: 4 stubs (270 committed); `11_connectivity` 0 shorts; `10_check`
  ERC **true**, 74 nets, 270 pins; net list vs map **0 differences**. Backups `hub/WRO_HUB_v0.3_pre_v0.4.eprj2`,
  `WRO_HUB_v0.4_sch.eprj2`. `21` / `23` switched to polylines for the outline / silk lines. Robot J1–J4 vertical XT swap
  (D-030) is on hold until the user measures the XT pin diameters. **Next:** user Import Changes on hub PCB1.
- **2026-10-25 (hub v0.4 PCB)** — Import read back (`hub/data/pcb_inventory_v04_import.txt`): 84 footprints, the 8 retired
  resistors gone, J25 / J26 with +5V / GND pads. Free-spot search (DevKit, USB lane, SD module and hole keep-outs): the
  left power column had room for one XH-2 only, so both spares sit side by side above the SD module at (2460 / 2880, 1920).
  `21` (J25 / J26 targets, R14 / R2x dropped): 2 placed, 82 alreadyOk, **outline recreated as a polyline**; `22` 0 overlaps /
  off-board / holes, DevKit rows 1000 mil; `23` 129 texts + 7 polyline lines (title v0.4, "E B C" at Q1, J25 / J26 labels);
  `25` 84 designators; `24` 0 hits; `26` 0 on pads / labels / each other; DRC only Connection Error (270 pins / 74 nets).
  Backup `hub/WRO_HUB_v0.4_pcb.eprj2`. **Next:** user placement review of hub PCB1, then routing.
- **2026-10-25 (part-count review: hub v0.4, robot v0.5, station v0.2)** — Item-by-item review; the user approved
  every verdict, with two changes (a second obstacle port on the robot PCB; Q1 = 2N2222A on the hub).
  - **Hub, applied:** DHT pull-ups R21–R27 removed (module pull-ups + the S3's internal one; a hub pull-up hid empty
    ports from the firmware probe). E-stop pull-down R14 removed (module pull-down + `INPUT_PULLDOWN`; J20 is 3V3).
    D1 1N4148 now **fitted** (coil buzzer + NPN switch = no freewheel path). Q1 = 2N2222A (E-B-C footprint;
    P2N2222A is C-B-E). 3V3 budget corrected (CJ6107A33GW per the YD V1.4 schematic: ≈ 0.15 A average / 0.4 A peak).
  - **Hub, kept on purpose** (the user's removal proposals were rejected, reasons in hub v0.4 §3):
    - DHT 1 k series (cable mis-crimp / ESD protection);
    - E-stop 1 k;
    - PROX 10 k + 1 k (the Schottky only pulls LOW; the pull-up absorbs the 1N5819 leakage);
    - the 330 Ω and the 7 × 220 Ω LED hops (≈ 3 m unterminated hops through the hub);
    - C3–C9;
    - C2 footprint-only, now written as "do not stuff by default".
  - **Robot, applied:**
    - D3 removed: J9.1 = +5V, "unplug J9 before Mega USB" (D-022).
    - R1 removed (D-023).
    - C1 + C2 at J4 removed (D-024).
    - New J15 XH-3 obstacle port, wired-OR with J11 on the PCB → GPIO2; placed (3225, H − 105), offline check
      0 problems (D-025).
    - TP1–TP6 deleted, probe points listed in spec §3.9 (D-026).
    - DevKit LDO enough (D-027); battery divider verified unchanged.
    - Result: 66 parts, 51 nets.
  - **Station:** D1 BAT85 → 1N5819 DO-41; C1 removed; 3V3 outcome recorded.
  - **Docs:**
    - archives `hub/WIRING-PLAN-v0.3.md`, `Robot/WIRING-PLAN-v0.4.md` and `station/STATION-SPEC-v0.1.md`, each with a
      supersession banner;
    - `Robot/DECISIONS.md` D-022…D-027.
  - **Robot EasyEDA (done):**
    - schematic: 66 parts, 51 nets, ERC true, `sch_audit` 0 differences;
    - PCB1 import read back (66 footprints); J15 placed in the top-right corner; R14 / R15 55 mil down for J10's pin
      names;
    - silkscreen regenerated; `22` 0 problems, `24` 0 hits, DRC only unrouted.
    - Details: `Robot/README.md` §6.
  - **EasyEDA auto-updated to 3.2.206:** `pcb_PrimitiveLine` is copper-only now, and the robot's board outline + silk
    lines had vanished. Recreated as polylines; scripts and pipeline templates switched (robot D-028, playbook §7).
    **The hub board likely lost its outline / silk lines too: check when WRO_HUB is opened.**
  - **Next:** the user reviews robot PCB1. Then switch EasyEDA to WRO_HUB for its v0.4 edits
    (`scripts/hub/14_remove_v04.js` + outline / silk polyline fix).
- **2026-10-16 (robot placement)** — Floor plan approved (95.0 × 97.5 mm, native-USB lane, no zip-tie holes: D-019…D-021).
  `scripts/robot/21` placed 75/75 on the first run; `22` clean (zones, height classes, 42 polarity nets, socket geometry);
  silkscreen 79 labels + 75 designators, `24` 0 hits; `27_drc` only unrouted. Next: user review, then offline routing.
  Details: [`Robot/README.md`](Robot/README.md) §6.
- **2026-10-16 (robot schematic)** — WRO_Robot `P1.Schematic1` drawn by `scripts/robot/11…15`: 75 parts in 7 blocks (A4),
  200 stubs, 32 NC, 53 nets; ERC **true** (panel 0/0/0, 4 Info designator-style notes), `sch_audit.py` 0 differences.
  Waiting for the user's review + Import Changes. Details: [`Robot/README.md`](Robot/README.md) §6.
- **2026-10-16 (robot v0.4)** — Plan v0.4 approved; robot work now follows the `pcb-pipeline` skill / attendance process with
  its own handoff [`Robot/README.md`](Robot/README.md) and log [`Robot/DECISIONS.md`](Robot/DECISIONS.md) (D-001…D-017).
  User calls: ≈ 11 V LM386 rail kept (+ JP1 5 V fallback), LM386 modules re-headered pins-down, Mega XH-4 + I²C 1 kΩ
  protection, XT30 buck feeds, **100 % through-hole** (THT gate in `scripts/robot/07`: 32 types, 0 SMD). Spec v0.4 has
  designators (75 parts, 53 nets). Schematic scripts `scripts/robot/10…15` ready; `10_resolve` 32/32. Root `CLAUDE.md` added.
- **2026-10-13 (robot plan v0.3)** — Robot plan rewritten as **v0.3 PROPOSAL** in [`Robot/WIRING-PLAN.md`](Robot/WIRING-PLAN.md)
  (v0.2 archived as `Robot/WIRING-PLAN-v0.2.md`). User decisions: 3S LiPo; **two of the team's LM386 modules + two speakers on
  the battery** through a `VAMP` drop/filter stage (2 × 1N4007 + 1.1 Ω 1 W + 1000 µF → ≈ 11.3 V at 12.6 V, inside the
  LM386M-1's 4–12 V); **RC522 on an XH-7 cable** (RST on GPIO9, 47 Ω on SPI, FSPI default pins 10/11/12/13); ports for a front
  obstacle sensor (XH-3, 3V3), an I²C port like the hub's (XH-4) and two buttons (MUTE + spare, tactile + XH-2 each).
  Fixes found in review: v0.2's 10 k/20 k divider put a 3.3 V DFPlayer TX at 2.2 V, below the S3's VIH of 0.75 × VDD
  (datasheet v2.2 Table 5-4) → now 1 k + DNP 2.2 k; Mega 2-pin power port collided with the 2-pin speaker port → Mega is now
  XH-5 power + data (1N5819 against USB back-feed); switch harness down to VH-2 (battery-side current ≤ ≈ 4 A). Added DFPlayer
  BUSY (GPIO8), battery sense (GPIO1, ÷6), bulk caps at the 5 V input and CAM port. Motor pins re-checked against the S3
  power-up glitch table (Table 2-2): no HIGH glitch or boot pull-up on 4/5/6/7/15/16. New read-only LCSC searches
  `scripts/robot/04…06_*.js` (XH-5 C157991, XH-7 C144398, VH-2 C485345, 1×4 C541851, 1N4007 C108355, 1N5819 C526214,
  47 Ω C3402507, 100 k C1364475, 1.1 Ω 1 W C1365792). Awaiting user review + §10 answers before the schematic.
- **2026-10-12 (PCB placement)** — WRO_HUB PCB1: outline 5200 × 3900 mil (132 × 99 mm), 4 × M3 corner holes,
  all 90 footprints placed by `scripts/hub/21_place_pcb.js` (floor plan in hub spec §7; orientation of every
  connector verified from its pads; offline dry run `sim_place` caught 2 collisions before touching the board).
  121 silkscreen labels + DevKit/SD outlines (`23`), designators tidied (`25`); checks `22`/`24`/`26` clean;
  PCB DRC = only the 282 unrouted pads of 74 nets. Lessons: the EasyEDA window can drop mid-run (extension
  reconnects with a new window id) — make every script idempotent; `pcb_PrimitivePad.getAll()` returns component
  pads too. **Waiting for user review of the placement → then routing.**
- **2026-10-12 (later)** — Robot plan rewritten as **v0.2** after the user's architecture call: the
  **Arduino Nano is deleted** — the YD-ESP32-S3 N16R8 (same DevKit as the hub, rows 25.40 mm) now runs the
  motors itself on 6 GPIOs (ENA 4, IN1–IN4 5/6/7/15, ENB 16, LEDC 1 kHz) with two 10 k ENA/ENB pull-downs
  replacing the Nano's boot brake; the whole Nano UART protocol layer becomes firmware §11.3 (NVS replaces
  EEPROM tuning; web motor panel applies locally). Also per user: **WS2812 + 74HCT125 dropped**, **LM386
  amp module mounted flat** on its own 1×3/1×2 female sockets (DAC path, SPK pins left netless), and a
  **2-pin XH "Molex-type" 5 V power port for the Mega**. RC522 on FSPI 21/12/11/13, DFPlayer UART1 14/17,
  CAM on hardware UART2 TX 18 (SoftwareSerial gone), optional IR on ADC1 1/2/8/9/10. New sockets verified
  live via the bridge (1×22 C46595983, 1×3 C541850, XH-2 C378939 — `scripts/robot/03_*.js`). v0.1 archived
  as `Robot/WIRING-PLAN-v0.1.md` (hub-style supersession banner added).
- **2026-10-12 (review 2)** — The 1 fatal ERC error was J4.1 and J5.4 (XT mounting tabs) on the same point after the
  re-arrangement; the "stray wire" removed in review 1 had been EasyEDA joining them. `scripts/hub/13_fix_overlap.js`
  nudged J5 by +20 (0.2 in) and redrew its two stubs. **ERC PASSES** (`check()` → `true`; only 2 info notes that
  U1A/U1B don't follow letters+number designators — kept on purpose). Net list re-verified identical to spec §5.
  Next: user approval → PCB import (GUI "Apply" click needed).
- **2026-10-12 (review 1)** — User re-arranged the hub schematic into the A4 frame and reported ERC 62 warnings
  (60 wires with their net name twice + floating pins). `scripts/hub/12_cleanup.js`: removed the 60 double-named
  wires, one stray unnamed wire (J4.1–J5.4 mounting tabs) and the one-pin S8_DOUT stub; set No Connect on 22 pins;
  `09` redrew the 60 stubs once. Verified: 282 wires, one `Name` each, 74 nets identical to the approved list,
  0 shorts. ERC via API now reports **1 fatal error** whose text is only visible in the GUI DRC panel → asked user.
  Station spec answers recorded (module pin orders, modules on board, local buzzers).
- **2026-10-12 (robot)** — Robot Carrier spec **v0.1 proposed** in
  [`Robot/WIRING-PLAN.md`](Robot/WIRING-PLAN.md): DOIT ESP32 DevKit V1 (30-pin) + Arduino Nano dual-socket
  carrier mirroring the shipped `ESP32_RFID_MP3_RAIL` / `NANO_LFR_RAIL` pin maps 1:1; RC522 on a drop-in
  1×8 header, DFPlayer on 2×8 @ 18.0 mm, L298N module + both bucks kept off-board (XT60 feeds / XT30 return,
  hub gender scheme), KCD4 rocker with both poles paralleled, Nano-reflash disconnect jumpers JP1/JP2,
  74HCT125 for WS2812 data, Mega display link deliberately not wired (GPIO16/17 DFPlayer conflict).
  All LCSC numbers verified live via read-only bridge searches (`scripts/robot/01…02_*.js`).
  Awaiting the §10 open-item answers before schematic work. `WRO_Robot.eprj2` is still an empty project
  shell (verified: 0 schematics/boards/components) — same starting state the hub had before its plan.
  Note: the vision MCP (`zai-mcp-server`) times out because its Z.AI key returns HTTP 429 "Insufficient
  balance — please recharge" (probe: `tools/_zai_probe.mjs`, since removed); recharge at z.ai to restore
  `analyze_image`. The plan was grounded in the firmware audit instead of the wiring PNG.
- **2026-10-12 (later)** — User created EasyEDA project **WRO_HUB**. Station boards defined (MQ-5 with 68 k/100 k
  divider on the station, IR module at 5 V with its own BC557 buzzer + Schottky to the hub) → hub spec updated
  (no MQ divider on the hub, 100 nF per MQ input) + draft `station/STATION-SPEC.md`. Hub schematic drawn with
  `scripts/hub/06…10` (resolve → place → dump pins → named stubs → check): 90 parts, 283 stubs, 75 nets, net list
  matches spec §5 pin-by-pin; ERC 0 errors / 62 warnings (21 intentionally unconnected pins + others — see the
  DRC panel). First stub attempt shorted header pins (centre-based stub direction) → all wires deleted and redone
  by pin rotation (lessons in §2). Waiting for user review + station questions.
- **2026-10-12** — Spec **v0.3 approved** and written to `hub/WIRING-PLAN.md` (v0.2 archived as
  `hub/WIRING-PLAN-v0.2.md`). User simplified for a beginner build: no SMD, no fuses/MOSFETs/power diodes,
  KCD4-203-B DPDT rocker (I battery / O off / II adapter), XT30 for the 5 V return, one owned 74HCT125, LED chain
  routed through 8 × XH-8 station ports, proximity shared per station pair, port 8 without DHT/MQ, DHT/PROX on
  3V3 as in the old WROOM build. THT parts found via read-only LCSC searches (`scripts/hub/01…05_*.js`).
- **2026-10-11 (later)** — Hub plan **v0.2** written (`hub/WIRING-PLAN.md`; v0.1 archived as
  `hub/WIRING-PLAN-v0.1.md`): MCU → YD-ESP32-S3 N16R8 (row spacing likely 25.4 mm, not Espressif's
  22.86 mm — measure!), DC-priority back-to-back P-FET selector + UVLO, DPST panel switch on one VH-6
  plug, XT60/XT30 gender scheme, S3 GPIO map, LCSC survey via bridge (read-only). Waiting on §10 answers.
- **2026-10-11** — Pipeline researched, installed, verified end-to-end (bridge + extension + helper +
  Claude Code skill junction). Board spec derived from `firmware/hub` pin map. Next: step 1 (create project).
