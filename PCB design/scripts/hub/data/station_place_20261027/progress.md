# H-003 ledger — user's station-only placement request, 2026-10-27 local time

- User delegates routing-oriented geometry choices; only currently unplaced station-group parts may move.
- Bridge/context/visible window confirmed; fresh backup before_station_place_20261027-0128.epro2, 449966 bytes.
- Current board: −675..4695 × 0..3975 mil. 77 parts, 35 routed top-copper segments, baseline DRC 16 Physical
  Error + 238 Connection Error. Earlier SB-removal/rocker placement state is not the source.
- Scope: 37 off-board station parts, J10–J17, R31–R37, C3–C9, R41–R44, R51–R54, R61–R67.
  Freeze all other 40 parts, including unrelated off-board R4, R6 and R13. Freeze all free silk, all
  existing copper, outline/holes, nets, footprints and schematic.
- Agent's placement choice: top row S1→S4 left-to-right, lower row S5→S8 right-to-left, vertical ports;
  short seven LED hops; ADC filters under the DevKit near input pins; zone buffers close to the MCU.
- Ruling: keep unrelated unplaced LED-buffer parts R4/R6 and E-stop R13 frozen. The user scoped the
  station group, not all off-board parts. Cost if wrong: those three still need placement later.
- Ruling: preserve all free station label/pin text at its existing location, not regenerate or move it.
  Only component-owned attributes may follow their component. Cost if wrong: free station silk remains
  off-board/stale until the user authorizes a dedicated station-label cleanup.
- Tests first: new model test script observed RED before model existed; eight model checks now pass.
- Offline model has zero target/fixed-body/edge/hole collisions at 20 mil body gaps, 10 mil edge gap,
  40 mil mounting-ring clearance. All MQ cap signal pads <275 mil from the actual matching ADC socket pin.
- First write attempt ABORTED before any move: fixed-part equality guard detected concurrent changes
  to BZ1, D1, R5 and R7–R10. All station targets were unchanged. Do not overwrite the new unrelated edits.
- Ruling: retain original captures with initial_ prefixes, take a fresh full-project checkpoint and
  update only the baseline used to freeze current unrelated state; rerun the complete model before
  writing. Cost if wrong: simultaneous edits could be missed; the write still requires exact live
  equality immediately before touching any target and the final export is compared to the new backup.
- Fresh guarded backup: WRO_HUB_before_station_place_guard_20261027-0148.epro2, 516774 bytes;
  refreshed baseline includes the user's concurrent edits. DRC baseline is now 25 Physical + 234 Connection.
- All eight model tests passed again before writes; writer moved exactly 37 parts and froze 40, saved true.
- Final full export: WRO_HUB_after_station_place_20261027-0148.epro2, 501765 bytes, archive integrity checked.
- Exact-export acceptance GREEN: only 37 COMPONENT poses + 91 component-owned ATTR geometry transforms;
  no other PCB record changes, no other document changes, no new DRC findings. Existing 25 Physical +
  234 Connection findings unchanged. All actual pad-net transforms and fixed-part inventories match.
- Final actual-body-bound check also passes. Broader regression: 22 model tests pass (8 placement,
  7 hub rocker, 7 station). Historical 13 SB tests skip generic discovery by design; placement export
  gate is executed directly and passes. Guarded backup contains 50 routed segments (the user added
  15 since the initial capture), all exact-byte preserved. Archive hashes recorded in summary.json.
- Final self-review: scope and frozen-state guard, pose-only raw deltas, actual pad/body checks,
  archive persistence, and no-new-DRC checks reviewed; no independent reviewer was used.
- Scope limitations retained: no unrelated off-board placement, no free-station-silk movement, no
  routing/DRC repairs, no commit/push/order. No newly deferred code-review minor findings.
