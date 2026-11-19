# Hub — latest edited-board fabrication review package

**Revision: 2026-10-28, after the user's R5/SD_SCK/+5V edits and a fresh GND regeneration.**

Use [WRO_HUB_GERBER_20261028_AFTER_USER_EDITS.zip](WRO_HUB_GERBER_20261028_AFTER_USER_EDITS.zip)
for JLCPCB's Gerber/drill **preview and prototype fabrication review**. It supersedes October 27 files.
The matching editable-project archive is [WRO_HUB_PROJECT_20261028.epro2](WRO_HUB_PROJECT_20261028.epro2).

## Verified

- 2,331 newly regenerated GND pieces, 77 components, 633 tracks, 280 vias.
- Fresh ERC clear. No physical PCB DRC findings or independent copper/drill/connectivity violations.
- Current schematic agrees with all 256 named PCB pin memberships across 74 nets.
- All 548 round drill positions/sizes and six plated 7.8 × 1.8 mm slots match the saved geometry.
- Outline is one 136.398 × 100.965 mm rectangle. Use **two layers, 1 oz copper** for the reviewed rules.
- No order or payment has been submitted.

Independent review confirmed the current geometry and prompted regression-tested checker fixes
for rotated/reused slots and floating fill-only islands. Final Hub suite: 41 passed, 13 historical
fixture-dependent cases skipped. See [REVIEW.md](REVIEW.md) and [TEST-RESULTS.txt](TEST-RESULTS.txt).

## Review/hold points before ordering

1. **One Netlist Error / Import Changes synchronization finding remains.** It is not a copper-clearance
   finding, but its exact ECO cause has not been enumerated. Do not describe the board as entirely
   DRC-clean. Manually review differences without allowing J3's custom slots to be replaced by VH-6.
2. Verify the real KCD4 switch's blade fit, slot tolerance, mechanical support and contact sequence.
   Confirm Q1's actual 2N2222A lead order and the real microSD module underside clearance.
3. ≈8 A full-load voltage drop and temperature remain unqualified. A geometry check is not a current rating.
4. Inspect the fabricator's rendered copper, holes and plated slots and visually review the PCB.

The [raw BOM](WRO_HUB_RAW_BOM_20261028.csv) is EasyEDA's tab-delimited CSV, preserved as evidence.
Its J3 footprint text is still a legacy library alias although all six actual custom slots are verified.
Its Q1 row still names SS8050, whereas the approved assembly uses a user-supplied 2N2222A; this is **not**
an approved automated PCBA parts list. The bare PCB needs the Gerber/drill ZIP, not a CPL or PCBA order.

See [the full release review](../../RELEASE-2026-10-28.md), [AUDIT.json](AUDIT.json),
[MANUFACTURING-INSPECTION.json](MANUFACTURING-INSPECTION.json), [independent review](REVIEW.md)
and [SHA256SUMS.txt](SHA256SUMS.txt).
