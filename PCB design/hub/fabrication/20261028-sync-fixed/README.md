# Hub — synchronization fixed, standard Gerber refreshed

**Fresh strict PCB DRC: zero findings. Fresh ERC: passed with no detailed findings.**

The reviewed change was25 J3 metadata values only. Native preservation proves all copper, GND,
footprints, net assignments, placements, image object and image resource data unchanged.

- [Standard Gerber ZIP](WRO_HUB_GERBER_20261028_SYNC_FIXED.zip)
- [Matching editable-project archive](WRO_HUB_PROJECT_20261028_SYNC_FIXED.epro2)
- [Raw EasyEDA BOM](WRO_HUB_RAW_BOM_20261028_SYNC_FIXED.csv)
- [Complete verification](SYNC-VERIFICATION.json)
- [Checksums](SHA256SUMS.txt)
- [Model-test result](TEST-RESULTS.txt):42passed,13historical fixture-dependent cases skipped.
- [Cause, fix and review notes](../../SYNC-FIX-2026-10-28.md)

## Do not miss the artwork gate

The user's raster silkscreen image remains in the project, but **ordinary Gerber export omitted
its image geometry**. An Image-only layer3 export produced an empty GTO. This ZIP includes the
existing ordinary vector silkscreen, not a converted rendition of the raster artwork.

**Do not order this expecting the raster image to print.** Conversion to suitable monochrome/vector
artwork, or a verified supported color-silkscreen workflow, is a separate step and was not performed.
The image has not been deleted, moved or otherwise altered.

Use the previously reviewed two-layer1oz bare-PCB rules. Actual-part switch fit/support/contact
sequence, transistor pinout and full-load thermal testing remain hardware gates; no order or
payment was submitted. The raw BOM retains legacy library aliases and is not automated PCBA approval.
