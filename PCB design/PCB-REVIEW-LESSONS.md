# Reusable PCB design/review lessons from the Hub repair

These are project-level lessons, not an EasyEDA API reference. Read the current board's spec and
handoff before using any historical script. Current Hub status: [release review](hub/RELEASE-2026-10-28.md).

## 1. Preserve the user's work and prove the saved result

- Keep one connected, visible editor window; check project UUID, document UUID/type and visibility
  before writes. A bridge reconnect changes window IDs. A successful health response alone does not
  establish which board is open.
- Full project backup first, native before/after comparison last. Keep identity, geometry, net
  assignments and metadata in the comparison; canvas getters alone have reported changes that did
  not persist. A smaller ZIP is not proof that only the intended objects were removed.
- A guard abort is not a reason to loosen it. Inspect the mismatch. This repour encountered two
  re-identified tracks with identical body payloads; the complete track multiset and all remaining
  non-fill records were proved equal before refreshing the ID-based guard. If geometry changes,
  stop and regenerate the plan instead.
- Do not kill all Python processes to stop one search. Capture the process ID of the task you start
  and terminate only that task if needed. Earlier broad process termination was not a safe pattern.

## 2. Local library API failures can affect far more than the requested part

- On this EasyEDA Pro build, modifying an actual project footprint in its footprint editor with
  documented pad primitives and saving was verified persistent. Component-pad overrides and whole
  source replacement were not reliable substitutes.
- **Do not repeat the disabled Hub device-library modify path.** A J3 request corrupted all 28
  DEVICE metadata dictionaries in the original project. Knowing the project-library token did not
  make the method safe. Recovery was a new-identity import from a verified checkpoint, followed by
  complete metadata equivalence checks.
- Reopening the old project name or observing a correct canvas does not establish recovery.
  Keep failed exports clearly diagnostic. Do not fabricate from them.
- Instance overrides are useful but not a complete custom-library migration. They can leave stale
  library names, BOM aliases, 3D models and ECO differences. Inspect actual exported BOMs and pin maps;
  do not standardize back to a catalog part that is not the installed component.
- When the actual ECO list contains only confirmed descriptive-property changes, a reviewed,
  backed-up metadata-only reconciliation can be safe. Disable unnecessary track-net updating and
  verify the saved result. The Hub's final Netlist Error was25stale J3 property values, not its slots.
- Failed local imports returned no result and left a zero-byte local file; they also confused the
  active project-info API until a clean saved-window reopen. Validate the import result, project
  identity and new native export before calling it successful. Do not edit internal SQLite/history
  tables as an improvised recovery method.

## 3. Mechanical fit and finished drilling are separate from nominal copper rules

- Ask for actual pin/blade dimensions, pitch, orientation and module underside envelope. A visually
  similar library part is not mechanical evidence. Check insulated-body clearance as well as pads.
- In native project PAD records, slot width is overall slot length and height is cutter diameter.
  In the API slot argument the order is diameter, then overall length. Read the reference each time.
- Component-pad hole getters in this project use a scale different from native/free-pad data.
  Prefer saved native geometry for dimensional checks and inspect the emitted Excellon tools and hits.
- Verify PLATED status, cutter diameter, tool-centre travel, total slot length and position in output.
  Six slot commands alone do not establish correct size, orientation, plating or pad-to-net mapping.
- Match complete transformed slot geometry one-to-one. Matching only midpoint, diameter and length
  would accept a 90° rotated slot or allow accidental reuse of one manufactured slot.
- Compare hole-to-hole and hole-edge-to-foreign-copper clearances, not merely pad-to-pad gaps.
  Keep manufacturing tolerance/real-part fit gates separate from a nominal annular-ring pass.

## 4. Static fill polygons are not an automatic pour

- After any pad/trace/via/outline edit, regenerate static fills from **current** geometry. Restoring
  old polygons can short a new route even if the old layout was DRC-clean.
- Different source-negative rails remain foreign to GND. Extra fill isolation is not permission to
  merge BAT−, DC− and GND, nor does it increase gaps between existing tracks.
- Preserve interior holes when working with polygons; the Hub writer emits hole-free pieces. Use
  actual polygon comparison and saved copper checks, not bounding-box equality.
- Recheck all named networks, disconnected islands, thermal connections, antenna projection, edge
  spacing and source isolation on the saved result. A morphology operation discourages slivers but
  is not proof of minimum current-carrying neck width or an 8 A power rating.
- Check fill-only islands explicitly: reporting only disconnected pads/vias misses isolated copper
  polygons. Reject every saved fill outside the main GND network.
- Keep generation and acceptance tolerances explicit. Microscopic pad offsets and export rounding
  must fit within the reported tolerance, not be silently called exact geometry.
- Historical model tests must use temporary output directories; they must not overwrite the active
  user's pour plan or review evidence.

## 5. Separate electrical, mechanical, manufacturing and release checks

- Use strict DRC, but also inspect real copper. Report physical findings and synchronization findings
  separately. Never say "DRC zero" if a Netlist Error is still present.
- Verify current schematic pin-wire membership against current PCB pad membership. Comparing a PCB
  to an older copy of itself does not establish agreement with the present schematic.
- A verified pin/net match does not explain an ECO error. Do not label it harmless or attribute it to
  one component without seeing the exact proposed differences. Never suppress the rule to get green.
- A raster image visible on a silk layer is not proof of printable Gerber artwork. Here, an Image-only
  ordinary Gerber export had no geometry. Preserve the source image and verify its intended output
  before ordering; conversion to monochrome/vector or color-silk production is a separate decision.
- ERC/DRC panels update asynchronously; read a fresh settled result rather than a stale panel.
- Check text height/stroke plus actual text bounds after changing size. Audit the current labels,
  not stale bounds from before the user's moves. Footprint graphics and body fit need visual review too.
- Keep the module's actual RF radiating projection distinct from a larger conservative plane keep-out.
  Neither a clear keep-out nor nominal trace widths guarantee RF or thermal performance.

## 6. Make manufacturing deliverables traceable and honest

- A unique dated release folder should contain Gerber/drill ZIP, matching complete project, raw BOM,
  audit/inspection reports, checksums and a short release-status README.
- Inspect the actual Gerber outline, copper aperture dimensions and **all** drill positions. Do not
  infer the outline from native line records when actual CAM output is available.
- The EasyEDA export here includes vias in both the main through-drill file and a standard separate
  via-drill subset. Preserve/export the normal package and review it in the fabricator's CAM preview.
- Preserve raw BOM evidence and explain legacy aliases. Do not fabricate supplier numbers or silently
  turn a library transistor into a physically different part. Bare-PCB output is not PCBA approval.
- A copper-weight change requires another DFM review. Existing ~0.152 mm gaps pass cited 1 oz rules,
  not the cited 0.16 mm two-layer 2 oz limit. Physical switch fit, contact action and full-load thermal
  checks remain hardware gates; do not label a prototype geometry check as product qualification.
- No order without the user's visual/fabrication review. Make unresolved synchronization, assembly
  and hardware gates explicit in the final answer and inside the manufacturing folder itself.
