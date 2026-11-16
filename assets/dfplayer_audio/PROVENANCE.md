# PROVENANCE — DFPlayer placeholder audio

## Status: ORIGINAL, project-generated, placeholder only

The 14 files in `mp3/` are **original test cues synthesized entirely in-process**
by this project. No external copyrighted sound, music loop, stock clip, YouTube
audio, or third-party TTS output was read, downloaded, wrapped, or relabeled to
produce them. They are meant **only** for DFPlayer hardware/software bring-up
testing and **must be replaced** with team-recorded bilingual narration before
any public demonstration or competition submission.

## Generation environment

| Item | Value |
|---|---|
| Generated (local time) | 2026-08-02 ~20:45 Asia/Dhaka (UTC+06:00) |
| Generated (UTC) | 2026-08-02T14:45Z |
| Host OS | Linux 7.0.0-28-generic x86_64 |
| FFmpeg | 6.1.1-3ubuntu5 (Ubuntu), built `--enable-libflite --enable-libmp3lame` |
| FFprobe | 6.1.1-3ubuntu5 |
| Build toolchain | gcc 13 (Ubuntu 13.2.0-23ubuntu3) |
| TTS engine | FFmpeg lavfi `flite` source (CMU Flite, English), voice `slt` |
| Tone synthesis | FFmpeg lavfi `aevalsrc` (damped sinusoids) |
| MP3 encoder | FFmpeg `libmp3lame` |

## Exact recipe

The complete, deterministic generator is committed at
[`generate_tracks.sh`](generate_tracks.sh). Run it from this directory and it
rebuilds `mp3/0001.mp3`–`0014.mp3` identically.

Per-track construction:

- **English (0001–0007):** `aevalsrc` damped sinusoid bell on the station's
  pentatonic root (0.6 s) → 0.15 s silence → Flite `slt` speaking
  `"Station N. <Name>."` → closing bell (0.6 s). Concatenated, gain 0.85,
  `alimiter` ceiling 0.80, encoded CBR 128 / 44.1 kHz / mono.
- **Bangla (0008–0014):** `aevalsrc` damped-sinusoid three-note ascending motif
  (root → 1.4983× fifth → 2.0× octave), played twice with a 0.30 s gap. Same
  limiter/encode profile. **No speech** (no Bangla voice available).

The five Flite voices available in this build are `awb`, `kal`, `kal16`, `rms`,
`slt`; `slt` was chosen for clarity and consistency across all English tracks.

## License / rights dedication

The audio assets in `mp3/` are original works created by this project. To make
them maximally reusable and unambiguous, the project dedicates them to the
**public domain via CC0 1.0 Universal**:

> The person who associated a work with this deed has dedicated the work to the
> public domain by waiving all of his or her rights to the work worldwide under
> copyright law, including all related and neighboring rights, to the extent
> allowed by law. You can copy, modify, distribute and perform the work, even
> for commercial purposes, all without asking permission.

Why CC0 (and not, say, MIT): CC0 is the standard rightsholder-applied tool for
waiving copyright and related/neighboring rights in a creative asset such as
audio. MIT is a software-source license and is not designed for audio assets.
"Public Domain Mark" was **not** used because that is only an informational
label for works whose worldwide public-domain status is already established by
an external authority; here the project itself is the rightsholder and is
actively making the dedication, so CC0 is the correct mechanism.

### Important boundary: engine ≠ asset

FFmpeg (GPL/LGPL) and CMU Flite (BSD-like) are **third-party software** used as
tools to synthesize these assets. Their licenses cover the **software itself**,
not the audio output produced by running it on original parameters. The CC0
dedication applies to the **generated audio files only**. Using these files does
not impose FFmpeg's or Flite's software-license terms on the listener, and this
project's CC0 dedication does not claim to relicense FFmpeg or Flite. If the
team redistributes the generator script alongside FFmpeg/Flite binaries, the
software licenses of those binaries must still be respected separately.

> Note on TTS output rights: CMU Flite's voice databases and the legal status of
> raw TTS output can raise separate questions in some jurisdictions. Because the
> English Flite speech here is intentionally short, factual
> ("Station N. <Name>."), non-expressive placeholder text — and is documented as
> a temporary test cue to be replaced — the CC0 dedication is applied in good
> faith by the project as the operator that ran the synthesis. If the final
> museum build keeps any synthesized voice, the team should re-confirm Flite's
> current output terms at that time.

## Why these are placeholders — replacement policy

The canonical build plan (see `/BUILD_PLAN.md`, `/findings.md`) requires the
team/children to **write and record** the real bilingual narration. Reasons
these generated cues must not ship as final:

1. **No real content.** English cues speak only "Station N. <Name>." with no
   cultural/historical narration; Bangla cues carry no words at all.
2. **No Bangla voice.** Local Flite cannot speak Bangla; mispronouncing Bangla
   via an English voice would be worse than a chime and culturally inappropriate.
3. **TTS ≠ performance.** The build plan values children's/team voices for the
   competition presentation; synthesized speech does not meet that bar.

### When recording the real tracks, record this provenance for each file

- script author + proofreader (Bangla script must be culturally reviewed; note
  the known typos flagged in `findings.md` for the draft descriptions);
- speaker name (and, for child speakers, documented parent/guardian consent
  covering recording, editing, exhibition playback, and any distribution);
- recording date, recorder/operator, equipment;
- any edits and the conversion recipe (re-encode to the same 44.1 kHz / mono /
  CBR 128 profile);
- source WAV SHA-256 and final MP3 SHA-256;
- the chosen rights statement. Team recordings are best described as
  *"original Smart Museum recordings used with contributor/guardian permission"*.
  **Do not** label team voice recordings "CC0" or "public domain" unless every
  relevant rightsholder (including the speaker and, for children, their
  guardian) has validly made that dedication — performer/voice rights may
  survive even after copyright is addressed.

## DFRobot DFPlayer constraints honored

From DFRobot's official DFR0299 documentation (inspected read-only in the
project's prior DFPlayer preflight):

- FAT16/FAT32 TF card up to 32 GB — the project card (`/dev/sda1`, label
  `MSMAUDIO`) is FAT32/MBR, already formatted in a prior subtask.
- Deterministic playback uses a root-level `/mp3/` folder with four-digit names
  `0001.mp3`… and the library API `playMp3Folder(n)`. Generic `play(n)` follows
  filesystem **copy order**, which is unreliable, so firmware must use the
  folder API and the card must contain **only** the intended numeric files.
- macOS `._` metadata files and desktop index files can be mistaken for tracks;
  this is why only numeric MP3s live in `mp3/` and all documentation lives
  outside it.
- No hot-plugging — power the DFPlayer off before inserting/removing the card.

## Per-track summary

See [`TRACK_MAP.csv`](TRACK_MAP.csv) for the full machine-readable map
(track, language, station, artifact EN/BN, audio kind, root Hz, spoken English
text, duration, max volume, size, SHA-256) and [`SHA256SUMS`](SHA256SUMS) for
the standalone hash list.

| Track | Lang | Stn | Kind | Duration | Peak | SHA-256 (first 12) |
|---|---|---:|---|---|---|---|
| 0001 | EN | 1 | speech+bell | 4.02 s | −4.4 dB | 66e5c45486fc… |
| 0002 | EN | 2 | speech+bell | 3.47 s | −2.9 dB | ec4a6bc999f3… |
| 0003 | EN | 3 | speech+bell | 4.44 s | −4.0 dB | 9efca9259947… |
| 0004 | EN | 4 | speech+bell | 3.66 s | −3.7 dB | 0a95604d8549… |
| 0005 | EN | 5 | speech+bell | 3.32 s | −4.0 dB | 74530632f262… |
| 0006 | EN | 6 | speech+bell | 4.10 s | −2.1 dB | ccb258fa8c92… |
| 0007 | EN | 7 | speech+bell | 4.39 s | −2.1 dB | e1a5386117fb… |
| 0008 | BN | 1 | chime motif | 3.94 s | −4.4 dB | dffab9a18256… |
| 0009 | BN | 2 | chime motif | 3.94 s | −4.4 dB | ee505ecad34e… |
| 0010 | BN | 3 | chime motif | 3.94 s | −4.4 dB | f66a7d6f4195… |
| 0011 | BN | 4 | chime motif | 3.94 s | −4.4 dB | 03380c731d00… |
| 0012 | BN | 5 | chime motif | 3.94 s | −4.4 dB | c3f6fdad4be6… |
| 0013 | BN | 6 | chime motif | 3.94 s | −4.4 dB | da50df1b4419… |
| 0014 | BN | 7 | chime motif | 3.94 s | −4.4 dB | 54729d45b812… |
