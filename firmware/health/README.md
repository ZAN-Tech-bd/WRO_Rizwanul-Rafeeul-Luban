# Health — Artifact monitoring (PlatformIO project)

Watches one artifact's environment and reports it to the Hub, so the admin sees
per-artifact temperature, humidity and gas (the "protecting heritage" story).

**On the network:** broadcasts `MSG_HEALTH { stationId, tempX10, humidX10, gasRaw }`
every few seconds → Hub stores it → admin dashboard shows green/yellow/red.

> Set `THIS_STATION_ID` in [`src/main.cpp`](src/main.cpp:1) to the artifact this
> board watches, matching the Hub order:
> 1=jamdani, 2=nakshi, 3=liberation, 4=terracotta, 5=ektara, 6=palki, 7=tiger.

## Status
⬜ Skeleton. Implement in Phase 6.

## Planned wiring
| Part | ESP32 pin | Notes |
|------|-----------|-------|
| DHT11 data | GPIO… | 3.3/5 V per module; 10 kΩ pullup |
| MQ-135 AO | GPIO32–39 (ADC1) | **own buck rail** (~150 mA heater each); 24–48 h burn-in |
| Power | separate buck for heaters | don't share with ESP32 logic rail |

Notes: readings are a relative trend, not calibrated ppm — present vs threshold.
Build: `cd firmware/health && pio run -t upload`.
