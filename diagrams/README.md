# Diagrams — how the working system fits together

Nine sheets covering the five working projects: `ESP32_RFID_MP3_RAIL`, `NANO_LFR_RAIL`,
`firmware/cam`, `firmware/hub`, and `firmware/display`.

Everything on these sheets was read out of the running code. Where a README or an old
note disagreed with the code, **the code won** — and the sheets say so where it happens.

## Opening them

Each `.drawio` file opens directly in the Draw.io extension in VS Code, or at
[app.diagrams.net](https://app.diagrams.net) via *File → Open*. They are fully editable —
boxes are grouped into their outlined areas, so dragging an area moves its contents.

To save a picture: *File → Export as → PNG*, at 2× scale for printing.

## The sheets

| # | File | What it answers |
|---|---|---|
| 01 | [01_working_combo_system_context.drawio](01_working_combo_system_context.drawio) | The whole system on one page — which links are live, which are ignored, which cannot be wired |
| 02 | [02_board_wiring_map.drawio](02_board_wiring_map.drawio) | Which pin goes to which pin, on every board, and what travels down each wire |
| 03 | [03_radio_and_networks.drawio](03_radio_and_networks.drawio) | Two Wi-Fi networks plus a direct board-to-board radio, and why all three must stay on channel 1 |
| 04 | [04_one_station_stop.drawio](04_one_station_stop.drawio) | One station stop, step by step: card read → stop → talk → photo → moving again |
| 05 | [05_emergency_path.drawio](05_emergency_path.drawio) | The emergency button, press to all-clear, and everything it switches off on the way |
| 06 | [06_robot_states.drawio](06_robot_states.drawio) | Every state the robot can sit in, and the exact thing that moves it to the next one |
| 07 | [07_photo_pipeline.drawio](07_photo_pipeline.drawio) | The life of one photo, and the four places a photo gets thrown away on purpose |
| 08 | [08_hub_two_jobs.drawio](08_hub_two_jobs.drawio) | The two jobs the hub runs side by side, and the rules that keep them from tripping over each other |
| 09 | [09_message_reference.drawio](09_message_reference.drawio) | Every message format: the 24-byte radio packet, all the wire commands, and all the web addresses |

## Which one do you want?

- **Building or rebuilding the rig** → 02, then 03, then 09
- **Understanding how the tour behaves** → 04, then 06, then 05
- **Chasing a bug** → 06 (robot won't move), 07 (photo missing), 08 (hub feels slow),
  09 (a link is dead)
- **Explaining the project to somebody** → 01, then 04

## Reading the sheets

Colour means the same thing on every sheet:

| Colour | Board |
|---|---|
| Teal | Robot ESP32 |
| Blue | Hub ESP32 |
| Amber | ESP32-CAM |
| Indigo | Arduino Nano and the motors |
| Violet | Arduino Mega screen |
| Red | Emergency, or something that failed |

Line style means the same thing too: a **solid** line is live in the current code, a
**dashed** line is either optional, ignored, or impossible to wire — and every sheet
carries its own key in the bottom-left corner.

## Three things the sheets flag as odd

These are all real, and all visible in the code:

1. **The camera's "photo ready" radio ping goes nowhere.** The camera sends it after each
   successful upload; the hub receives it and drops it. Harmless, but it is not doing what
   its name suggests.
2. **Nothing reports where the robot is.** The hub is built to receive location messages and
   the dashboard is built to show them, but no board in these five projects sends one. The
   "Advance robot" button on the staff page is a demo stand-in.
3. **The screen cannot be wired as documented.** The notes say the Mega's RX1 goes to the
   robot's GPIO17 — but GPIO17 is already the DFPlayer wire. The screen works, and is tested
   over USB, but it is not part of the running system today.

## Older files

`system_architecture.*`, `tour_workflow.*` and `sequence.*` are earlier Graphviz sketches
from before the code was read end to end. They are kept for reference but the nine sheets
above supersede them.
