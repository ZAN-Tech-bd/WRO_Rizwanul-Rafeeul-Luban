# ESP32 RFID -> DFPlayer + Nano rail controller

Tap an RFID card on the MFRC522 reader and the ESP32 plays the matching MP3
track on the DFPlayer Mini. The two control cards also start or stop the Nano
rail controller. For station cards only, it waits two seconds after stopping
and then tells the ESP32-CAM to capture and upload a photo to the Museum Hub
while the station audio is still playing.

## Wiring

### MFRC522 (SPI, default ESP32 VSPI pins)
| MFRC522 | ESP32 |
|---------|-------|
| SDA/SS  | GPIO5 |
| SCK     | GPIO18 |
| MOSI    | GPIO23 |
| MISO    | GPIO19 |
| RST     | not used (same as your reader sketch) |
| 3V3/GND | 3V3/GND |

### DFPlayer Mini (UART2)
| DFPlayer | ESP32 |
|----------|-------|
| TX       | GPIO16 (RX2) |
| RX       | GPIO17 (TX2) via ~1k resistor |
| VCC/GND  | 5V/GND |
| Speaker  | SPK_1 / SPK_2 |

### Nano rail controller (UART1)
| Nano | ESP32 |
|------|-------|
| D1/TX | GPIO25/RX1 through a 10 kΩ / 20 kΩ divider |
| D0/RX | GPIO26/TX1 |
| GND | GND |

The Nano must run `NANO_LFR_RAIL/NANO_LFR_RAIL.ino` and use 9600 baud. The
Nano UART wires must be disconnected while flashing the Nano.

### ESP32-CAM photo trigger (software serial, TX only)

| Robot ESP32 | ESP32-CAM | Notes |
|-------------|-----------|-------|
| GPIO14 (TX) | GPIO13 (RX) | 9600 baud, 3.3 V; no divider required |
| GND | GND | Common ground is mandatory |

The link is one-way: the robot sends `SHOOT <station>\n`, and the camera's own
firmware captures the JPEG and uploads it to the Hub. Camera replies remain on
the camera's USB console. Disconnect the GPIO14-to-GPIO13 wire while flashing
the ESP32-CAM.

## Hub emergency (ESP-NOW, channel 1)

The robot listens for the Museum Hub's emergency broadcast. Nothing is wired
between the two boards: the Hub's push switch (GPIO21) — or the admin dashboard
— raises the state, and the Hub re-broadcasts it as a burst on every change plus
a 2 s heartbeat. A robot that reboots or misses a packet re-syncs within ~2 s.

An emergency **outranks the whole card map**:

| While the emergency is up | Behaviour |
|---|---|
| Station cards (0002–0008) | **Ignored** — no stop, no audio, no photo |
| START card (`18 C4 0F BA`) | **Ignored** — there is nothing to start into |
| Unknown cards | Ignored, as always |
| END card (`A3 C0 38 15`) | **Acts**: `STOP\n`, rail latched |
| `D8 36 D4 B9` (station 3) | **Acts**: `STOP\n`, rail latched — emergency only |
| USB serial `1`–`9` | Refused (it would stop the rail mid-evacuation) |

The moment the alarm arrives the robot cuts narration, drops any pending camera
shot, clears the stop latch and sends `START\n`, so it drives past every station
to the first **stop card** it meets and stops there. Cards it ignores are still
logged on the event tape and the field console, so it is obvious what was
skipped.

Two cards can end an evacuation run: the END card at the far end of the rail,
and `D8 36 D4 B9`, which gives the robot a nearer stop point. That second card
is **emergency-only** — on a normal tour it is station 3 and behaves exactly as
it always has: rail stopped, `0012.mp3` played, photo taken, rail resumed. It
only becomes a stop card while the alarm is up.

When the Hub **clears** the emergency the robot does not silently resume: the
rail is latched stopped and only the **START card** begins the tour again. This
is deliberate — after an evacuation the robot is off-tour and a human
repositions it.

> Because the alarm means "clear the track", a robot that is **power-cycled
> while the emergency is still up** adopts it within ~2 s and drives to the END
> card on its own. Clear the emergency at the Hub before working on the robot.

Requirements: both SoftAPs must stay on **channel 1** (`AP_CHANNEL` here,
`HUB_CHANNEL` on the Hub, `museum::kEspNowChannel` in the shared header). A
mismatch makes the link go silent with no error, so `AP_CHANNEL` is checked
against the protocol header with a `static_assert` at compile time.

`MuseumProtocol.h` and `EspNowCompat.h` in this folder are symlinks to
`firmware/common/`. Never replace them with copies — the Hub and every node must
agree on the packet layout byte for byte.

Without the Hub, press **`e`** on the USB serial console to toggle the emergency
locally for a bench test. A real Hub overrides that on its next broadcast.

## Hub mute (ESP-NOW, `MSG_AUDIO`)

The admin dashboard's blue **Mute robot** button broadcasts `MSG_AUDIO` with
`FLAG_AUDIO_MUTED`; the robot answers by setting the DFPlayer to **volume 0**.

Everything else about the tour is **unchanged**: cards still stop the rail,
narration still plays and still reports finished, the two-second station photo is
still taken, and the rail still restarts on time. The robot runs the whole tour
silently. That is exactly why mute sets the volume rather than calling
`player.stop()` — a stopped track never reports finished, so the rail would sit
at the station until the 90 s `AUDIO_MAX_MS` fail-safe fired.

Mute is **orthogonal to the emergency**: neither one changes the other, and a
muted robot still evacuates normally.

`currentVolume` keeps the level to return to, so the field console's slider still
works while muted — it just records the level and the speaker stays at 0 until
the Hub releases the mute. The Hub re-broadcasts the state like the emergency, so
a robot that reboots mid-tour comes back muted if that is still the setting.

Without the Hub, press **`m`** on the USB serial console to toggle it locally for
a bench test. A real Hub overrides that on its next broadcast.

## Robot field console (SoftAP)

The robot ESP32 also hosts a small service interface for field tuning and
diagnosis. It does not need the Hub or internet.

1. Join Wi-Fi **`ESP32-MP3-RAIL`** with password **`museum2026`**.
2. Open **`http://192.168.4.1/`**.
3. Move the volume slider to a value from 0 through 30 and press **Set volume**.
4. Set **Motor L** and **Motor R** from 0 through 255. Press **Apply live** to
   update Nano RAM, or **Save to Nano** to also persist both values in EEPROM.
5. Read the live event tape for RFID reads, Nano commands/replies, audio events,
   camera scheduling/triggers, emergency transitions, and web volume changes.

A red banner covers the page whenever the Hub emergency is up, and the **Rail**
readout shows `EMERGENCY - running to END card` / `EMERGENCY - stopped at END
card`. The header shows whether the ESP-NOW listener started.

While the Hub mute is on, the *Speaker level* panel is tagged **muted by Hub**
and a note explains that the slider only sets the level to return to — otherwise
the page would show a healthy "20" next to a silent speaker.

The page polls once per second. Logs use a bounded 4 KiB RAM buffer: old complete
lines are discarded as new lines arrive, so leaving the robot running cannot
exhaust memory. The log is diagnostic only and clears on reboot or from the
page's **Clear** button. The web API rejects volume values outside 0..30.

Motor values are not guessed on the ESP32. At startup it sends `GET`; after an
update it sends `SET ML n`, `SET MR n`, and `GET`. The page displays values from
the Nano's `TUNE ML=n MR=n` response. `NANO_LFR_RAIL` applies `SET` immediately
when driving; when stopped, the pair is used by the next `START`. **Save to
Nano** sends `SAVE` after both updates so they survive a Nano reset.

| Method + path | Purpose |
|---|---|
| `GET /` | Responsive field-console page |
| `GET /api/state` | Current volume/muted/audio/rail/card/camera/emergency state plus event tape |
| `POST /api/volume?v=0..30` | Validate and apply DFPlayer volume |
| `POST /api/motors?ml=0..255&mr=0..255` | Apply both Nano PWM values live |
| `POST /api/motors/save?ml=0..255&mr=0..255` | Apply and save both values to Nano EEPROM |
| `POST /api/log/clear` | Clear the bounded event tape |

Put MP3 files on the SD card in a folder named `MP3` as `0001.mp3` ... `0008.mp3`.

## UID -> Track/action map

| Action | Track | RFID UID (hex) |
|--------|------:|----------------|
| Start rail + play intro | 0001 | `18 C4 0F BA` |
| Stop rail; no new audio | — | `A3 C0 38 15` |
| Station audio | 0002 | `64 14 F6 B3` |
| Station audio | 0003 | `4A 6B DF 34` |
| Station audio | 0004 | `D8 36 D4 B9` |
| Station audio | 0005 | `94 60 6B B2` |
| Station audio | 0006 | `18 48 F4 BA` |
| Station audio | 0007 | `94 58 1A B3` |
| Station audio | 0008 | `09 4C B1 92` |

Every mapped audio card first sends `STOP\n` to the Nano. When the DFPlayer
reports that the track finished, the ESP32 sends `START\n` so the rail moves
again.

## DFPlayer offline = silent full tour

`setup()` probes the DFPlayer once with `player.begin()`. If the module is not
answering at boot — e.g. its **TX or RX wire is disconnected**, the SD card is
missing, or the module has no power — `begin()` blocks for ~1 s and returns
`false`, leaving `audioReady = false` for the whole run. The robot still boots
fully (RFID, Nano rail, web console and the ESP‑NOW emergency link all work).

In that state the tour runs **silently instead of stalling**: every audio card
(START and station cards) still sends `STOP\n`, then `beginAudio()` arms a
fixed **3 s timed dwell** in place of the track. The station photo still fires
during the dwell (it is scheduled at 2 s), and when the dwell elapses the ESP32
sends `START\n`, so the rail moves on exactly as if the narration had finished.
The robot completes the whole tour, station by station, with no sound.

| Card | DFPlayer online | DFPlayer offline |
|---|---|---|
| START | stop, play intro, `START` on finish | stop, 3 s silent dwell, `START` |
| Station | stop, play track, photo at 2 s, `START` on finish | stop, photo at 2 s, 3 s silent dwell, `START` |
| END | stop + latch | stop + latch (same) |
| Emergency | unaffected | unaffected |

If the module is lost *mid‑tour* (it was present at boot, so `audioReady` is
stale), a station card stops the rail and the 90 s `AUDIO_MAX_MS` fail‑safe
then leaves it stopped — reconnect the module or reboot after re‑wiring.

For station tracks 0002 through 0008, a non-blocking `millis()` timer schedules
one camera command two seconds after audio starts (after the STOP command). The
mapping is track 2 -> station 1 through track 8 -> station 7. If narration ends
or errors before the timer expires, the pending photo is cancelled so the
camera never shoots after the rail may have resumed. The START and END cards do
not schedule photos.

The start card clears the stop latch, plays `MP3/0001.mp3`, and sends `START\n`
after the intro finishes. The end card sends `STOP\n`, stops any current
DFPlayer audio, selects no new track, and latches the rail stopped. Station
audio cannot clear that latch; only the start card can start the rail again.

> All UIDs are full 4-byte values, stored in the same byte order the reader prints.

The bytes are stored in the same order the reader prints them
(`Card UID: 64 14 F6 B3` -> `{0x64,0x14,0xF6,0xB3}`).

## How it behaves

* Tap the start card -> the Nano stays stopped while `0001.mp3` plays, then starts.
* Tap the end card -> the Nano hard-brakes and remains stopped.
* Tap a station card -> the Nano stops, its track plays, a photo is triggered
  after two seconds, then the rail starts when audio finishes.
* Holding the card on the reader does **not** replay (the card is halted after
  each read; remove it and tap again to replay).
* Unknown UID -> "UID not in map" on Serial, nothing plays.
* Hub emergency raised -> audio cut, pending photo dropped, rail started, and
  every card but the end card ignored until the end card stops it. Clearing the
  emergency leaves the rail latched stopped; the start card resumes the tour.

## Adding a new card

1. Open the Serial Monitor (115200) and tap the card; read the printed
   `Card UID: XX XX XX XX`.
2. Add one row to the `TRACKS[]` table in the sketch with those bytes and the
   track number.

## Serial control (for testing)

`1`..`9` play a track, `+`/`-` volume, `s` stop, `v` read the DFPlayer's **actual**
volume, `e` toggle the emergency and `m` toggle the Hub mute for a bench test.
`1`..`9` are refused while an emergency is active, because starting a track stops
the rail.

`v` queries the module's own volume register (command `0x43`) instead of trusting
the number the ESP32 last sent, so a suspected volume problem can be settled by
measurement. It is refused while a narration is playing: the query consumes one
DFPlayer response, and swallowing the "play finished" event would leave the rail
parked at the station until the 90 s fail-safe.

### Volume after a mute/unmute cycle

Measured with `v`: the module reports the correct volume at boot **and** after a
mute/unmute cycle, so the ESP32 never sends a wrong value. If the output still
sounds quieter after unmuting, the module is under-applying gain when it comes
back from 0, not mis-storing it. The level is therefore sent a second time
250 ms after every mute transition (`scheduleVolumeReassert()`), which is the
usual cure for that.

If a module still under-applies it, the alternative is to mute with
`player.disableDAC()` / `enableDAC()` (command `0x1A`) and never touch the volume
register at all. That is cleaner in principle, but support varies across clones —
a module that ignores `0x1A` would leave the narration audible while the
dashboard says muted, so confirm by ear before relying on it.
