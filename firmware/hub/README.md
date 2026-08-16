# Hub — the Track ESP32 (PlatformIO project)

This **one ESP32 does two jobs**, split across its two cores so neither interrupts
the other:

| Core | Runs |
|------|------|
| **Core 0** (`webTask`) | WiFi SoftAP **MuseumHub** (ch 1) + ESP-NOW receive + WebServer + captive-portal DNS — serves the web app and the `/api` endpoints |
| **Core 1** (`loop`) | ESP-NOW drain + **emergency button/siren** + ESP-NOW emergency broadcast + Hub sensor scan + NeoPixel LED show (lights the station board the robot is at; double-flashes red on emergency) |

## Emergency (button → Hub → robot)

The proven `Emergency_Switch.ino` logic is merged in verbatim and now drives the
whole system:

1. **Press** the GPIO21 button → emergency **latches on**. Track LEDs double-flash red,
   the GPIO4 siren wails a two-tone ambulance pattern.
2. The Hub is the **single source of truth** and broadcasts the emergency state
   over ESP-NOW. A **change of state outranks everything**: it goes out on the
   next Core 1 pass (~5 ms) and is then repeated `EMERGENCY_BURST_COUNT` times
   `EMERGENCY_BURST_GAP_MS` apart, because a single unacknowledged broadcast can
   be lost and the robot would otherwise keep touring for up to 2 s with the
   siren already sounding. After the burst the state falls back to a plain 2 s
   heartbeat, so a node that reboots or misses everything still re-syncs within
   one interval. The Hub never tracks who is listening.
3. The **robot evacuates**: it cuts narration, drops any pending photo, drives,
   and **ignores every card except the END card**, where it stops and latches.
   `ESP32_RFID_MP3_RAIL` (rail robot) and `ESP32_RFID_MP3_LFR` (line-follower
   robot) both listen; the LFR build stops at its station-7 exit card instead.
4. **Clear it** by holding the button 3 s, or from the admin dashboard
   (`POST /api/emergency?on=0`). Either way the robot stops so it can be
   repositioned — it does not silently resume the tour. Restarting it takes the
   **START card**, the same card that begins every normal tour.

> The siren uses `tone()`, which needs a **passive** buzzer. For a plain
> **active** buzzer, swap the `tone()` call in `updateSiren()` for
> `digitalWrite(BUZZER_PIN, HIGH)` / `(…, LOW)`: same wail rhythm, no pitch
> sweep. A passive buzzer driven by `digitalWrite` is nearly silent, so this has
> to match the part actually fitted.

Because this lives on the Hub, the standalone `Emergency_Switch/` sketch is a
bench reference only — its board is not needed for the demo.

## Mute switch (ESP-NOW out)

The blue **Mute robot** button under *Robot status* on the admin dashboard
silences the robot's narration: the Hub latches `audioMuted` and broadcasts
`MSG_AUDIO` with `FLAG_AUDIO_MUTED`, and the robot drops its DFPlayer to
volume 0.

The tour is **otherwise unchanged** — cards still stop the rail, narration still
plays and still reports finished, and the rail still restarts on time. The robot
simply runs the whole tour silently, which is why the mute sets volume instead of
stopping playback: a stopped track never reports finished, so the rail would sit
at the station until the robot's 90 s fail-safe fired.

Delivery reuses the emergency's proven rule (`dueForBroadcast()`): send on
change, repeat as a burst, then a 2 s heartbeat. So the state is a **latch**, not
a one-shot — a robot that reboots mid-tour comes back muted if that is still the
setting, and the dashboard button always shows what the robot is really doing.

`MSG_AUDIO` is deliberately its own message type rather than a flag on
`MSG_EMERGENCY`: the physical emergency-button node also broadcasts
`MSG_EMERGENCY` and knows nothing about the mute, so its packets would clear it
on every press. Adding the type and flag did not change the 24-byte packet
layout, so the emergency / health / cam nodes need no reflash.

## Robot status (ESP-NOW in)

The guide ESP32 broadcasts `MSG_LOCATION` on every card scan / start / stop plus
a 1 s heartbeat. `stationId` is `track - 1` (tracks 2–8 → stations 1–7), which
the Hub maps to `STATION_IDS`. `robotMoving` lapses back to "narrating" after
`ROBOT_MOVE_SETTLE_MS` (2.5 s) without a report, so the heartbeat must stay
faster than that.

**Both boards must stay on WiFi channel 1.** The robot's own `ESP32-MP3` AP and
this Hub's `MuseumHub` AP are both pinned there; ESP-NOW only works when the
channels match, and a mismatch fails silently.

Shared state (`gState`) is guarded by a mutex. The LEDs use the ESP32 **RMT**
peripheral (via Adafruit_NeoPixel), so WiFi traffic does **not** glitch them.

## Wiring

| Part | ESP32 pin | Notes |
|------|-----------|-------|
| WS2812 strip DIN | **GPIO15** | seven 8-pixel boards in one chain (56 pixels). VCC→5V, GND→GND |
| **Passive** buzzer **+** | **GPIO4** | **−** → GND. Siren uses `tone()`, so it must be a **passive** buzzer. If it draws >20 mA, drive via a 2N2222 transistor instead of direct GPIO |
| Emergency button OUT | **GPIO21** | 3-pin module (VCC/GND/OUT), **HIGH when pressed**, own pull-down |
| microSD **MISO** | **GPIO19** | camera photo storage (VSPI) — **optional** |
| microSD **MOSI** | **GPIO23** | |
| microSD **SCK** | **GPIO18** | |
| microSD **CS** | **GPIO5** | |
| microSD **VCC** | 5V | only for modules with an onboard AMS1117-3.3 + level shifter. A **bare** breakout must be fed 3.3V |
| DHT11 / MQ data | **GPIO14, 16, 17, 27** | configured per pin as DHT11 or MQ D0; these GPIOs can drive a DHT11 start pulse |
| DHT11 / MQ data | **GPIO34, 35, 36, 39** | configured as MQ inputs; input-only, with usable ADC1 channels but no DHT11 output drive |
| Power | 5V (USB or buck) | keep logic and motor rails separate on the *robot*; the hub is low-power |

Pins are defined in [`include/config.h`](include/config.h:1) — change them there if needed.

The microSD is genuinely optional: without a card the Hub keeps the photo
gallery in LittleFS instead, and the dashboard behaves identically. The boot log
says which one is in use (`[photos] backend=sd` or `backend=littlefs`).

## Artifact sensors wired to the Hub (DHT11 + MQ pin scan)

The sensor loom is described in [`include/config.h`](include/config.h:94). Every
entry in `HUB_SENSOR_PINS` has an explicit `HUB_SENSOR_ROLE`:

- `ROLE_DHT11` sends the DHT11 start pulse, reads and checks all 40 bits, and
  distinguishes a valid frame, a held-low line, no reply, a partial frame, a
  checksum failure, and a pin that cannot drive the protocol.
- `ROLE_MQ_D0` is never driven. It polls the comparator level, counts confirmed
  transitions, and also samples raw ADC/millivolts when the pin is on ADC1.

GPIO34/35/36/39 are input-only on the ESP32. They have no output driver or
internal pull resistors, so they cannot run a DHT11 at all; they are appropriate
for MQ digital/analog observation. GPIO14 and GPIO27 are on ADC2, which WiFi
owns while the Hub SoftAP is active, so firmware intentionally does not publish
misleading analog numbers from those two pins.

### Reading and mapping the results

The same live state is available in three places:

- `http://192.168.4.1/scan` — a self-contained diagnostic page compiled into
  the firmware, so it is available even before the separate LittleFS web bundle
  is refreshed;
- `GET /api/sensors` — compact JSON for scripts and `curl`;
- the **Hub sensors — pin scan** and **Artifact health** tables in the deployed
  admin dashboard.

The serial log reports every DHT result, MQ level/transition, analog movement,
and the artifact-health rows sent to the dashboard. `HUB_SENSOR_STATION` maps a
pin to station 1–7, or `0` to keep it diagnostic-only. A station row can combine
DHT temperature/humidity with MQ gas without either source erasing the other.
Stations without a fresh physical measurement receive explicitly marked
derived room estimates; remote ESP-NOW `MSG_HEALTH` readings remain measured
and take precedence until they become stale.

The gas threshold is installation-specific. Current hardware reports high raw
counts for clean air and low counts for gas, so `HUB_GAS_THRESHOLD_RAW` follows
that measured polarity. Calibrate it from the live raw readings rather than
assuming a generic MQ module curve.

## Camera photos

Every JPEG the ESP32-CAM uploads is stored as its own file under `/photos` on
whichever backend mounted:

```
/photos/IMG_00001_S1.JPG      seq 00001, station 1 (jamdani)
/photos/IMG_00002_S3.JPG      seq 00002, station 3 (liberation)
```

The sequence number and station live in the file name, so both survive a reboot
without an index file. The Hub keeps the newest `GALLERY_MAX_PHOTOS` (default
12, see `include/config.h`) and deletes older ones automatically — the card
never fills up during a demo.

The admin dashboard shows the newest photo large, the previous ones as a
thumbnail strip, and opens any of them full-size on tap.

## Build & flash (PlatformIO)

1. Open **this `firmware/hub/` folder** in VS Code with the PlatformIO extension, or on the command line:
   ```bash
   cd firmware/hub
   pio run -t upload          # flash the firmware
   pio device monitor         # serial monitor (115200)
   ```
2. The serial output prints `Hub ready. Visitor: http://192.168.4.1/  Admin: /admin.html`.
   It also prints the firmware-resident sensor diagnostic URL, `/scan`.

## Serve the web app (upload the files to flash)

1. Copy the web app into the data folder (keep the subfolders):
   ```
   webapp/index.html, visitor.html, admin.html  ->  firmware/hub/data/
   webapp/css/style.css                          ->  firmware/hub/data/css/
   webapp/js/data.js, hub.js, qr.js, vendor/jsQR.js -> firmware/hub/data/js/
   ```
2. Upload the filesystem:
   ```bash
   pio run -t uploadfs
   ```

## The QR flow (what to print)

Generate all QR codes locally with one command (see [`../../tools/generate_qr.py`](../../tools/generate_qr.py)):

```bash
pip install "qrcode[pil]"
python tools/generate_qr.py     # writes PNGs to tools/qr_output/
```

This produces **three** gate codes:

1. **WiFi-join** (`01_wifi_join.png`) — visitor scans with their phone's normal camera to join **MuseumHub**.
2. **Check-in — in-page scan** (`02_checkin_inpage.png`) — encodes `MUSEUM-ENTRY-2026`. Visitor opens `http://192.168.4.1/`, taps **Scan QR**, snaps this (needs offline `jsQR` in `data/js/vendor/`).
3. **Check-in — native scan** (`03_checkin_native.png`) — encodes `http://192.168.4.1/api/enter`. Visitor scans with their phone's normal camera → auto check-in (no `jsQR`). The simplest, most reliable entry.

Methods 2 and 3 both reach the same waiting → admin-approve → dashboard flow.

## API (all at `http://192.168.4.1`)

| Method + path | Returns |
|---|---|
| `GET /` , `/visitor.html`, `/admin.html`, `/css/…`, `/js/…` | the web app files (from LittleFS) |
| `GET /api/state` | `{robot:{station,status}, emergency, muted, approved, pending, health:[…], sensors:[…], requests:[…], image:{bytes,station,seq,count,store}}` |
| `GET /scan` | firmware-resident DHT11/MQ pin-scan and artifact-health page |
| `GET /api/sensors` | `{periodMs,pins:[{pin,dht,ok,tries,inputOnly,line,role,station,temp?,hum?,d0?,flips?,adc?,mv?}]}` |
| `POST /api/checkin` | `{id}` — visitor requests entry (in-page scanner) |
| `GET /api/enter` | native-camera scan: creates a request, redirects to `/index.html?autoid=<id>` |
| `GET /api/request?id=` | `{id,status}` (pending/approved/denied) |
| `POST /api/approve?id=` · `POST /api/deny?id=` | admin decision |
| `POST /api/emergency?on=1\|0` | toggle emergency |
| `POST /api/audio/mute?on=1\|0` | mute / unmute the robot's narration (latched, re-broadcast) |
| `POST /api/robot/advance` | **test**: nudge the robot to the next station (no hardware needed) |
| `POST /api/image/upload?station=N` | ESP32-CAM photo, `multipart/form-data` field `image` |
| `GET /api/image` | the newest photo (never cached) |
| `GET /api/image?seq=N` | one specific photo. A seq is written once and never rewritten, so this response is cacheable forever — that is what stops the thumbnail strip re-downloading on every poll |
| `GET /api/gallery` | `{backend,count,max,newest,photos:[{seq,bytes,stationId,station,age}]}`, newest first. `age` is ms since arrival, or `-1` for photos recovered after a reboot |
| `POST /api/gallery/clear` | delete every stored photo |

## Test it without any other hardware

After flashing + uploading the web files: join WiFi **MuseumHub** / `museum2026`
on a phone and a laptop → open `http://192.168.4.1/` on the phone and
`http://192.168.4.1/admin.html` on the laptop → use **Advance robot (test)** and
**Start emergency (test)** on the admin page. The phone's visitor view and the
NeoPixel LEDs react live.

> **Editor squiggles:** open this folder in VS Code with the **PlatformIO**
> extension — it configures the include paths, so `Arduino.h` resolves and the
> red squiggles disappear.
