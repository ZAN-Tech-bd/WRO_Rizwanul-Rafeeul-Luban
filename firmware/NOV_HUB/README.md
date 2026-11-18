# NOV_HUB — six-station ESP32 museum Hub

All six station ports use physical DHT11, MQ-5 analog, and shared proximity
inputs, using the pin map from `StationLedTest`. The Hub publishes unavailable
temperature and humidity when a DHT11 fails to respond. The LED chain has 48
pixels across S1–S6.

A separate copy of the existing full Hub firmware, adapted for the
**YD-ESP32-S3 N16R8 / Hub PCB v0.4**. The original `firmware/hub/` is untouched;
`ORIGINAL_HUB_SHA256.json` records hashes of all 18 original owned files.

## The dashboard is inside the ESP32 firmware

This is **not a separate website, server, or frontend deployment**.
The redesigned entry, visitor and admin pages live in this Hub project.
`tools/embed_dashboard.py` compresses their local assets and generates a
flash-resident header during every PlatformIO build. `src/main.cpp` serves
those compiled bytes directly from the ESP32 at **http://192.168.4.1/**.

One application upload installs the dashboard AND Hub code. **No `uploadfs`,
Node server, internet, CDN, remote font, or external image host is required.**
LittleFS remains available for camera-photo storage; the dashboard does not
rely on separately copied filesystem assets or stale files from the old Hub.
The `data/` folder is source material for the embedded dashboard, not a second
application. Browser-only tests may open those files locally; they clearly
say DEMO/PREVIEW and never represent hardware measurements.

## Approved station and RFID catalog

| Route | Artifact | UID | Sensor source | Physical pixels |
|---|---|---|---|---|
| Start | Start card | **83 54 6A 28** | Control card | None |
| 1 | Terracotta Plaque | **64 14 F6 B3** | Wired inputs | 0–7 / J10 |
| 2 | Wooden boat | **4A 6B DF 34** | Wired inputs | 8–15 / J11 |
| 3 | Rickshaw Art | **D8 36 D4 B9** | Wired inputs | 16–23 / J12 |
| 4 | Traditional pottery | **94 60 6B B2** | Wired inputs | 24–31 / J13 |
| 5 | Bamboo flute | **18 48 F4 BA** | Wired inputs | 32–39 / J14 |
| 6 | Bamboo basket | **94 58 1A B3** | Wired inputs | 40–47 / J15 |
| Reserved | Jamdani shari | **09 4C B1 92** | Inactive; not in route | None |
| End | End card | **13 8C 5A 28** | Control card; not station 8 | None |

User confirmed six active artifacts; Jamdani remains recorded but inactive.
"Bamboo flute" corrects the supplied spelling "bamboo fluet" without changing
its UID. Source of truth: `include/StationCatalog.h`; browser metadata in
`data/js/data.js` is checked against it by the host tests. `/api/stations`
exposes all nine cards including Start, End, and the reserved artifact.

### RFID integration boundary

The **robot** reads RFID, not the Hub. The Hub receives numeric `stationId`
values 1–6 over the existing ESP-NOW protocol. Updating this catalog does
**not** reprogram the robot's RFID reader or its audio tracks.

The robot firmware sends its current station and Nano-confirmed motion state
over ESP-NOW on channel 1. Flash the robot controller separately to enable
these reports. Stations 1–6 use tracks 0010–0015, and Jamdani is reserved. The
robot accepts the current Start and End cards and the legacy control cards.
Station 0 clears the current artifact.

## Physical sensor wiring

| Input | S1 / J10 | S2 / J11 | S3 / J12 | S4 / J13 | S5 / J14 | S6 / J15 |
|---|---|---|---|---|---|---|
| DHT11 | GPIO15 | GPIO16 | GPIO17 | GPIO18 | GPIO38 | GPIO39 |
| MQ-5 AO | GPIO4 | GPIO5 | GPIO6 | GPIO7 | GPIO1 | GPIO2 |
| Active-low PROX | GPIO41 | GPIO41 | GPIO42 | GPIO42 | GPIO3 | GPIO3 |

DHT module station J3: **3V3, GND, DATA**. MQ station J4: **5V, GND, AO**,
with its fourth pin/DO unused. Each MQ line is divided by the station's
68k/100k divider; the Hub has C3/C4 100nF at the ADC, not a series resistor.
ADC uses 12-bit raw counts and calibrated `analogReadMilliVolts()` values;
reported MQ source mV = ADC mV × 1.68.

Each DHT11 uses the Adafruit reader every three seconds. The Hub staggers this
across the six stations at 500 ms intervals. MQ samples are refreshed once per
second. Each pair shares one debounced proximity line. The admin dashboard
shows all three zones above the readings table. A detection stays visible as
recent for three seconds after the input clears.

The gas status uses a baseline learned independently from twelve consecutive
stable samples on each input. It shows Normal, Elevated or High based on how
far the signal moves in the sensor's response direction. Start the Hub in
the stations' ordinary conditions so it can learn those baselines. Temperature
and humidity estimates use the closest fresh measured stations and appear
with an approximation mark. The API marks their source as `estimated`.
When all MQ inputs are near zero, the Hub waits for a signal before learning.
If all three proximity inputs are also low, it leaves proximity unavailable.

### Recorded bench modifications and validation limits

- User reports R31–R37 removed after DHT worked when bypassed. A removed
  series resistor must be replaced by **a 0Ω link/bridge** to make a direct
  path; an empty open footprint does not connect a sensor. Check continuity
  on the actual assembled board. These links remove series GPIO protection:
  never put 5V on a DHT data pin. The original design value was 1kΩ.
- The actual removed resistor values and exact original failure cause were
  not established. The earlier 220kΩ-mis-stuff explanation was a hypothesis,
  not a confirmed diagnosis. Both hardware and reader implementation changed.
- R54 was accidentally removed too. It belongs to PROX zone 4 (GPIO48/S7+S8),
  not DHT. Refit its designed 1kΩ before using that zone. NOV_HUB does not use it.
- S1 and S2's 16-pixel LED chain was visually confirmed by the user. DHT library
  operation was user-confirmed on GPIO15; GPIO16/MQ/IR still require validation
  with this integrated build. A sampled ADC value alone does not prove module
  presence or gas sensitivity; a floating AO input can also produce numbers.
- **Do not release lighter gas or solvent vapour near the powered MQ heater.**
  This demo is not a certified gas/fire alarm. Use the sensor manufacturer's
  ventilation, warm-up, calibration and test instructions.

## LEDs and retained Hub features

GPIO45 drives the 48-pixel GRB chain at 800 kHz through the same Adafruit
NeoPixel RMT driver as `StationLedTest`. The Hub repeats frames every 20 ms. The idle
animation flows through S1–S6. At a station, its eight pixels are green during
narration and blue while moving. Each active proximity pair blinks amber.
An emergency overrides both patterns and
double-flashes all 48 pixels red at brightness 60 alongside the buzzer. Each
board passes data to the next one. Keep J10–J15 connected consecutively, and
power off before changing station cables.

Existing check-in approval/denial, robot advance test, narration mute,
physical emergency button on GPIO21, buzzer on GPIO47, ESP-NOW broadcasts,
captive DNS and multipart camera upload/gallery are retained. The optional
microSD pins now match the S3 PCB: SCK12/MISO13/MOSI11/CS14; photos fall back to
LittleFS if no card is present. This inherited demo admin is not authenticated;
use only on a controlled/trusted demo network.

The responsive bilingual dashboard has local artifact line illustrations,
a six-stop route with real/simulated badges, explicit disconnected state,
raw/uncalibrated sensor labels, visitor approvals, safety controls and gallery.
No simulated robot motion starts automatically; the admin Advance button is
an explicit test action. All display source is embedded in the Hub image.

## Build / deploy

From repository root:

```sh
/home/user/.platformio/penv/bin/pio run -d firmware/NOV_HUB
```

**Flashed 2026-11-11 on user request.** The connected ESP32-S3 N16R8 was
identified as WCH serial `5C83470067`, MAC `10:20:ba:74:72:80`; this is a
DIFFERENT adapter from the earlier `5C83468930` bench board. Upload and flash
hash verification succeeded, then the uploader reset the board. No additional
runtime/hardware tests were run, as requested. For subsequent uploads, first
confirm which physical Hub is connected and close its serial monitor:

```sh
/home/user/.platformio/penv/bin/pio run -d firmware/NOV_HUB -t upload --upload-port /dev/serial/by-id/usb-1a86_USB_Single_Serial_5C83470067-if00
/home/user/.platformio/penv/bin/pio device monitor --port /dev/serial/by-id/usb-1a86_USB_Single_Serial_5C83470067-if00 --baud 115200 --dtr 0 --rts 0
```

No dashboard uploadfs step. The N16R8 project uses the default16MB partition
layout; back up important existing flash/SD photos before deploying a changed
partition layout (LittleFS may be formatted if the old layout is incompatible).

### Bluetooth monitoring and firmware updates

The first install uses USB. Once NOV_HUB boots, `MuseumHub-BLE-OTA` provides
live diagnostics and application updates over Bluetooth LE. The existing
16 MB layout already contains two OTA app slots. The embedded dashboard
updates with the application, and the photo filesystem is outside both slots.

Install `bleak` in your Python environment, then enable Bluetooth on the laptop.
From the repository root, run this terminal monitor with the ESP powered by
the Hub. Press Ctrl-C to close it before starting an update.

```sh
python3 firmware/NOV_HUB/tools/hub_ble.py monitor
```

Add `--once` after `monitor` for a single reading. With multiple Hubs nearby,
add `--address <BLE-address>` before `monitor` or `flash`. Only one Bluetooth
client can connect at a time. The monitor shows uptime, the app slot and build,
Wi-Fi and ESP-NOW status, the emergency button and alarm, the reported robot
state, and S1–S6 readings. `P41`, `P42` and `P3` show the raw proximity GPIO
levels. Each station reports temperature, humidity, raw MQ
ADC counts with ADC pin millivolts, and proximity. `DHT--` indicates an
unavailable reading. The same status line appears on USB serial at 115200 baud. LED counts describe the configured
chain. Check illumination and the buzzer at the bench.

```sh
pio run -d firmware/NOV_HUB
python3 firmware/NOV_HUB/tools/hub_ble.py flash firmware/NOV_HUB/.pio/build/NOV_HUB/firmware.bin
```

The build requires `openssl`. It creates a private `.ota-signing.key` in the
project folder on first build and embeds the matching public key. Keep a private
backup and reuse that key for future updates. Restoring OTA access with a new
key requires a USB install. The Hub verifies the signed manifest and firmware
digest, writes the inactive app slot, and waits for the alarm and button to
clear before rebooting. An interrupted transfer before commit leaves the
running app selected. The tool confirms the build in the other slot after
reconnecting.

On 2026-11-18, the ESP32-S3 with USB serial `5C83470067` and BLE address
`10:20:BA:74:72:81` was flashed over USB, then updated again over BLE. The
uploader confirmed the expected build booting from `app1`. The board also
rejected an invalid signature and kept running after a partial upload disconnected.

The wired-sensor build `48afa290` was then installed over BLE on Hub power
with USB unplugged. It booted from `app0` and returned readings from all six
ports over BLE. S1, S4, S5, and S6 returned DHT11 readings. S2 and S3 reported
unavailable temperature and humidity during the check.

Build `cda21d50` expanded the chain to all 48 pixels. It was installed over
BLE on Hub power and reported `leds=48 gpio=45` after rebooting into `app1`.

Build `78bf791d` added the dashboard proximity zones and trialed SPI DMA. The
bench report that S5 and S6 went dark led to restoring the original RMT driver.
The RMT build `88133583` was installed over BLE and reported 48 LEDs on GPIO45
and all three raw proximity inputs. Physical illumination and sensor activation
require a bench check.

Build `ade145dd` was installed on the Hub over USB. USB and BLE diagnostics
confirmed the expected build and the USB-only sensor guard. Robot station
reports and live camera uploads still need the powered hardware check.

Join **MuseumHub**, password **museum2026**, and open:

- `http://192.168.4.1/` — visitor entry.
- `http://192.168.4.1/visitor.html` — six-stop gallery guide.
- `http://192.168.4.1/admin.html` — embedded control desk.
- `http://192.168.4.1/scan` — firmware-built diagnostics.
- `/api/state`, `/api/sensors`, `/api/stations` — state and catalog JSON.

## Verification

```sh
python3 firmware/NOV_HUB/tests/test_profile.py
```

The host suite compiles the actual sensor C++ and JSON/LED functions with fake
hardware. It checks the six station mappings, missing and stale DHT values,
raw MQ readings, shared proximity, eight-pixel station boundaries, idle and
emergency LEDs, catalog and frontend agreement, and embedded dashboard assets.

Browser-only checks covered desktop1440 and mobile390 rendering, English/Bangla,
entry/approve/deny, mute, emergency banner, robot advance, missing real readings,
measured/uncalibrated values and disabled controls on connection loss.

### Historical 2026-11-11 deployment

Reflashed successfully on 2026-11-11 ~13:49 UTC to the same ESP32-S3 N16R8
(WCH5C83470067, MAC10:20:ba:74:72:80). Firmware and embedded dashboard were
updated together; uploader verified flash hashes and reset the board.
That deployment used simulated sensor values with 16 physical LEDs and the
physical emergency switch and buzzer. No post-upload hardware test was run.

## Live ESP-CAM gallery

The camera receives a `SHOOT <station>` command from the robot controller and
uploads its JPEG to `/api/image/upload?station=N` on MuseumHub Wi-Fi. The Hub
shows stored camera photos by station. It uses LittleFS when a microSD card
is unavailable.

### Historical simulated ESP-CAM profile

The six supplied images in `picutures/` are now bundled into the Hub firmware.
The first appears at boot, then the next arrives every **20,000 ms**, with a
120-second loop through all six. All dashboard clients share the Hub's timer;
the next1.5-second dashboard poll displays each arrival. Existing thumbnail and
full-size gallery controls work. The camera panel is labelled **Simulated camera**
so these supplied photographs are never represented as live hardware captures.

Photo order (filename sort, no invented artifact/station assignments):

1. WhatsApp Image 2026-11-11 at 7.54.59 PM (1).jpeg
2. WhatsApp Image 2026-11-11 at 7.54.59 PM.jpeg
3. WhatsApp Image 2026-11-11 at 7.55.00 PM.jpeg
4. WhatsApp Image 2026-11-11 at 7.55.01 PM (1).jpeg
5. WhatsApp Image 2026-11-11 at 7.55.01 PM.jpeg
6. WhatsApp Image 2026-11-11 at 7.55.40 PM.jpeg

Original files were not changed. Optimized copies are800×600 JPEGs, each under
60KB, in `data/images/camera-demo/`. `CAMERA_DEMO_PHOTOS.json` records original
source names/hashes and embedded paths. Camera-demo metadata deliberately uses
station0/unknown because the filenames do not establish a reliable station map.

The existing `/api/state`, `/api/gallery`, and `/api/image?seq=N` routes now
expose this simulated arrival stream, including source/index/age metadata.
The gallery keeps the six most recent arrivals. **Restart slideshow** returns
to photo1 without deleting originals or stored real photos. Real ESP-CAM uploads
remain separate and are not displayed while this demo mode is enabled.
No extra web host, SD card, filesystem upload, or real camera is needed.

Build and host timing checks passed (20s boundaries, six-photo wrap, restart,
gallery JSON and embedded gzip round-trip). Flashed successfully on2026-11-11
~14:06 UTC to WCH5C83470067 / ESP32-S3 N16R8 MAC10:20:ba:74:72:80; uploader
verified flash hashes and reset the board. App1,309,817B flash/47,780B RAM,
16embedded assets425,795gzip bytes. No post-upload physical image viewing test.

### Visible wording update — 2026-11-11 ~14:11 UTC

Dashboard mode/source badges are hidden. The camera panel uses Camera journal,
photo captions show their sequence, and the route uses neutral station labels
in both languages. Internal source metadata and documented data generation
remain unchanged; the UI does not claim live capture or calibrated gas safety.
Reflashed successfully to the same confirmed ESP32-S3; uploader verified hashes.
