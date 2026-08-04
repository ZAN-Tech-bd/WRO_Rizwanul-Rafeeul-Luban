# Cam — ESP32S-CAM (GC2145) photo node (PlatformIO project)

Captures a still photo on a USB-serial command and saves it as a JPEG on the
module's **own onboard microSD** card (`/photos/IMG_####.JPG`). **No Wi-Fi and
no Hub upload yet** — that is a single clearly-marked seam for later.

> Status: ✅ **command-driven capture-to-SD firmware** (GC2145 + SD_MMC 1-bit).
> Network/Hub upload is intentionally **not** implemented in this phase.

---

## Hardware (exact)

| Part | Notes |
|------|-------|
| Board | **ESP32S-CAM** seated on an **ESP32-CAM-MB Type-C** programmer |
| Programmer | The CAM-MB has a built-in **CH340** USB-serial chip — appears as `/dev/ttyUSB0` (Linux), no FTDI/UNO needed |
| Camera | **GC2145** (ribbon is labelled `GC2145`). Prior test detected sensor **PID `0x2145`**. |
| Storage | microSD in the ESP32-CAM onboard slot (**FAT32** preferred) |
| Power | stable 5 V via the CAM-MB USB-C; use a data-capable cable |

### ⚠️ GC2145 caveat (read this first)
The common OV2640 examples **will not work** on this board. The GC2145 **cannot
output native JPEG**. This firmware therefore:

1. grabs frames as **RGB565** (not `PIXFORMAT_JPEG`),
2. converts each frame to JPEG **in software** via `frame2jpg()` (`img_converters.h`), then
3. writes the JPEG to the card.

The proven-stable settings used here (matching the working test in
[`ESP32 CAM.pdf`](../../ESP32%20CAM.pdf)): **XCLK 10 MHz**, `PIXFORMAT_RGB565`,
**QVGA 320×240**, **1 framebuffer in PSRAM**.

### SD_MMC 1-bit mode (no pin conflicts)
`SD_MMC` runs in **1-bit mode**, which uses only **GPIO2 (D0) / GPIO14 (CLK) /
GPIO15 (CMD)**. That leaves every camera pin and **GPIO4 (the onboard flash LED)**
free, so the camera and SD never fight over pins. (4-bit mode, which would also
take GPIO4/12/13, is deliberately **not** used.)

### Flash LED
Optional and off by default. Because SD is 1-bit, **GPIO4 is safe** to drive —
use `FLASH ON` / `FLASH OFF`. It is **not** fired automatically during capture
(the ESP32-CAM rail is brownout-prone; reliability first).

---

## Build / flash

```bash
cd firmware/cam
pio run                # build
pio run -t upload      # flash  (upload_speed = 115200, already set)
pio device monitor     # serial @115200
```

`board = esp32cam` (AI-Thinker profile, PSRAM enabled), `huge_app.csv`
partition (3 MB app, no SPIFFS — photos live on the SD, not flash).

### CAM-MB upload / run procedure
The CAM-MB sometimes does **not** auto-enter download mode. If `pio run -t
upload` hangs on `Connecting...`, do this once:

1. **Hold the CAM-MB BOOT (IO0) button** (or jumper IO0→GND).
2. **Press and release RST** once.
3. **Release BOOT** (or remove the IO0→GND jumper).
4. Run `pio run -t upload`. You should see `Chip is ESP32...` then `Writing at ...`.
5. After the upload, **press RST once** to boot normally into the firmware.

Use a **data-capable** USB-C cable (charge-only cables give "No serial data
received"). Upload speed is fixed at **115200** for reliability on the CAM-MB.

---

## Serial commands (USB @115200, case-insensitive, newline-terminated)

| Command | Action |
|---------|--------|
| `CAPTURE [station]` | Acquire one RGB565 frame → software JPEG → save `/photos/IMG_####.JPG`. Optional integer station id is echoed in the result. |
| `STATUS` | Camera/SD/PSRAM/heap/card diagnostics (PID, card type, sizes, free space, photo count). |
| `LIST` | List saved photos in `/photos` (name + size). |
| `INIT` | Re-initialise camera and SD without rebooting (bring-up retry). |
| `FLASH ON` / `FLASH OFF` | Toggle the onboard flash LED (optional). |
| `HELP` | Show the command list. |

The input line buffer is hard-bounded (no dynamic `String`); the command verb is
upper-cased, the optional numeric argument is left as-is.

### Filenames / collision safety
Photos are written to a predictable folder with a zero-padded, never-overwriting
sequential name:

```
/photos/IMG_0001.JPG
/photos/IMG_0002.JPG
...
```

On every `CAPTURE`, the firmware scans `/photos` for the highest existing
`IMG_####.JPG` index and uses `index + 1`. The counter therefore **survives
reboots** with no extra config file. Each JPEG is fully written, `close()`-synced,
then **reopened read-only to verify** the on-card size equals the encoded size.

---

## Expected serial log

Boot (no card):
```
========================================
 Smart Museum -- ESP32S-CAM (GC2145) node
 capture-to-microSD  (no Wi-Fi/Hub yet)
========================================
[boot] PSRAM present (X.X MB), free heap ~XXX KB
[cam] init OK. sensor PID=0x2145 (GC2145)  PSRAM=yes (XXXX KB)
[sd]  SD_MMC.begin() FAILED -- no card, wrong FS, or poor seating
[boot] ready. Commands: CAPTURE [station] | STATUS | LIST | INIT | FLASH ON/OFF | HELP
```

Boot (with FAT32 card):
```
[sd]  mounted /sdcard (1-bit)  type=SDHC/SDXC  card=XXXX.X MB  fsTotal=XXXX.X MB  fsFree=XXXX.X MB  photos=0
```

`CAPTURE` success:
```
[capture] OK /photos/IMG_0001.JPG  320x240  12345 bytes (verified)  station=0
```

`CAPTURE` failure (examples):
```
[capture] FAIL SD card not mounted -- insert a FAT32 microSD, then send INIT
[capture] FAIL RGB565->JPEG conversion (frame2jpg) failed
```

---

## microSD requirements

- **FAT32** preferred (also works with FAT16/SDSC). SDHC/SDXC (4–32 GB) is ideal.
- Insert before/while powered; if it was inserted after boot, send `INIT`.
- A 16 GB card holds thousands of QVGA JPEGs (each a few KB at quality 15).
- If `STATUS` reports `CARD_NONE`, reseat the card and run `INIT`.

---

## Future Hub integration (not wired in this phase)

The integration point is a single stub: [`uploadToHubJpeg()`](src/main.cpp) in
`src/main.cpp`. Later (after the core demo is solid) this node will:

1. join the Hub's Wi-Fi **MuseumHub** as a client on channel 1 (it is a **client
   only — it never hosts**),
2. `POST http://192.168.4.1/api/image/upload` with the raw JPEG body just saved,
3. send a `museum::MSG_IMAGE_READY` packet over ESP-NOW (channel 1).

The shared ESP-NOW packet is [`../common/MuseumProtocol.h`](../common/MuseumProtocol.h).
Nothing in this phase joins Wi-Fi or performs any HTTP.
