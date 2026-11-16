# Firmware — one PlatformIO project per board

Most nodes are ESP32s that talk over **ESP-NOW on channel 1** (no router, no
internet). Phones and the admin laptop are **clients** of the Hub. The display
node is a **separate Arduino Mega 2560** (AVR, no Wi-Fi) — it is driven by the
robot's ESP32 over a UART wire, not over the ESP-NOW bus.

```
                 ESP-NOW bus — channel 1 — small status packets
   ┌───────────┬───────────┬───────────┬──────────────┐
[robot]     [health]   [emergency]   [cam]         [hub = Track ESP32]
LOCATION     HEALTH     EMERGENCY     photo         SoftAP "MuseumHub" ch1
 (station)  (T/H/gas)   (button)     (HTTP upload)  + web server + /api
   │                                                    │
   │ UART text cmds                                     │
   ▼                                                    │
[display]                                              │
Mega 2560 + TFT                          phones / laptop (clients)
(animated robot face)
```

## Projects

| Folder | Board / role | Status |
|--------|--------------|--------|
| [`hub/`](hub/README.md) | **ESP32 — Museum Hub**: SoftAP + web server + ESP-NOW + LEDs + siren + emergency button + sensors | ✅ built |
| [`robot-esp32-rfid-mp3/`](robot-esp32-rfid-mp3/README.md) | **ESP32 — Guide robot brain**: RFID, DFPlayer narration, Nano control, camera trigger, emergency listener | ✅ built |
| [`robot-nano-rail/`](robot-nano-rail/NANO_LFR_RAIL.ino) | **Arduino Nano — motor driver**: START / STOP over UART to an L298N | ✅ built |
| [`cam/`](cam/README.md) | **ESP32-CAM (GC2145)**: capture, save to SD, upload to the Hub | ✅ built |
| [`display/`](display/README.md) | **Arduino Mega 2560 + TFT**: animated robot face | ✅ built |
| [`display-standalone/`](display-standalone/README.md) | The face sketch as its own mini-repo (with licence/provenance) | ✅ reference |
| [`common/`](common/README.md) | Shared protocol headers (ESP-NOW packet) | ✅ |

The shared ESP-NOW packet is in [`common/MuseumProtocol.h`](common/MuseumProtocol.h:1).

> The `display/` target does **not** use the shared protocol — it is not an
> ESP-NOW peer. It receives display commands as text lines over UART. See
> [`display/README.md`](display/README.md:1) for the command set.

## Build any node

```bash
cd firmware/<node>      # e.g. hub
pio run -t upload       # flash
pio device monitor      # serial monitor
```

> Rule that bites: **ESP-NOW + the Hub SoftAP must share channel 1.** It is set in
> every node (see each `platformio.ini` / config). Don't change it on only one board.
