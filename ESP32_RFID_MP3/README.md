# ESP32 RFID -> DFPlayer

Tap an RFID card on the MFRC522 reader and the ESP32 plays the matching MP3
track on the DFPlayer Mini.

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

Put MP3 files on the SD card in a folder named `MP3` as `0001.mp3` ... `0007.mp3`.

## UID -> Track map

| Track (mp3) | RFID UID (hex) | Label   |
|------------:|----------------|---------|
| 0001        | `18 C4 0F BA`  | START   |
| 0002        | `64 14 F6 B3`  | Track 2 |
| 0003        | `4A 6B DF 34`  | Track 3 |
| 0004        | `D8 36 D4 B9`  | Track 4 |
| 0005        | `94 60 6B B2`  | Track 5 |
| 0006        | `18 48 F4 BA`  | Track 6 |
| 0007        | `94 58 1A B3`  | Track 7 |
| 0008        | `09 4C B1 92`  | Track 8 |

The START card (`18 C4 0F BA`) plays `0001.mp3` (tour intro).

> All UIDs are full 4-byte values, stored in the same byte order the reader prints.

The bytes are stored in the same order the reader prints them
(`Card UID: 64 14 F6 B3` -> `{0x64,0x14,0xF6,0xB3}`).

## How it behaves

* Tap a mapped card -> its track plays.
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
