# ESP32 Museum Guide Controller

Merge of [`ESP32_MP3_LFR`](../ESP32_MP3_LFR) (DFPlayer + WiFi web UI + Nano
line-follower link) and [`ESP32_RFID_MP3`](../ESP32_RFID_MP3) (MFRC522 card
reader). One ESP32 now runs the whole tour: it reads station cards, narrates
them on the DFPlayer, and starts/stops the Arduino Nano line follower around
each stop.

Both source sketches are left untouched so they remain flashable fallbacks.

## Tour behaviour

Tap a mapped card and the ESP32:

1. sends `STOP` to the Nano (the robot halts at the station),
2. plays that station card's track on the DFPlayer,
3. sends `START` when the track finishes, so the robot moves to the next stop.

The **START card is silent for now**: it does not play `0001.mp3`; it sends
`START` to the Nano immediately.

The auto-resume in step 3 is only **armed** when it is safe to move:

* the robot was already line-following when the card was tapped, **or**
* the card is the silent START card (one tap launches the tour immediately).

So tapping a station card on the bench plays the audio but leaves the robot
stopped. Pressing **Stop** or **LFR STOP** (web or serial) cancels a pending
auto-resume, so a finishing track can never restart a robot you deliberately
stopped.

If the DFPlayer is missing, the sketch still boots: audio is disabled, and a
card that would have armed the resume starts the robot immediately instead of
waiting for a track that will never play.

## Hub link (ESP-NOW, channel 1)

### Out: where the robot is

Every card scan, start and stop broadcasts a `MSG_LOCATION` packet, plus a 1 s
heartbeat. The Hub's dashboard "Robot" panel and its track LED follow it live.
`stationId` is `track - 1`, so tracks 2–8 are stations 1–7; the START card is
not a station and reports movement only.

The heartbeat is not optional: the Hub drops the robot back to "narrating"
after 2.5 s without a report (`ROBOT_MOVE_SETTLE_MS`), otherwise the dashboard
would stay stuck on "moving" for the rest of the demo.

### In: emergency → evacuation

The Hub owns the emergency state and re-broadcasts it every 2 s, so this sketch
just applies whatever it is told, acting only on a change. When it turns **on**:

* narration is cut and the line follower starts immediately,
* **every station card is ignored** — no stop, no audio, no photo; the robot
  reports passing the station and keeps driving,
* only the **station-7 exit card** (track 8, `09 4C B1 92`) stops it.

When the Hub **clears** the emergency the robot stops and waits for a normal
**LFR START** or a START-card tap. It does not silently resume the tour: after
an evacuation the robot is off-tour and should be repositioned by a human.

Because the Hub re-sends on a timer, a robot that reboots mid-emergency or
misses a packet picks the state up within ~2 s.

> **Both APs must stay on channel 1.** ESP-NOW only works when the two boards
> share a WiFi channel. This sketch's `ESP32-MP3` AP and the Hub's `MuseumHub`
> AP are both pinned to 1 — change one without the other and the link goes
> silent with no error message anywhere.

Serial `e` toggles the emergency locally for bench testing without a Hub. With
a Hub powered on it will be overridden within ~2 s — that is the Hub being
authoritative, not a bug.

### Station photos

At step 2 the ESP32 also sends `SHOOT <station>` to the ESP32-CAM, which
captures a JPEG and POSTs it to the Hub dashboard. The **START card is skipped**
— it is the tour intro, not an exhibit. Cards map tracks 2–8 to stations 1–7.

The trigger is fire-and-forget: the cam needs 1–3 s to join Wi-Fi, capture,
encode and upload, but none of that blocks the robot, which only writes ~10
bytes. If the Hub is unreachable the cam skips the shot; the tour is unaffected.

The cam is a client of the Hub's `MuseumHub` AP, which is a *different* network
from this sketch's `ESP32-MP3` AP — the Hub must be powered on for photos to
land. Power the cam from its own 5 V rail, not the motor rail: the ESP32-CAM
browns out easily and a capture during motor inrush is the classic failure.

## Wiring

The three peripherals share no pins.

### MFRC522 (SPI, default ESP32 VSPI pins)
| MFRC522 | ESP32 |
|---------|-------|
| SDA/SS  | GPIO5 |
| SCK     | GPIO18 |
| MOSI    | GPIO23 |
| MISO    | GPIO19 |
| RST     | not used (same as the standalone reader sketch) |
| 3V3/GND | 3V3/GND |

### DFPlayer Mini (UART2)
| DFPlayer | ESP32 |
|----------|-------|
| TX       | GPIO16 (RX2) |
| RX       | GPIO17 (TX2) via ~1k resistor |
| VCC/GND  | 5V/GND |
| Speaker  | SPK_1 / SPK_2 |

Put MP3 files on the SD card in a folder named `MP3` as `0001.mp3` … `0008.mp3`.

### Arduino Nano line follower (UART1)
| Nano | ESP32 | Notes |
|------|-------|-------|
| D1 (TX) | GPIO25 (RX) | through a 10k/20k divider — Nano TX is 5 V |
| D0 (RX) | GPIO26 (TX) | ESP32 3.3 V TX drives the Nano directly |
| GND | GND | common ground is mandatory |

D0/D1 are also the Nano's USB upload pins: **disconnect both wires while
flashing the Nano**, then reconnect.

### ESP32-CAM photo trigger (software serial, TX only)
| ESP32 | ESP32-CAM | Notes |
|-------|-----------|-------|
| GPIO27 (TX) | GPIO13 (RX) | 9600 baud, 3.3 V both sides — no divider |
| GND | GND | common ground is mandatory |

All three hardware UARTs are already used (USB, Nano, DFPlayer), so this link
is software serial. It is write-only and sends ~10 bytes per station, which is
comfortably reliable at 9600 even with the Wi-Fi AP running.

**Disconnect this wire while flashing the cam**, then reconnect.

GPIO5 is an ESP32 strapping pin, but SPI chip-select idles HIGH, so the boot
state is safe.

## UID -> Track map

| Track (mp3) | RFID UID (hex) | Label   |
|------------:|----------------|---------|
| 0001        | `18 C4 0F BA`  | START   |
| 0002        | `64 14 F6 B3`  | Track 2 |
| 0003        | `4A 6B DF 34`  | Track 3 |
| 0004        | `D8 36 D4 B9`  | Track 4 |
| 0005        | `94 60 6B B2`  | Track 5 |
| 0006        | `18 48 F4 BA`  | Track 6 |
| 0007        | `94 58 1A B3`  | Track 7 |
| 0008        | `09 4C B1 92`  | Track 8 |

The START card (`18 C4 0F BA`) does not play audio for now. It begins line
following immediately.

All UIDs are full 4-byte values, stored in the same byte order the reader
prints them (`Card UID: 64 14 F6 B3` -> `{0x64,0x14,0xF6,0xB3}`). The lookup
requires an exact length match, so a 7-byte card cannot collide with a 4-byte
entry that shares its first four bytes.

### Adding a new card

1. Open the Serial Monitor (115200) and tap the card; read the printed
   `Card UID: XX XX XX XX`.
2. Add one row to the `TRACKS[]` table with those bytes, the track number and
   a label.

## Nano command protocol

9600 baud, 8N1, newline-terminated. The ESP32 only ever sends the deterministic
forms so it always knows which state it asked for:

* `START` — force line following on
* `STOP` — force stop

The Nano replies `READY:STOPPED` at boot and `STATE:RUNNING` / `STATE:STOPPED`
after every change. The ESP32 parses those to keep its displayed state honest,
and forwards every line to the USB serial monitor prefixed with `<- Nano:`.

At boot the ESP32 sends one `STOP`, so an ESP32 reset can never leave the robot
driving on a stale command.

## Web UI

Join the AP `ESP32-MP3` / `12345678` and open `http://192.168.4.1`.

The page is plain HTML forms — no JavaScript — and shows the line-follower
state, the last card scanned, volume and DFPlayer health, plus buttons for
tracks 1–8, pause/resume/stop, volume, and LFR START/STOP. Status is a snapshot
at page load; press **Refresh** to update it.

## Serial control (115200)

| Key | Action |
|-----|--------|
| `1`..`9` | play that track |
| `p` / `r` / `s` | pause / resume / stop |
| `+` / `-` | volume up / down |
| `f` / `g` | LFR start / stop |
| `c` | test photo — `SHOOT` the last station, without tapping a card |
| `?` | print state (LFR, narrating, resume armed, audio, volume, last photo) |

Manual playback and manual LFR commands both clear a pending auto-resume —
whatever you do by hand wins over the card that is playing.

## Timing constants

| Constant | Value | Purpose |
|---|---|---|
| `DEBOUNCE_MS` | 1500 | ignore the same card re-tapped immediately |
| `NARRATION_MIN_MS` | 1500 | ignore stale "play finished" reports that some DFPlayer clones emit right after a new play command |
| `NARRATION_MAX_MS` | 90000 | resume anyway if the DFPlayer never reports the end of a track |

Raise `NARRATION_MAX_MS` above the length of your longest narration.

## Build

```sh
pio run -d ESP32_RFID_MP3_LFR            # compile
pio run -d ESP32_RFID_MP3_LFR -t upload  # flash /dev/ttyUSB0
```

Verified: PlatformIO build passes with no warnings (RAM 13.9%, flash 60.9%),
and `pio check --skip-packages` reports no high or medium findings.
