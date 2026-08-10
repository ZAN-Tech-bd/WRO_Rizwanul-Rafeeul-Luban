# ESP32 RFID -> DFPlayer + Nano rail controller

Tap an RFID card on the MFRC522 reader and the ESP32 plays the matching MP3
track on the DFPlayer Mini. The two control cards also start or stop the Nano
rail controller.

## Wiring

### MFRC522 (SPI, default ESP32 VSPI pins)
| MFRC522 | ESP32 |
|---------|-------|
| SDA/SS  | GPIO5 |
| SCK     | GPIO18 |
| MOSI    | GPIO23 |
| MISO    | GPIO19 |
| RST     | not used (same as your reader sketch) |
| 3V3/GND | 3V3/GND |

### DFPlayer Mini (UART2)
| DFPlayer | ESP32 |
|----------|-------|
| TX       | GPIO16 (RX2) |
| RX       | GPIO17 (TX2) via ~1k resistor |
| VCC/GND  | 5V/GND |
| Speaker  | SPK_1 / SPK_2 |

### Nano rail controller (UART1)
| Nano | ESP32 |
|------|-------|
| D1/TX | GPIO25/RX1 through a 10 kΩ / 20 kΩ divider |
| D0/RX | GPIO26/TX1 |
| GND | GND |

The Nano must run `NANO_LFR_RAIL/NANO_LFR_RAIL.ino` and use 9600 baud. The
Nano UART wires must be disconnected while flashing the Nano.

Put MP3 files on the SD card in a folder named `MP3` as `0001.mp3` ... `0008.mp3`.

## UID -> Track/action map

| Action | Track | RFID UID (hex) |
|--------|------:|----------------|
| Start rail + play intro | 0001 | `18 C4 0F BA` |
| Stop rail; no new audio | — | `A3 C0 38 15` |
| Station audio | 0002 | `64 14 F6 B3` |
| Station audio | 0003 | `4A 6B DF 34` |
| Station audio | 0004 | `D8 36 D4 B9` |
| Station audio | 0005 | `94 60 6B B2` |
| Station audio | 0006 | `18 48 F4 BA` |
| Station audio | 0007 | `94 58 1A B3` |
| Station audio | 0008 | `09 4C B1 92` |

Every mapped audio card first sends `STOP\n` to the Nano. When the DFPlayer
reports that the track finished, the ESP32 sends `START\n` so the rail moves
again. If audio is unavailable or times out, the rail remains stopped as a
safe fallback.

The start card clears the stop latch, plays `MP3/0001.mp3`, and sends `START\n`
after the intro finishes. The end card sends `STOP\n`, stops any current
DFPlayer audio, selects no new track, and latches the rail stopped. Station
audio cannot clear that latch; only the start card can start the rail again.

> All UIDs are full 4-byte values, stored in the same byte order the reader prints.

The bytes are stored in the same order the reader prints them
(`Card UID: 64 14 F6 B3` -> `{0x64,0x14,0xF6,0xB3}`).

## How it behaves

* Tap the start card -> the Nano stays stopped while `0001.mp3` plays, then starts.
* Tap the end card -> the Nano hard-brakes and remains stopped.
* Tap a station card -> the Nano stops, its track plays, then the rail starts.
* Holding the card on the reader does **not** replay (the card is halted after
  each read; remove it and tap again to replay).
* Unknown UID -> "UID not in map" on Serial, nothing plays.

## Adding a new card

1. Open the Serial Monitor (115200) and tap the card; read the printed
   `Card UID: XX XX XX XX`.
2. Add one row to the `TRACKS[]` table in the sketch with those bytes and the
   track number.

## Serial control (for testing)

`1`..`9` play a track, `+`/`-` volume, `s` stop.
