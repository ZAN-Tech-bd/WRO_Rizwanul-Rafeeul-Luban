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
| [`hub/`](hub/README.md) | **ESP32 — Track ESP32 = the Hub**: SoftAP + web server + ESP-NOW aggregator + LEDs | ✅ built |
| [`robot/`](robot/README.md) | **ESP32 — Guide robot**: drive, RFID stops, Bangla/EN audio, broadcasts LOCATION | ⬜ skeleton (Phase 1–3) |
| [`display/`](display/README.md) | **Arduino Mega 2560 + TFT shield** — animated robot face, language menu, station/emergency screens; UART-driven by the robot ESP32 | ✅ builds |
| [`health/`](health/README.md) | DHT11 + MQ-135 at each artifact; broadcasts HEALTH | ⬜ skeleton (Phase 6) |
| [`emergency/`](emergency/README.md) | Big push button; broadcasts EMERGENCY | ⬜ skeleton (Phase 5) |
| [`cam/`](cam/README.md) | ESP32-CAM; uploads a photo to the Hub | ⬜ skeleton (Phase 6) |

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
