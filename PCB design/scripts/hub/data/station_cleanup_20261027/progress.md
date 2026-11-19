# H-004 ledger — authorized station-silk and R4/R6/R13 follow-up

- Fresh backup WRO_HUB_before_station_cleanup_20261027-0157.epro2, 501763 bytes, ZIP integrity clean.
- Scope: only R4, R6, R13; their own designators; 8 existing station names + 64 pin labels + old off-board title.
  All 74 other components, all routes/nets/outline/holes/footprints/schematic and unrelated silk frozen.
- User already delegated placement judgment; this follow-up explicitly authorizes these previously excluded objects.
- Source net mapping: R4 LED_DIN–GND; R6 LED_BUF–S1_DIN; R13 ESTOP–ESTOP_IO.
- Ruling: put source termination R6 next to U2 output and R13 beside the MCU E-stop input; do not move
  fixed connectors/MCU/driver or drag routes. Cost if wrong: later routing could be awkward; proximity
  and body-clearance tests constrain placement and all existing copper remains unchanged.
- Measured real string/designator bounding boxes and pad shapes before planning. Existing string IDs/texts,
  station-group ordering and pin-label content verified; no blanket silk regeneration.
- RED observed before model existed; first model rejected R6/U2 body overlap and S5 5V/R64-pad proximity.
  Corrected only the proposed R6 pose and S5 label offset. Six offline tests now pass with zero
  scoped silk/pad/edge/text overlaps and zero resistor body/edge/hole problems.
- Ruling: keep original label text/identity and increase pin labels to 32 mil font / 6 mil stroke;
  move only the existing board title as the remaining off-board global text. Cost if wrong: text
  could be clipped or confused; actual-bbox and exact-export checks are required after writing.
- First write guard aborted with ZERO writes: U2 was rotated 180° and C1 moved during planning.
  Preserve both user edits. Refresh the checkpoint and move only the proposed R4/R6 poses to U2's
  current input/output side; do not rotate U2 back. Model and silk tests must pass on refreshed state.
- Refreshed before-write backup WRO_HUB_before_station_cleanup_guard_20261027-0209.epro2,
  496099 bytes. All six tests passed on current U2/C1 geometry before executing.
- Writer saved exactly R4/R6/R13, three designators, 73 existing station/title strings. Other 74 parts frozen.
- Full final export WRO_HUB_after_station_cleanup_20261027-0209.epro2, 495870 bytes, ZIP integrity clean.
- Export acceptance GREEN: only authorized pose/owned-attribute/string geometry and styles changed;
  62 existing copper segments and every other PCB/project record unchanged. Actual 76 adjusted text
  objects have zero pad/edge/other-text overlaps. DRC unchanged: 25 Physical + 230 Connection, no new finding.
- Broader regression: 28 model tests passed; 13 historical SB explicit-archive tests skip generic
  discovery by design. Cleanup full-export acceptance executed directly and passed.
- Final self-review only, not independent fabrication review. No newly deferred minor findings.
  Existing DRC/J3/XT issues remain outside scope; no commit/push/order. Hashes in summary.json.
