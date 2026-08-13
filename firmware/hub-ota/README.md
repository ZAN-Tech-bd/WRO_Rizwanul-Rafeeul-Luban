# Hub — the Track ESP32 (OTA-enabled copy)

This **one ESP32 does two jobs**, split across its two cores so neither interrupts
the other:

| Core | Runs |
|------|------|
| **Core 0** (`webTask`) | WiFi SoftAP **MuseumHub** (ch 1) + ESP-NOW receive + WebServer + captive-portal DNS — serves the web app and the `/api` endpoints |
| **Core 1** (`loop`) | ESP-NOW drain + **emergency button/siren** + ESP-NOW emergency broadcast + NeoPixel LED show (lights the station the robot is at; blinks red on emergency) |

## Emergency (button → Hub → robot)

The proven `Emergency_Switch.ino` logic is merged in verbatim and now drives the
whole system:

1. **Press** the GPIO21 button → emergency **latches on**. Track LEDs blink red,
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
| WS2812 strip DIN | **GPIO15** | one LED per station (7). VCC→5V, GND→GND |
| **Passive** buzzer **+** | **GPIO4** | **−** → GND. Siren uses `tone()`, so it must be a **passive** buzzer. If it draws >20 mA, drive via a 2N2222 transistor instead of direct GPIO |
| Emergency button OUT | **GPIO21** | 3-pin module (VCC/GND/OUT), **HIGH when pressed**, own pull-down |
| microSD **MISO** | **GPIO19** | camera photo storage (VSPI) — **optional** |
| microSD **MOSI** | **GPIO23** | |
| microSD **SCK** | **GPIO18** | |
| microSD **CS** | **GPIO5** | |
| microSD **VCC** | 5V | only for modules with an onboard AMS1117-3.3 + level shifter. A **bare** breakout must be fed 3.3V |
| DHT11 / MQ data | **GPIO14, 16, 17, 27** | probed as DHT11. These four can drive the start pulse a DHT11 needs |
| DHT11 / MQ data | **GPIO34, 35, 36, 39** | probed as DHT11 too, but **input-only** — see below. Their ADC1 channel is sampled as an MQ analog input |
| Power | 5V (USB or buck) | keep logic and motor rails separate on the *robot*; the hub is low-power |

Pins are defined in [`include/config.h`](include/config.h:1) — change them there if needed.

The microSD is genuinely optional: without a card the Hub keeps the photo
gallery in LittleFS instead, and the dashboard behaves identically. The boot log
says which one is in use (`[photos] backend=sd` or `backend=littlefs`).

## Artifact sensors wired to the Hub (DHT11 + MQ pin scan)

The sensor loom was built before anyone recorded which sensor went to which
GPIO, so the Hub **probes every pin in `HUB_SENSOR_PINS` as a DHT11 in turn**
(one pin every `HUB_SENSOR_PERIOD_MS`, default 2.5 s → a 20 s sweep for eight
pins) and reports, per pin, exactly what came back:

| Result | Means |
|---|---|
| `ok` | 40 bits with a matching checksum — **a DHT11 really is on this pin** |
| `no_response` | the line never answered the 20 ms start pulse — nothing, or not a DHT11 |
| `line_low` | the pin was already LOW before the start pulse — a short, or an analog output sitting under the logic threshold |
| `frame_timeout` | something answered, then stalled part-way through the 40 bits — wiring/length/power problem |
| `checksum` | all 40 bits arrived but the checksum failed — a DHT11 that needs a better supply or a shorter wire |
| `unprobed` | the sweep has not reached this pin yet |

### GPIO34/35/36/39 cannot run a DHT11 — at all

Those four are **input-only** on the ESP32: no output driver and no internal
pull-up/pull-down. `pinMode(34, OUTPUT)` is a silent no-op, so the 18 ms
start-low pulse a DHT11 needs never reaches the wire and the sensor is never
asked to speak. They will always report `no_response`, whatever is soldered to
them. This is a chip limitation, not a wiring fault, and no firmware can work
around it.

What they *can* do is analog: they carry **ADC1** channels, which keep working
while WiFi is up (ADC2 — which covers GPIO14 and GPIO27 in this list — is owned
by the WiFi driver and is never sampled here, because a number from it would be
a lie rather than a reading). That makes them the right home for an **MQ-135
analog output**, and the scan samples them as raw counts + millivolts on the
same pass. So: if a DHT11 is currently on 34/35/36/39, move it to a pin that can
drive — GPIO25, 26, 32 and 33 are all free on this build.

### Reading the results

Three places, all showing the same data:

- **`http://192.168.4.1/scan`** — a self-contained page served **from firmware**,
  not from LittleFS. This is the one that matters after a BLE update: BLE OTA
  replaces the app partition only, so a cable-free flash cannot ship a new
  `admin.html`. `/scan` arrives with the firmware and needs no `uploadfs`.
- **`GET /api/sensors`** — the same table as JSON, for `curl`.
- **The admin dashboard** — a "Hub sensors — pin scan" card, after the next
  `pio run -t uploadfs` (USB) refreshes the web bundle.

The serial log prints every probe as it happens:

```
[sensors] GPIO14 DHT11 OK   27.0 C  62.0 %RH  (7/7 good)
[sensors] GPIO16 DHT11 no_response   no ADC on this pin
[sensors] GPIO34 DHT11 no_response   analog 1820 (1466 mV)  [input-only: DHT11 cannot work here]
```

### Once you know the wiring

Set `HUB_SENSOR_STATION` in [`include/config.h`](include/config.h:1) — same
order as `HUB_SENSOR_PINS`, `0` for unmapped. A mapped pin's readings then also
fill the dashboard's **Artifact health** table, exactly like a remote ESP-NOW
health node. Temperature and humidity are written without touching `gas`, so a
DHT11 and an MQ on the same station each update only what they actually measure.

> One caution while the wiring is unknown: on the four drivable pins the probe
> pulls the line low for 20 ms every sweep. Against a DHT11 that is the protocol;
> against an MQ module's analog output it briefly pulls a resistive divider node
> low, which is harmless. Once a pin is known to be an MQ, drop it from
> `HUB_SENSOR_PINS` (or move it to an ADC1 pin) so it stops being pulsed.

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

## Build & flash

This is the **BLE-OTA-enabled** copy of the Hub (`firmware/hub-ota`). The
firmware adds a Bluetooth LE OTA service (NimBLE); flash it once over USB, then
reflash over Bluetooth forever after.

1. Open **this `firmware/hub-ota/` folder** in VS Code with the PlatformIO extension, or on the command line:
   ```bash
   cd firmware/hub-ota
   pio run -e hub -t upload     # USB flash (cable) — first time only
   pio device monitor           # serial monitor (115200)
   ```
2. The serial output prints `Hub ready. Visitor: http://192.168.4.1/  Admin: /admin.html`, then `[ble-ota] advertising OTA service (NimBLE). ...`.

## Over-the-air updates over Bluetooth LE (no cable, no Wi-Fi switch)

This Hub flashes over **Bluetooth LE**, not Wi-Fi. The Hub keeps running its
`MuseumHub` SoftAP + ESP-NOW on Wi-Fi the whole time, and BLE is a **separate
radio** — so the laptop you flash from does **not** have to join `MuseumHub` or
give up its internet. It pushes the image over Bluetooth.

> Why BLE instead of Wi-Fi (`espota`) here: `espota` would ride the Hub's own
> SoftAP and force the laptop onto `MuseumHub` (which has no internet). BLE
> sidesteps that entirely. The trade-off is speed: BLE is slower than Wi-Fi/USB.

**First time — USB (once).** The BLE-OTA firmware must already be on the chip:
```bash
cd firmware/hub-ota
pio run -e hub -t upload        # USB, first time only
```

**Every time after — over Bluetooth:**
```bash
cd firmware/hub-ota
pip install bleak                                   # once
pio run -e hub                                      # build -> .pio/build/hub/firmware.bin
BLE_OTA_PASSWORD='...' python3 tools/ble_ota_upload.py .pio/build/hub/firmware.bin
```
The uploader scans for the Hub, sends the OTA password as an `AUTH` command,
then (once accepted) streams the image into the **inactive** OTA slot
(write-with-response, so every chunk is acked), verifies the byte count, then
tells the Hub to commit and reboot. While the flash is being written, the Core 1
show (NeoPixels, siren, ESP-NOW) parks itself so nothing competes with the
write. A ~1.2 MB image takes a couple of minutes over BLE.

### The OTA service is password-gated

The GATT service is otherwise open to anyone in BLE range who discovers its
UUIDs (they aren't secret — they're right here in the repo), so every session
must present `BLE_OTA_PASSWORD` (set in
[`include/config.h`](include/config.h:1), default `CHANGE-ME-WRO2026`) as an
`AUTH` command before the Hub accepts `BEGIN`/`DATA`. The Hub disconnects a
client after 5 wrong attempts on one connection to block quick guessing.

**Change the default password before the venue** — edit `BLE_OTA_PASSWORD` in
`include/config.h`, reflash once over USB, and pass the same value to the
uploader via `--password`, the `BLE_OTA_PASSWORD` env var, or the interactive
prompt if you omit both:
```bash
python3 tools/ble_ota_upload.py .pio/build/hub/firmware.bin --password 'your-new-password'
```

Be clear about what this password does and doesn't protect against: it stops
casual/opportunistic flashing by someone who wanders up with a BLE scanner,
but the write is **not link-encrypted**, so a dedicated attacker capturing the
handshake with a BLE sniffer could still recover it in plaintext. For real
cryptographic protection you'd want NimBLE bonding (`WRITE_ENC`) on top of
this, which was left out here because it adds OS-dependent pairing prompts
that are more likely to strand you mid-demo than stop a determined attacker.
To rotate the UUIDs as well (belt-and-suspenders), change them in
[`src/ble_ota.cpp`](src/ble_ota.cpp:1) and the matching ones in
[`tools/ble_ota_upload.py`](tools/ble_ota_upload.py:1).

## Flash layout (tuned partition table — why OTA never runs out of room)

ESP‑IDF's stock `default.csv` gives each OTA app slot only **1.25 MiB** and
hands **1.375 MiB** to LittleFS. For this project that is the wrong balance:
the firmware is already 955,125 bytes (72.9% of a stock slot), while the exact
web bundle is **308,483 bytes** (`jsQR.js` is 256,885 bytes of that).

An application that exceeds its OTA slot is rejected by the build/uploader; it
does not normally boot and behave randomly. The practical problem is lost
growth headroom or a failed OTA attempt. Separately, a full LittleFS can reject
photo staging; the upload handler checks short writes and returns an error
without accepting the image. The balanced layout keeps margin on both sides.

This copy ships its own [`partitions.csv`](partitions.csv:1) that rebalances
the 4 MB flash with **no gaps**, so nothing runs short:

| Partition | Type | Start | Size | Holds |
|-----------|------|-------|------|-------|
| `nvs` | data/nvs | 0x09000 | 20 KB | non‑volatile config |
| `otadata` | data/ota | 0x0E000 | 8 KB | which OTA slot is active |
| `app0` | app/ota_0 | 0x10000 | **1.5 MiB** | one selectable firmware image (1,177,337 B linked use → **74.9%**, incl. NimBLE BLE) |
| `app1` | app/ota_1 | 0x190000 | **1.5 MiB** | alternate/inactive OTA destination |
| `spiffs` | data/spiffs | 0x310000 | **896 KiB** | 308,483 B web bundle + upload scratch + gallery fallback |
| `coredump` | data/coredump | 0x3F0000 | 64 KiB | post‑mortem dumps |

Result: PlatformIO reports **395,527 bytes of linked-section headroom** (the
NimBLE BLE stack costs ~222 KB of flash vs. the Wi-Fi `espota` build, which is
the price of BLE OTA). The 896 KiB partition has **609,021 bytes after the raw
deployed web payload** (before LittleFS metadata) for its temporary upload and
gallery fallback. Normal verified QVGA JPEGs are about 2 KB, so the configured
12-photo fallback has ample working margin. The 65,535-byte per-image ceiling is
an acceptance bound, not a promise that twelve worst-case images plus scratch
must fit simultaneously. The build confirms the linked application side:
`Flash: 74.9% (used 1177337 bytes from 1572864 bytes)`.

The two slots make interrupted uploads safe: OTA writes the inactive slot and
switches boot selection only after a valid image is received, so the currently
running image is not overwritten in place. This design does **not** claim
automatic rollback if a valid new image boots but later misbehaves; that needs
a separate ESP-IDF rollback/health-confirmation policy.

### One‑time switch (over USB)

Changing the partition layout rewrites the whole flash map, so it needs a
**one‑time erase + USB reflash** — do this before relying on OTA:

```bash
cd firmware/hub-ota
pio run -e hub -t erase          # wipe flash (old layout gone)
pio run -e hub -t upload         # flash firmware over USB
pio run -e hub -t uploadfs       # upload web files to LittleFS
```

After that, every update goes over Bluetooth
(`python3 tools/ble_ota_upload.py .pio/build/hub/firmware.bin`) and
the tuned layout is permanent — you only repeat the erase if you change
`partitions.csv` again.

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
| `GET /api/state` | `{robot:{station,status}, emergency, approved, pending, health:[…], sensors:[…], requests:[…], image:{bytes,station,seq,count,store}}` |
| `GET /scan` | the DHT11/MQ pin-scan page, served from **firmware** (survives a BLE-only update) |
| `GET /api/sensors` | `{periodMs, pins:[{pin,dht,ok,tries,inputOnly,station,temp?,hum?,adc?,mv?}]}` — `temp`/`hum` only once a pin has produced a valid DHT11 frame, `adc`/`mv` only on ADC1 pins |
| `POST /api/checkin` | `{id}` — visitor requests entry (in-page scanner) |
| `GET /api/enter` | native-camera scan: creates a request, redirects to `/index.html?autoid=<id>` |
| `GET /api/request?id=` | `{id,status}` (pending/approved/denied) |
| `POST /api/approve?id=` · `POST /api/deny?id=` | admin decision |
| `POST /api/emergency?on=1\|0` | toggle emergency |
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
