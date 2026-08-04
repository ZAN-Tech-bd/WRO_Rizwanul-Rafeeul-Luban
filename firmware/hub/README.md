# Hub — the Track ESP32 (PlatformIO project)

This **one ESP32 does two jobs**, split across its two cores so neither interrupts
the other:

| Core | Runs |
|------|------|
| **Core 0** (`webTask`) | WiFi SoftAP **MuseumHub** (ch 1) + ESP-NOW receive + WebServer + captive-portal DNS — serves the web app and the `/api` endpoints |
| **Core 1** (`loop`) | NeoPixel LED show (lights the station the robot is at; blinks red on emergency) + buzzer |

Shared state (`gState`) is guarded by a mutex. The LEDs use the ESP32 **RMT**
peripheral (via Adafruit_NeoPixel), so WiFi traffic does **not** glitch them.

## Wiring

| Part | ESP32 pin | Notes |
|------|-----------|-------|
| WS2812 strip DIN | **GPIO15** | one LED per station (7). VCC→5V, GND→GND |
| Active buzzer **+** | **GPIO4** | **−** → GND. If it draws >20 mA, drive via a 2N2222 transistor instead of direct GPIO |
| microSD **MISO** | **GPIO19** | camera photo storage (VSPI) — **optional** |
| microSD **MOSI** | **GPIO23** | |
| microSD **SCK** | **GPIO18** | |
| microSD **CS** | **GPIO5** | |
| microSD **VCC** | 5V | only for modules with an onboard AMS1117-3.3 + level shifter. A **bare** breakout must be fed 3.3V |
| Power | 5V (USB or buck) | keep logic and motor rails separate on the *robot*; the hub is low-power |

Pins are defined in [`include/config.h`](include/config.h:1) — change them there if needed.

The microSD is genuinely optional: without a card the Hub keeps the photo
gallery in LittleFS instead, and the dashboard behaves identically. The boot log
says which one is in use (`[photos] backend=sd` or `backend=littlefs`).

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
| `GET /api/state` | `{robot:{station,status}, emergency, approved, pending, health:[…], requests:[…], image:{bytes,station,seq,count,store}}` |
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
