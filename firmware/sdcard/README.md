# ESP32 SD Card Reader (`firmware/sdcard`)

A small PlatformIO project that mounts a microSD card over SPI and dumps its
contents to the USB serial monitor. Useful as a standalone bring-up tool or as
the basis for logging/reading data in the Smart Museum robot.

## Wiring (VSPI default pins)

| SD module | ESP32 GPIO | Notes |
|-----------|-----------|-------|
| VCC       | 5V        | Use a module **with** an onboard 3.3V regulator (AMS1117). See voltage note below. |
| GND       | GND       | Common ground is mandatory. |
| MISO      | GPIO 19   | VSPI MISO |
| MOSI      | GPIO 23   | VSPI MOSI |
| SCK       | GPIO 18   | VSPI SCK |
| CS / SS   | GPIO 5    | VSPI CS — must be passed to `SD.begin()` |

These are the **default VSPI pins** on a standard ESP32 dev board, so no
re-mapping is needed. Change them in [`include/config.h`](include/config.h) if
you ever move the card to other pins.

### ⚠️ Voltage safety

The microSD card itself is a **3.3V device** and the ESP32 GPIOs are 3.3V.

* The common red/blue microSD modules with an **AMS1117-3.3 regulator + 74LVC125A**
  buffer accept a 5V VCC safely and output 3.3V on MISO — **this is the safe
  setup**, which matches the 5V wiring above.
* If your module is a **bare breakout** (just the socket + pull-up resistors,
  no regulator / no level shifter), power it from **3.3V, not 5V**. Never feed
  5V into the ESP32 GPIO pins.

## Build & run

```bash
cd firmware/sdcard
pio run -t upload          # build + flash
pio device monitor         # open serial monitor (115200 baud)
```

## What it does

On boot it:

1. Mounts the card and prints **type, total size, and used bytes**.
2. Lists the **root directory** and the **full directory tree** (folders + files).
3. Dumps the contents of `/test.txt` if that file exists.

Then it accepts serial commands so you can browse the card interactively:

| Command       | Action |
|---------------|--------|
| `LS`          | List the root directory |
| `TREE`        | List root + all subfolders (recursive) |
| `READ /a.txt` | Print a file's text contents |
| `INFO`        | Reprint card type + size |
| `HELP`        | List commands |

Set the serial monitor to **115200 baud, line ending = newline (NL)**.

## Troubleshooting

* **`Mounting SD card... FAILED.`** → card not seated, loose jumper, wrong CS
  pin, or a bare 3.3V-only module fed 5V (or vice-versa).
* **Intermittent CRC errors** → drop `SD_SPI_FREQ_MHZ` in [`config.h`](include/config.h)
  to 1–2 MHz; long Dupont wires hate fast SPI.
* **Format** the card as **FAT32** (or FAT16 for ≤2GB). exFAT/NTFS are not
  supported by this driver.
