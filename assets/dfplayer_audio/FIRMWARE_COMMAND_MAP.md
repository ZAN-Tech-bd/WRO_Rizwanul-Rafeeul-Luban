# DFPlayer ↔ ESP32 Firmware Command Map (planned, not yet implemented)

This document is a **mapping reference only**. It records how the firmware in a
future task should select an SD-card track from a `(station, language)` pair,
which DFPlayer API call to use, and the serial commands planned for manual
testing. **No firmware is implemented in this file.** It is written so the next
implementation task has a single source of truth for the on-card layout that was
just copied to the `MSMAUDIO` card.

## SD-card layout actually on the card

The prepared card (`MSMAUDIO`, FAT32, MBR, one partition `/dev/sda1`) contains
exactly:

```
<card root>/
└── mp3/
    ├── 0001.mp3 … 0007.mp3   (English, stations 1–7)
    └── 0008.mp3 … 0014.mp3   (Bangla, stations 1–7)
```

Nothing else is on the card — no metadata, no `._` files, no playlists, no
artwork, no README/hash/provenance files. This was verified by on-card SHA-256
(14/14 match `SHA256SUMS`) and `ffprobe` on every file (MP3 / 44.1 kHz / mono /
128 kb/s CBR).

## Track → (station, language) mapping

For a station id `s` in `1..7`:

| Language | Library `playMp3Folder()` argument | Resolves to file |
|---|---|---|
| English (`LANG == 0`) | `s`            | `/mp3/000s.mp3`  (`0001`–`0007`) |
| Bangla  (`LANG == 1`) | `s + 7`        | `/mp3/000(s+7).mp3` (`0008`–`0014`) |

This is the **only** heap-free, deterministic mapping. It must use the library's
`playMp3Folder(track)` API (DFRobot DFPlayer Mini `/MP3/NNNN.mp3` folder mode),
**not** global copy-order `play(n)`, because `play(n)` follows filesystem copy
order and the DFRobot documentation explicitly warns that it is unreliable and
can pick up stray entries.

## Required DFRobotDFPlayerMini API surface (planned)

The implementation task will use the `DFRobotDFPlayerMini` library
(`dfrobot/DFRobotDFPlayerMini` in PlatformIO; "DFRobotDFPlayerMini by DFRobot"
in Arduino IDE Library Manager). The intended calls:

| Library call | Purpose |
|---|---|
| `myDFPlayer.begin(Serial1, true, false)` | start the module on a **hardware** UART (RX/TX); `true` = ACK, `false` = no amplifier echo |
| `myDFPlayer.volume(N)` | set volume `0..30` (clamped) |
| `myDFPlayer.playMp3Folder(track)` | play `/mp3/NNNN.mp3` deterministically — **primary call** |
| `myDFPlayer.stop()` | stop the current track |
| `myDFPlayer.pause()` / `myDFPlayer.start()` | pause/resume |
| `myDFPlayer.readState()` / `readType()` / `read()` | poll playback status / event type / payload in the loop |

`playMp3Folder()` is the deterministic folder playback function. The deprecated
`playFolder(<2-char-folder>, <3-char-file>)` form is **not** used; the four-digit
numeric form with the root-level `mp3` folder is.

## Hardware notes (must be confirmed at implementation time)

- DFPlayer Mini RX is 3.3 V logic but ESP32 TX is 3.3 V — a **direct** ESP32 TX →
  DFPlayer RX wire is acceptable for the data line. The DFPlayer **TX** line
  should still go through a level consideration because some modules drive TX at
  the module's VCC; if the module is powered at 5 V, use a resistor divider or a
  level shifter on DFPlayer TX → ESP32 RX. **Confirm the module's VCC before
  wiring; do not assume 3.3 V.**
- Power the DFPlayer from a stable 5 V rail, not the ESP32's 3.3 V LDO, and keep
  a 1 kΩ–2 kΩ series resistor on the speaker line if testing without a speaker.
- Use a **hardware** `Serial1` (ESP32 GPIO16/17 or the chosen pins), not
  `SoftwareSerial`, because `SoftwareSerial` on ESP32 is unreliable.
- The SD card must be inserted **with DFPlayer power off** (DFRobot warns hot
  insertion can corrupt the index / cause malfunction).

## Serial control protocol (planned for the next task)

For manual bring-up, the firmware will accept line-based ASCII commands over USB
Serial (`Serial`) at 115200 baud. All commands echo an acknowledgement. The
planned command set:

| Serial command | Action | Track mapping |
|---|---|---|
| `LANG 0` | set language = English | subsequent `STATION n` plays English |
| `LANG 1` | set language = Bangla  | subsequent `STATION n` plays Bangla |
| `STATION <n>` | play narration for station `n` (`1..7`) in current language | English: `playMp3Folder(n)`; Bangla: `playMp3Folder(n + 7)` |
| `PLAY <t>` | play explicit track `t` (`1..14`) directly | `playMp3Folder(t)` |
| `STOP` | stop current track | `myDFPlayer.stop()` |
| `VOL <0..30>` | set volume (clamped) | `myDFPlayer.volume(v)` |
| `STATE` | print DFPlayer state + current `(station, language)` | `readState()/readType()` |

This mirrors the display firmware's existing `LANG 0` / `LANG 1` / `STATION <id>`
convention so the two subsystems stay consistent. `PLAY <t>` is a low-level
escape hatch for direct track testing without the station/language indirection.

## Consistency with existing firmware conventions

- `LANG 0` = English, `LANG 1` = Bangla — matches `webapp/js/data.js` and the
  existing `firmware/display/` language convention.
- Station ids `1..7` map to the canonical museum artifact order: 1 Jamdani
  Saree, 2 Nakshi Kantha, 3 Liberation War Artifacts, 4 Terracotta Plaques,
  5 Ektara, 6 Palki (Palanquin), 7 Royal Bengal Tiger — matches
  `webapp/js/data.js` `ARTIFACTS`.
- The shared `MuseumProtocol.h` already carries a `MSG_LOCATION`/station id
  field; the audio node should derive its `playMp3Folder()` argument from the
  same station id it receives over ESP-NOW so there is no second mapping table.

## Verification to repeat at firmware bring-up

1. Insert the `MSMAUDIO` card with DFPlayer **power off**.
2. `PLAY 1` … `PLAY 14` — confirm each plays the expected distinct cue
   (English spoken station names; Bangla ascending chimes with rising root
   pitch per station).
3. `LANG 0` / `STATION 3` → expect English Liberation War Artifacts cue.
4. `LANG 1` / `STATION 3` → expect Bangla chime, root E4.
5. `STATE` → confirm `readType()` returns a non-error status after each play.

These checks use only the placeholder cues currently on the card. When the team
records real bilingual narration, only the file contents change — the
`(station,language) → track` mapping, the `playMp3Folder()` call, and this
command map stay the same.
