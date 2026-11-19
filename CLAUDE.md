# WRO Smart Museum — project rules for AI agents

WRO 2026 "Robots Meet Culture" museum-guide project: a guide **robot** (ESP32-S3, RFID stops, DFPlayer narration,
L298N rail drive, ESP32-CAM photos, Mega face display) and a **hub** (ESP32-S3 dashboard, station sensors, LEDs, emergency),
linked by ESP-NOW. This file covers the **PCB work** in `PCB design/`; firmware lives in `firmware/`, `ESP32_RFID_MP3_RAIL/`.

## Read first (PCB work, in this order)
1. `PCB design/README.md`: shared handoff (bridge pipeline, hub + station status, API lessons, session log).
2. The board you work on:
   - **Robot:** `PCB design/Robot/README.md` (handoff, **resume from "Next steps"**), `WIRING-PLAN.md` (**spec v0.6 = source of
     truth**), `DECISIONS.md` (why every choice was made — don't re-litigate without new facts).
   - **Hub:** `PCB design/hub/WIRING-PLAN.md` (v0.4 approved). **Station:** `PCB design/station/STATION-SPEC.md` (v0.4, routed 2026-10-26).
3. The **`pcb-pipeline` skill** (`C:\Users\tahfi\.claude\skills\pcb-pipeline\PLAYBOOK.md`): standard process, gates,
   conventions, API lessons. The `easyeda-api` skill is the API reference — never guess an `eda.*` method.
   Reference project with the full pipeline (ordered board): `D:\projects\attendance_project\PCB\`.

## Hard rules
- **Plan before building**; ask the user about unknown physical facts; update the spec **before** changing scripts.
- **One EasyEDA window**, and the right project before any write: robot = **WRO_Robot** (`PCB design/Robot/WRO_Robot.eprj2`),
  hub = **WRO_HUB** (`PCB design/hub/WRO_HUB.eprj2`), station = **station_pcb** (`PCB design/station/station_pcb.eprj2`). Every writing script checks
  `eda.dmt_Project.getCurrentProjectInfo()` and aborts on another project; `PCB_V01` (attendance) is never touched here.
- Keep the EasyEDA window **visible** while scripts write (hidden windows freeze timers; wire creates don't commit).
- Scripts live in `PCB design/scripts/<board>/` (numbered; header says what they do and whether they write). Idempotent,
  time-boxed (≤ 20 s), batched, no same-run retries. Run: `node tools/easyeda-bridge/eda.mjs exec -f "<script>"`.
- The robot board is **100 % through-hole** (user rule, D-013): no SMD footprint may enter its spec or schematic.
- GUI-only steps belong to the user: Import Changes → Apply, visual reviews, ordering.
- After every finished step update the board README (status + session log), `DECISIONS.md` and the spec; add new API
  lessons to the playbook §7.
