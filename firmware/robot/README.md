# Robot — Guide Robot (PlatformIO project)

The robot that drives the track, stops at each artifact, and narrates in Bangla/English.

**On the network:** broadcasts `MSG_LOCATION { stationId, moving }` when it reaches
a station → the Hub lights that station's LED and the visitor page shows it. Listens
for `MSG_EMERGENCY` → reroutes to the nearest exit. (Channel 1.)

> `stationId` **must match the Hub's `STATION_IDS` order** in
> [`../hub/include/config.h`](../hub/include/config.h:1):
> 1=jamdani, 2=nakshi, 3=liberation, 4=terracotta, 5=ektara, 6=palki, 7=tiger.

## Status
⬜ Skeleton. Implement in Phase 1–3.

## Planned wiring (fill in during Phase 1)
| Part | ESP32 pin | Notes |
|------|-----------|-------|
| L298N IN1/IN2 (left) | GPIO… | PWM on one of each pair (ledc) |
| L298N IN3/IN4 (right) | GPIO… | |
| 5-array IR sensor | GPIO… ×5 | digital inputs |
| RC522 (SPI) | SCK/SDA/MOSO/MISO/RST + SDA(SS) | **3.3 V only** |
| DFPlayer RX | GPIO… via **1 kΩ** | TX→RX through resistor |
| OLED SSD1306 (I2C) | SDA/SCL | station name |
| NeoPixel arrow | GPIO… | direction |
| Front obstacle IR | GPIO… | pause for people |
| Power | separate bucks for logic vs motors + 1000 µF cap | brownout prevention |

Build: `cd firmware/robot && pio run -t upload`.
