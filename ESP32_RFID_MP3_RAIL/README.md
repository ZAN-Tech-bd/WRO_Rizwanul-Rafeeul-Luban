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
| END card (`A3 C0 38 15`) | **The only card that acts**: `STOP\n`, rail latched |
| USB serial `1`–`9` | Refused (it would stop the rail mid-evacuation) |

The moment the alarm arrives the robot cuts narration, drops any pending camera
shot, clears the stop latch and sends `START\n`, so it drives past every station
to the END card and stops there. Cards it ignores are still logged on the event
tape and the field console, so it is obvious what was skipped.

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
| `GET /api/state` | Current volume/audio/rail/card/camera/emergency state plus event tape |
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
again. If audio is unavailable or times out, the rail remains stopped as a
safe fallback.

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

`1`..`9` play a track, `+`/`-` volume, `s` stop, `e` toggle the emergency for a
bench test. `1`..`9` are refused while an emergency is active, because starting
a track stops the rail.
