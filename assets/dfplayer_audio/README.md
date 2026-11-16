# DFPlayer Audio — Smart Museum Test Cues (placeholders)

Locally generated, 100% originally synthesized **placeholder** audio for the
Smart Museum Guidance System DFPlayer Mini. These tracks exist **only** to let
the team verify the DFPlayer ↔ ESP32 wiring, the `/mp3/NNNN.mp3` playback path,
and serial station/language selection during hardware bring-up. **They are not
final narration** and must be replaced with team-written, team-recorded
bilingual narration before any public/competition use (see `PROVENANCE.md`).

## What is in here

```
tools/dfplayer_audio/
├── README.md            ← this file
├── TRACK_MAP.csv        ← machine-readable track→station/language map
├── PROVENANCE.md        ← generation recipe, tool versions, license, replacement policy
├── SHA256SUMS           ← sha256 of each of the 14 mp3s
├── generate_tracks.sh   ← deterministic generator (re-run to reproduce)
└── mp3/                 ← the ONLY directory to copy onto the SD card
    ├── 0001.mp3 … 0007.mp3   (English, stations 1–7)
    └── 0008.mp3 … 0014.mp3   (Bangla, stations 1–7)
```

The `mp3/` directory contains **exactly 14 files**, named `0001.mp3`–`0014.mp3`,
nothing else. No documentation, no `._` files, no playlists, no artwork — this is
deliberate so DFPlayer indexing cannot pick up stray entries (see DFRobot's
copy-order warning in `PROVENANCE.md`).

## Track map

Firmware mapping is heap-free and deterministic: for station id `s` (1–7),
English track = `s`, Bangla track = `s + 7`. Use the library's
`playMp3Folder(trackNumber)` API (the `/mp3/NNNN.mp3` folder mode), **not**
global copy-order `play(n)`.

| Track | Language | Station | Artifact | Audio content |
|---|---|---:|---|---|
| `0001.mp3` | English | 1 | Jamdani Saree | bell + "Station 1. Jamdani Saree." + bell |
| `0002.mp3` | English | 2 | Nakshi Kantha | bell + "Station 2. Nakshi Kantha." + bell |
| `0003.mp3` | English | 3 | Liberation War Artifacts | bell + "Station 3. Liberation War Artifacts." + bell |
| `0004.mp3` | English | 4 | Terracotta Plaques | bell + "Station 4. Terracotta Plaques." + bell |
| `0005.mp3` | English | 5 | Ektara | bell + "Station 5. Ektara." + bell |
| `0006.mp3` | English | 6 | Palki (Palanquin) | bell + "Station 6. Palki Palanquin." + bell |
| `0007.mp3` | English | 7 | Royal Bengal Tiger | bell + "Station 7. Royal Bengal Tiger." + bell |
| `0008.mp3` | Bangla | 1 | Jamdani Saree | ascending chime motif (root→fifth→octave ×2), root C4 |
| `0009.mp3` | Bangla | 2 | Nakshi Kantha | chime motif, root D4 |
| `0010.mp3` | Bangla | 3 | Liberation War Artifacts | chime motif, root E4 |
| `0011.mp3` | Bangla | 4 | Terracotta Plaques | chime motif, root G4 |
| `0012.mp3` | Bangla | 5 | Ektara | chime motif, root A4 |
| `0013.mp3` | Bangla | 6 | Palki (Palanquin) | chime motif, root C5 |
| `0014.mp3` | Bangla | 7 | Royal Bengal Tiger | chime motif, root D5 |

Each station has a unique root pitch drawn from the C-major pentatonic scale
(C4 D4 E4 G4 A4 C5 D5), so even the non-verbal Bangla cues are audibly
distinguishable from one another during serial testing.

### Why Bangla tracks have no voice

There is **no local Bangla TTS engine** on this machine (FFmpeg's Flite is
English-only). Synthesizing Bangla words with an English voice would produce
gibberish that could be mistaken for a real, poor-quality narration. Instead
the Bangla side uses a clearly non-verbal musical cue pattern. Real Bangla
narration must be recorded by the team (see `PROVENANCE.md`).

## Encoding profile (verified on every track)

- Container/codec: MP3, MPEG-1 Layer III
- Sample rate: 44.1 kHz
- Channels: mono (1)
- Bitrate: CBR 128 kb/s
- Peak level: −2.1 dB to −4.4 dB (no clipping)
- Duration: 3.3–4.4 s (short enough for rapid hardware testing)
- Metadata: none (`-map_metadata -1`)
- Cover art / video stream: none
- All 14 SHA-256 hashes distinct (see `SHA256SUMS`)

Validation was done with `ffprobe` + `ffmpeg -af volumedetect`; full per-track
numbers are in `TRACK_MAP.csv`.

## How to regenerate (reproducibility)

```bash
cd tools/dfplayer_audio
./generate_tracks.sh     # regenerates mp3/0001.mp3 .. mp3/0014.mp3
sha256sum mp3/*.mp3 | sed 's#mp3/##'   # compare against SHA256SUMS
```

Exact tool versions, the full recipe, and the license/replacement policy are in
[`PROVENANCE.md`](PROVENANCE.md).

## SD-card copy status: DONE (2026-08-02)

The 14 files from `mp3/` were copied onto the prepared `MSMAUDIO` FAT32 card
(`/dev/sda1`, UUID `6831-01A4`) into a root-level `/mp3/` directory, in
ascending numeric order, using plain `cp` (the FAT32 mount writes no desktop
metadata). On-card verification: 14/14 SHA-256 match `SHA256SUMS`, 14/14
byte-identical to source via `cmp`, and `ffprobe` on every on-card file reports
MP3 / 44.1 kHz / mono / 128 kb/s CBR. The card root contains only `mp3/` — no
`.DS_Store`, `._*`, `.Trash*`, `Thumbs.db`, `desktop.ini`, or `System Volume
Information`. The card was then `sync`ed, cleanly unmounted, and the parent
device power-off/ejected via `udisksctl`.

If you ever need to re-copy, copy **only the contents of `mp3/`** (not this
README, not `generate_tracks.sh`, not `SHA256SUMS`/`PROVENANCE.md`) into a
`/mp3/` directory at the card root, in ascending numeric order, and re-verify
the on-card hashes against `SHA256SUMS`.

## Firmware command map (next task)

See [`FIRMWARE_COMMAND_MAP.md`](FIRMWARE_COMMAND_MAP.md) for the planned
ESP32 / DFRobotDFPlayerMini API surface (`playMp3Folder()`,
`begin(Serial1,...)`, etc.), the `(station, language) → track` formula, and the
serial bring-up command protocol (`LANG 0/1`, `STATION <n>`, `PLAY <t>`, `STOP`,
`VOL`, `STATE`). That document is a reference only — no firmware is implemented
there.
