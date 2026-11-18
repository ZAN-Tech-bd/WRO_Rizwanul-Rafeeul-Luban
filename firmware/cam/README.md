# ESP32-CAM GC2145 photo node

The [current museum firmware runbook](../README.md) covers the Hub and the
robot. This target accepts camera commands from USB and the robot UART. It
encodes a JPEG in RAM, uploads it to the Hub when connected, and can save a
copy to its onboard microSD.

## Hardware and wiring

| Item | Current setup |
|---|---|
| Camera | ESP32S-CAM on a CAM-MB USB programmer with a GC2145 ribbon sensor |
| Camera profile | AI-Thinker ESP32-CAM, PSRAM, 10 MHz XCLK, RGB565, QVGA 320×240, one framebuffer |
| Encoding | Software JPEG via `frame2jpg()` at quality 15 |
| Robot trigger | Robot ESP32-S3 **GPIO18 → J8.3 → camera GPIO13**, common ground, 3.3 V UART at 9600 baud, 8N1 |
| microSD | Optional FAT32, 1-bit SD_MMC on GPIO2/14/15. Send `INIT` to mount it. |
| Flash LED | GPIO4, off on boot. Use `FLASH ON` and `FLASH OFF`. |

The camera drops two stale frames before taking the JPEG so it uses the
current station view. Its optional microSD backup uses the next free photo
index and verifies the written file size.

The robot sends one `SHOOT N` during a station stop. The camera's GPIO13
receives the command on UART1. The USB console stays on its own UART at
115200 baud and echoes incoming robot commands as `[link] <- "SHOOT N"`.

## Build and flash over USB

Run from the repository root:

```sh
pio run -d firmware/cam -e cam
pio run -d firmware/cam -e cam -t upload --upload-port /dev/serial/by-id/usb-1a86_USB_Serial-if00-port0
pio device monitor --port /dev/serial/by-id/usb-1a86_USB_Serial-if00-port0 --baud 115200
```

Check the actual serial path first. The CAM-MB typically shows up as a CH340
USB adapter. The firmware uses `huge_app.csv` with a single 3 MB app partition.
If the adapter appears but upload stays at `Connecting...`, hold **BOOT/IO0**,
tap and release **RST**, then release BOOT and retry. Tap **RST** normally to
run the program after upload if the carrier stays in download mode.

## Camera commands

Commands are case-insensitive and end with a newline.

| Command | Action |
|---|---|
| `STATUS` | Show camera PID, PSRAM, SD, Wi-Fi and last JPEG information. |
| `SHOOT N` | Capture immediately, join MuseumHub if needed, then upload. Use station 0 for a bench photo. |
| `CAPTURE [N]` | Capture into RAM. Upload if already on MuseumHub and back up if SD is mounted. |
| `SEND [N]` | Join MuseumHub if needed and send the cached JPEG again. |
| `WIFI` | Connect to MuseumHub or report its IP, RSSI and channel. |
| `INIT` | Retry camera initialization and mount the optional SD. |
| `LIST` | List files in the camera SD's `/photos` folder. |
| `AUTO <ms>` / `AUTO` | Start or stop periodic capture. Minimum interval is 1000 ms. |
| `FLASH ON` / `FLASH OFF` | Control the camera's flash LED. |
| `HELP` | Show the commands. |

## Verify the photo path

On boot, look for camera PID `0x2145`, the robot receiver on GPIO13, and
association with `MuseumHub` on channel 1. On the camera console, send
`SHOOT 0`. Expect a 320×240 JPEG and `POST multipart JPEG -> HTTP 200`.
The Hub will show it in the live gallery. The camera sends the JPEG as the
multipart part `image` to `http://192.168.4.1/api/image/upload?station=N`.
The Hub's dashboard polls for committed gallery entries.

If MuseumHub is unavailable, `SHOOT` captures first and then attempts to
connect for up to 15 seconds. Use `SEND` after the Hub returns to resend the
latest cached JPEG. The RAM cache is cleared on reboot. With a card mounted,
backups are stored as `/photos/IMG_####.JPG` using the next available index.

## Bench verification log

- **2026-11-18** — flashed and verified end to end on the CAM-MB (CH340,
  enumerated as `/dev/ttyUSB0` on this bench; `cam_cmd.sh` defaults to
  `/dev/ttyUSB1`, so set `CAM_PORT=/dev/ttyUSB0`). Boot reports
  `sensor PID=0x2145 (GC2145)` with 4 MB PSRAM and joins `MuseumHub`
  channel 1 at 192.168.4.5. `SHOOT 0` captured a 320×240 JPEG (2308 bytes)
  and the Hub answered `POST multipart JPEG -> HTTP 200`. Quirks confirmed
  on this carrier: esptool's DTR/RTS auto-reset never enters download mode
  (`No serial data received`) — hold IO0, tap RST, release IO0 while
  esptool is retrying. After a verified write the chip can sit silent in
  download mode; one RST tap with BOOT released boots the app.
