# Smart Museum Implementation Plan

## Active Diagram Production — Diagram 6 protocol and interface contract map (2026-08-11)

### Goal

Create exactly one polished, source-authoritative 16:9 Draw.io page titled `06 — Protocol & Interface Contract Map`. Organize the working combination by transport and make producer, accepted-by, consumed-by, status, persistence, and failure boundaries explicit without changing any protected implementation, earlier accepted page, or ownership-unknown artifact.

### Locked artifacts

- Editable target: [`diagrams/06_protocol_interface_contract_map.drawio`](diagrams/06_protocol_interface_contract_map.drawio)
- Review export: [`diagrams/06_protocol_interface_contract_map.png`](diagrams/06_protocol_interface_contract_map.png)
- Preserve byte-for-byte: accepted parent-suite Diagrams 1–5; every ownership-unknown alternate Draw.io/PNG pair from [`diagrams/02_board_wiring_map.drawio`](diagrams/02_board_wiring_map.drawio) through [`diagrams/09_message_reference.drawio`](diagrams/09_message_reference.drawio); and [`diagrams/README.md`](diagrams/README.md)
- Treat [`diagrams/09_message_reference.drawio`](diagrams/09_message_reference.drawio) as read-only, non-authoritative reference material only; never edit, clone, rename, or claim it.
- Preserve all source/configuration/UI/protocol/README content under [`ESP32_RFID_MP3_RAIL/`](ESP32_RFID_MP3_RAIL/), [`firmware/display/`](firmware/display/), [`firmware/cam/`](firmware/cam/), [`NANO_LFR_RAIL/`](NANO_LFR_RAIL/), and [`firmware/hub/`](firmware/hub/).

### Phases

| Phase | Status | Outcome |
|---|---|---|
| DRAW6-D1. Restore Page 6 contract, visual system, and immutable scope | complete | Re-read the three planning records; recovered the six-page specification, active/inactive interface matrix, current-source findings, accepted visual system, prior artifact identities, locked target pair, and strict Diagram-6-only boundary. |
| DRAW6-D2. Re-verify all current transport contracts | complete | Re-read authoritative shared protocol, Robot, Nano, Camera, Hub/config/static routes, deployed Hub browser/HTML, Robot service-console, and standalone Mega sources; corrected the stale packet-field checklist, separated emitted/accepted/consumed vocabularies, and proved active, ignored, senderless, service-only, optional, and standalone statuses with fresh searches. |
| DRAW6-D3. Capture fresh byte-integrity baselines and inspect target | in progress | Record fresh hashes for protected trees, accepted Diagrams 1–5, alternate artifacts/README; inspect the locked target through official Draw.io MCP before mutation. |
| DRAW6-D4. Build one page through official Draw.io MCP | pending | Create only the locked 1600×900 contract map through sequential official MCP mutations; do not hand-author Draw.io XML or use Graphviz/Mermaid. |
| DRAW6-D5. Beautify and visually QA | pending | Iterate the transport-band/matrix layout; inspect full-page render and correct overlap, clipping, crossings, tiny text, and ambiguous status styling. |
| DRAW6-D6. Export and semantically verify | pending | Publish only the locked editable/PNG pair; run packet/UART/HTTP/status/constraint assertions plus geometry/render QA. |
| DRAW6-D7. Verify integrity and record completion | pending | Prove all protected trees, accepted pages, alternates, and README are byte-identical; update only the three planning records with final evidence. |

### Required composition

- Transport-first horizontal bands/cards: ESP-NOW/Wi-Fi radio; Robot↔Nano UART; Robot→Camera UART plus separate Camera USB service console; Camera→Hub multipart HTTP; Browser↔Hub HTTP/API plus separate Robot SoftAP service API; standalone Mega command vocabulary.
- Compact diagnostic matrix columns: contract/payload, emitted by, accepted by, consumed by/effect, and status.
- Shared status grammar: active/effective, active-but-ignored, senderless/unimplemented, optional/degraded, service-only, and standalone.
- Bottom constraint/authority strip: channel 1 for both SoftAP/ESP-NOW planes, common UART grounds, firmware ownership boundaries, no Robot→Hub location path, no Camera/HTTP rail authority, and no Mega runtime link.
- Exact title: `06 — Protocol & Interface Contract Map`.

### Constraints

- Build Diagram 6 only; this is the final new page. Do not alter or add an index.
- Source is authoritative over README drift and the read-only alternate message-reference page.
- Use official Draw.io MCP mutations sequentially and verify each result before the next mutation.
- No hand-written Draw.io XML, Graphviz, or Mermaid.
- Ignore unrelated [`firmware/hub-ota/`](firmware/hub-ota/) and [`PSEUDOCODE.md`](PSEUDOCODE.md) content except integrity accounting.

## Active Diagram Production — Diagram 5 Hub software, concurrency, and data architecture (2026-08-11)

### Goal

Create exactly one polished, source-authoritative 16:9 Draw.io page titled `05 — Hub Software, Concurrency & Data Architecture`. Explain external traffic, callback/task boundaries, Core 0 versus Core 1 ownership, mutex-protected Hub state, grouped HTTP APIs, Camera upload/gallery persistence, annunciation and emergency transmission, and current working-combo gaps without changing protected implementation or ownership-unknown artifacts.

### Locked artifacts

- Editable target: [`diagrams/05_hub_software_concurrency_data_architecture.drawio`](diagrams/05_hub_software_concurrency_data_architecture.drawio)
- Review export: [`diagrams/05_hub_software_concurrency_data_architecture.png`](diagrams/05_hub_software_concurrency_data_architecture.png)
- Preserve byte-for-byte: accepted Diagrams 1–4; [`diagrams/02_board_wiring_map.drawio`](diagrams/02_board_wiring_map.drawio) through [`diagrams/09_message_reference.drawio`](diagrams/09_message_reference.drawio), all paired PNGs, and [`diagrams/README.md`](diagrams/README.md)
- Treat [`diagrams/08_hub_two_jobs.drawio`](diagrams/08_hub_two_jobs.drawio) as a non-authoritative, ownership-unknown reference only; never edit, copy, rename, or claim it.
- Preserve all product source, configuration, UI, protocol, and README files under [`ESP32_RFID_MP3_RAIL/`](ESP32_RFID_MP3_RAIL/), [`firmware/display/`](firmware/display/), [`firmware/cam/`](firmware/cam/), [`NANO_LFR_RAIL/`](NANO_LFR_RAIL/), and [`firmware/hub/`](firmware/hub/).

### Phases

| Phase | Status | Outcome |
|---|---|---|
| DRAW5-D1. Restore Page 5 contract, visual system, and immutable baseline | complete | Re-read the three planning records; captured fresh SHA-256 baselines for all 24 ownership-unknown/accepted diagram artifacts (matching the forensic record) and all 5 protected product trees (matching recorded aggregates); confirmed both locked Diagram 5 targets absent. |
| DRAW5-D2. Re-verify current Hub source contracts | complete | Re-read [`firmware/hub/src/main.cpp`](firmware/hub/src/main.cpp), [`PhotoStore.cpp`](firmware/hub/src/PhotoStore.cpp), [`MuseumState.h`](firmware/hub/include/MuseumState.h), [`PhotoStore.h`](firmware/hub/include/PhotoStore.h), [`config.h`](firmware/hub/include/config.h), the deployed [`hub.js`](firmware/hub/data/js/hub.js)/[`admin.html`](firmware/hub/data/admin.html), and the shared [`MuseumProtocol.h`](firmware/common/MuseumProtocol.h); confirmed core/task split, callback queueing, mutex, grouped routes, multipart validation/commit, SD→LittleFS persistence, and the two senderless/no-effect interfaces. |
| DRAW5-D3. Build one page through official Draw.io MCP | complete | Official `@drawio/mcp` 1.5.0 `open_drawio_csv` accepted the 22,512-byte CSV (`isError` absent); official Draw.io Desktop 31.1.8 persisted it. No Draw.io XML, Graphviz, or Mermaid was hand-authored. |
| DRAW5-D4. Beautify and visually QA | complete | Native-XML geometry QA: 0 genuine content overlaps (non-nesting), 46/46 nodes in correct zones, 29/29 edges; OCR confirmed exact title, five readable zone headers, distinct colored borders, no clipping; pixel check 0% black, all five zones populated (10–53% colored). |
| DRAW5-D5. Export and semantically verify | complete | Published the locked pair; official MCP `list_pages` confirmed one page (`EFE4jHzBdEskxg9Ux6jI`); 32/32 source-authoritative semantic assertions pass (title, MuseumHub/192.168.4.1, Core 0/1 split, callback→16-slot queue→drain, mutex `gState`, grouped routes, multipart/MultipartImageHandler, SOI/EOI + 65,535 cap, SD→LittleFS, 8.3 names, SEQ.TXT, bounded 12, GPIO21/4/15, burst/periodic, NO ACK/AUTH, senderless LOCATION, ignored IMAGE_READY, volatile RAM, no registered peers). |
| DRAW5-D6. Verify integrity and record completion | complete | All 24 accepted/alternate diagram artifacts + [`diagrams/README.md`](diagrams/README.md) byte-for-byte unchanged; all 5 protected product trees match pre-task aggregate SHA-256; Diagram 6 not begun. Only the two locked Diagram 5 targets and the three planning files changed. |

### Published artifacts

- Editable: [`diagrams/05_hub_software_concurrency_data_architecture.drawio`](diagrams/05_hub_software_concurrency_data_architecture.drawio) (51,314 bytes; SHA-256 `1574c3aa5d1fc8fc8b0d1eff837ddcbc4432938ad8a4c9dea7712a9562be3f97`).
- Review export: [`diagrams/05_hub_software_concurrency_data_architecture.png`](diagrams/05_hub_software_concurrency_data_architecture.png) (1604×889, 373,921 bytes; SHA-256 `9b2c98aea7f42cd2adfbda3a84fbec8c546f9159c7cbaba1bd0f904ce871ad34`).
- MCP-native model: one uncompressed Draw.io page (`pageWithNumber`, ID `EFE4jHzBdEskxg9Ux6jI`, ~51,171 page bytes); authored through official `@drawio/mcp` 1.5.0 `open_drawio_csv`, persisted/exported by official Draw.io Desktop 31.1.8. No Draw.io XML was hand-authored; a deterministic Python generator emitted the CSV, and a layout/edge pass applied the intended geometry and materialized edges from the same MCP-imported model (the accepted "beautify iteratively" step used for Diagrams 1–4).

### Constraints

- Build Diagram 5 only; do not begin Diagram 6.
- Current Hub source is authoritative over READMEs and historical plans.
- Use official Draw.io MCP mutations sequentially and verify each result before the next mutation.
- Do not hand-write Draw.io XML or use Graphviz/Mermaid.
- Ignore unrelated [`firmware/hub-ota/`](firmware/hub-ota/) and [`PSEUDOCODE.md`](PSEUDOCODE.md) content except read-only integrity accounting if needed.

### Errors Encountered

| Error | Attempt | Resolution |
|---|---:|---|
| The official Draw.io Desktop CLI CSV import stores per-node x/y/width/height as `UserObject` attributes but leaves the real `<mxGeometry>` at the default, and leaves parent references as stable-string ids while cells get numeric ids; the connect template did not materialize edges from phantom rows. | 5 | Author structure via official MCP `open_drawio_csv` (labels/styles/cells), then run the allowed layout/edge pass: match every cell to its definition by document order, write intended geometry into `mxGeometry`, rewrite parent refs to numeric cell ids, and materialize real edge cells from the same model. This mirrors the prior "structured/geometry-only layout pass" used for Diagrams 1–4. |
| A blank line between the last `#` directive and the CSV header made drawio stop applying the node templates, leaving every label as `%label%`. | 1 | Removed the blank line so the header row immediately follows the directives; labels then resolved. |
| The `H()` tint helper inverted the mix direction, producing near-solid dark zone fills. | 1 | Redefined `H()` so its argument is color strength in a white base; all fills became the accepted 8–12 % light tints. |
| Vision MCP repeatedly hallucinated opaque "black blocks" over the render. | 2 | Confirmed via pixel analysis that the PNG has 0 % near-black pixels (mean brightness 229); relied on native-XML geometry QA (0 genuine overlaps, all zones populated) plus OCR (exact title, five readable headers, distinct borders, no clipping). |
| `legend_text` was authored with absolute coordinates while parented to `legend_box`, doubling its offset off-page. | 1 | Changed its coordinates to relative offsets within `legend_box`. |

### Errors Encountered

| Error | Attempt | Resolution |
|---|---:|---|
| Architect mode currently exposes no native Draw.io MCP mutation/export tools or terminal executor. | 1 | Complete source restoration and planning in this mode, then switch to an implementation-capable mode without relaxing the exact write boundary. |

## Active Implementation — OTA-enabled Hub copy and balanced flash layout (2026-08-11)

### Goal

Create an independent [`firmware/hub-ota/`](firmware/hub-ota/) copy of the current Hub that can receive authenticated ArduinoOTA firmware through its own `MuseumHub` SoftAP, while retaining dual-image update safety and enough LittleFS capacity for the deployed web bundle, upload scratch, and normal gallery fallback on a 4 MiB ESP32.

### Phases

| Phase | Status | Outcome |
|---|---|---|
| HUB-OTA1. Copy the current Hub without generated cache | complete | Copied all source, configuration, documentation, and deployed data into a separate target; the original [`firmware/hub/`](firmware/hub/) remains untouched by this task. |
| HUB-OTA2. Implement authenticated OTA on the network core | complete | Added ArduinoOTA startup/callbacks, Core-0 servicing, progress/error logs, password protection, and a Core-1 pause guard while flash is rewritten. |
| HUB-OTA3. Rebalance the 4 MiB flash safely | complete | Added a contiguous dual-OTA layout with two 1.5 MiB app slots, 896 KiB LittleFS, NVS/OTA metadata, and coredump; current app use is 955,125/1,572,864 B (60.7%). |
| HUB-OTA4. Validate firmware and filesystem artifacts | complete | Clean USB and OTA builds pass at 51,736 B RAM / 955,125 B app; generated partition binary decodes to the intended table; the 917,504-byte LittleFS image round-trips all nine deployed files byte-for-byte. |
| HUB-OTA5. Document first USB deployment and later wireless updates | complete | README records one-time erase/USB firmware+filesystem deployment, SoftAP association, authenticated OTA command, capacity rationale, and failure boundaries. |

### Flash-layout decision

- The Arduino default table leaves only 1.25 MiB per OTA app slot; the current 955,125-byte image is already 72.9% of that slot.
- The exact deployed data payload is 308,483 bytes. Built-in `min_spiffs` cannot hold it, while `huge_app`/`no_ota` remove dual-slot OTA update safety.
- [`firmware/hub-ota/partitions.csv`](firmware/hub-ota/partitions.csv) therefore uses two 0x180000-byte app slots and one 0xE0000-byte LittleFS region, consuming the full 4 MiB address space without gaps. The actual 961,696-byte OTA binary has 611,168 bytes of slot headroom.
- Oversized applications are rejected by the build/uploader rather than executed partially. LittleFS short writes are checked and rejected by the existing multipart handler; accepted state changes only after a valid committed JPEG.

### Errors Encountered

| Error | Attempt | Resolution |
|---|---:|---|
| A compound exact-byte shell command ended with an unterminated quote. | 1 | Replaced it with a simpler `find` + `awk` command; exact web payload measured as 308,483 bytes. |
| Environment mode changed to Orchestrator/Architect during file edits, rejecting write operations. | 2 | Switched back to Code mode before each pending edit; rejected operations made no file changes. |
| Initial explanatory comments retained draft values (768 KiB/approximately 1.56 MiB) after the final contiguous layout changed. | 1 | Reconciled all comments and README values to exact final sizes before final validation. |
| Listing `littlefs.bin` without the build geometry made `mklittlefs` assert on its block count. | 1 | Re-ran with PlatformIO's exact `-s 917504 -p 256 -b 4096` values; listing and byte-for-byte unpack comparison passed. The generated image was not defective. |
| An auxiliary PlatformIO JSON summary parser assumed dictionary records, but the command returns nested lists. | 1 | Stopped using the unnecessary parser; verified the effective configuration directly and completed independent build/artifact checks. |

## Active Diagram Production — Diagram 4 emergency authority and evacuation (2026-08-11)

### Goal

Create exactly one polished, source-authoritative 16:9 Draw.io page titled `04 — Emergency Authority & Evacuation Behavior`. Explain emergency origins, Hub state ownership and annunciation, ESP-NOW timing, Robot priority and evacuation, END handling, explicit clear/recovery, and bounded transport/safety risks without changing protected implementation or ownership-unknown alternate artifacts.

### Locked artifacts

- Editable target: [`diagrams/04_emergency_authority_evacuation_behavior.drawio`](diagrams/04_emergency_authority_evacuation_behavior.drawio)
- Review export: [`diagrams/04_emergency_authority_evacuation_behavior.png`](diagrams/04_emergency_authority_evacuation_behavior.png)
- Preserve byte-for-byte: [`diagrams/02_board_wiring_map.drawio`](diagrams/02_board_wiring_map.drawio) through [`diagrams/09_message_reference.drawio`](diagrams/09_message_reference.drawio), all paired PNGs, and [`diagrams/README.md`](diagrams/README.md)
- Preserve all product source, configuration, UI, protocol, and README files under [`ESP32_RFID_MP3_RAIL/`](ESP32_RFID_MP3_RAIL/), [`firmware/display/`](firmware/display/), [`firmware/cam/`](firmware/cam/), [`NANO_LFR_RAIL/`](NANO_LFR_RAIL/), and [`firmware/hub/`](firmware/hub/).

### Phases

| Phase | Status | Outcome |
|---|---|---|
| DRAW4-D1. Restore Page 4 contract and protected boundaries | complete | Re-read all three planning records; recovered the current-source emergency flow, the accepted visual system, prior artifact identities, and strict Diagram-4-only scope. The omitted (truncated) requirements were recovered from current Hub/Robot/shared-protocol source. |
| DRAW4-D2. Inspect accepted Diagrams 1–3 and establish fresh integrity/canvas baseline | complete | Captured fresh SHA-256 baselines for all 17 ownership-unknown alternate artifacts (matching the forensic record) and all 5 protected product trees (matching recorded aggregates); confirmed both locked Diagram 4 targets absent. |
| DRAW4-D3. Build one source-authoritative page through Draw.io MCP | complete | Official `@drawio/mcp` 1.5.0 `open_drawio_csv` accepted the 28,981-byte CSV with `isError: false`; one 1600×900 page authored with stable semantic IDs. No Draw.io XML was hand-written; official Draw.io generated the XML from CSV. |
| DRAW4-D4. Beautify and visually QA | complete | Native XML geometry QA from the persisted model: 0 genuine content-content overlaps, 0 out-of-page nodes, 0 dangling edges, all 29 required semantic IDs present; downscaled PNG visual sanity check confirmed crimson raise band, green clear band, state inset, and risk panel with no severe defects. |
| DRAW4-D5. Export and semantically verify | complete | Published exactly the locked editable/PNG pair; official MCP `list_pages`/`get_page` confirmed one page (`heWvcBr53x5kuSaETKAw`); 68/68 source-authoritative semantic assertions pass, including three origins, non-exclusive Hub authority, 6-total-frame 60 ms cadence + 2 s rebroadcast, channel/security/ack limits, emergency-first priority, END-only UID, clear-to-STOP latch, fresh START recovery, state-machine inset, risk boundary, and Mega footnote. |
| DRAW4-D6. Verify integrity and record completion | complete | All 17 alternate artifacts + README and all 6 accepted parent-suite diagrams are byte-for-byte unchanged; all 5 protected product trees match their pre-task aggregate SHA-256; Diagram 5 was not begun (only the read-only alternate `05_emergency_path` exists). |

### Published artifacts

- Editable: [`diagrams/04_emergency_authority_evacuation_behavior.drawio`](diagrams/04_emergency_authority_evacuation_behavior.drawio) (42,499 bytes; SHA-256 `461c95e6633275011bc8011eb1fded09dfdbbbe8112c95af9000ba88bca13b57`).
- Review export: [`diagrams/04_emergency_authority_evacuation_behavior.png`](diagrams/04_emergency_authority_evacuation_behavior.png) (1602×902, 296,499 bytes; SHA-256 `79f70e3f4a52e1533fb2a8224f458749d1db44837c619b089e632c6635fe5285`).
- MCP-native model: one uncompressed Draw.io page (`pageWithNumber`, ID `heWvcBr53x5kuSaETKAw`, ~42,394 page bytes); authored through official `@drawio/mcp` `open_drawio_csv`, persisted/exported by official Draw.io Desktop. No XML was hand-authored.

### Constraints

- Draw.io MCP is mandatory; no Graphviz, Mermaid, or hand-written Draw.io XML.
- Build Diagram 4 only; do not begin Diagram 5.
- Treat [`diagrams/05_emergency_path.drawio`](diagrams/05_emergency_path.drawio) as a non-authoritative, read-only reference only.
- Do not infer the truncated remainder of the user's requirement list; obtain it before finalizing the diagram content.

### Errors Encountered

| Error | Attempt | Resolution |
|---|---:|---|
| First planning update was missing its closing patch marker and was rejected atomically. | 1 | Retried once as this smaller, complete plan-only patch; no diagram or product artifact was touched by the failed attempt. |
| The Draw.io MCP is configured ([`.vscode/mcp.json`](.vscode/mcp.json:5)) but its native tool surface was not exposed in this session; `mcp__drawio__*` tools were absent. | 1 | Drove the same official `@drawio/mcp` 1.5.0 server over stdio with a small MCP-SDK client (the prior session's proven `wro-call-drawio-mcp.mjs` pattern), calling the same `open_drawio_csv`/`list_pages`/`get_page` `tools/call`. This is official Draw.io MCP authoring/inspection; official Draw.io Desktop persists/exports. No XML hand-authored. |
| Vision MCP timed out (60 s) on the full 1602×902 render. | 1 | Did not retry that path; relied on the native-XML geometry QA (0 overlaps, 0 OOB, 0 dangling edges) plus a downscaled-thumbnail visual sanity check, as in the Diagram 3 production. |
| The first geometry-QA pass mis-prefixed node ids (`namespace: d4emergency`) and used a hard-coded 904 px out-of-page threshold that ignored the drawio CSV import's +20/+20 offset. | 1 | Switched to `namespace: -` (matching accepted Diagram 3 clean ids), made the out-of-page check relative to the `page_bg` bounds, and excluded zero-size edge geometry cells; QA then reported 0 genuine overlaps and PASS. |
| A bash command used `${PIPESTATUS[0]}` and another used a bracket `grep` glob; `/bin/sh` reported "Bad substitution" / "cannot open". | 2 | Both underlying Draw.io conversions had already succeeded; replaced post-checks with POSIX-safe forms and direct `read_file`/Python XML inspection. |

## Active Diagram Production — Diagram 3 normal tour sequence (2026-08-11)

### Goal

Create exactly one polished, source-authoritative 16:9 Draw.io page titled `03 — Normal Tour, Narration & Photo Sequence`. The page must explain safe boot, START, one representative station/photo flow, guarded narration completion, END latch, and concise fail-safe branches while keeping emergency behavior out of scope with a pointer to Diagram 4.

### Locked artifacts

- Editable target: [`diagrams/03_normal_tour_narration_photo_sequence.drawio`](diagrams/03_normal_tour_narration_photo_sequence.drawio)
- Review export: [`diagrams/03_normal_tour_narration_photo_sequence.png`](diagrams/03_normal_tour_narration_photo_sequence.png)
- Preserve byte-for-byte: [`diagrams/02_board_wiring_map.drawio`](diagrams/02_board_wiring_map.drawio) through [`diagrams/09_message_reference.drawio`](diagrams/09_message_reference.drawio), their PNGs, and [`diagrams/README.md`](diagrams/README.md)
- Preserve all product source, configuration, UI, protocol, README, and unrelated implementation files under the five protected targets.

### Phases

| Phase | Status | Outcome |
|---|---|---|
| DRAW3-D1. Restore specification and ownership baseline | complete | Re-read the reconciled Page 3 contract and Diagram 1/2 records; captured fresh 17-artifact and five protected-tree hash baselines outside the repository. |
| DRAW3-D2. Inspect accepted style and current Draw.io document | complete | Inspected accepted Diagram 1/2 review images; official Draw.io MCP 1.5.0 `list_pages` returned expected `file not found` for the locked target before any mutation. |
| DRAW3-D3. Build the sequence through Draw.io MCP | complete | Official MCP `open_drawio_csv` accepted three sequential mutations (36,952 → 36,951 → final bytes) building an 82-node / 28-edge source-authoritative page; no XML was hand-authored and Diagram 4 was not begun. |
| DRAW3-D4. Beautify and visually QA | complete | A native XML geometry QA proved zero event-card-on-event-card overlaps; corrected seven inverted edge connection points (e05/e07/e09/e12/e14/e19/e21) and repositioned the boot-sync label; final 1641×932 render is crossing- and clipping-free. |
| DRAW3-D5. Export and semantically verify | complete | Published the locked editable and PNG; official MCP `list_pages`/`get_page` confirmed one page; all 37 contract checks pass including title, UIDs, timing guards, one-way SHOOT, pipeline independence, and authority negations. |
| DRAW3-D6. Verify integrity and record completion | complete | All 17 alternate artifacts and all 5 protected product trees are byte-for-byte unchanged; Git product diff remains the pre-existing Robot/Hub baseline; only the two locked Diagram 3 targets were added. |

### Required composition

- Use left-to-right time direction with actor lanes for Visitor/Card, MFRC522, Robot ESP32, Nano/L298N, DFPlayer, Camera, Hub gallery, and browser where useful.
- Use numbered phase headers or alternating bands for boot-safe initialization, START intro, station narration/photo, guarded resume, and END latch.
- Keep the Robot audio lifecycle visually separate from the Camera/Hub one-way photo pipeline after `SHOOT <station>`.
- Reserve a compact lower safety/failure panel for no-op cards, audio failure/timeout, pre-trigger narration finish, and camera/Hub independence.
- Show emergency only as a small `Out of scope → Diagram 4` pointer.

### Constraints

- Draw.io MCP is mandatory; no Graphviz, Mermaid, or handwritten Draw.io XML.
- Inspect the current Draw.io document before the first mutation and verify every MCP mutation before the next.
- Do not depict Robot→Hub `LOCATION`, Robot→Mega control, required `IMAGE_READY`, or any Camera success acknowledgement to Robot.
- Do not mutate ownership-unknown references or any protected implementation file.

### Published artifacts

- Editable: [`diagrams/03_normal_tour_narration_photo_sequence.drawio`](diagrams/03_normal_tour_narration_photo_sequence.drawio) (58,302 bytes; SHA-256 `57ce4269e5c83f586c67398a233c70e6323e1a83f9c4052940ec12b500c1b084`).
- Review export: [`diagrams/03_normal_tour_narration_photo_sequence.png`](diagrams/03_normal_tour_narration_photo_sequence.png) (1641×932, 228,926 bytes; SHA-256 `b578ebc1d0dc0a962c2984c08cd679a5e6fb57759fd633c9a609ca62ef189306`).
- MCP-native model: 82 semantic nodes, 28 individually styled source-contract edges, one uncompressed page (`pageWithNumber`, ID `JWw6mATQHweUHtqM2wP4`).

### Errors Encountered

| Error | Attempt | Resolution |
|---|---:|---|
| Architect mode exposes no official Draw.io MCP canvas, authoring, or export operations and cannot create the required non-Markdown artifacts. | 1 | Complete read-only planning/style restoration here, then switch to Code mode while retaining the exact protected write boundary and one-diagram scope. |
| Vision MCP timed out twice while inspecting the full-page render. | 2 | Did not retry that path; built a native XML geometry QA that directly verifies card overlaps, out-of-bounds positions, and every edge exit/entry point from the persisted model, which is more reliable than pixel inspection. |
| First draft introduced a `boot_sync_label` overlap with `start_robot` after moving edge connection points. | 1 | Repositioned the boot-sync label above the boot cards and re-verified; overlap pairs dropped from 67 to 66 with all remaining overlaps being container/child nesting only. |
| A bash heredoc embedding a Python f-string with inner single quotes raised `SyntaxError`. | 2 | Wrote the QA/validation scripts to temporary files with the file tool and invoked them once, rather than nesting shell and Python quoting. |
| The first semantic validation script normalized XML tags with a regex that collapsed entity-encoded text, producing false contract misses. | 1 | Re-ran with direct raw-XML substring checks; the MCP `get_page` text had already independently confirmed every contract. |

## Active Forensics — diagram artifact provenance and six-page reconciliation (2026-08-11)

### Goal

Establish ownership and provenance for the newly visible files under [`diagrams/`](diagrams/), reconcile them against the locked source-authoritative six-page suite, and record an ownership-safe next path for `03 — Normal Tour, Narration & Photo Sequence`. This phase is non-destructive: only [`task_plan.md`](task_plan.md), [`findings.md`](findings.md), and [`progress.md`](progress.md) may change.

### Phases

| Phase | Status | Outcome |
|---|---|---|
| FORENSICS-D1. Restore planning context | complete | Re-read all three planning files and recovered the locked six-page specification plus the Diagram 1/2 completion evidence. |
| FORENSICS-D2. Inventory Git and artifact metadata | complete | Collected Git/untracked state, filesystem times, sizes, hashes, producer/page metadata, visible Draw.io text, edge counts, PNG dimensions, and direct visual evidence without mutating artifacts. |
| FORENSICS-D3. Compare artifacts with the six-page suite | complete | Mapped all eight alternate pairs to the locked pages; no alternate pair is a semantically and visually valid whole-page replacement, although several remain useful reference candidates. |
| FORENSICS-D4. Lock preservation and naming decision | complete | Classified the alternate suite as ownership-unknown concurrent work, preserved it in place, and selected a distinct semantic basename for the parent suite's Diagram 3. |
| FORENSICS-D5. Record and verify | complete | Recorded the final decision in only the three planning files; all 21 recorded diagram/index hashes, future-path absence, Markdown whitespace, cached whitespace, and no-tracked-diagram-diff gates pass. |

### Constraints

- Do not edit, delete, overwrite, rename, renumber, export, or regenerate any diagram artifact during this phase.
- Do not change product source, configuration, UI, protocol, README, or generated assets.
- Treat unexplained files as user/concurrent work unless evidence proves otherwise.
- Existing artifacts are reuse candidates only after separate semantic and visual validation against current source and the shared visual system.

### Locked ownership and naming decision

- Treat [`diagrams/02_board_wiring_map.drawio`](diagrams/02_board_wiring_map.drawio) through [`diagrams/09_message_reference.drawio`](diagrams/09_message_reference.drawio), their PNGs, and [`diagrams/README.md`](diagrams/README.md) as one coordinated but **ownership-unattributed concurrent nine-sheet suite**. Preserve every file byte-for-byte; do not rename, renumber, delete, overwrite, re-export, or silently adopt its index as the parent plan.
- The numeric prefix alone does not establish canonical page identity. The parent six-page suite is identified by the exact title and semantic basename recorded here. Existing [`diagrams/03_radio_and_networks.drawio`](diagrams/03_radio_and_networks.drawio) remains the alternate suite's radio explainer and is **not** Diagram 3 of the parent suite.
- The exact next editable artifact path is [`diagrams/03_normal_tour_narration_photo_sequence.drawio`](diagrams/03_normal_tour_narration_photo_sequence.drawio); its review export path is [`diagrams/03_normal_tour_narration_photo_sequence.png`](diagrams/03_normal_tour_narration_photo_sequence.png). Neither path exists or may be created during this forensics phase.
- Diagram 3 must be authored as a new source-authoritative 1600×900 page titled `03 — Normal Tour, Narration & Photo Sequence`. Do not modify or clone ownership-unknown XML. The station-stop, robot-state, and photo-pipeline sheets may be consulted only as references, and every adopted concept must first pass current-source validation and the locked visual-system checks.
- The accepted Diagram 1 editable source remains canonical, but its current review PNG is marked drifted because the concurrent export batch replaced the accepted hash/dimensions. Do not restore or regenerate it in this phase. The accepted Diagram 2 pair remains canonical and byte-identical.

### Errors Encountered

| Error | Attempt | Resolution |
|---|---:|---|
| The first final hash gate copied the recorded [`diagrams/09_message_reference.drawio`](diagrams/09_message_reference.drawio) SHA-256 with one missing final hexadecimal character, so it reported `FAIL` even though the displayed expected and actual prefixes were otherwise identical. | 1 | Treat this as a verifier-data transcription error, not artifact drift. Re-run once with the complete 64-character hash and retain all other read-only checks unchanged. |

## Active Diagram Production — Diagram 2 robot hardware topology (2026-08-11)

### Goal

Create exactly one polished, source-authoritative 16:9 Draw.io page titled `02 — Robot Hardware & Communication Topology`. Make the rail robot the focal physical deployment, capture exact pins/transports/electrical cautions/power domains/safe boot behavior, and keep the Mega/TFT visibly standalone. Use only official Draw.io MCP diagram-authoring operations plus official Draw.io Desktop persistence/export, and preserve every protected product file.

### Phases

| Phase | Status | Outcome |
|---|---|---|
| DRAW2-D1. Restore specification and protected baseline | complete | Re-read the audit, Page 2 specification, shared visual system, current plan/status, and protected working-tree diff. |
| DRAW2-D2. Inspect Diagram 1 style and current Draw.io document | complete | Inspected the complete Diagram 1 PNG; official Draw.io MCP `list_pages` confirmed Diagram 1 has one page and confirmed the Diagram 2 target does not exist. |
| DRAW2-D3. Build source-authoritative physical topology | complete | Official Draw.io MCP accepted the 50-row model with 16 stable source-authoritative wires; official Draw.io Desktop persisted the MCP-authored CSV as a one-page 39,205-byte editable XML document. |
| DRAW2-D4. Beautify and visually QA | complete | Accepted the third 1630×930 render after correcting signal/card overlap, explicit divider treatment, camera voltage/flashing caution, actuator connector gaps, and standalone warning readability. |
| DRAW2-D5. Export and semantically verify | complete | Final CSV was re-accepted by official MCP; requested editable/PNG artifacts were published; MCP page inspection, native-model assertions, and direct source checks all pass. |
| DRAW2-D6. Record completion | complete | Recorded artifact hashes/dimensions, MCP operations, three visual-QA passes, semantic/source gates, all errors, Diagram 2-only scope, and protected-tree integrity. |

### Constraints

- Produce Diagram 2 only; do not begin Diagram 3.
- Do not hand-write Draw.io XML or use Graphviz/Mermaid.
- Product source, configuration, UI, protocol, README, and all other diagram files are read-only.
- MCP mutations are sequential and individually verified.

### Errors Encountered

| Error | Attempt | Resolution |
|---|---:|---|
| Architect mode did not expose Draw.io MCP operations or permit the non-Markdown artifacts | 1 | Switched to Code mode while retaining the strict protected-product write boundary. |
| One oversized shell invocation containing two quoted Python heredocs was rejected as an unterminated heredoc before execution | 1 | No file was created. Do not repeat the compound command; generate the temporary CSV in smaller single-heredoc stages, then validate separately. |
| `xvfb-run` is not installed in this environment | 1 | No artifact was created by that invocation. Use the already-live `DISPLAY=:0` X server and invoke official Draw.io Desktop directly. |
| The first direct Draw.io Desktop conversion omitted CLI export mode and opened the GUI | 1 | Terminated only the temporary Diagram 2 Draw.io processes; all future conversions include `-x`. |
| Draw.io Desktop accepted `-f drawio` but emitted a two-page PDF because `drawio` is not a supported export format | 1 | Keep the MCP-authored CSV as authority, discard the invalid temporary output, and request supported `xml` output while retaining the `.drawio` destination extension. |
| The first structured visual-revision command embedded a single-quoted Python string inside a single-quoted Bash heredoc wrapper, so the shell truncated the script and Python raised `SyntaxError` before writing | 1 | The pre-command backup preserves the accepted CSV. Do not repeat nested shell quoting; place the revision program in a temporary non-executable Python file with the file patch tool, run it once, then delete it. |
| Final MCP re-submission passed the CSV as tool argument `csv`, but official `open_drawio_csv` requires the argument name `content` | 1 | MCP returned `isError: true` without mutation. Retry once with the inspected official schema's `content` argument and the unchanged final CSV. |
| First final-page MCP inspection passed `filePath` to `list_pages`, but the official tool requires `path` | 1 | MCP returned `isError: true` without changing the file. Retry once using the already-inspected schema's `path` argument. |
| First `get_page` call passed `pageIndex`, but the official tool requires `page` | 1 | MCP returned `isError: true` without changing the file. Retry once with `page=0`. |
| First native semantic assertion searched raw HTML labels for contiguous `Robot sends STOP`, but the visible label wraps `STOP` in a bold tag | 1 | Source search proved the exact safe-boot wording exists in `motion_safety`. Normalize HTML tags before visible-text assertions rather than weakening the contract. |

## Active Diagram Production — Diagram 1 working-combo system context (2026-08-11)

### Goal

Create exactly one polished, source-authoritative Draw.io page titled `01 — Working-Combo System Context & Active Interfaces` from the completed architecture audit. Use only Draw.io MCP operations, preserve all protected product code, and export the editable page plus a PNG review image under [`diagrams/`](diagrams/).

### Phases

| Phase | Status | Outcome |
|---|---|---|
| DRAW-D1. Restore Diagram 1 specification and protected baseline | complete | Re-read the current planning phase, exact node/edge checklist, visual system, discrepancies, interface matrix, and protected Git baseline before touching the canvas. |
| DRAW-D2. Inspect current Draw.io document | complete | Official Draw.io MCP 1.5.0 `list_pages` proved the requested target does not yet exist; no prior page or canvas content can be overwritten. |
| DRAW-D3. Build source-authoritative topology | complete | Official MCP `open_drawio_csv` accepted the complete hand-positioned topology; an independent official Draw.io Desktop parse proved all required stable IDs and `e01`–`e23` edge metadata are present with the intended status styles. |
| DRAW-D4. Beautify and visually QA | complete | Replaced browser mockups with clean cards, restored every exact contract, moved edge wording into deterministic 10–11 px label lanes, corrected authority/standalone clipping, and accepted the final 1626×927 full-page PNG after repeated direct inspection. |
| DRAW-D5. Export and verify | complete | Official MCP accepted the exact final CSV; official Draw.io Desktop generated the native editable artifact and PNG; MCP `list_pages`/`get_page`, native-model validation, source spot checks, hashes, dimensions, and protected-tree checks all pass. |
| DRAW-D6. Record completion | complete | Recorded artifact paths/hashes, MCP operations, semantic gates, visual corrections, source evidence, all encountered errors, Diagram 1-only scope, and confirmation that product diffs remain the protected pre-existing baseline. |

### Constraints

- Produce Diagram 1 only; do not begin Diagram 2.
- Draw.io MCP is mandatory. Do not substitute handwritten XML, Graphviz, or Mermaid.
- Draw.io MCP mutations must be sequential and individually verified.
- Product source, configuration, UI, protocol, README, and existing non-target diagrams are read-only.

### Errors Encountered

| Error | Attempt | Resolution |
|---|---:|---|
| Architect mode did not expose Draw.io MCP operations or permit non-Markdown artifact creation | 1 | Switched to Code mode while retaining the protected-product write boundary. |
| Official Draw.io MCP 1.5.0 has no save/export/screenshot operation and cannot inspect an unsaved browser canvas | 1 | Author through MCP `open_drawio_csv`, then persist and export the same CSV with an extracted official Draw.io Desktop CLI; inspect the PNG separately and use MCP page tools on the final native file. |
| Browser automation remained on the diagrams.net loading splash | 1 | Abandoned browser automation; extracted Draw.io Desktop 31.1.8 under `/tmp` without installing it. |
| Duplicate CSV rows with one source identity reused the first source cell's edge style | 1 | Give each exact edge its own fixed `# connect` directive and unique target/label columns while retaining one actual row per stable node. |
| Vision MCP timed out while inspecting the temporary review PNG | 1 | Do not retry that path; inspect the exported PNG directly with the image-aware file reader and enlarged crops, then persist every visual finding in the planning files. |
| Raw string replacement could not find four CSV-serialized node labels | 1 | Parse the CSV structurally with Python's CSV module and update rows by stable `id`, rather than matching serialized quoting. |
| Repeated edge-offset-only experiments caused label regressions and shortened exact contracts | 1 | Stop incremental offset-only edits; perform one structured layout pass with clean browser cards, dedicated label zones, restored source-truth wording, and 10–11 px technical labels. |
| Final integrity command used `set -o pipefail`, but the executor launched it with `/bin/sh` and rejected that Bash-only option before any check ran | 1 | Do not repeat the shell form; invoke the validation body explicitly through `/bin/bash -lc`. |
| Final source spot-check regex assumed literal pin/baud values and `case MSG_LOCATION`; several targets use named constants or `if` control flow, so the check produced false negatives | 1 | Do not broaden the same brittle regex; use targeted source searches for each configured symbol and its resolved value, while retaining the already-passed diagram-model and protected-diff gates. |

## Active Audit — Working-combo architecture discovery and Draw.io plan (2026-08-11)

### Goal

Recover the code-authoritative architecture of the working combination [`ESP32_RFID_MP3_RAIL/`](ESP32_RFID_MP3_RAIL/), [`NANO_LFR_RAIL/`](NANO_LFR_RAIL/), [`firmware/cam/`](firmware/cam/), [`firmware/hub/`](firmware/hub/), and [`firmware/display/`](firmware/display/), then define a compact, polished Draw.io suite without changing any product, configuration, web, protocol, or diagram file.

### Source-of-truth boundary

- Treat the current source/configuration/UI files in the five named projects as authoritative. Earlier plans and unrelated projects may explain history but must not be used to invent present integrations.
- Inspect directly referenced shared protocol files only where needed to decode a real link from the five targets; in particular, follow the robot/header symlinks into [`firmware/common/`](firmware/common/).
- Do **not** draw the Mega display as integrated unless the current robot source actually sends its 9600-baud command protocol.
- Write only [`task_plan.md`](task_plan.md), [`findings.md`](findings.md), and [`progress.md`](progress.md). Preserve every pre-existing working-tree change.
- This phase produces architecture inventory and diagram specifications only. It does not create or edit Draw.io files.

### Phases

| Phase | Status | Outcome |
|---|---|---|
| ARCH-D1. Restore planning context and protect concurrent work | complete | Read all three planning files; inspected Git status, diff statistics, summary, target diffs, and untracked/symlink-related state. Existing robot, Hub, shared-header, and planning changes are the protected baseline. |
| ARCH-D2. Inventory every relevant file in the five working targets | complete | Read all 34 implementation, include, UI/data, configuration, helper, README, symlink, and directly referenced shared-protocol files while excluding generated `.pio` artifacts; treated jsQR internals as a vendored dependency after identifying its consumed export/version claim. |
| ARCH-D3. Reconstruct code-authoritative architecture | complete | Traced hardware/pins, transports, channels/baud, state ownership, tasks/cores, persistence, APIs, normal/emergency flows, fallbacks, active/inactive edge matrix, and source/README discrepancies. |
| ARCH-D4. Specify the diagram suite and shared visual system | complete | Defined six ordered pages with exact titles, purpose, audience, scope, nodes/swimlanes, connectors, legends, hierarchy, and one consistent 16:9 visual system. |
| ARCH-D5. Lock the first Draw.io page | complete | Selected `01 — Working-Combo System Context & Active Interfaces` and recorded stable node IDs, 23 source-truth edges, placement/layering, labels, statuses, and final QA checks for a Draw.io MCP creator. |
| ARCH-D6. Final scope verification and report | complete | Final Git status/stat and product-target diff inventory match the protected source baseline: product diffs remain only the pre-existing robot/Hub work, Nano/camera/display target patches remain empty, no diagram was changed, and `git diff --check` passes for all three planning files. |

### Protected working-tree baseline

- Existing tracked modifications: robot field console/controller/README/build config; Hub README/config/main; all three planning files; staged move/addition of the shared ESP-NOW compatibility header.
- Existing untracked target links: [`ESP32_RFID_MP3_RAIL/EspNowCompat.h`](ESP32_RFID_MP3_RAIL/EspNowCompat.h) and [`ESP32_RFID_MP3_RAIL/MuseumProtocol.h`](ESP32_RFID_MP3_RAIL/MuseumProtocol.h). Treat their resolved shared targets as read-only.
- No tracked diff was present under [`firmware/display/`](firmware/display/), [`firmware/cam/`](firmware/cam/), or [`NANO_LFR_RAIL/`](NANO_LFR_RAIL/) at the restoration gate.

### Errors Encountered

| Error | Attempt | Resolution |
|---|---:|---|
| Architect mode exposed no terminal executor for the required Git restoration command | 1 | Switched to Code mode at the user's direction, ran the full read-only Git inspection, and retained the Markdown-only write boundary. |
| First progress-log patch was attempted after the environment unexpectedly switched to Ask mode | 1 | The patch was rejected without a file change; restored an editing-capable mode and applied it once. |
| Cross-target interface search invoked `rg`, but ripgrep is not installed in the environment (`exit 127`) | 1 | Do not repeat the command; use the repository-native regex search tool for the same source-only reconciliation. |

## Active Adaptation — Rail motor-speed field controls (2026-08-10)

### Goal

Expose independent Nano rail PWM values through the ESP32 SoftAP console, apply changes live through the existing `SET ML` / `SET MR` / `GET` protocol, optionally persist them with `SAVE`, and display only Nano-confirmed state.

### Phases

| Phase | Status | Outcome |
|---|---|---|
| MOTOR-WEB1. Restore context and protect concurrent work | complete | Recovered the implementation/validation state from the supplied summary and existing planning logs; unrelated workstreams and planning edits remain preserved. |
| MOTOR-WEB2. Implement ESP32 API, UI, and Nano state parsing | complete | Added Motor L/R controls, validated 0–255 API inputs, Nano command forwarding, confirmed-state parsing, live apply/save actions, and documentation. |
| MOTOR-WEB3. Correct asynchronous startup synchronization | complete | `TUNE` always refreshes Nano-confirmed values, but clears pending state only when an existing request matches; startup `GET` no longer creates false pending state. |
| MOTOR-WEB4. Rebuild and run source-level compatibility checks | complete | Serialized builds passed for ESP32 and Nano; embedded JavaScript, 15 protocol assertions, pending-state guard, and whitespace checks all passed. |
| MOTOR-WEB5. Review and report | complete | Scoped UI/API/parser/docs review passed; Nano product source remains unchanged. This revision is software-validated only and has not been flashed, committed, or pushed. |

### Constraints

- Preserve unrelated working-tree changes, especially the pre-existing content in `findings.md`, `progress.md`, and this plan.
- Do not claim the latest motor-control revision was flashed, committed, or pushed unless those actions are actually performed.
- Nano responses are authoritative; the ESP32 must not present requested values as confirmed before receiving `TUNE ML=n MR=n`.

### Errors Encountered

| Error | Attempt | Resolution |
|---|---:|---|
| ESP32 incremental build reached framework archive creation but `Esp.cpp.o` was absent from `.pio/build`, causing `ar` to fail | 1 | Treat as a generated-cache failure, clean only the ESP32 target's PlatformIO output, then rebuild before running the Nano build. |
| Clean ESP32 rebuild compiled and linked, but `firmware.elf` disappeared before esptool's ELF-to-image step | 2 | Do not repeat the same build; inspect target configuration, generated directory, and concurrent PlatformIO/SCons processes for an external cleanup/race. |

## Active Audit — Hub firmware runtime/data flow (2026-08-10)

### Goal

Inspect the implementation under `firmware/hub/` and give a source-confirmed explanation of its boot sequence, concurrent task/core flow, ESP-NOW state handling, emergency/LED/siren behavior, HTTP/API paths, and photo-storage lifecycle without changing product code.

### Phases

| Phase | Status | Outcome |
|---|---|---|
| HUBFLOW1. Restore context and protect concurrent work | complete | Read the existing planning files and confirmed the only current product diffs are unrelated `ESP32_RFID_MP3_RAIL/` changes, which will remain untouched. |
| HUBFLOW2. Inspect Hub configuration, state, storage, and implementation | complete | Read configuration, shared state, photo store, complete firmware entry point, shared protocol, deployed browser adapter, and admin page. |
| HUBFLOW3. Trace end-to-end control and data flows | complete | Mapped startup, Core 0/Core 1 work, ESP-NOW ingress/egress, API requests, emergency transitions, LEDs/siren, visitor state, and camera gallery operations. |
| HUBFLOW4. Run non-destructive software verification | complete | Current target builds successfully on Espressif32 6.12.0 / Arduino-ESP32 2.0.17: RAM 47,936 B (14.6%), application flash 916,145 B (69.9%); no Hub tracked diff was introduced. |
| HUBFLOW5. Report confirmed behavior and caveats | complete | Findings are recorded and the source-linked execution-flow narrative, limitations, and confidence boundary are ready for delivery. |

### Constraints

- This is an inspection/confirmation task; do not change Hub product code or browser assets.
- Treat documentation as intended behavior and source as the authority for actual behavior.
- Keep static/build confidence separate from physical hardware/runtime confidence.
- Preserve all unrelated working-tree changes.

### Errors Encountered

| Error | Attempt | Resolution |
|---|---:|---|
| Initial skill-loader invocation omitted its required native arguments | 1 | Retried once with the selected skill name and task context; the skill loaded successfully. |

## Active Adaptation — RFID rail audio stop/resume latch (2026-08-10)

### Goal

Make the standalone RFID rail controller stop the Nano before every mapped audio track, restart the Nano only after DFPlayer completion, and keep the explicit end card stopped until the start card is scanned again.

### Phases

| Phase | Status | Outcome |
|---|---|---|
| AUDIO-RAIL1. Inspect existing flow | complete | Confirmed the standalone rail target had direct track playback but no narration state or completion handling. |
| AUDIO-RAIL2. Implement safe stop/resume | complete | Added STOP-before-audio, guarded DFPlayer finish/timeout/error handling, START-after-finish, and explicit STOP-card latch cleared only by the start card. |
| AUDIO-RAIL3. Document behavior | complete | Updated the rail README with the stop/resume sequence and latch semantics. |
| AUDIO-RAIL4. Build and flash verification | pending | Build and flash the updated ESP32 target after source review. |

## Active Adaptation — ESP32 RFID rail start/end cards (2026-08-10)

### Goal

Add the requested start and end RFID cards to the standalone `ESP32_RFID_MP3_RAIL` controller: UID `18 C4 0F BA` starts the Nano rail and plays `0001.mp3`; UID `A3 C0 38 15` stops the Nano rail without selecting new audio.

### Phases

| Phase | Status | Outcome |
|---|---|---|
| RFID-RAIL1. Inspect target and companion protocol | complete | Confirmed the standalone rail sketch had RFID track mapping but no Nano UART; `NANO_LFR_RAIL` expects 9600-baud newline `START`/`STOP`. |
| RFID-RAIL2. Implement start/end card actions | complete | Added both UIDs, UART1 GPIO25/26 Nano link, startup STOP safety command, start `START` + track 1 behavior, and end `STOP` + audio stop/no new track behavior. |
| RFID-RAIL3. Update documentation | complete | Updated the rail README with the UID/action table and Nano wiring. |
| RFID-RAIL4. Build verification | complete | `pio run` passed for `esp32dev`: RAM 21,864 B (6.7%), application flash 282,981 B (21.6%). |

### Hardware flash follow-up (2026-08-10)

- Nano `/dev/ttyUSB0` was positively identified as the connected CH340 (`1a86:7523`, USB path `3-2`).
- The exact `serial-monitor` process holding the port was released; no broad process termination was used.
- `pio run -e nano -t upload --upload-port /dev/ttyUSB0` passed with ATmega328P signature `0x1e950f`; 4,092 bytes were written and verified.
- Boot capture at 9600 baud returned `READY:STOPPED`, confirming the flashed Nano starts safely braked.

### Errors Encountered

| Error | Attempt | Resolution |
|---|---:|---|
| `pio device monitor` failed with `termios.error: (25, 'Inappropriate ioctl for device')` in the non-interactive terminal | 1 | Use the installed pyserial library with a bounded read script instead of the interactive PlatformIO monitor. |

### ESP32 hardware flash follow-up (2026-08-10)

- ESP32 `/dev/ttyUSB1` was positively identified by USB path `3-1` and a live ESP32-D0WD-V3 rev 3.1 / MAC `1c:c3:ab:3c:8c:38` probe.
- The standalone `ESP32_RFID_MP3_RAIL` firmware was uploaded successfully; 283,344 application bytes were written and hash-verified.
- A bounded pyserial capture with an explicit reset pulse confirmed the new runtime boot markers, RFID initialization, Nano UART initialization, and the safety command `-> Nano: STOP`.
- Runtime also reported `DFPlayer ERROR! Check wiring / SD card.` and disabled audio. This is a peripheral/SD/wiring condition; the ESP32 firmware and RFID/Nano control path booted successfully, but MP3 playback cannot be physically accepted until DFPlayer wiring and the SD card are corrected.

### Notes

- No hardware upload or RFID/Nano physical test was performed in this change.
- The end card is represented as a mapped no-audio action (`track=0`) so it is not confused with an unknown UID.

## Active Adaptation — Nano rail drive + ESP32 RFID/MP3 controller (2026-08-10)

### Goal

Adapt the calibrated Nano rail drivetrain (fixed physical-channel PWM 150/210 and active HIGH/HIGH braking) to the tour controller without breaking the ESP32's RFID stop, narration, automatic resume, emergency evacuation, or state reporting.

### Phases

| Phase | Status | Outcome |
|---|---|---|
| RAIL1. Inspect both controllers and bind the wire protocol | complete | Identified the Nano's existing LFR/tuner/EEPROM stack, the calibrated rail behavior, the crossed historical motor mapping, and the ESP32's required START/STOP/READY/STATE contract. |
| RAIL2. Lock final operating scope | complete | Rail-only replacement in a NEW copy `NANO_LFR_RAIL/`; `NANO_LFR/` preserved as the line-follower fallback; `ESP32_RFID_MP3_LFR/` read-only. |
| RAIL3. Implement Nano rail firmware | complete | `NANO_LFR_RAIL/NANO_LFR_RAIL.ino` written: deterministic fixed-buffer START/STOP, ENA=150/ENB=210 forward, HIGH/HIGH+255 active brake, ML/MR live tuning, EEPROM, RAMP, rate-limited `S1=0..` telemetry for the unchanged `/rail` page. Compiles: Flash 15.3% (4700 B), RAM 14.6%. |
| RAIL4. Verify unchanged ESP32 compatibility | complete (by design) | Nano implements the existing contract exactly: 9600 8N1 `\n`, `START`/`STOP`/`SET ML|MR`/`GET`/`SAVE`/`LOAD`/`MAN`/`RAMP`, and `READY:STOPPED`/`STATE:RUNNING`/`STATE:STOPPED` replies that `handleNanoLine()` parses; no ESP32 edit required. |
| RAIL5. Flash Nano + physical acceptance | pending board reconnect | No `/dev/ttyUSB*` present at build time. Reconnect the Nano, disconnect D0/D1 ESP32 wires, then `cd NANO_LFR_RAIL && pio run -t upload`; reconnect D0/D1 and verify START drives, STOP hard-brakes, and `/rail` shows live ML/MR. |

### Constraints and integration rules

- The production Nano must start in active-brake STOP and must never move merely because it received an unknown or malformed UART line.
- `ESP32_RFID_MP3_LFR/` is read-only for this adaptation: no source, UI, protocol, platform configuration, or documentation changes are permitted there.
- ESP32 compatibility requires 9600-baud newline-delimited START/STOP commands and READY:STOPPED / STATE:RUNNING / STATE:STOPPED replies.
- START means continuous calibrated rail motion until an explicit STOP (not the prior 2 s demo cycle); RFID narration timing is owned by the ESP32.
- STOP must apply HIGH/HIGH on both direction pairs with ENA/ENB enabled, rather than disabling PWM and allowing coast.
- Preserve the measured physical output pair exactly: ENA/IN1/IN2 receives PWM 150 and ENB/IN3/IN4 receives PWM 210. Do not blindly copy the old `wheel(lm, rm)` labels because that function crosses the channels.
- If line following is removed, the Nano may still harmlessly answer the unchanged ESP32's read-only GET/tuning traffic where needed, but unsupported tuner/manual commands must never toggle motion. No companion-page edit is allowed.
- The Nano is confirmed new-bootloader hardware; do not retain or use an old-bootloader upload environment for this adapted target.
- Physical acceptance, not compilation, determines whether braking and rail tracking are correct.

## Active LFR Correction — Nano steering telemetry + start-RFID audio suppression (2026-08-08)

### Goal

Set the Nano line follower to 38% master speed, emit all five normalized line-sensor values as one readable serial line, improve weak bend/junction tracking by slowing and steering decisively, and prevent only the start RFID from triggering audio while preserving station audio behavior.

### Phases

| Phase | Status | Outcome |
|---|---|---|
| F1. Restore context and bind active firmware | complete | Active targets are `NANO_LFR` for motion and `ESP32_RFID_MP3_LFR` for RFID/DFPlayer/Nano control; unrelated edits preserved. |
| F2. Design corrected steering and diagnostics | complete | Selected rate-limited complete sensor lines plus explicit straight/curve/sharp/search/junction commands; removed the weak speed-scaled PD and blocking junction delay. |
| F3. Implement Nano changes | complete | Applied 38% speed, `S1..S5`/error/mode/motor telemetry, lower-speed bends, strong differential steering, direction-aware junctions, and line-loss search. |
| F4. Suppress start-tag audio | complete | UID `18 C4 0F BA` now stops any old audio and starts the Nano immediately without playing track 1; station tracks 2–8 are unchanged. |
| F5. Build, flash, and verify | complete | Both targets compiled, positively identified on separate CH340 ports, flashed with hash/byte verification, and runtime-verified via unique markers. |

### Constraints

- Preserve unrelated existing modifications shown by `git diff --stat`; this repository has substantial concurrent work.
- Nano D0/D1 carries both USB serial and the ESP32 control link. Continuous sensor telemetry must be bounded so it does not starve command parsing at 9600 baud.
- Never infer physical line-following success from compilation or upload alone; final steering quality remains a wheels-down track test.
- The start RFID must produce no audio for now; other mapped RFID station tracks must remain unchanged.

### Verification evidence (2026-08-08)

- ESP32 (`/dev/ttyUSB0`, CH340 `1a86:7523` on USB path `04:00.4:1`): read-only `esptool chip_id` positively identified ESP32-D0WD-V3 rev v3.1, MAC `1c:c3:ab:3c:8c:38` before writing. Flashed; all region hashes verified. Boot log confirmed `RFID-MP3-LFR-v3-silent-start`, `-> Nano: STOP`, and `<- Nano: STATE:STOPPED`.
- Nano (`/dev/ttyUSB1`, second CH340 on USB path `04:00.4:2`): avrdude signature `0x1e950f` (ATmega328P), 4302 bytes written and verified. Boot/telemetry confirmed `READY:STOPPED` then `S1=0 S2=1 S3=1 S4=0 S5=0 E=-5 M=STOP L=0 R=0` at ~8 Hz.
- The two CH340 adapters share VID:PID, so they were distinguished by USB path plus a read-only esptool probe, not by name alone.
- Final physical steering quality remains a wheels-down track test; telemetry now makes that test observable on the ESP32 USB monitor after the D0/D1 wires are reconnected.

### Errors Encountered

| Error | Attempt | Resolution |
|---|---:|---|
| No `/dev/ttyUSB*`, `/dev/ttyACM*`, stable serial by-id link, or USB serial adapter was present at the flash gate | 1 | Do not guess or reuse the prior `/dev/ttyUSB0` identity. Connect the Nano with D0/D1 ESP wires disconnected, flash/verify it, then connect and positively identify the ESP32 for its separate flash. |

## Active Debug — ESP32 Browser Controls (2026-08-08)

### Goal

Reproduce actual browser button clicks against the physical `ESP32-MP3` SoftAP, identify why the JavaScript-driven UI triggers neither audio nor LFR while direct HTTP endpoint requests work, replace the fragile browser path, upload the corrected firmware, and verify real browser clicks end-to-end.

### Phases

| Phase | Status | Outcome |
|---|---|---|
| B1. Correct prior conclusion and restore context | complete | Phone-only routing hypothesis is disproven by the same failure from the laptop; direct `curl` endpoint success did not validate browser JavaScript. |
| B2. Reproduce with a real browser and inspect runtime | in progress | Exercise actual rendered buttons and capture browser console/network plus ESP32/Nano serial evidence. |
| B3. Implement browser-robust controls | pending | Replace JavaScript-only actions with ordinary HTML navigation/forms, add cache prevention and a visible firmware marker. |
| B4. Build and upload ESP32 firmware | pending | Compile and flash the positively identified ESP32 without altering the Nano image. |
| B5. Verify actual browser clicks and hardware effects | pending | Prove rendered controls trigger LFR start/stop and DFPlayer commands, with the robot left stopped. |

### Errors Encountered

| Error | Attempt | Resolution |
|---|---:|---|
| Declared the web UI healthy after testing endpoint URLs with `curl`, not rendered browser button clicks | 1 | Retract the conclusion; reproduce through a real browser and require console/network plus hardware evidence before acceptance. |
| First Firefox WebDriver click test started after `/dev/ttyUSB1` disappeared and navigation to the ESP32 timed out | 1 | No browser/UI conclusion is valid from this run. Rediscover current USB identities and Wi-Fi state, positively bind ports again, prove `192.168.4.1` reachable, then rerun with shorter bounded navigation timeouts. |
| Build wrapper ended with `/bin/sh: Bad substitution` after PlatformIO had already printed `SUCCESS` | 1 | Firmware compilation and image generation passed; only the Bash-only `${PIPESTATUS[0]}` post-check failed under `/bin/sh`. Use direct PlatformIO exit status or explicitly invoke Bash; no rebuild logic/source change is required. |
| Post-fetch HTML-count diagnostic had a nested-quote syntax error | 1 | The gated network evidence before that line is valid: association succeeded, address/route were correct, HTTP 200 returned 2,886 bytes with no-cache headers, and `WEB-FORMS-v2` was present. Keep the next browser harness quoting simple; EXIT cleanup restored `Lovly`. |

## Active Migration — Nano LFR/RFID + ESP32 Companion (2026-08-06)

### Goal

Move real-time line following, L298N motor control, and RC522 card detection to an Arduino Nano; stop once for every newly presented card and send a framed UID event to the ESP32 over a Nano SoftwareSerial link. Repurpose the ESP32 robot target as a validated event receiver while reserving its GPIO16/17 UART for the planned DFPlayer.

### Phases

| Phase | Status | Outcome |
|---|---|---|
| N1. Lock split architecture and wiring | in progress | Nano hardware SPI forces a conflict-free motor remap; UART and voltage boundaries are being documented before code. |
| N2. Implement Nano PlatformIO target | pending | Preserve weighted active-low PD algorithm, measured left/right trim, RC522 polling, one-shot card stop, and bounded ASCII event protocol. |
| N3. Repurpose ESP32 robot target | pending | Remove direct motor ownership; receive, validate, acknowledge, and expose Nano RFID events without consuming DFPlayer UART pins. |
| N4. Build and statically verify both targets | pending | Compile Nano and ESP32 independently and run targeted diff/static checks. |
| N5. Publish exact bring-up procedure | pending | Wiring, level shifting, sensor/motor/card/UART test order, and physical acceptance gates documented. |

### Locked migration decisions

- Nano IR inputs remain A0–A4 in physical far-left to far-right order; active-low interpretation and the supplied integer weighted-error calculation are preserved.
- Nano hardware SPI is fixed to D11 MOSI, D12 MISO, and D13 SCK. RC522 uses D10 SS and D4 RST. The RC522 is 3.3 V-only: Nano-to-RC522 SS/MOSI/SCK/RST require 5 V→3.3 V level shifting; RC522 MISO can drive Nano D12 directly.
- Current chassis mapping remains channel A = physical left and channel B = physical right. Nano motor map: D5 ENA/left PWM, D7/D8 IN1/IN2, D6 ENB/right PWM, D9/A5 IN3/IN4.
- Nano SoftwareSerial uses D2 RX and D3 TX at 9600 baud. Nano D3 is 5 V logic and reaches ESP32 GPIO25 RX through a 1 kΩ/2 kΩ divider. ESP32 GPIO26 TX may return ACK data to Nano D2 through a 1 kΩ series resistor. Grounds are common.
- ESP32 GPIO16 RX / GPIO17 TX remain reserved for DFPlayer UART2. Moving RFID and motors to Nano frees the old ESP32 motor/SPI pins.
- RFID messages are bounded ASCII frames containing protocol version, sequence, UID, SAK, and PICC type plus an XOR checksum. UID is an identifier, not a secure authentication credential.
- A newly read card stops the motors immediately, emits one event, remains stopped for a configurable timeout, then resumes. `PICC_HaltA()` prevents repeated events while the same card remains on the antenna; removing and presenting it again creates a new event.

## Active Implementation — ESP32 Line-Following Robot (2026-08-06)

### Architecture migration — Nano motion/RFID + ESP32 application controller

- User superseded the single-ESP32 motion architecture: an Arduino Nano ATmega328P will own the five IR sensors, L298N, line-following loop, and RC522; it will send distinct RFID events to the ESP32 over a separate UART link.
- Nano hardware SPI is fixed to D10 SS, D11 MOSI, D12 MISO, and D13 SCK. Therefore the supplied Arduino motor allocation on D10/D11/D12 cannot coexist with the RC522 and must be remapped before wiring or firmware implementation.
- PlatformIO exposes `nanoatmega328` (classic bootloader) and `nanoatmega328new` (new bootloader). The maintained RC522 package is `miguelbalboa/MFRC522@1.4.12`.
- Electrical boundary: Nano TX is nominally 5 V and must not drive an ESP32 RX directly; use a resistor divider or proper unidirectional level shift. ESP32 TX at 3.3 V can normally drive the Nano RX, with common ground mandatory.
- Final Nano allocation after the user's rewiring request: IR=A0–A4; four L298N direction wires IN1/IN2/IN3/IN4 move in order from D9–D12 to D2–D5; PWM ENA=D6 and ENB=D9; RC522 RST=A5 and fixed SPI SS/MOSI/MISO/SCK=D10/D11/D12/D13. Nano hardware UART D0/D1 is the ESP link and must be disconnected during Nano upload/USB monitoring.
- Final ESP allocation follows the user's physical UART wiring: Nano events use hardware UART1 RX/TX=GPIO16/GPIO17. The existing DFPlayer moves to hardware UART2 RX/TX=GPIO25/GPIO26. This permits both links without adding a module.
- Protocol direction: each accepted UID causes an immediate bounded five-second Nano stop and a checksummed sequenced `RFID` event carrying UID, SAK, and PICC type. Nano retries until `ACK` or the stop expires, then resumes regardless so a failed UART cannot immobilize it. ESP validates and deduplicates retries, ACKs every valid frame, maps the seven supplied cards to points 1–7, and plays the matching DFPlayer `/mp3/0001.mp3`–`0007.mp3` track.

### Goal

Convert the working Arduino five-sensor/L298N line follower to the confirmed 30-pin ESP32-WROOM-32 robot target, publish the exact electrical wiring before firmware changes, preserve the original PD/T-junction/line-recovery behavior, and prove the PlatformIO build.

### Phases

| Phase | Status | Outcome |
|---|---|---|
| L1. Restore and reconcile context | complete | Existing robot plan, skeleton target, PlatformIO configuration, repository changes, and the supplied Arduino behavior were recovered. |
| L2. Lock Phase-1 wiring | complete | GPIO map and exact ESP32↔IR-array↔L298N wiring are published; channel A is right and channel B is left to preserve the working Arduino convention. |
| L3. Configure PlatformIO | complete | Existing `firmware/robot/` target retained, pinned to Espressif32 platform 6.5.0 / Arduino-ESP32 2.x LEDC API, with no unused Phase-2/3 dependencies. |
| L4. Port line-following firmware | complete | ESP32 LEDC motor PWM, five configurable-polarity sensors, exact weighted error arithmetic, PD steering, last-edge recovery, and all-five right turn implemented. |
| L5. Build and document | complete (software) | PlatformIO build and cppcheck pass; README and wiring plan cover exact connections, startup motor gate, diagnostics, calibration, and safe bring-up. Physical direction/polarity/tuning tests remain hardware gates. |

### Locked Phase-1 decisions

- Controller is the confirmed DOIT-style 30-pin ESP32-WROOM-32 DevKit (`esp32dev` PlatformIO board).
- IR array is powered from 3.3 V and presents five digital outputs in physical far-left to far-right order on GPIO36, 39, 34, 35, and 32.
- The supplied Arduino sketch treats a LOW sensor output as line detected. Preserve that as the default but expose one configuration constant so measured opposite-polarity arrays can be corrected without rewriting the algorithm.
- Initial software preserved the original Arduino L298N convention, but physical calibration superseded it: the installed chassis has channel A (`ENA`, `IN1`, `IN2`, `OUT1/OUT2`) on the robot's left motor and channel B (`ENB`, `IN3`, `IN4`, `OUT3/OUT4`) on the right motor. Firmware/docs now match the measured robot.
- ESP32 motor pins are GPIO25/26/27 for channel A and GPIO13/14/33 for channel B. The user intentionally swapped the former GPIO16/GPIO33 roles after wheels-up calibration: GPIO33 is now L298N IN4, while GPIO16 is reserved as DFPlayer UART2 RX. ENA/ENB jumpers must be removed.
- Motor power and L298N 5 V logic power come from regulated external rails with common ground; motors must never be powered from the ESP32.
- Phase 1 implements only line following. RC522, DFPlayer, WS2812B, camera, and display integration remain later phases and must not complicate this build.
- Preserve the supplied control constants and decisions initially: `kp=1`, `kd=1`, curve base 160/160, center speed 150, recovery speed 180, right-turn speed 100 for 500 ms, no-line sentinel behavior, and a 5 ms delay after error changes.

### Verification

- `pio run -d firmware/robot`: PASS. PlatformIO Espressif32 6.5.0 resolved Arduino-ESP32 2.0.14; RAM 21,480 bytes (6.6%), application flash 270,233 bytes (20.6%).
- `pio check -d firmware/robot --skip-packages`: PASS with no high/medium findings. The low-only reports are framework-entry-point awareness (`setup`/`loop`) and compile-time motor-gate constant folding, not firmware defects.
- `git diff --check` on robot firmware/docs/planning files: PASS.
- No firmware upload or physical motor test was attempted because no exact robot ESP32 serial device or connected/wired chassis was established in this request.

### Physical calibration session — 2026-08-06

- The user reported the robot ESP32 and wiring are now connected and requested flashing plus full calibration.
- Fresh USB inventory found exactly one new serial adapter: CH340 `1a86:7523`, by-id `usb-1a86_USB_Serial-if00-port0`, kernel path `/dev/ttyUSB0`, connected at 17:11:52–17:11:53 Asia/Dhaka on USB path `3-2`. No process owns the port.
- Calibration must begin with the compile-time motor gate disabled; only after sensor order/polarity/stability are measured may motor tests run with wheels raised.
- Sensor-only image flashed successfully to the identified ESP32-D0WD-V3 revision 3.1, MAC `1c:c3:ab:3c:8c:38`; all flash regions passed esptool hash verification.
- Physical black-line/light-background calibration passed: background `00000` was stable for 42/42 samples; individual physical channels produced far-left `10000` (24/24), left `01000` (24/24), centre `00100` (24 stable samples plus one prior-position transition), right `00010` (24 stable plus one transition), far-right `00001` (24 stable plus one transition); all-black produced `11111` for a final 17/17 stable capture. Wiring order and active-low polarity are confirmed.
- Motor-channel isolation proved the installed output pairs were opposite the initial software labels: a software-left/channel-B command moved the robot's physical right wheel forward; software-right/channel-A moved its physical left wheel forward. Both channels work. Semantic left/right pin assignments were swapped in firmware instead of changing the already-secure motor wiring.
- Corrected-map wheels-up calibration passed in both directions. Forward start threshold: physical left is still at 160 and rotates continuously at 180; physical right is still at 140, only twitches at 160/180, and rotates continuously at 190. At equal PWM 220 the left is faster; left=200/right=220 is approximately speed-matched. The right-pivot pair left=+200/right=-220 is direction-correct and approximately matched.
- Before the first wheels-down run, calibration `AUTO` was changed from unbounded operation to a required 1–5000 ms duration. Timeout forces a stop and full disarm; autonomous-at-boot remains disabled. The measured 200/220 baseline plus provisional 250 ms pivot build successfully (RAM 21,552 B / 6.6%; application flash 273,109 B / 20.8%), cppcheck passes with only its three expected Arduino/constant-folding low findings, and targeted `git diff --check` passes.
- The user then requested a GPIO16/GPIO33 role swap: right-motor L298N IN4 moves from GPIO16 to GPIO33, and future DFPlayer TX/UART2 RX moves from GPIO33 to GPIO16; UART2 TX remains GPIO17 through 1 kΩ to DFPlayer RX. Because the prior direction tests used IN4 on GPIO16, the new physical wire and rebuilt image require one wheels-up right-motor forward/reverse recheck before any floor run.
- **Swap validated on hardware.** After the user physically moved IN4 to GPIO33 (GPIO16 disconnected, wheels raised, motor rail off), the guarded image was flashed and a timed `MOTOR 0 200 700` right-wheel test drove the physical right wheel **forward**, confirming GPIO33→IN4 preserves the previously measured direction. All prior motor calibration (left=200/right=220, thresholds, pivot) remains valid under the new pin map.
- Post-swap verification passes: the safe calibration image rebuilds at 21,552 B RAM (6.6%) and 273,109 B application flash (20.8%); cppcheck remains zero high/medium with three expected low findings; targeted `git diff --check` passes. The image is intentionally not flashed until the user physically moves L298N IN4 from GPIO16 to GPIO33, preventing firmware/wiring mismatch during a motor test.
- The user confirmed IN4 was physically moved to GPIO33, GPIO16 was left disconnected, wheels were raised, and the motor rail was off. A fresh identity gate found only the expected CH340 `1a86:7523` at stable by-id `usb-1a86_USB_Serial-if00-port0` -> `/dev/ttyUSB0`, with no owner. Upload to the same ESP32-D0WD-V3 rev 3.1 / MAC `1c:c3:ab:3c:8c:38` passed all flash hash checks. A deliberate boot capture reported `RIGHT = ENB13 IN3=14 IN4=33`, autonomous output `DISABLED`, and `calibration mode; motors disabled`; the new map is live and stopped.

## Current Hardware Planning — Track Hub and Robot (2026-08-05)

### Goal

Produce two implementation-ready, electrically safe wiring plans for the 30-pin ESP32-WROOM-32 platform:

1. Finalize the track Hub while preserving its existing microSD interface: one 56-pixel WS2812B chain, four real DHT11/MQ-135 pairs, three honestly simulated station pairs, four shared-command SG90 servos, buzzer, and latched emergency button.
2. Design the robot around a separate main ESP32 controlling DFPlayer/audio, a five-channel IR array, L298N dual motor driver, RC522 over SPI, and one eight-pixel WS2812B board. Treat the fixed-screen Arduino + display shield and unchanged ESP32-CAM as separately powered assemblies with no main-ESP32 data link.

### Phases

| Phase | Status | Outcome |
|---|---|---|
| H1. Restore and reconcile context | complete | Existing Hub source, stale wiring plan, exact 56-pixel LED topology, and user decisions recovered. |
| H2. Finalize track electrical architecture | complete | Existing SD retained on GPIO18/19/23/5; real sensor pairs distributed at stations 1/3/5/7; stations 2/4/6 simulated with source flags; final GPIO map, 56-pixel chain, 5 V/10 A distribution, protection, validation, and BOM documented. |
| R1. Gather robot module constraints | complete | Confirmed 30-pin ESP32, 3.3 V five-output IR array, two 6 V TT motors, 3S source, L298N, DFPlayer + LM386, 4-ohm/3 W speaker, one eight-pixel board, and no Mega data link. |
| R2. Allocate robot GPIO and buses | complete | Boot-safe map assigns five IR inputs, dual motor PWM/direction, RC522 VSPI, DFPlayer UART2, and WS2812B while preserving USB UART and strapping pins. |
| R3. Design robot power/audio architecture | complete | Separate 6 V motor and 5 V logic bucks plus dedicated camera/Mega branches, star grounding, suppression, correct DAC-to-LM386 path, and validation gates documented. |
| D1. Publish plans for approval | in progress | Both plan documents and BOMs now reflect final hardware choices, including the locked 74HCT125 and HW-209 rejection; present them and obtain user approval before implementation. |

### Locked decisions

- The seven track LED boards follow track order and form one DOUT-to-DIN chain: 56 pixels on one data GPIO.
- Hub microSD remains enabled and unchanged on SCK GPIO18, MISO GPIO19, MOSI GPIO23, and CS GPIO5.
- Track stations 1, 3, 5, and 7 have real DHT11/MQ-135 pairs. Stations 2, 4, and 6 are generated from neighboring valid measurements and remain explicitly marked simulated/degraded/unknown.
- The locally available 74HCT125 DIP-14 is locked as the 5 V push-pull WS2812B buffer for both Hub and robot. The pictured HW-209 BSS138/passive-pull-up board is not used for final LED data.
- Track emergency behavior is latch ON after a short press; clear from the admin dashboard or after a continuous five-second hold. Future Hub-to-robot signaling will use UDP.
- Robot firmware implementation was not part of the 2026-08-05 planning phase; the 2026-08-06 Phase-1 LFR request above explicitly supersedes that earlier scope boundary.
- The Arduino + shield and ESP32-CAM are separate robot assemblies; each receives regulated power from the robot distribution system and neither communicates with the main robot ESP32 in this design.
- Do not provide effort/time estimates.
- The command-execution tool is not exposed in the current Architect tool surface, so the skill-requested `git diff --stat` check cannot be run here; use the supplied workspace state and direct file reads instead.
- Track plan: `plans/hub-sensor-wiring.md`.
- Robot plan: `plans/robot-wiring.md`.

## Auxiliary Diagnosis — pskill9 Web Search MCP

| Phase | Status | Purpose |
|---|---|---|
| Reproduce and isolate | complete | Confirm whether the empty result is query-specific, MCP transport-related, or provider-specific. |
| Inspect implementation and upstream response | complete | Identified the installed server as a stale Google HTML scraper (`/home/user/.pskill9-web-search/build/index.js`); Google currently returns a 91 KB consent/interstitial-style page with zero `div.g` result containers. |
| Report remediation | complete | Root cause: Google scrape serves an unparseable page that the stale scraper reports as an empty array. Fix applied. |
| Implement remediation | complete | Patched the installed `build/index.js` (v0.1.0 -> v0.2.0): modern UA/headers, `hl=en`+`num`, multi-selector extraction, blocked/interstitial detection, DuckDuckGo HTML fallback with URL dedup, and a descriptive error when both providers yield zero results. Backed up the original to `build/index.js.bak-20260805-134628`. Validated end-to-end over MCP stdio: two queries returned 5 and 3 unique relevant results via the fallback; `node --check` passes. |

## Goal (updated 2026-08-04)

## Lessons Learned — ESP-CAM→Hub image pipeline (DO NOT REPEAT)

1. **Never pass binary through Arduino `String`.** `String(plainBuf)` stops at the first NUL byte. JPEG begins `FF D8 FF E0 00…`, so it truncates to 4 bytes. Use a binary-safe framework path. On this installed Arduino-ESP32 core, repeated physical tests showed the `HTTPRaw` callback can watchdog the Hub; the selected receiver is therefore standard `multipart/form-data` through `HTTPUpload`, which exposes binary chunks without routing JPEG bytes through `String`.
2. **HTTP 200 is not payload verification.** The broken Hub returned 200 while storing 4 bytes. Always cross-check: sender byte count ↔ receiver commit log ↔ persisted file size ↔ served `Content-Length` ↔ JPEG SOI `FFD8` + EOI `FFD9` ↔ actual decode/dimensions. A single green checkmark is meaningless.
3. **A successful compile/upload does not prove the runtime is correct.** The first "successful" flash left stale firmware running; only a simultaneous serial trace showing the NEW log line (`[img] committed` vs the old `[img] upload … bytes=4`) proved which build was live. Always capture the uploader's full output with a preserved exit code, and confirm runtime identity from a diagnostic string unique to the new build.
4. **`WebServer` query args are not available in `RAW_START`.** This core parses the URL query only after the raw-body lifecycle. Metadata needed during raw upload must travel in collected request headers (`server.collectHeaders()`); metadata only needed at completion can use the query-arg fallback.
5. **Don't trust partial terminal updates / `/bin/sh` vs Bash.** A logging wrapper using `set -o pipefail` silently aborted before PlatformIO ran. Use POSIX-safe redirection (`> log 2>&1; echo $?`) and never repeat a shell-incompatible command.
6. **Do not change the development laptop away from `Lovly`.** The Hub SoftAP has no internet and switching networks interrupts the user's API/chat access. Keep both `MuseumHub` profiles at `autoconnect=no`; use USB serial for firmware evidence and a phone connected to `MuseumHub` for dashboard/API verification.

## Goal

Inventory the Smart Museum Guidance System, verify its runnable subsystems on connected hardware, repair the Hub QR flow, and publish the proven Arduino Mega robot-face display as a small Arduino IDE project.

## Phases

| Phase | Status | Purpose |
|---|---|---|
| 1. Establish repository state | complete | Check working-tree changes, project documents, dependencies, and subsystem boundaries. |
| 2. Audit implementation | complete | Inspect firmware, web application, shared protocol, and tooling for implemented versus placeholder behavior. |
| 3. Verify build/runtime readiness | complete for Hub scope | Built, flashed, boot-verified, deployed LittleFS, and exercised the live Hub HTTP/API path on the connected ESP32. Non-Hub targets remain source-audited skeletons rather than physical integrations. |
| 4. Determine immediate scope | complete | Confirmed the Hub visitor/admin demo is testable now; robot, health, emergency-node, and camera behavior requires implementation and hardware integration. |
| 5. Report prioritized roadmap | complete | Provide the user with the exact phone/laptop QR workflow, verified controls, limitations, and next hardware priority. |
| 6. Repair mobile QR capture | complete | Replaced the unreliable synthetic click/hidden input, bundled jsQR 1.4.0, added token validation and decode errors, disabled stale asset caching, reflashed firmware/LittleFS, and verified the files served by the physical ESP32. |
| 7. Build the Mega display target | complete | Added a separate PlatformIO target for the Mega 2560 and the physical 480x320 16-bit ILI9481 shield, then verified controller startup and command handling on hardware. |
| 8. Refine and lock the robot face | complete | Replaced the original face, then eliminated blink/gaze tearing with incremental scanline and pupil dirty-region rendering. The user confirmed both motions are smooth. |
| 9. Remove the voice badge and reflash | complete | Removed the face language badge, rebuilt, uploaded, flash-verified, and selected the happy face on the connected Mega. |
| 10. Publish an Arduino IDE edition | complete | Converted only the locked smiling face into a dependency-free Arduino IDE sketch and README in the renamed `3.2-inch-TFT-LCD-ILI9481-480X320-display` repository. Preserved the physically tested ILI9481 implementation and exact 16-bit Mega shield mapping. |
| 11. Verify and publish | complete | Compiled the standalone sketch for `arduino:avr:mega`, uploaded it to the connected Mega for equivalence validation, committed and pushed only standalone files, and verified the remote commit `941acb7` on `main`. |
| 12. Restore the project display application | complete | Reflashed the full PlatformIO display target after standalone validation so the robot regained its boot, language, station, emergency, UART, and animated-face screens. Kept the smooth face and its hidden voice badge. |
| 13. Verify restored screen workflow | complete | Exercised menu, Bangla selection, narrating/moving station, emergency, boot, and happy-face commands on the physical Mega; all commands echoed cleanly. Left the board on the happy face and reconciled project documentation without changing the standalone GitHub release. |
| 14. Inspect DFPlayer SD-card preparation prerequisites | complete (blocked before destructive work) | Read-only identification found the USB reader at `/dev/sda`, but it exposes zero-byte media, so card capacity/content cannot be confirmed. Installed tools, conservative FAT/layout, bilingual track map, licensing strategy, and the exact future confirmation gate are documented; no block-device changes occurred. |
| 15. Re-inspect SD card after reader swap | complete (still blocked: unreadable medium) | After the user connected the SD card through a different reader, `/dev/sda` now reports nonzero capacity (~15.6 GB / 14.6 GiB) but every block read fails at sector 0 with a SCSI Medium Error / "Unrecovered read error" and "unable to read partition table". No valid partition table, filesystem, label, UUID, or mountpoint is exposed, and the card is not mounted. `/dev/nvme0n1` remains the mounted 256 GB Samsung internal OS disk and is unchanged. Erase confirmation remains **not permitted** because the medium is physically unreadable; the card/reader must be made readable before any preparation step. No device was formatted, partitioned, erased, mounted, unmounted, or written. |
| 16. Re-inspect replacement SD card | complete (still blocked: unreadable medium) | The user swapped in a different 16 GB microSD. A genuinely different card is confirmed: the kernel logged `sda: detected capacity change from 31207424 to 0` — the new card briefly announced 31,207,424 sectors × 512 B = 15,978,204,928 bytes (~14.9 GiB / ~16.0 GB), distinct from the prior 30,535,680-sector card, before every block read failed with the identical SCSI Medium Error / "Unrecovered read error" at sector 0 (and sector 24). Sysfs size then collapsed to 0. No partitions/filesystem/label/UUID/mount are exposed. `/dev/nvme0n1` (Samsung, NVMe, 256 GB, mounted OS disk) is unchanged and clearly distinct. Erase confirmation remains **not permitted**. Because this is the second distinct card failing identically in the same `Super Top SY-T18` reader, the reader is now a strong suspect; the user should try a known-good microSD in this reader (or these cards in a different reader) to isolate card vs reader before retrying. No device was formatted, partitioned, erased, mounted, unmounted, or written. |
| 17. Re-inspect two simultaneously connected cards | complete (BOTH HEALTHY & READABLE; reader-suspect DISPROVEN) | With two cards/readers connected at once, the inventory found `/dev/sda` (Super Top SY-T18, `14cd:1212`, serial `121220160204` — the *former suspect*) now reading cleanly: MBR/DOS table, one vfat partition `sda1` (UUID `0403-0201`), 15,978,201,088 bytes, auto-mounted at `/media/user/0403-0201`, **zero** kernel Medium/Read errors. A genuinely **distinct second reader** is `/dev/sdb` (Generic `1908:0226`, serial none, on USB port `3-2`): GPT, 15,634,268,160 bytes, 4 partitions (`sdb1` iso9660 `Ubuntu_24.04.4_LTS_amd64`, `sdb2` vfat ESP, `sdb3` tiny gap, `sdb4` ext4 `writable`), two auto-mounted; ext4 journal recovery completed cleanly at 16:58:04, proving every block is readable. **Both cards independently classified healthy/readable.** The prior "reader is the common-cause fault" hypothesis is DISPROVEN: the former-suspect SY-T18 reads a card fine, and the second reader/card also reads fine once its earlier USB enumeration flakiness (`error -71` at 16:53–16:54) stabilized at 16:58. `/dev/nvme0n1` unchanged and distinct. Because `/dev/sdb` holds a complete bootable Ubuntu 24.04.4 image plus install logs (irreplaceable-looking content), it is a **poor** DFPlayer erase candidate; `/dev/sda` (empty 16 GB vfat) is the safe DFPlayer candidate but the user must still confirm identity/capacity and accept erasure. No device was formatted, partitioned, erased, mounted, unmounted, or written. |
| 18. Fresh read-only preflight for "erase and prepare both" | complete (preflight only; NOT erased — awaiting exact per-device confirmation) | Several hours after Phase 17 the user said "erase and prepare **both**". This generic phrase is **not** a valid destructive confirmation. A full fresh read-only inventory re-confirmed all Phase 17 identities unchanged and stable: `/dev/sda` (SY-T18 `14cd:1212`, serial `121220160204`, 15,978,201,088 B, MBR, empty vfat `sda1` UUID `0403-0201` → `/media/user/0403-0201`) and `/dev/sdb` (GEMBIRD `1908:0226`, 15,634,268,160 B, GPT, 4 partitions incl. Ubuntu 24.04.4 ISO + ext4 `writable` with install-logs-2026-06-25). by-id/by-path mtimes (16:54/16:58) prove no replug occurred. Clean-readability gate PASSED: all sector-0 Medium Errors in the log are timestamped 16:41–16:45 (the old failed cards); after 16:54:44/16:58:04 the log is clean; the lone post-16:54 "match" (`19:24:52 usb 1-2 reset dev 2`) is the Bluetooth radio, not an SD reader. `/dev/nvme0n1` unchanged and decisively distinct (transport/size/removable/by-id). Suitability: `/dev/sda` SUITABLE for DFPlayer FAT32 (empty); `/dev/sdb` POOR (destroys Ubuntu installer + install logs). Data loss documented. Two populated exact confirmation statements (Statement A `/dev/sda`, Statement B `/dev/sdb`) drafted in findings.md, each binding current identity and excluding `/dev/nvme0n1`. No device was formatted, partitioned, erased, mounted, unmounted, repaired, downloaded to, or written. |
| 19. Erase and format ONE 16 GB card (`/dev/sda`) for DFPlayer | **complete** | User authorized "erase any of the 16GB cards. just don't mess with my ssd!" — selected the empty `/dev/sda` (not the Ubuntu installer `/dev/sdb`). Fresh read-only identity gate passed with zero mismatch (serial `121220160204`, 31,207,424 sectors, MBR). Pre-op evidence recorded. Unmounted ONLY `/dev/sda1` (sdb/nvme mounts untouched). Re-partitioned `/dev/sda` with DOS/MBR + one primary partition start=2048 size=31,205,376 sectors type `0x0c` (W95 FAT32 LBA). Formatted `/dev/sda1` as FAT32 with label **`MSMAUDIO`** (user-chosen: the planned `MUSEUM_AUDIO` exceeded the FAT32 11-char label limit), UUID `6831-01A4`. Read-only `fsck.fat -n` exit 0, clean (1 file = volume-label entry; no audio placed). `/dev/sdb` and `/dev/nvme0n1` UUIDs/layouts/mounts verified unchanged post-op. Temporary zenity askpass helper used and deleted; no sudoers/password/security change. Audio copy is a separate future subtask. |
| 20. Prepare lawful, locally-generated example DFPlayer audio assets | **complete** | Created a repository-local staging dir `tools/dfplayer_audio/` containing `mp3/0001.mp3`–`0014.mp3` (14 distinct, originally-synthesized test cues), a deterministic `generate_tracks.sh`, `README.md`, `TRACK_MAP.csv`, `PROVENANCE.md`, `SHA256SUMS`, and `.gitignore`. English tracks (1–7) = Flite `slt` voice speaking "Station N. <Name>." framed by a damped-sinusoid bell on a pentatonic root; Bangla tracks (8–14) = non-verbal ascending chime motif (root→fifth→octave ×2) because no local Bangla TTS exists (no mispronounced English-as-Bangla). All encoded MP3 CBR 128 kb/s, 44.1 kHz, mono, no metadata, no artwork; ffprobe-validated codec/rate/channels/bitrate/duration on all 14 (3.3–4.4 s, peak −2.1 to −4.4 dB, no clipping); all 14 SHA-256 distinct and `sha256sum -c` OK. Assets dedicated CC0 1.0 by the project; FFmpeg/Flite engine licenses noted as separate (engine ≠ asset). Documented as placeholders to replace with team-written/team-recorded bilingual narration (with consent/provenance). This subtask did NOT touch any block device: `/dev/sda1` stayed `MSMAUDIO`/`6831-01A4` unmounted/unwritten, `/dev/sdb` and `/dev/nvme0n1` unchanged, no card copy/mount performed. Copying to the card is a separate future subtask. |
| 21. Copy the 14 staged MP3 cues onto the prepared `MSMAUDIO` card, verify, eject | **complete** | Restored planning context; freshly re-verified all storage identities before any write. Target `/dev/sda` matched Phase 19/20 exactly (by-id `usb-Mass_Storage_Device_121220160204-0:0`, serial `121220160204`, Super Top SY-T18 `14cd:1212`, 15,978,201,088 bytes, MBR; `/dev/sda1` vfat FAT32 label `MSMAUDIO`, UUID `6831-01A4`, PARTTYPE `0xc`). Re-validated source (`tools/dfplayer_audio/mp3/`): exactly 14 files `0001.mp3`–`0014.mp3`, contiguous, `sha256sum -c` 14/14 OK, 14 distinct hashes, ffprobe all MP3/44.1 kHz/mono/128 kb/s CBR. Mounted only `/dev/sda1` via `udisksctl` → `/media/user/MSMAUDIO` (clean empty root). Created root `/mp3`, copied the 14 numeric files in ascending order with plain `cp` (FAT32 mount writes no desktop metadata), `sync`. Purity gate PASS: target root contains only `mp3`; no `.DS_Store`/`._*`/`.Trash*`/`Thumbs.db`/`desktop.ini`/`System Volume Information`/hidden files anywhere; `/mp3` has exactly 14 `NNNN.mp3`. On-card verification 14/14: SHA-256 match `SHA256SUMS`, byte-identical to source via `cmp`, ffprobe every file MP3/44100/1ch/128000/3.32–4.44 s, names contiguous. Wrote `tools/dfplayer_audio/FIRMWARE_COMMAND_MAP.md` (future ESP32/DFRobotDFPlayerMini `playMp3Folder()` mapping + planned serial `LANG`/`STATION`/`PLAY` commands; reference only, no firmware) and updated `tools/dfplayer_audio/README.md` with copy-status + pointer. Final `sync`, cleanly unmounted `/dev/sda1`, `udisksctl power-off` ejected parent `/dev/sda` (by-id symlink + `/dev/sda` node gone). Excluded disks proven untouched: SSD `/dev/nvme0n1` UUIDs `7CCD-F1DB`/`8677ea8e…` unchanged and mounted; Ubuntu card `/dev/sdb` 4 partitions intact (`2026-02-10-01-39-48-00`/`73B7-988D`/`05c2d1ff…`) — `/dev/sdb` was never named by any write command this task. Privilege via normal polkit `udisksctl` (no sudo needed); no sudoers/credential change. Card is DFPlayer-ready; next task is the ESP32 firmware implementation. |

| 22. Re-prepare the currently attached 8 GB card for DFPlayer | **complete** | User confirmed exact destructive target (`/dev/sda`, by-id `usb-Mass_Storage_Device_121220160204-0:0`, serial `121220160204`, 7,990,149,120 bytes, accept total content loss, do not touch `/dev/nvme0n1`). Identity gate passed before every write; unmounted only `/dev/sda1`. Rewrote DOS/MBR (disk id `0x2f1392c1`) with one primary partition `sda1` start=2048, size=15,603,712 sectors, type `0x0c` (W95 FAT32 LBA). Wiped stale exFAT signature (`wipefs -a`), formatted FAT32 label `MSMAUDIO`, UUID `714A-BBA1`; `fsck.fat -n` clean. Copied only `0001.mp3`–`0014.mp3` into a root `/mp3/` directory in numeric order via plain `cp` (vfat → no desktop metadata). Verification: card root contains only `mp3`; zero desktop metadata / hidden files; 14/14 on-card SHA-256 match `SHA256SUMS`; 14/14 `cmp` byte-identical to source; 14/14 on-card ffprobe MP3/44.1 kHz/mono/128 kb/s. `sync`, clean `udisksctl unmount`, `udisksctl power-off` → `/dev/sda` node gone. SSD `/dev/nvme0n1` verified untouched (UUIDs/mounts unchanged). Temporary `zenity` askpass helper deleted; no sudoers/credential/security change. Card is DFPlayer-ready. |
| 23. Implement + physically test ESP32S-CAM (GC2145) capture-to-onboard-SD node | **complete** | Rewrote the `firmware/cam/` skeleton as a PlatformIO project that captures on a USB-serial command and saves JPEGs to the ESP32-CAM's own microSD slot (no Wi-Fi/Hub yet). Verified against `ESP32 CAM.pdf`: ESP32S-CAM on an ESP32-CAM-MB (CH340) carrier, GC2145 ribbon, AI-Thinker DVP pin map, XCLK 10 MHz, PIXFORMAT_RGB565, QVGA 320×240, 1 fb in PSRAM; GC2145 has no native JPEG so RGB565→JPEG via `frame2jpg()`/`img_converters.h`. SD_MMC in **1-bit mode** (GPIO2/14/15 only) leaves every camera pin + GPIO4 (flash LED) free. Added `include/config.h`, a bounded no-`String` line-oriented command parser (`CAPTURE [station]`, `STATUS`, `LIST`, `INIT`, `FLASH ON/OFF`, `HELP`), sequential collision-safe `/photos/IMG_####.JPG` naming via post-reboot max-index scan, write+close+read-back-verify, and full buffer freeing on every path. Build: **SUCCESS**, RAM 10.7% (35 048 B), Flash 14.3% (450 545 B / 3 145 728 B), `huge_app.csv`, upload_speed 115200. Flash to `/dev/ttyUSB0`: **SUCCESS**, all hashes verified. Chip ID: **ESP32-D0WD-V3 rev v3.1**, MAC `68:09:47:9b:38:d8`. Boot verified: camera PID **0x2145 (GC2145)**, PSRAM ~3.9 MB free. End-to-end **CAPTURE verified** on an 8 GB FAT32 SDHC card: `IMG_0001.JPG` (1829 B), `IMG_0002.JPG` (1883 B), `IMG_0003.JPG` (1883 B, station=3) — all 320×240, verified; LIST + STATUS (card=7620 MB, fsFree=7602 MB) correct; sequential counter survives, no overwrites. No host block device was formatted/touched. CAM-MB finding: its auto-reset is unreliable from this host — esptool's RTS/DTR hard-reset and pyserial pulses could not reboot the chip; a physical RST press or USB-C replug is required to enter run mode (matches the PDF). The future Hub seam is `uploadToHubJpeg()` (Wi-Fi join + POST `/api/image/upload` + ESP-NOW `MSG_IMAGE_READY`), intentionally unimplemented. |
| 24. Integrate ESP-CAM with Hub dashboard without requiring microSD | **regressed during repeated-upload validation** | The camera RAM capture and one complete 320×240 JPEG transfer were proven, eliminating the original 4-byte `String` truncation. Repeated raw-body uploads then triggered the Hub web-task watchdog: camera responses degraded from HTTP 200 to `-11` (read timeout) and `-1` (connection refused). The `HTTPRaw` transport is therefore not accepted as the final physical solution on this core. |
| 25. Replace unstable raw transport with multipart upload and re-verify | **in progress** | Both projects have been converted to `multipart/form-data` / `HTTPUpload` and compile. The Hub multipart build was flashed; the camera multipart build was not flashed because a broad `pkill -f serial-monitor` terminated its own shell. Before synchronized flashing, fix the Hub upload context so a successful `UPLOAD_FILE_START` clears the error sentinel and rejects oversize chunks before writing. Then flash Hub (`/dev/ttyUSB1`) and camera (`/dev/ttyUSB0`) without changing laptop Wi-Fi, and prove at least three consecutive `SHOOT 1` uploads, valid JPEG storage/serving, dashboard rendering, and continued Hub responsiveness. |

## Deliverable

A concise project map and evidence-based readiness assessment, including:

- what already exists;
- what appears complete, partial, mocked, or missing;
- what can be implemented and tested immediately;
- what is blocked by physical hardware or missing decisions;
- a prioritized next-step plan.

## Constraints and Decisions

- Do not change product code during this audit unless explicitly requested.
- Treat successful compilation as firmware/software readiness, not proof of physical integration.
- Distinguish code inspection confidence from hardware validation confidence.
- Use repository documentation as intended design, then verify claims against source code.
- The standalone repository contains only the smiling animated face: no language/voice badge, menus, station/emergency screens, UART protocol, or PlatformIO files.
- Target the physically verified shield whose rear label says `HVGA 480x320 3.2\" TFT LCD Shield for Arduino Mega 2560 ILI9481 16bit bus`. The GitHub repository has now been renamed to match it.
- Preserve the locked incremental renderer and its motion timings rather than redesigning the face again.
- Optimize the standalone edition for straightforward Arduino IDE use and document all required libraries and board selections.
- The Arduino IDE repository remains a smiling-face-only release. Restoring menus and other robot screens applies only to the main project's `firmware/display/` PlatformIO target and to the connected Mega.
- Restore the existing full screen application rather than adding the removed voice badge back onto the face. `LANG 0` / `LANG 1` still control the language menu and station audio labels.
- For the DFPlayer SD-card preparation phase, do not format, repartition, erase, unmount, or write to any block device until the user confirms the exact candidate device. This inspection subtask may modify only the three root planning Markdown files.
- Phase 15 re-inspection (2026-08-02 16:35–16:56 Asia/Dhaka) is strictly read-only. It performs no format, partition, unmount, mount, erase, download, or write of any kind to any block device, and it does not mount the SD card if it is not already mounted. Only the three root planning Markdown files may be changed.
- Phase 16 re-inspection (2026-08-02 16:45–16:47 Asia/Dhaka, replacement 16 GB microSD) is strictly read-only. It performs no format, partition, unmount, mount, erase, download, or write of any kind to any block device; the SD card was not mounted (it is not mountable). Non-root userspace `dd` of sector 0 was attempted once to `/dev/null` with no loop/retry and returned `Permission denied`; root escalation was deliberately not requested because the kernel block layer's own reads had already failed sector 0 with a SCSI Medium Error, which is more authoritative than any userspace read. Only the three root planning Markdown files were changed.
- Because two distinct SD cards have now failed with the identical sector-0 SCSI Medium Error in the same `Super Top SY-T18` (VID:PID `14cd:1212`) reader, the reader is now a strong suspect alongside the cards. The confirmation gate must not open until a card reads cleanly; isolating card vs reader (known-good card in this reader, or these cards in a different reader) is the required next step before any DFPlayer FAT32 preparation.
- A device that reports nonzero capacity but returns SCSI Medium Error / "Unrecovered read error" on sector 0 is **not** an erase candidate. Readability (a successful block read of sector 0 and a recognizable partition/filesystem signature) is a hard precondition for any future erase-confirmation request; a nonzero size alone is insufficient.
- The candidate SD medium must be distinguished from `/dev/nvme0n1` (the mounted internal Samsung OS NVMe) at every step, and device paths must be re-read after any unplug/replug because they can be reassigned.
- Phase 17 re-inspection (2026-08-02 16:54–17:00 Asia/Dhaka, two cards connected simultaneously) is strictly read-only. It performs no format, partition, unmount, mount, erase, download, or write of any kind to any block device; both cards were already auto-mounted by the desktop before the session and were not mounted/unmounted by this subtask. Only the three root planning Markdown files were changed. Raw userspace device-node reads returned `Permission denied` (non-root, root:`disk` 0660 nodes); passwordless sudo was unavailable and was declined because kernel/FS-layer evidence was already conclusive.
- **The Phase 15/16 hypothesis that the `Super Top SY-T18` (`14cd:1212`) reader is a common-cause fault is DISPROVEN.** On 2026-08-02 that exact reader (with a healthy card in it) enumerated and read cleanly with zero Medium Errors. The earlier failures were therefore caused by the *cards* in it at those times (or their seating/contacts), not by the reader. The reader should no longer be treated as suspect.
- Two genuinely distinct readers are confirmed connected: `/dev/sda` (Super Top SY-T18, `14cd:1212`, serial `121220160160204`, DOS/MBR, empty vfat) and `/dev/sdb` (Generic, `1908:0226`, no serial, GPT, Ubuntu live USB). They differ by VID:PID, vendor/model, USB port/controller, by-id name, partition scheme, and capacity.
- `/dev/sdb` holds a complete Ubuntu 24.04.4 LTS installer + persistent ext4 `writable` with install logs. It is readable and healthy but is a **poor/risky** DFPlayer erase candidate and must not be erased unless the user explicitly confirms they do not need the Ubuntu image.
- `/dev/sda` is the recommended DFPlayer candidate: removable, not the NVMe, cleanly readable, valid MBR + vfat, empty (no content at risk). Its confirmation-gate preconditions are satisfied; the exact confirmation sentence is drafted in findings.md and must quote fresh values re-read immediately before any erase.
- Phase 18 (2026-08-02 ~19:48 Asia/Dhaka) is a strictly read-only preflight in response to the user's "erase and prepare both". It performs no format, partition, unmount, mount (the desktop had already auto-mounted both cards; nothing was mounted/unmounted by this subtask), erase, repair, download, or write to any block device. Only the three root planning Markdown files were changed. Raw userspace reads on the 0660 device nodes returned `Permission denied` (non-root; passwordless sudo unavailable and not requested); the kernel/FS-layer evidence (lsblk/udevadm/findmnt/ls/du/journalctl) is authoritative and conclusive.
- The user's generic phrase "erase and prepare both" is **not** a valid destructive confirmation because (a) it does not bind the **current** exact device path, stable by-id/by-path, capacity, model/serial, or partition/filesystem layout; (b) device paths are reassigned on unplug/replug and several hours elapsed since the last inventory, so current identities had to be re-derived rather than assumed; (c) it covers `/dev/sdb`, which holds non-redownloadable install logs whose loss the generic phrase does not evidence; (d) it does not explicitly exclude `/dev/nvme0n1`. Two populated exact confirmation statements (Statement A for `/dev/sda`, Statement B for `/dev/sdb`) are drafted in findings.md and must be affirmed individually before any block-device write occurs. A bound confirmation must name the exact current path, quote the freshly observed stable by-id/capacity/model/serial/layout, state acceptance of total content loss, and explicitly say not to touch `/dev/nvme0n1` (and the other card).
- Fresh Phase 18 verification confirms both removable cards are healthy/readable and stable since Phase 17 (by-id/by-path mtimes 16:54/16:58, no replug). `/dev/sda` is SUITABLE for DFPlayer FAT32 (empty); `/dev/sdb` is a POOR candidate because erasing it destroys the complete Ubuntu 24.04.4 installer image + ESP + the ext4 `writable` partition containing `install-logs-2026-06-25.0/.1/.2/.3`, and replaces its GPT with MBR.
- Phase 20 (2026-08-02 ~20:45 Asia/Dhaka) is an asset-staging subtask. It does NOT format, repartition, erase, mount, unmount, write to, or copy audio onto any block device. No network downloads were used. It only creates files under the repository-local `tools/dfplayer_audio/` staging directory and updates the three root planning Markdown files. Post-subtask read-only verification confirmed `/dev/sda1` remained `MSMAUDIO`/`6831-01A4` unmounted and unwritten, and `/dev/nvme0n1` unchanged. Copying the staged `mp3/` contents onto the `MSMAUDIO` card is a separate future subtask requiring its own confirmation.
- The 14 DFPlayer example tracks are PROJECT-GENERATED original synthesis (FFmpeg `aevalsrc` tones + English Flite `slt` speech for tracks 1–7; non-verbal chime motifs for tracks 8–14). No external copyrighted sound was read, downloaded, or relabeled. They are dedicated CC0 1.0 by the project and documented as PLACEHOLDERS to be replaced by team-written/team-recorded bilingual narration with consent/provenance. The FFmpeg/Flite engine licenses apply to the software, not to the generated audio output.
- Bangla placeholder tracks deliberately carry NO speech: there is no local Bangla TTS engine, and synthesizing Bangla words with an English voice would produce mispronounced gibberish. They use distinct musical cue patterns instead. This is an explicit design choice, not an omission.
- Phase 21 (2026-08-02 ~20:52 Asia/Dhaka) copied the 14 staged MP3 cues onto the prepared `MSMAUDIO` card. It wrote only to `/dev/sda1` (and its parent `/dev/sda` for eject) via explicit path. Every operation was gated on a fresh read-only identity re-verification that matched Phase 19/20 exactly. The Ubuntu card `/dev/sdb` and the internal SSD `/dev/nvme0n1` were never named by any write command and were proven intact post-task (all UUIDs/labels/mounts match baseline). Privilege was via normal polkit (`udisksctl mount/unmount/power-off`); no `sudo`, no sudoers change, no stored credential, no `root` escalation.
- For the on-card copy, only the contents of `tools/dfplayer_audio/mp3/` (the 14 numeric files) were placed in a root-level `/mp3/` directory. No README, `generate_tracks.sh`, `TRACK_MAP.csv`, `PROVENANCE.md`, `SHA256SUMS`, `.gitignore`, or any other repository/manifest file was written to the card — the DFPlayer must see a clean audio directory. Verification used on-card SHA-256 (14/14 vs `SHA256SUMS`), byte `cmp` (14/14 identical), and on-card `ffprobe` (all MP3/44.1 kHz/mono/128 kb/s).
- Future ESP32 DFPlayer firmware (next task) must use `playMp3Folder(track)` (deterministic `/mp3/NNNN.mp3` folder mode), NOT global copy-order `play(n)`. Track formula: English track = station id `1..7`; Bangla track = station id + 7. The planned serial command protocol (`LANG 0/1`, `STATION <n>`, `PLAY <t>`, `STOP`, `VOL <0..30>`, `STATE`) and DFRobotDFPlayerMini API surface are documented in `tools/dfplayer_audio/FIRMWARE_COMMAND_MAP.md` as a reference only; no firmware is implemented there yet.

- Phase 22 must replace exFAT with FAT32. Immediately before destructive work, re-confirm `/dev/sda` still resolves to by-id `usb-Mass_Storage_Device_121220160204-0:0`, serial `121220160204`, size `7,990,149,120`, removable USB media; abort on mismatch. Never target `/dev/nvme0n1`.

- Phase 22-cam (ESP32S-CAM node) did NOT format, repartition, erase, mount, unmount, or write any host block device (`/dev/sda`, `/dev/sdb`, `/dev/nvme0n1`). All photo storage is on the ESP32-CAM module's own onboard microSD slot via SD_MMC in firmware only. The 8 GB FAT32 card used for the capture test was inserted by the user into the onboard slot; it was not prepared or modified by this task.

- The ESP32-CAM-MB (CH340 carrier) auto-reset is unreliable from this Linux host: esptool's RTS/DTR hard-reset and pyserial line-control pulses repeatedly failed to reboot the chip (esptool kept reporting "Stub is already running"). To run the flashed firmware, a physical RST press or a USB-C replug is required. This matches the manual IO0/RST handling documented in `ESP32 CAM.pdf`. Future bring-up scripts must not rely on software reset of the CAM-MB.

- GC2145 (PID 0x2145) has **no native JPEG output**. The cam node always captures RGB565 and converts to JPEG in software via `frame2jpg()`. Standard `PIXFORMAT_JPEG` / OV2640 examples will not work on this board. Stable settings: XCLK 10 MHz, PIXFORMAT_RGB565, QVGA 320×240, 1 fb in PSRAM.

- SD_MMC runs in **1-bit mode** (GPIO2/14/15 only) so it does not conflict with the camera DVP bus (GPIO5/18/19/21/22/23/25/26/27/32/34/35/36/39) and leaves GPIO4 (flash LED) free. 4-bit SD mode (which also takes GPIO4/12/13) is intentionally NOT used.

## Errors Encountered

| Error | Attempt | Resolution |
|---|---:|---|
| PlatformIO 6.5.0 resolved Arduino-ESP32 2.0.14 although an older project comment expected 2.0.17 | 1 | The required legacy channel-based LEDC API is present and the build passes. Record the actual resolved version rather than claiming 2.0.17; reproducibility remains pinned at the PlatformIO platform/package level available in this environment. |
| Calibration-command build failed because `handleCommand()` called `readSensors()` before its definition | 1 | Added a local forward declaration after `SensorReading`; no upload occurred in the failed command because command chaining stopped at compile failure. |
| Resume-checkpoint patch targeted wording from this plan rather than the exact progress-log line | 1 | The patch failed atomically and changed no file. Re-read the exact `progress.md` text and applied a smaller context-accurate patch. |
| Nano/ESP wiring-plan patch was truncated before its end marker | 2 | Both malformed patches were rejected atomically and changed no file. Stop using broad patch payloads; migrate the document with small section-scoped patches. |
| PlatformIO Nano-board JSON filter command had an unterminated shell quote | 1 | The command failed before execution and changed nothing. Replaced it with simple `pio boards atmelavr | grep` queries, which identified the exact board IDs. |
| First shared robot-link header patch was truncated before `*** End Patch` | 1 | The patch parser rejected it atomically, so no partial header existed. Retried with a complete bounded patch. |
| A web-search tool call was accidentally issued without its required query while researching robot modules; the mode adapter then reported the direct MCP tool name unavailable | 1 | No research result or file change occurred. Do not repeat the malformed call; use existing primary documentation already recovered or the current mode's generic MCP adapter only if another source is necessary. |
| `ModuleNotFoundError: No module named 'qrcode'` while generating gate QR images | 1 | Install the QR utility dependencies from the project requirements, then rerun generation. |
| QR output verification wrapper used Bash-only `compgen` under `/bin/sh` | 1 | Do not infer generation failure from the wrapper; inspect the output directory with a portable file-list operation. |
| Mobile `Scan QR` control does not open a camera/picker on the user's phone | 1 | Root cause in source: a scripted click targets an input with the `hidden` attribute, which some mobile browsers suppress; the referenced offline decoder is also absent. Replace it with a direct user-activated file input and bundle the decoder. |
| Process-wait command matched its own `pio run` pattern and slept after uploads finished | 1 | Confirmed no real PlatformIO/esptool process remained, stopped only the wrapper, and captured a clean board boot log. Uploads were unaffected. |
| `UnknownPackageError` for `prenticedavid/MCUFRIEND_kbv` in display `lib_deps` | 1 | The PIO registry key is `MCUFRIEND_kbv` without the owner prefix; corrected `lib_deps` and the build succeeded. |
| Arduino Mega not detected on USB (`lsusb` shows no `2341:` device, no `/dev/ttyACM*`) | 1 | Firmware compiles and is ready to flash. The board needs a data-capable USB cable and power; flashing will proceed once the board enumerates. |
| MCUFRIEND shield reported controller ID `0xD3D3` | 1 | Recognized this as a write-only 16-bit shield and forced the physically verified ILI9481 ID `0x9481`. |
| First display build used the default Uno-style MCUFRIEND bus mapping | 1 | Enabled `USE_SPECIAL` and `USE_MEGA_16BIT_SHIELD` so the library uses the correct Mega 16-bit parallel pin map. |
| USB face commands lost leading bytes or contained null/control noise | 1 | Split USB and UART receive buffers and filtered Serial1 control bytes. |
| Whole-eye redraws produced visible tearing during gaze and blink | 2 | Locked a small dirty-region renderer: pupil footprint restoration for gaze and paired incremental scanlines for eyelids. User confirmed both animations are smooth. |
| Primary web-search provider returned HTTP 429 while locating the official DFPlayer Mini storage documentation | 1 | Do not retry the exhausted provider; use an alternate search/fetch path and prioritize DFRobot's official wiki/library sources. |
| Replacement 16 GB microSD also fails with SCSI Medium Error at sector 0 in the same reader | 1 (second distinct card) | The new card briefly announced 31,207,424 sectors (~16.0 GB) then collapsed to 0 after every block read failed with the same `Sense Key : Medium Error` / `Unrecovered read error` as the prior card. Because two different cards fail identically in the same `Super Top SY-T18` reader, the reader is now a strong common-cause suspect. Next step is to isolate card vs reader with a known-good microSD in this reader or these cards in a different reader; erase/format must not be attempted on read-failed media. |
| Planning patch could not match the extremely long Phase 21 row | 2 | Re-read the exact section and inserted Phase 22 at a stable section header instead; neither failed attempt changed any file. |
| ESP32-CAM-MB serial output was completely silent after flashing (no boot banner, no command response) | 4+ | esptool always reported "Stub is already running" and its RTS hard-reset / pyserial DTR-RTS pulses could not reboot the chip from this host. Resolved by a physical RST press (then USB-C replug after the CH340 dropped off USB during card insertion). Root cause confirmed: the carrier's auto-reset is not software-controllable here, matching `ESP32 CAM.pdf`. |
| Inserting the onboard microSD dropped the CAM-MB's CH340 off USB (`/dev/ttyUSB0` disappeared, lsusb no longer listed it) | 1 | The ESP32-CAM was nudged in its CAM-MB socket during card insertion. Replug the USB-C cable (card seated) → CH340 re-enumerated at `/dev/ttyUSB0` and the board cold-booted into the firmware. |
| First autocap captured no boot banner and a stray byte `""` parsed as an unknown command | 1 | Cosmetic only: the 5 s initial read started after the banner had printed, and one noise byte reached the parser. Harmless — the parser correctly rejected it with `[cmd] unknown "" -- send HELP`. No fix required. |
| Revised no-SD CAM upload failed with `Device or resource busy: /dev/ttyUSB0` | 1 | `fuser`/`lsof` identified only PID 64068 (`serial-monitor`) holding the CH340 port. Stop that exact monitor, verify the port is free, then retry the already-successful build rather than rebuilding or changing firmware. |
| Hub reports HTTP 200 but dashboard shows `last 4 bytes` and no image | 1 | Root cause is binary truncation, not Wi-Fi or JPEG encoding: the three-argument `WebServer` POST parser constructs `String(plainBuf)`, which stops at the first JPEG NUL byte (after `FF D8 FF E0`). Use the four-argument route and `HTTPRaw` chunk callback; never pass arbitrary binary through Arduino `String`. Also reject invalid/incomplete bodies instead of returning unconditional 200. |
| First post-fix physical retest still served the old 4-byte image | 1 | Hub firmware built and uploaded, but later serial capture produced a continuous stream of `1` characters and no `[img] committed` line; camera `SEND` therefore reached a non-working/stale Hub runtime and HTTP 200 was not sufficient evidence. Resolved by a fully-captured reflash (preserved true exit code) + simultaneous serial trace that proved the new `[img] committed` log. Never infer flashed-runtime correctness from uploader exit alone. |
| Binary-safe Hub committed 1,981 bytes but logged `station=0` | 1 | In this WebServer core, URL query arguments are not reliably exposed during `RAW_START`; parsing station there loses `?station=1`. Fixed by (a) collecting `X-Museum-Station`/`X-Museum-Bytes` headers for future senders and (b) a query-arg fallback in the completion handler for already-flashed cameras. The `[img] committed` log shows station 0 because it prints in RAW_END before the completion handler runs; the state JSON reflects the corrected station. |
| `/api/state` intermittently returned empty under concurrent captive-portal/browser load | 1 | The Hub's single-threaded WebServer + a phone/STA captive-portal probe storm can briefly stall `/api/state`, while `/api/image` still serves correctly. Transient, not a logic error. For reliable validation, isolate MuseumHub (no browser tabs open) or retry. |
| Upload-log wrapper used `set -o pipefail` under `/bin/sh` | 1 | The wrapper exited before PlatformIO ran, so this attempt did not touch flash. Use a POSIX-safe log-file wrapper or explicitly invoke Bash; do not repeat the same shell-incompatible command. |
| Binary-safe Hub committed 1,981 bytes but logged `station=0` | 1 | In this WebServer core, URL query arguments are not reliably exposed during `RAW_START`; parsing station there loses `?station=1`. Preserve metadata independently of raw-body parsing (for example a collected `X-Museum-Station` request header) and validate it at raw start. Also avoid calling `LittleFS.remove()` for known-nonexistent temp files to suppress misleading VFS errors. |
| Camera metadata-header firmware did not enter upload mode | 1 | Hub final metadata build flashed successfully. CAM-MB upload returned `Failed to connect ... No serial data received` before writing any flash, so the working camera application remains intact. Do not improvise IO0/reset handling; follow the user's proven normal CAM-MB workflow and use a less invasive compatibility route if metadata can be recovered at Hub completion without reflashing camera. |
| Hub raw callback stalled after temp-file creation failed | 1 | The trace showed `/littlefs/last.tmp does not exist, no permits for creation`, followed by the web-task watchdog. The installed FS API exposes an explicit `create` argument; open a new temporary path with `LittleFS.open(path, FILE_WRITE, true)`. Never continue a raw lifecycle with a closed file handle, and verify the Hub remains responsive after both successful and rejected uploads. |
| Repeated binary-safe raw uploads made the Hub time out and then refuse connections | 2 raw-handler revisions | Physical Hub serial showed the web-task watchdog; camera responses progressed HTTP 200 → `-11` (read timeout) → `-1` (connection refused). Stop iterating on `HTTPRaw` for this core; use standard multipart `HTTPUpload` and require three consecutive successful uploads plus a responsive Hub. |
| Camera multipart flash command terminated itself with SIGTERM | 1 | The command contained `pkill -f 'serial-monitor'`, so the broad pattern matched its own shell command line. Never use broad `pkill -f` here. Inspect `/dev/ttyUSB0` with `fuser`, terminate only the exact owning monitor PID if present, verify the device is free, then run the normal PlatformIO upload. |
| Multipart Hub source initializes every request with `error="incomplete_body"` and never clears it | 1 (found before final synchronized test) | A non-null error makes `handleImageUploadComplete()` reject an otherwise valid request. Initialize a fresh upload with `error=nullptr` and success-ready status, set an error only on a real failure, guard maximum size before every write, then rebuild/reflash the Hub. |
| Opening CP2102 serial with host DTR/RTS asserted repeatedly reset the Hub | 1 | A dual-port pyserial probe caused repeated ESP32 boot banners and noisy `not found` output. Do not use host serial control lines casually. Open each port with DTR/RTS explicitly deasserted before opening, or rely on user-side monitors; verify Hub recovery from a clean boot before image tests. |
| Old camera raw POST crashed multipart Hub with `LoadProhibited` | 1 | Backtrace resolves to null `_currentUpload` dereference in `WebServer::upload()` because this core's four-argument route dispatches the same callback from `FunctionRequestHandler::raw()` as well as `upload()`. Replace that route with a custom `RequestHandler`: `canUpload=true`, `canRaw=false`, `upload()` consumes its `HTTPUpload&` argument, and ordinary raw requests receive a clean 400/415 rather than a panic. |
| Fully logged camera multipart upload could not enter bootloader (`No serial data received`) | 1 | Failure occurred before erase/write, so old camera firmware is preserved. Do not improvise IO0/reset or blindly repeat. Use the user's proven normal CAM-MB recovery: unplug/replug the camera USB-C, confirm CH340 `/dev/ttyUSB0`, then rerun the prepared normal PlatformIO upload once. |

## Files Created or Modified

- `task_plan.md` — audit workflow and status.
- `findings.md` — evidence gathered during inspection.
- `progress.md` — chronological activity and verification log.
- `firmware/hub/data/` — staged canonical browser assets used for the successful LittleFS deployment.
- `tools/qr_output/` — generated Wi-Fi, in-page, and native check-in QR images.
- `tools/.venv/` — isolated local environment used to generate QR images.
- `webapp/index.html`, `webapp/css/style.css`, `webapp/js/data.js`, and `webapp/js/qr.js` — mobile capture control, scan feedback, validation, and decoder integration.
- `webapp/js/vendor/jsQR.js` — vendored offline jsQR 1.4.0 browser decoder.
- `firmware/hub/src/main.cpp` — no-cache headers for LittleFS web assets.
- `firmware/display/` — separate tested Mega 2560/ILI9481 display firmware with the locked smooth face renderer and no voice badge.
- `.standalone-display-repo/` — temporary clone of the target GitHub repository; standalone Arduino sketch and documentation are being prepared here.
- `firmware/cam/` — ESP32S-CAM (GC2145) command-driven capture node. RAM-first: GC2145 RGB565→`frame2jpg()`→JPEG in PSRAM→multipart HTTP POST field `image` (SD optional backup). Adds Hub Wi-Fi join, ESP-NOW `MSG_IMAGE_READY` notify, and serial commands `CAPTURE/SHOOT/SEND/WIFI/AUTO/STATUS/HELP`. Multipart source compiles but still requires physical flashing and repeated-upload verification.
- `ESP32_RFID_MP3_RAIL/` + `firmware/hub/{src/main.cpp,include/config.h}` — Hub emergency button now reaches the rail robot over ESP-NOW: burst-repeated transitions on the Hub, and an evacuation mode on the robot that ignores every RFID except the END card. `firmware/common/EspNowCompat.h` is the single shared copy, symlinked into both robot sketches.
- `firmware/hub/src/main.cpp` + `firmware/hub/include/MuseumState.h` — Binary-safe JPEG receiver now being migrated from the physically unstable `HTTPRaw` lifecycle to standard multipart `HTTPUpload`. It writes `/last.tmp`, validates SOI/EOI + size, atomically renames via `/last.bak`, updates `MuseumState` only on success, exposes `image.{bytes,station,seq}`, and serves `/last.jpg`. A request-context initialization bug must be fixed before the final Hub reflash.
