# Museum HUB Carrier — Wiring Spec v0.4 (ESP32-S3, beginner build, 100 % through-hole)

> **SYNC UPDATE 2026-10-28:** the one Netlist Error was traced to25 stale J3 PCB descriptive fields
> and resolved through a reviewed metadata-only ECO; **strict DRC and ERC are now clear**. Custom
> slots, all nets/copper/GND, positions and the new image data are preserved in the saved export.
> [Fix details](SYNC-FIX-2026-10-28.md). Raster artwork is not ordinary Gerber geometry: the image
> remains in the project, but conversion/verified color-silk handling is still needed to print it.

> **CURRENT 2026-10-28:** the user's last R5/SD_SCK/+5V edits are preserved and GND is regenerated
> (2,331 pieces, 682top/1,649bottom; 12/40/50mil design clearances). R5 is now at (1605,1305),90°.
> See [the current release review](RELEASE-2026-10-28.md) for the matching Gerber/project and all checks.
> ERC passes; physical DRC has zero findings, but one unresolved synchronization finding remains.
> All256 current named pin-net memberships match. This is a two-layer1oz prototype fabrication-review
> package, not unconditional production approval; real-part fit, Q1 pinout and ≈8A qualification remain.
> Earlier editing-state and checkpoint paragraphs below are history, not the current board state.

> **Fabrication-blocker correction COMPLETE (2026-10-27, executed in the isolated repair project):** Q1 kept
> 0.75 mm holes / 1.27 mm pitch; pad width 1.115 mm gives a minimum nominal ring ≈0.1825 mm (JLC absolute
> 1 oz minimum met; 0.25 mm preferred impossible at this pitch). XT60 J1/J2/J4 drills 4.0 mm (pads 5.5/6.0 mm
> kept), XT30 J5 2.5 mm drills / 3.5 mm pads — all persisted at the project-footprint level with pad IDs,
> centres, angles and nets preserved. J3 carries the custom KCD4-203-B slot footprint and KCD4 BOM identity
> via instance attributes (catalogue supplier fields cleared; fresh ERC true). All 187 visible silk objects
> are ≥40 mil high / 6 mil stroke with 11 bounded relocations; the +3V3 pair that crossed the antenna
> projection was rerouted at full 0.8 mm width keeping its endpoints; GND was regenerated from the corrected
> geometry (12 mil normal / 40 mil left / 50 mil source clearances). Verified in the saved release export
> `WRO_HUB_REPAIRED_catalogue_cleared_20261027-2040.epro2`: physical DRC0, ERC true, independent copper/drill/
> hole/edge/annular checks pass, and the whole-change allowlist shows nothing else modified.
> Gerber/BOM capture has since completed; the current review supersedes this checkpoint. Synchronization
> review, real-switch slot fit on a1:1 print, and ≈8A worst-case thermal qualification remain.

> **Final-layout GND review (user 2026-10-27, in progress):** regenerate from the saved final layout,
> never restore stale fill polygons. New-plane clearance: 12 mil normal, 40 mil in the left power
> area, 50 mil from BAT+/BAT−/DC+/DC−/V12. Preserve source-negative net separation and the antenna
> keep-out on both layers. Three unattached GND stitches may be locally relocated into connected
> ground; components and signal/power routes remain fixed. Correct the reversed J5 polarity legend.
> Review already found Q1 annular-ring and J5 terminal-fit blockers; DRC alone is not fabrication approval.

> **Current editing state (2026-10-27):** the user authorized removing GND fills on both copper
> layers to edit the board. All 1843 backed-up polygons are removed; all other design records are
> unchanged. This is actual fill removal, not layer visibility. Do not fabricate this editing state.
> After user edits, repair/regenerate GND using the current geometry rather than replaying the old
> polygons. Full before/final archives and exact JSON backup are recorded in the shared handoff.

> **H-004 authorized follow-up (user 2026-10-27, saved and export-verified):** place R4/R6/R13 and relocate the
> left-behind station silk. Scope is exactly those three resistor poses, their designators, eight
> station names, 64 pin labels, and the existing off-board board title; all other design state stays.
> R6 goes near U2's LED_BUF output, R4 near its LED input, R13 near the MCU ESTOP input. Station
> labels use the existing text/IDs in verified pin order, not a whole-board silk regeneration.
> All 76 adjusted silk objects clear pads/edges/other text in actual readback; all other 74 components,
> 62 existing copper segments and other project records unchanged. DRC unchanged (25 physical, 230
> unconnected). [H-004 report](STATION-CLEANUP-2026-10-27.md). Current R4/R6 poses follow the user's
> concurrent 180° rotation of U2; U2 and C1 were not changed by this cleanup.

> **H-003 station-only placement (user 2026-10-27, saved and export-verified):** the user hand-placed/routed the rest
> of the board and moved the station group off-board. Place only J10–J17, R31–R37, C3–C9,
> R41–R44, R51–R54 and R61–R67 (37 parts, all verified off-board). Other 40 parts, including
> unplaced R4/R6/R13, existing copper, outline, holes, schematic and free silk are frozen.
> Agent's routing-oriented choice: two rows of four vertical ports in S1→S8 serpentine order,
> hop resistors below the rows, MQ filters near actual ADC pins, and zone resistors nearby.
> Source: refreshed full backup `WRO_HUB_before_station_place_guard_20261027-0148.epro2`; old layout scripts
> are not the source. The current outline is x −675..4695, y 0..3975 mil (user edits). Full-export diff:
> only the 37 component poses and 91 owned attribute transforms changed; all other PCB records/project
> documents unchanged. Eight model tests and actual-pad checks pass; DRC unchanged (25 physical, 234
> unconnected). Free station labels remain off-board by scope. Details: [H-003 report](STATION-PLACEMENT-2026-10-27.md).

> **SB subtraction approved 2026-10-26 (H-002):** remove SB1–SB7 (C492401) from PCB1,
> their designator silk, any existing bypass note, and only copper exclusive to their pads.
> Target **84 → 77 PCB footprints**; all other placements, footprints, pad nets, net definitions,
> copper, pours and outline stay unchanged. R61–R67 and J10–J17 are explicitly protected.
> This supersedes the SB port/BOM/layout/silk rows below. After PCB-only deletion produced a
> schematic/PCB mismatch, the user explicitly authorized deleting the same seven schematic
> symbols and only their exclusive named wire stubs; every other symbol/net/connection stays.
> Do not run Import Changes: it can revert the custom J3 footprint. Historical builders still
> contain SBs: do not replay schematic/placement/whole-board silk builders on this revision.

> **Status: APPROVED by the user (v0.3 2026-10-12; v0.4 part-count review 2026-10-25).** This is the source of truth for
> the schematic and PCB. Supersedes v0.3 → [`WIRING-PLAN-v0.3.md`](WIRING-PLAN-v0.3.md), v0.2 (SMD / MOSFET power path) →
> [`WIRING-PLAN-v0.2.md`](WIRING-PLAN-v0.2.md) and v0.1 → [`WIRING-PLAN-v0.1.md`](WIRING-PLAN-v0.1.md).
> Plan of record (v0.4): `C:\Users\tahfi\.claude\plans\pasted-content-id-5437-task-curious-stardust.md`.
> Firmware today ([`firmware/hub/include/config.h`](../../firmware/hub/include/config.h)) still uses the old ESP32-WROOM pins; §10 maps old → new.
> **Retired designators (v0.4): R14, R21–R27; SB1–SB7 removed 2026-10-26 (H-002).**
>
> **v0.4 changes (2026-10-25; the board was placed, not routed). Each line: decision · why · what was rejected.**
> - **DHT pull-ups R21–R27 (7 × 10 k) removed; series R31–R37 (7 × 1 k) kept.** The 3-pin DHT11 modules carry their own
>   4.7–10 k pull-up and the firmware reads with the S3's internal pull-up; a hub pull-up also stops the firmware's
>   continuity probe from telling an empty port from a silent sensor. The 1 k stays as protection for a GPIO on a 1.5 m
>   hand-crimped cable that carries 5 V two pins away (a mis-crimp puts ≈ 1.1 mA into the clamp instead of the buck's
>   current). Rejected: removing both; keeping both.
> - **E-stop pull-down R14 (10 k) removed; series R13 (1 k) kept.** J20.1 is 3V3, so the module can never exceed 3.3 V;
>   the module's own pull-down + firmware `INPUT_PULLDOWN` hold the idle LOW. The 1 k limits ESD / hot-plug current on the
>   most-handled cable. Rejected: a direct wire (safe by level, but unprotected).
> - **D1 1N4148 is fitted (no longer "optional").** A 12 mm passive buzzer is a coil; the old build's push-pull GPIO
>   freewheeled it, but the NPN low-side switch has no path at turn-off, so the collector would avalanche at every PWM
>   edge. Rejected: dropping D1 (the user's proposal). Bring-up: ohmmeter on the buzzer (§9).
> - **Q1 = 2N2222A (user)** instead of the SS8050, same TO-92 footprint wired E-B-C; check the pin-out of the part bought (§3.5).
> - **Kept unchanged, with the reason now written down:** C2 stays footprint-only (§3.6, §9); C3–C9 (§3.4); PROX 10 k +
>   1 k (§3.3); the 330 Ω and the 7 × 220 Ω LED hops (§3.2).
> - **3V3 budget corrected** (§1): ≈ 0.15 A average / ≈ 0.4 A peak on the DevKit's CJ6107A33GW, not 0.35 / 0.7 A.
> - **Two spare 5 V outputs J25, J26 (user, 2026-10-25):** JST XH 2-pin B2B-XH-A (C378939, 2.5 mm), pin 1 = +5V, pin 2 = GND
>   (house rule: supply on pin 1). For extra 5 V loads (fans, lights, a sensor); they share the buck's 5 V budget (§1).
> - Result: **84 footprints** (90 − 8 removed + 2 spare ports).
> - **Vertical XT connectors + drill sizes (user, 2026-10-26; robot D-030 / D-031):** the team owns the normal vertical
>   (top-entry) XT parts, not the right-angle XT60PW / XT30PW. J1 / J2 = XT60UPB-M (C2918078), J4 = XT60UPB-F
>   (C9900007500, library-only id; hand-soldered, bought locally), J5 = XT30UPB-M (C428721). Genders unchanged, nets per
>   pin unchanged, mounting tabs gone. Holes: XT60 **4.0 mm** (pads 5.5 / 6.0 mm kept), XT30 **2.5 mm** (pad 3.5 mm),
>   set on the PCB by `scripts/hub/44_xt_pads.js` after every Import Changes. The power column is re-placed for the
>   vertical bodies (cables now plug in from above). Rejected: keeping right-angle footprints (parts don't fit).
> - **Direct-mount rocker revision in progress (user, 2026-10-26):** J3's VH-6 harness is being replaced by the actual
>   KCD4-203-B switch. User reconfirmed the original drawing's 10.5 mm pitch / 11 mm pole spacing; the proposed generic
>   11 mm / 10–10.3 mm alternative was rejected. See §2.4 for the binding footprint specification and verification limits.

---

## 0 · What this board is

A hand-soldered carrier for the **YD-ESP32-S3 N16R8** DevKit. Every part is through-hole, there is one logic
chip (the **74HCT125** the team already owns), and every off-board wire ends in a connector.
Bare PCB order only — no JLC assembly.

| Decision | Choice |
|---|---|
| Power source select | Manual 3-position DPDT rocker **KCD4-203-B**: I = battery, O = off, II = 12 V adapter |
| Protection | None (no fuses, MOSFETs or power diodes — user decision). Unplug the LiPo when not in use |
| 12 V | XT60 in (battery, adapter) and out (to the external 10 A buck) |
| 5 V | Back from the buck on **XT30** — a different size, so a LiPo can never be plugged into the 5 V rail |
| Stations | 8 ports, one **8-pin JST-XH** cable each to a station board: 5V, GND, 3V3, DHT, MQ, PROX, LED DIN, LED DOUT |
| Pin budget | 28 usable S3 pins → proximity is shared by pairs of stations; port 8 has no DHT/MQ |
| Servos | Up to 4, all on one shared signal |
| Other | Onboard buzzer, emergency-button port, I²C port, microSD module lying flat |

Reference drawings in this folder:
- `YD-ESP32-S3 N16R8_footprint.png` — rows **25.40 mm** apart, board 27.94 × 57.15 mm, pin 1 at 1.91 mm from the
  top edge, antenna overhangs the top edge by 6.24 mm (63.39 mm total).
- `Switch_KCD4-203-B-dimension.jpg` — 2 rows × 3 spade tabs (6.3 mm), rows 11 mm apart, tabs 10.5 mm apart; panel mount.
- `Micro-SD-Card-Adapter-Module-Dimensions.jpg` — 46 × 24 mm; pins CS, SCK, MOSI, MISO, VCC, GND; has its own
  AMS1117-3.3 + level shifter → **VCC = 5 V**, signals are 3.3 V-safe.

---

## 1 · Power path

```
3S LiPo ──XT60──► J1 BAT IN :  BAT+ ─► switch pole 1 "I"      BAT− ─► switch pole 2 "I"
12 V adapter ─panel jack─XT60─► J2 DC IN :  DC+ ─► pole 1 "II"    DC− ─► pole 2 "II"
          (J3 = direct-mounted KCD4-203-B on six plated slots; middle "O" = both poles open)
          pole 1 COM = V12        pole 2 COM = GND (system ground)
V12 / GND ─► J4 XT60 → buck 10 A IN ;  buck 5 V OUT ─► J5 XT30 ─► +5V rail → everything on the board
Red   LED + 2.2 kΩ : BAT+ → GND   lights only in position I  (battery selected and plugged in)
Blue  LED + 2.2 kΩ : DC+  → GND   lights only in position II
Green LED + 1 kΩ   : +5V  → GND   buck output present
```

- Both wires of the unused source are open, so the adapter can never charge or touch the battery.
- I ↔ II passes through O, so the two sources are never connected together. The hub restarts when you switch.
- **BAT− and DC− are separate nets on the PCB, not GND.** They become GND only through the switch.
- No transistor is needed for the source LEDs: the switch closes only the selected source's LED loop.
- The ESP gets +5V straight into its 5Vin pin. The YD board's own diode stops USB from back-feeding the rail —
  **keep the YD board's IN-OUT solder jumper open.**
- +3V3 (stations' DHT11s, I²C, E-stop) comes from the DevKit's own LDO: **CJ6107A33GW** on the YD-ESP32-S3 CoreBoard V1.4
  schematic (SOT-223, 1 A, 0.5 V dropout at 1 A, 10 µF in / out, fed from 5Vin through the board's D3). Clones vary: read
  the marking on your board (an AMS1117 needs ≥ 4.8 V at 5Vin). Load: S3 + SoftAP ≈ 120 mA, 7 DHT11 ≈ 4 mA, E-stop
  module ≈ 3 mA, I²C module ≤ 20 mA → **≈ 0.15 A average, ≈ 0.4 A peak** (Wi-Fi TX bursts of a few ms). With ≈ 1.4 V
  across it that is ≈ 0.2 W, ≈ +20 °C (θJA ≈ 90 °C/W): no separate 3.3 V regulator (two regulators must never share
  the DevKit's 3V3 pins).

### Current budget
| 5 V load | Typical | Worst case |
|---|---|---|
| 7 × MQ-5 heaters + 7 IR modules / station buzzers | 1.2 A | 1.4 A |
| 56 × WS2812B (7 boards × 8) at firmware brightness 60/255 | ≤ 0.8 A | 3.4 A (full white) |
| 1–4 × SG90 | 0.1 A | 2.6 A (all stalled) |
| ESP32-S3 + Wi-Fi, SD module, buzzer, 74HCT125 | 0.5 A | 0.6 A |
| **Total** | **≈ 2.5–3 A** | **≈ 8 A** |

Per station port ≤ 0.7 A (MQ-5 heater + 8 LEDs + IR module + station buzzer) → JST-XH (3 A per pin) is fine;
use 22 AWG for 5V/GND.
12 V side worst case ≈ 4 A. A 2200 mAh 3S pack lasts roughly 1.5 h — the MQ heaters alone draw about 1 A.

---

## 2 · Ports and harnesses

### 2.1 Port table
| Ref | Port | Board connector (LCSC) | Pins 1 → n |
|---|---|---|---|
| J1 | Battery in | XT60UPB-M, male, vertical (C2918078) | −, + |
| J2 | DC adapter in | XT60UPB-M, male, vertical (C2918078) | −, + |
| J3 | Direct-mounted DPDT rocker | User-supplied KCD4-203-B, six custom plated slots (§2.4); no catalog supplier number | BAT+, V12, DC+, BAT−, GND, DC− |
| J4 | 12 V → buck | XT60UPB-F, female, vertical (C9900007500) | +, − |
| J5 | 5 V ← buck | XT30UPB-M, male, vertical (C428721) | −, + |
| J10–J16 | Stations S1–S7 | JST XH 8-pin B8B-XH-A (C157972) | 5V, GND, 3V3, DHT, MQ, PROX, DIN, DOUT |
| J17 | Station S8 (spare, LED-chain end) | JST XH 8-pin (C157972) | 5V, GND, 3V3, –, –, PROX, DIN, DOUT |
| J18 | I²C (expansion port) | JST XH 4-pin B4B-XH-A (C144395) | 3V3, GND, SCL, SDA |
| J20 | Emergency button | JST XH 3-pin B3B-XH-A (C144394) | 3V3, GND, SIG |
| J21–J24 | Servo 1–4 (one shared signal) | JST XH 3-pin (C144394) | 5V, GND, SIG |
| J25, J26 | Spare 5 V outputs (v0.4) | JST XH 2-pin B2B-XH-A (C378939) | 5V, GND |
| U1 | YD-ESP32-S3 socket | 2 × 1×22 female PM254V-11-22-H85 (C46595983), rows 25.40 mm | §4 |
| U6 | microSD module (flat) | 1×6 female PM254V-11-06-H85 (C2832269) | CS, SCK, MOSI, MISO, VCC, GND |

### 2.2 Rules
- **Every XH port has its supply on pin 1 and GND on pin 2.** A plug in the wrong XH port can never reverse
  polarity; the worst case is 3.3 V ↔ 5 V. Pin counts also differ by job (8 / 4 / 3). Silkscreen "3.3 V" on J18 and J20.
- **XT genders follow the LiPo/ESC convention** (the live side has the sockets): battery and adapter leads
  (XT60 female) fit only J1/J2; the buck input lead (XT60 male) fits only J4; the buck 5 V lead (XT30 female)
  fits only J5.
- **Station boards** (7 copies, spec in [`../station/STATION-SPEC.md`](../station/STATION-SPEC.md)): MQ-5 on 5V with
  its 68 k / 100 k divider on the station, so the MQ signal on the cable is ≤ 3.0 V. DHT on **3V3** (as in the old
  working build). IR obstacle module on 5V with its local BC557 + buzzer alarm; its OUT reaches the hub through a
  Schottky diode, so the hub only ever sees "pulled LOW = detected" (3.3 V-safe, and two stations can share one
  input). LED stick on 5V.

### 2.3 Harness pin-outs
| Cable | Hub end | Other end |
|---|---|---|
| Station S1–S8 | XH-8 housing | XH-8 on the station board, wired 1:1 (pin n ↔ pin n) |
| Rocker switch | J3 direct plated-slot mount | KCD4 terminal blades; no VH-6 harness in this revision (§2.4) |
| Battery | — | stock LiPo lead, XT60 female |
| DC adapter | — | panel 5.5 × 2.1 mm jack → 16–18 AWG → XT60 female |
| Buck | — | input lead XT60 male (to J4); output lead XT30 female (to J5); 16 AWG |
| Servo | XH-3: 5V, GND, SIG | servo plug: red, brown, orange |
| Emergency button | XH-3: 3V3, GND, SIG | module VCC, GND, OUT (HIGH = pressed, own pull-down) — preferred; a bare NO button between SIG and 3V3 also works but then relies on the S3's internal pull-down + debounce only |
| I²C | XH-4: 3V3, GND, SCL, SDA | module VCC, GND, SCL, SDA |

Rocker (KCD4-203-B) wiring — tabs seen from the back, 2 rows × 3:
| J3 pin | Net | Switch tab |
|---|---|---|
| 1 | BAT+ | pole 1, end tab "I" |
| 2 | V12 | pole 1, middle tab (COM) |
| 3 | DC+ | pole 1, end tab "II" |
| 4 | BAT− | pole 2, end tab "I" (same end as pin 1) |
| 5 | GND | pole 2, middle tab (COM) |
| 6 | DC− | pole 2, end tab "II" |

⚠️ On many rockers, pressing the "I" side connects the middle tab to the **opposite** end tab. Check with a
multimeter (continuity) before crimping; if it is reversed, swap the I and II wires on both poles.

### 2.4 Direct PCB mount — approved replacement for the J3 harness (2026-10-26)

> **Installed and export-verified; current isolated project footprint UUID `de6d9dbe09d94d61`.**
> Current schematic/PCB instance overrides name this custom footprint and KCD4-203-B; pin/net mapping
> is unchanged. The underlying catalog library retains a VH-6 alias and the PCB has one synchronization
> finding. Inspect proposed ECO differences manually; do not apply a change that replaces the six slots.
> Earlier failed imports/edits and the original damaged project are diagnostic only.

This subsection supersedes the panel-mounted VH-6 arrangement above when the direct-mount revision is applied.
The six electrical connections in §5 are unchanged; J3 is retained as the historical designator, but represents the
switch itself, not a connector. The old B6P-VH supplier number must not remain in its BOM entry.

- **Source:** `Switch_KCD4-203-B-dimension.jpg`; user explicitly reconfirmed its geometry on 2026-10-26.
- **Terminal centres, component-side view:** first pole pins 1 / 2 / 3 at x = −10.5 / 0 / +10.5 mm, y = +5.5 mm;
  second pole pins 4 / 5 / 6 at the same x positions, y = −5.5 mm. Both middle pins are common. Viewed from the
  solder side the pattern is mirrored. Actual I / II face-marking correspondence still requires a continuity check.
- **Blade orientation:** broad blade faces are visible in the drawing's end view; their 6.3 mm width runs along the
  11 mm pole-separation direction, not along the 10.5 mm contact-pitch direction.
- **Blade size supplied by user:** 6.3 ±0.10 mm wide × 0.8 ±0.03 mm thick. No physical switch is available yet.
- **Chosen plated-slot target:** 7.8 mm overall length × 1.8 mm width, long axis parallel to the pole spacing;
  ordinary routed slots have semicircular ends, NOT sharp internal corners. This deliberately oversized capsule
  must contain the rectangular blade corners. Copper pads: 9.0 × 3.0 mm oblongs, nominal annular ring 0.6 mm.
  No solder paste; solder mask open on both sides. Do not substitute an unplated mechanical cutout.
- **Design allowances (engineering choices, not measured part tolerances):** check the maximum 6.4 × 0.83 mm blade
  against a slot 0.10 mm smaller in both overall dimensions, with ±0.20 mm blade-centre displacement in each axis.
  This is a bounded geometric check, not a guarantee for arbitrary KCD4 variants. The fabricator must confirm
  finished plated-slot size/tolerance; do not silently double-compensate plating in the drill data.
- **Outer flange:** 34.5 × 25.5 mm, centred on the six contacts. Reserve a 36.5 × 27.5 mm courtyard (1 mm each side).
  Show the flange on top silkscreen/assembly documentation. The 26 × 21.8 mm lower body is a drawing reference,
  not a verified panel-cutout specification. No PCB body cutout is needed for terminal-through-board mounting.
- **Placement ruling:** preserve the user's other component placements, copper and mounting holes. If the current
  narrow power strip cannot contain the flange, add a local left-side outline extension rather than placing the
  body over the DevKit/USB path or replaying the obsolete whole-board placement script. Record the final bounds.
- **Implementation:** project footprint made in the in-place footprint editor with the documented pad API
  (`scripts/hub/52_rocker_fp_editor.js`), saved by the user, verified by a full project export
  (`export_project.py` + `inspect_export.py`). Component-pad overrides and document-source replacement are not used:
  neither persisted. J3's actual BOM name is KCD4-203-B and it no longer lists C20088586. The raw footprint
  text still has a legacy VH-6 alias; all six actual plated slots are inspected in the current Gerber/drill.
- **Historical placement / outline (user 2026-10-26: widen a bit, move only overlapping parts):** J3 at (−60, 1495) mil,
  rotation −90 (34.5 mm flange side along board y, pin 1 top right). Left board edge moved from −300 to −620 mil:
  rectangular board x −620..4600, y 0..3900 = **132.6 × 99.1 mm**. Moved with it: J4 to (−10, 445), J5 to
  (343.9, 513.3), LED3 to (780, 190), left M3 holes to (−450, 160) and (−450, 3730), plus their silk labels. Every
  other part stays where the user placed it. The earlier (−720, 1450) + outline-tab plan is superseded.
- **Mechanical/fabrication gate:** support the switch body and nearby PCB against operating force; the solder
  joints alone are not a verified mounting system. Before ordering, obtain the real switch, check blade spacing,
  width/thickness, solderability, I/O/II continuity and break-before-make action; print the footprint 1:1 and fit it.
  Without those checks this is a drawing-based prototype, not a physically verified production footprint.

---

## 3 · On-board circuits

### 3.1 74HCT125 (U2, DIP-14 in a socket) — same wiring as the old build, new GPIO numbers
| Pin | Name | Connection |
|---|---|---|
| 1 | /1OE | GND |
| 2 | 1A | GPIO45 (LED_DIN) + 10 k to GND |
| 3 | 1Y | 330 Ω → S1_DIN |
| 4 | /2OE | GND |
| 5 | 2A | GPIO46 (SERVO_IN) + 10 k to GND |
| 6 | 2Y | SERVO_BUF → 4 separate 220 Ω → J21–J24 SIG |
| 7 | GND | GND |
| 8 | 3Y | not connected |
| 9 | 3A | GND |
| 10 | /3OE | +5V |
| 11 | 4Y | not connected |
| 12 | 4A | GND |
| 13 | /4OE | +5V |
| 14 | VCC | +5V, with 100 nF directly across pins 14 and 7 |

The two 10 k pull-downs stop the LEDs flashing and the servos twitching while the S3 boots. They also hold the
strapping pins GPIO45/46 LOW at reset, which the S3 needs to boot.

### 3.2 LED chain through the hub
`S1_DOUT → 220 Ω → S2_DIN → … → S7_DOUT → 220 Ω → S8_DIN`; S8_DOUT ends at J17 pin 8 (chain end).
- LED power is parallel: each LED board gets 5 V from its own station port.
- Each hop runs out to a station and back over two cable lengths, driven by the station's own WS2812 →
  keep station cables ≤ about 1.5 m.
- **SB1–SB7 removed (user, 2026-10-26, H-002):** no onboard bypass remains between Sk_DIN and Sk_DOUT.
  Populate stations consecutively: an empty middle port interrupts the LED chain to later ports. R61–R67 stay.
- **Why the 220 Ω hops exist (v0.4 review: kept).** The old build chained its LED boards directly (DOUT → DIN over short
  wires, only the 330 Ω at the 74HCT125: `plans/hub-sensor-wiring.md` §5). Here every hop is station k → 1.5 m → hub →
  1.5 m → station k+1: ≈ 3 m of unterminated line driven by a WS2812's own DOUT. At ≈ 5 ns/m that is ≈ 15 ns one way,
  comparable to the 10–20 ns edges, so the line rings, which can double-clock bits (random colours / flicker). The 220 Ω
  and the outgoing cable's ≈ 100 pF round the edges (≈ 20 ns RC vs ±150 ns WS2812 timing tolerance), cut the reflection
  on the incoming half from +1 to ≈ +0.5, and limit back-powering to ≈ 20 mA when a station's DIN pin mates before its
  5V/GND (XH has no staggered pins). If one hop proves fine, a wire link fits the same footprint.

### 3.3 Proximity zones (shared inputs)
The PROX pins of S1+S2, S3+S4, S5+S6, S7+S8 are joined on the PCB into zones Z1–Z4 ("either station detects" = LOW).
Per zone: 10 k pull-up to +3V3 (an empty zone reads "nothing") + 1 k series → GPIO.
- **Both resistors stay (v0.4 review).** The station's Schottky can only pull LOW, so the hub must define HIGH. The S3's
  internal pull-up (≈ 45 kΩ) alone would let the zone float during reset and in any firmware path that forgets it, and
  would be 4.5× weaker against crosstalk from the LED data wire next to PROX in the cable. It also has to absorb the
  station diode's reverse leakage: the 1N5819 (station v0.2) leaks from the 5 V IR output *into* PROX (a few µA at 25 °C,
  ≈ 10× per 25 °C). Through 10 k that lifts the idle level by ≈ 0.1 V (≤ 3.8 V with ≤ 0.1 mA into the clamp in a hot
  room); through 45 kΩ it would sit well above VDD + 0.3 V. The 1 k is the same cable protection as on the DHT lines.

### 3.4 Per station S1–S7
- DHT: **1 k series → GPIO only** (v0.4: the 10 k hub pull-ups R21–R27 are gone). The DHT11 module's own pull-up
  (4.7–10 k, check with a meter: §9) holds the line HIGH; the firmware adds the S3's internal pull-up while it reads
  (open-drain). Without a hub pull-up, an empty port reads "floating" in the firmware's continuity probe, as intended.
  The 1 k protects the GPIO against a mis-crimped cable (5 V on pin 4 → ≈ 1.1 mA into the clamp) and hot-plug ESD; it
  costs nothing on the signal (S3 LOW at the cable ≈ 0.3 V with a 10 k module pull-up; RC ≈ 0.2 µs).
- MQ-5: arrives already divided by the station (68 k / 100 k, AOUT 5 V → 3.0 V), so **no divider on the hub**: the
  line goes straight to its ADC1 pin with **100 nF to GND** at the pin. That source is ~40 kΩ and shares the cable
  with the 800 kHz LED data; the capacitor keeps the reading steady. Firmware: AOUT = V_adc × 168 / 100.

### 3.5 Other
- **Buzzer** (old build): GPIO47 → 1 k → Q1 base (**2N2222A**, TO-92, user choice v0.4); 10 k base → GND; emitter → GND;
  collector → buzzer −; buzzer + → +5V. 12 mm passive 5 V buzzer on the board.
  - **Q1 pin-out:** the footprint is wired **E-B-C** (pads 1 / 2 / 3). PN2222A-style parts (E-B-C) go in as drawn; ON's
    P2N2222A is **C-B-E**: turn it round so its collector lands on pad 3. Read the datasheet of the part you buy; the silk
    shows "E B C". Base drive (3.3 − 0.7) / 1 k ≈ 2.5 mA saturates it at the buzzer's ≤ 0.1 A (forced β ≈ 40).
  - **D1 1N4148 across the buzzer (cathode to +5V) is fitted** (v0.4). A 12 mm passive buzzer is a coil (16–42 Ω). The
    old build's push-pull GPIO freewheeled it; the NPN has no path at turn-off, so without D1 the collector flies up to
    the transistor's breakdown (40 V for the 2N2222A) at every PWM edge. D1 (100 V, 450 mA repetitive) clamps it at
    ≈ 5.7 V. If the buzzer measures open (a piezo), fit a 1 kΩ in D1's pads instead (§9).
- **Emergency button**: SIG → 1 k → GPIO21 (HIGH = pressed, as the firmware expects). **No external pull-down** (R14
  removed in v0.4): the module's own pull-down + the firmware's `INPUT_PULLDOWN` (≈ 45 kΩ) hold it LOW; pressed reads
  3.3 × 45 / 46 ≈ 3.2 V. J20.1 is 3V3, so the module can never exceed 3.3 V. Note: a normally-open active-HIGH button
  can't detect a cut cable (unchanged design choice).
- **I²C**: 4.7 k pull-ups on SDA and SCL to +3V3. Plugging an I/O-expander module in here is how to add pins later.
- **Source LEDs**: §1.

### 3.6 Capacitors
- **100 nF at the 74HCT125** (fitted): the chip switches at 800 kHz and pulls short current spikes; the capacitor
  supplies them locally. The old build had it too.
- **470 µF at the servo ports** (C2, footprint only, **do not stuff by default**): one SG90 needs nothing. If 3–4 servos
  starting together reset the ESP (reset reason brown-out / power-on in the serial log), solder it in (+ = pad 1, square).
- **No capacitor at J5**: the buck has its own.
- **100 nF on each MQ input (C3–C9), kept (v0.4 review):** the station divider's source is 68 k ∥ 100 k = 40.5 kΩ; with
  100 nF that is τ ≈ 4 ms (≈ 39 Hz), ≈ 86 dB below the 800 kHz LED-data coupling, and it is the charge reservoir the S3's
  SAR ADC needs to sample a 40 kΩ source without error. MQ-5 readings change over seconds.

---

## 4 · DevKit pin map (U1)

U1A = left 22-pin header, U1B = right 22-pin header, pin 1 at the antenna end.

| U1A pin | GPIO | Net | | U1B pin | GPIO | Net |
|---|---|---|---|---|---|---|
| 1 | 3V3 | +3V3 | | 1 | GND | GND |
| 2 | 3V3 | +3V3 | | 2 | 43 (TX) | — |
| 3 | RST | — | | 3 | 44 (RX) | — |
| 4 | 4 | MQ_S1_ADC | | 4 | 1 | MQ_S5_ADC |
| 5 | 5 | MQ_S2_ADC | | 5 | 2 | MQ_S6_ADC |
| 6 | 6 | MQ_S3_ADC | | 6 | 42 | PROX_Z2_IO |
| 7 | 7 | MQ_S4_ADC | | 7 | 41 | PROX_Z1_IO |
| 8 | 15 | DHT_S1_IO | | 8 | 40 | DHT_S7_IO |
| 9 | 16 | DHT_S2_IO | | 9 | 39 | DHT_S6_IO |
| 10 | 17 | DHT_S3_IO | | 10 | 38 | DHT_S5_IO |
| 11 | 18 | DHT_S4_IO | | 11 | 37 | — (octal PSRAM) |
| 12 | 8 | I2C_SDA | | 12 | 36 | — (octal PSRAM) |
| 13 | 3 | PROX_Z3_IO | | 13 | 35 | — (octal PSRAM) |
| 14 | 46 | SERVO_IN | | 14 | 0 | — (BOOT) |
| 15 | 9 | I2C_SCL | | 15 | 45 | LED_DIN |
| 16 | 10 | MQ_S7_ADC | | 16 | 48 | PROX_Z4_IO |
| 17 | 11 | SD_MOSI | | 17 | 47 | BUZ_IO |
| 18 | 12 | SD_SCK | | 18 | 21 | ESTOP_IO |
| 19 | 13 | SD_MISO | | 19 | 20 | — (USB D+) |
| 20 | 14 | SD_CS | | 20 | 19 | — (USB D−) |
| 21 | 5V | +5V | | 21 | GND | GND |
| 22 | GND | GND | | 22 | GND | GND |

Notes: GPIO3 may carry a 10 k pull-up from the YD board (harmless on a pulled-up input). GPIO48 is also the
onboard RGB LED's pin, so that LED is not used. All MQ inputs sit on ADC1 (GPIO1–10), which keeps working with Wi-Fi on.

---

## 5 · Connection list (schematic source of truth)

Net names are what the schematic must use. "Sk" = station k.

Vertical XT parts (2026-10-26), 2 pins each: XT60UPB-M (J1, J2) 1 = −, 2 = +; XT60UPB-F (J4) 1 = +, 2 = − (pins named
only 1/2: the footprint's chamfered end is pad 2, XT rule chamfer = −; meter check at bring-up); XT30UPB-M (J5) 1 = −,
2 = +. Holes: XT60 4.0 mm, XT30 2.5 mm (`44_xt_pads.js`).

**Power**
- `BAT+`: J1.2, J3.1, R1 (2.2 k) → LED1 red anode; LED1 cathode → GND.
- `BAT-`: J1.1, J3.4.
- `DC+`: J2.2, J3.3, R2 (2.2 k) → LED2 blue anode; LED2 cathode → GND.
- `DC-`: J2.1, J3.6.
- `V12`: J3.2, J4.1.
- `GND`: J3.5, J4.2, J5.1, U1A.22, U1B.1/21/22, U2.7, U6 GND, all XH GND pins (incl. J25.2, J26.2).
- `+5V`: J5.2, U1A.21, U2.14, U2.10, U2.13, U6 VCC, J10–J17 pin 1, J21–J24 pin 1, J25.1, J26.1, BZ1 +, R3 (1 k) → LED3 green
  anode (cathode → GND), C2 + (servo 470 µF, not fitted).
- `+3V3`: U1A.1, U1A.2, J10–J17 pin 3, J18.1, J20.1, the PROX 10 k pull-ups R41–R44, I²C 4.7 k pull-ups.
- C1 100 nF: U2.14 ↔ U2.7.
- Probe voltages at the connector pins (there are no separate test points).

**Stations S1–S7** (k = 1…7; Jk' = J10…J16)
- `DHT_Sk`: Jk'.4 → R3k (1 k; R31…R37) → `DHT_Sk_IO` (U1 pin per §4). No hub pull-up (v0.4; R21–R27 retired).
- `MQ_Sk_ADC`: Jk'.5 → U1 pin directly; 100 nF from `MQ_Sk_ADC` to GND (C3…C9 for S1…S7).
- `PROX`: Jk'.6 → zone net (below).
- `Sk_DIN`: Jk'.7. `Sk_DOUT`: Jk'.8. No SBk pads (removed 2026-10-26); all port/hop-resistor mappings unchanged.

**Station S8** (J17): pins 1/2/3 = +5V/GND/+3V3; pins 4 and 5 not connected; pin 6 → `PROX_Z4`;
pin 7 = `S8_DIN`; pin 8 = chain end, No Connect on the hub (there is no test point; v0.3 text "→ TP5" was stale).

**LED chain**
- `LED_DIN` (U1B.15, GPIO45) → U2.2; 10 k U2.2 → GND.
- U2.3 (1Y) → 330 Ω → `S1_DIN`.
- `S1_DOUT` → 220 Ω → `S2_DIN`; … `S7_DOUT` → 220 Ω → `S8_DIN`.

**Proximity zones**
- `PROX_Z1` = J10.6 + J11.6; `PROX_Z2` = J12.6 + J13.6; `PROX_Z3` = J14.6 + J15.6; `PROX_Z4` = J16.6 + J17.6.
- Each `PROX_Zn`: 10 k to +3V3 → 1 k → `PROX_Zn_IO` (U1 pin per §4).

**Servos**
- `SERVO_IN` (U1A.14, GPIO46) → U2.5; 10 k U2.5 → GND.
- U2.6 (2Y) = `SERVO_BUF` → 220 Ω → J21.3; → 220 Ω → J22.3; → 220 Ω → J23.3; → 220 Ω → J24.3.
- U2.1, U2.4 → GND; U2.9, U2.12 → GND; U2.8, U2.11 open.

**Buzzer**: `BUZ_IO` (U1B.17) → R11 1 k → Q1 base (`BUZ_B`); R12 10 k Q1 base → GND; Q1 emitter → GND; Q1 collector →
`BUZ_N` → BZ1 −; BZ1 + → +5V; D1 1N4148 (fitted): anode `BUZ_N`, cathode +5V. Q1 symbol pins 1 = E, 2 = B, 3 = C.

**Emergency button**: J20.3 `ESTOP` → R13 1 k → `ESTOP_IO` (U1B.18). No pull-down on the hub (v0.4; R14 retired).

**I²C**: J18.3 = `I2C_SCL` (U1A.15), J18.4 = `I2C_SDA` (U1A.12); 4.7 k each to +3V3.

**microSD (U6)**: CS = `SD_CS` (U1A.20), SCK = `SD_SCK` (U1A.18), MOSI = `SD_MOSI` (U1A.17),
MISO = `SD_MISO` (U1A.19), VCC = +5V, GND = GND.

---

## 6 · Bill of materials (hand-soldered; LCSC numbers are for footprints — buy locally where cheaper)

| Qty | Refs | Part | LCSC | Notes |
|---|---|---|---|---|
| 2 | J1, J2 | XT60UPB-M (male, vertical) | C2918078 | hole 4.0 mm |
| 1 | J4 | XT60UPB-F (female, vertical) | C9900007500 | library-only id; hole 4.0 mm |
| 1 | J5 | XT30UPB-M (male, vertical) | C428721 | hole 2.5 mm, pad 3.5 mm |
| 1 | J3 | JST VH 6-pin B6P-VH | C20088586 | + housing VHR-6N + 6 × SVH-21T crimps |
| 8 | J10–J17 | JST XH 8-pin B8B-XH-A(LF)(SN) | C157972 | + XHP-8 housings + crimps |
| 1 | J18 | JST XH 4-pin B4B-XH-A(LF)(SN) | C144395 | |
| 5 | J20–J24 | JST XH 3-pin B3B-XH-A(LF)(SN) | C144394 | |
| 2 | J25, J26 | JST XH 2-pin B2B-XH-A(LF)(SN) | C378939 | spare 5 V outputs (v0.4) + XHP-2 housings |
| 2 | U1 | 1×22 female header 2.54 mm, 8.5 mm | C46595983 | rows 25.40 mm apart |
| 1 | U6 | 1×6 female header 2.54 mm | C2832269 | SD module gets straight male pins |
| 1 | U2 | SN74HCT125N (DIP-14) | C154102 | already owned |
| 1 | — | DIP-14 IC socket | — | same footprint as U2; buy locally |
| 1 | Q1 | **2N2222A** NPN, TO-92 (user choice v0.4; footprint wired E-B-C) | footprint C50229519 | PN2222A = E-B-C fits as drawn; P2N2222A = C-B-E → turn it round (§3.5) |
| 1 | D1 | 1N4148 (DO-35) | C84412 | **fitted** (coil-buzzer flyback, §3.5) |
| 1 | BZ1 | 12 mm passive buzzer, 5 V, 7.6 mm pitch | footprint C9900016920 | buy locally |
| 1 each | LED1–LED3 | 5 mm LED red, blue, green | footprint C9900102778 | buy locally |
| 7 | R4, R5, R12, R41–R44 | 10 kΩ ¼ W axial | C1519255 | 74HCT125 input pull-downs, buzzer base, PROX pull-ups (v0.4: R14, R21–R27 retired) |
| 14 | — | 1 kΩ ¼ W axial | C1364484 | series, buzzer base, green LED |
| 11 | — | 220 Ω ¼ W axial | C3537012 | LED hops ×7, servos ×4 |
| 1 | — | 330 Ω ¼ W axial | C1369900 | LED data out |
| 2 | — | 2.2 kΩ ¼ W axial | C3454390 | red/blue LEDs |
| 2 | — | 4.7 kΩ ¼ W axial | C1371435 | I²C |
| 8 | C1, C3–C9 | 100 nF ceramic, 5 mm pitch | C342794 | C1 at the 74HCT125; C3–C9 on the MQ inputs |
| 0 (1) | C2 | 470 µF 16 V radial, 8 mm | C109066 | footprint only, **do not stuff by default**; fit only if servos reset the ESP (§9) |
| 4 | H1–H4 | M3 holes at the corners (plated, no net) | — | + 4 × M3 standoffs; SD module far end on a foam pad / hot glue |

All resistors are Yageo MFR-25 (1 %, ¼ W) footprints, 10.3 mm pitch. Search scripts:
`PCB design/scripts/hub/01…05_*.js` (read-only LCSC library queries, 2026-10-12). The search API returns no stock
figures, so check availability when ordering.

---

## 7 · Board layout — v0.4b re-placement (2026-10-26, user: "shorter board, too much empty space bottom-left")

> **Routed 2026-10-27 (H-005):** R5 (1890, 932.5) and R7–R10 (x 1660…2020, y 1840, vertical, under the DevKit) placed;
> routing finished around the user's tracks: +5V trunk 2.5 mm, small 5 V loads / +3V3 / BUZ_N 0.8 mm, signals 0.35 mm,
> GND fills both layers + 231 stitching vias, nothing in the antenna strip. Strict DRC 0 errors (track cap 5.08 mm).

> **Current (2026-10-26, after the direct-mount switch and H-002):** **77 footprints**, SB1–SB7 removed from PCB
> and schematic, no bypass silk; [full verification](SB-REMOVAL-2026-10-26.md). **132.6 × 99.1 mm**
> (x −620..4600, y 0..3900 mil). The user
> has since hand-placed parts; the live board, not `layout_plan.py`, is the reference. Local moves for J3 are listed in
> §2.4; check the live board with `scripts/hub/live_space.py` (inventory + project export). Do NOT re-run
> `21b_place_pcb.js` or `23_silkscreen.js`: both would undo the user's and the switch's positions.
>
> **v0.4b plan (superseded by the live board):** **116.8 × 99.1 mm (4600 × 3900 mil)**, floor plan in `scripts/hub/layout_plan.py` (`python layout_plan.py new
> --js` → `21b_place_pcb.js`; preview `scripts/hub/data/layout_new.png`, old one `layout_old.png`). Checks: overlaps (20 mil),
> board edge, M3 holes, LOW-only parts under the DevKit / in the USB lane / under the SD module, **METAL15** (XT, VH, buzzer,
> electrolytic ≥ 15 mm from the antenna, robot rule), DevKit rows 1000 mil. Left → right:
> - **Power column** on the left edge: J1 battery, J2 DC, J3 switch harness, J4 → buck, stacked; the vertical XT parts
>   take their cables from above, so no edge-facing needed. J1 sits 15 mm from the antenna.
> - **DevKit** U1A x 560 / U1B x 1560 (1000 mil), top flush, antenna over the top edge (unchanged rule).
> - **USB lane** below the DevKit (cable plug path, LOW parts only): R1–R3, R11–R13, R15, R16, D1.
> - **Middle column:** J20 E-stop + J18 I²C (top) · Q1, C2, BZ1 · servo J21–J24 (2 × 2) · J25, J26, LED3 · J5 ← buck,
>   LED1, LED2 · SD header U6; the module lies flat over C1, U2, R4–R10 and its **card end overhangs the bottom edge by
>   ≈ 5 mm** (the module sits 8.5 mm up on its header, like the antenna over the top edge).
> - **Station block** same cells (S1→S4 up column A x 3130, S5→S8 down column B x 4090); odd cells' SB moved to the
>   right of the cell (clear of the next cell's label). Silkscreen regenerated from pad positions (`23` / `25` / `24`: 0 hits).
> - Holes: 4 × M3 at (170, 170), (4430, 170), (170, 3730), (4430, 3730) mil.
>
> The v0.3 layout below is history.

### 7.0 v0.3 layout (placed 2026-10-12 by `scripts/hub/21_place_pcb.js`, superseded)

**132.1 × 99.1 mm (5200 × 3900 mil)**, 2 layers, 1.6 mm FR-4, 1 oz copper, HASL. Origin bottom-left.

```
┌──────────────── DevKit antenna overhangs the top edge ────────────────────┐
│ LED BAT · LED DC  │                 │ E-stop · I²C    │ S4 port │ S5 port │
│ ◄ XT60 battery    │                 │ buzzer · C2     │ S3      │ S6      │
│ ◄ XT60 DC jack    │  DevKit U1A/U1B │                 │ S2      │ S7      │
│   switch J3 (VH-6)│                 │ SD module flat  │ S1      │ S8      │
│ ◄ XT60 → buck     │                 │ over U2 + R4–R10│         │         │
│   servo 1–4       │  USB lane       │ + C1            │  cell = port + DHT pull-up/series,     │
│ ◄ XT30 ← buck     │  (low parts)    │                 │  MQ cap, LED hop, bypass (+ zone R)    │
│ LED 5V            │                 │ card slot ▼     │         │         │
└────────────────────────────────────────────────────────────────────────┘
```
- **Power column** (left edge): J1 battery, J2 DC, J4 → buck, J5 ← buck — all mating faces out of the left edge
  (verified from pad positions); J3 switch harness with its pin table in silkscreen; servo ports J21–J24 stacked
  beside them.
- **DevKit**: U1A (x 1050) and U1B (x 2050) rows exactly 1000 mil apart, pin 1 75 mil below the top edge, so the
  DevKit's top edge is flush with the board and the antenna overhangs it. Only R1/R2 under the DevKit; the USB lane
  below it is empty (low parts only).
- **Middle column**: J20 E-stop + J18 I²C (top), their resistors, buzzer group, C2; the SD header U6 with the module
  lying flat over U2 (74HCT125), C1 and R4–R10 (all lower than the 8.5 mm header), card slot at the bottom edge.
  **No SD support holes** (positions of the module's holes unknown, and parts sit under it) — fix its far end with a
  foam pad or a dab of hot glue.
- **Station block**: identical cells — S1→S4 up column A, S5→S8 down column B (= LED chain order); zones are
  neighbouring pairs Z1 S1+S2, Z2 S3+S4, Z3 S5+S6, Z4 S7+S8. Each cell: XH-8 port, DHT series 1 k, MQ 100 nF,
  LED hop 220 Ω, SB bypass; odd cells also carry their zone's pull-up + series. (v0.4: the DHT pull-up spots R21–R27
  and R14's spot in the middle column stay empty; J25 / J26 added (spare 5 V) — **84 footprints**.)
- **J25 / J26 (v0.4):** side by side at (2460, 1920) and (2880, 1920) mil, rot 0, pin 1 (5V) on the right, in the free strip
  between the DevKit and C2, just above the SD module (the left power column had room for only one). Silk: "5V" / "GND"
  above the pins, "SPARE 5V OUT"; "E B C" above Q1. Outline and silk lines are polylines (EasyEDA 3.2.206).
- **Holes**: 4 × M3 (3.2 mm plated, 6 mm pad, no net) at (170, 170), (5030, 170), (170, 3730), (5030, 3730) mil.
- **Silkscreen** (`23_silkscreen.js`, `25_designators.js`): every port's name and per-pin functions, station and zone
  labels, LED-chain order, bypass note, switch pin table, polarity marks, DevKit and SD outlines; checked clear of
  pads (`24_check_silk.js`, `26_check_designators.js`). v0.4: title "WRO MUSEUM HUB v0.4" and "E B C" at Q1's pads.
- Verified (`22_check_placement.js` + PCB DRC): no overlaps, nothing off the board, holes clear, DRC shows only the
  282 not-yet-routed pads of the 74 nets.

---

## 8 · Checks before ordering
1. Station proximity (IR module at 5 V + Schottky diode): with a 10 k pull-up to 3.3 V on the hub side, the line
   reads ≤ 0.5 V when an object is detected and 3.3 V otherwise; two stations in parallel still read correctly.
2. Rocker: which end tab joins the middle tab in position I (§2.3).
3. Print the PCB at 1:1 and place the real DevKit, SD module, XT60/XT30 plugs and an XH-8 plug on the paper.

## 9 · Bring-up
0. **Before soldering** (v0.4):
   - Ohmmeter across the buzzer: **10–50 Ω = a coil → fit D1** (1N4148, cathode to +5V); open (MΩ) = a piezo → fit a
     1 kΩ in D1's pads instead.
   - Q1: match the 2N2222A's datasheet pin-out to the silk "E B C" (P2N2222A is C-B-E: turn it round).
   - Each DHT11 module, unpowered: DOUT ↔ VCC should read 4.7–10 kΩ (its own pull-up). If one reads open, tack a 10 k
     across that module's VCC and DOUT pins (the hub has no DHT pull-ups since v0.4).
   - **Leave C2 empty.** Fit it only if the S3 resets when 3–4 servos start together.
   - Note the LDO marking on the YD board (CJ6107 or AMS1117, §1).
1. No DevKit, no stations. Battery or bench supply on J1 with the switch in O → nothing lit.
2. Position I → red LED; J4 shows battery voltage; DC pins dead. Position II with the adapter → blue LED; battery pins dead.
3. Check the buck alone: 5.0–5.2 V and correct polarity **before** plugging it into J5. Then green LED, 5 V on every port pin 1.
4. Insert the DevKit (USB flash works with the hub powered or not); 3.3 V on every port pin 3.
5. LED chain with S1 only, then add stations in order. Then each DHT and MQ (the `/scan` page: an empty port reads
   "floating", a fitted DHT "pulled_high"), each proximity zone (either station of a pair triggers it), servos, buzzer,
   emergency button (released = LOW, pressed = HIGH), SD card, I²C.

---

## 10 · Firmware migration (separate task)
| Function | Old WROOM pin | New S3 pin |
|---|---|---|
| DHT S1–S7 | 14, 16, 17, 27 (4 stations) | 15, 16, 17, 18, 38, 39, 40 |
| MQ-5 S1–S7 (AOUT ÷ 1.68 on the station) | 34, 35, 36, 39 (4 stations) | 4, 5, 6, 7, 1, 2, 10 |
| Proximity zones Z1–Z4 | — | 41, 42, 3, 48 |
| LED data | 15 | 45 |
| Servo | 13 | 46 |
| Buzzer | 4 | 47 |
| microSD SCK / MISO / MOSI / CS | 18 / 19 / 23 / 5 | 12 / 13 / 11 / 14 |
| Emergency button | 21 | 21 |
| I²C SDA / SCL | — | 8 / 9 |

Also needed: `board = esp32-s3-devkitc-1`, octal PSRAM (`qio_opi`), 16 MB flash; LED pixel order follows the
station chain; remove the old input-only-pin logic in `HubSensors` (the S3 has none).

Input modes the v0.4 board relies on (keep them in the migration):
- **DHT:** keep `OUTPUT_OPEN_DRAIN | PULLUP` for the read (`HubSensors.cpp`): with no hub pull-up, the internal one plus
  the module's own is all there is. `probeAttachment()` then reports empty ports as "floating" again.
- **E-stop GPIO21:** keep `INPUT_PULLDOWN` (`main.cpp`): it is now the only hub-side pull-down; required for the
  bare-button option.
- **PROX zones:** plain `INPUT` is enough (external 10 k pull-ups R41–R44); LOW = detected; debounce.

## 11 · Carried over from the working WROOM build
DHT on 3V3 (now via the station cable) · the IR proximity alarm (now on each station board) · 74HCT125 pin-by-pin wiring incl. the 100 nF
and the tied-off spare channels · 330 Ω LED / 4 × 220 Ω servo resistors · 2N2222 buzzer driver · parallel LED power ·
microSD on 5 V · emergency button on GPIO21.
Not carried over: the second buck (one 10 A buck now) and the fuses.
New on the carrier because the topology is new (v0.4 review, kept): the 7 × 220 Ω LED hops (3 m hops through the hub,
§3.2), 1 k series on every GPIO that leaves on a station / E-stop cable, the PROX pull-ups (§3.3), and D1 (an NPN switch
instead of a push-pull GPIO drives the buzzer, §3.5).
