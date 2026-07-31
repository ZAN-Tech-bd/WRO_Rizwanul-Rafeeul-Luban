# Cam — ESP32-CAM photo node (PlatformIO project)

Captures one photo per station visit and uploads it to the Hub for the admin.

**On the network:** joins **MuseumHub** WiFi as a client (STA, channel 1) and
`POST http://192.168.4.1/api/image/upload` with a raw JPEG body. The CAM is a
**client only — it never hosts** (it's pin-starved and busy with the camera).

## Status
⬜ Skeleton / stretch goal. Implement in Phase 6 (after the core demo is solid).

## Wiring
| Part | Notes |
|------|-------|
| OV2640 camera | plugs into the ESP32-CAM board (ribbon) |
| Power | stable 5V; brownout-prone — use a good supply or external 5V to 5V pin |
| Programming | needs an FTDI/USB-UART; hold GPIO0→GND to flash |

Build: `cd firmware/cam && pio run -t upload` (board = `esp32cam`).
