# Firmware — one PlatformIO project per ESP32

All nodes talk over **ESP-NOW on channel 1** (no router, no internet). Phones and
the admin laptop are **clients** of the Hub.

```
                 ESP-NOW bus — channel 1 — small status packets
   ┌───────────┬───────────┬───────────┬──────────────┐
[robot]     [health]   [emergency]   [cam]         [hub = Track ESP32]
LOCATION     HEALTH     EMERGENCY     photo         SoftAP "MuseumHub" ch1
 (station)  (T/H/gas)   (button)     (HTTP upload)  + web server + /api
                                                       │
                                        phones / laptop (clients)
```

## Projects

| Folder | ESP32 role | Status |
|--------|------------|--------|
| [`hub/`](hub/README.md) | **Track ESP32 = the Hub**: SoftAP + web server + ESP-NOW aggregator + LEDs | ✅ built |
| [`robot/`](robot/README.md) | Guide robot: drive, RFID stops, Bangla/EN audio, broadcasts LOCATION | ⬜ skeleton (Phase 1–3) |
| [`health/`](health/README.md) | DHT11 + MQ-135 at each artifact; broadcasts HEALTH | ⬜ skeleton (Phase 6) |
| [`emergency/`](emergency/README.md) | Big push button; broadcasts EMERGENCY | ⬜ skeleton (Phase 5) |
| [`cam/`](cam/README.md) | ESP32-CAM; uploads a photo to the Hub | ⬜ skeleton (Phase 6) |

The shared packet is in [`common/MuseumProtocol.h`](common/MuseumProtocol.h:1).

## Build any node

```bash
cd firmware/<node>      # e.g. hub
pio run -t upload       # flash
pio device monitor      # serial @ 115200
```

> Rule that bites: **ESP-NOW + the Hub SoftAP must share channel 1.** It is set in
> every node (see each `platformio.ini` / config). Don't change it on only one board.
