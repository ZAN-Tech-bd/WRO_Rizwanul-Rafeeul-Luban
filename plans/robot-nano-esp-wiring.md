# Robot Wiring — Arduino Nano Motion/RFID + ESP32 Companion

> **SUPERSEDED (2026-08-08).** The RFID-on-Nano split and framed `$TAG…*HH` / `ACK` / `RESUME`
> protocol documented below were never built. The shipped robot (`NANO_LFR/NANO_LFR.ino` +
> `ESP32_RFID_MP3_LFR/ESP32_RFID_MP3_LFR.ino`) instead keeps RC522 on the **ESP32** — the Nano only
> does motors + line sensor — linked by a plain `GO`/`START`/`STOP` toggle over the Nano's **hardware**
> UART (D0/D1), not `SoftwareSerial` D2/D3. See the "Status check" section near the top of
> `BUILD_PLAN.md` for the full list of differences and the actual current pin map/protocol. Kept here
> for history; do not wire or code against this document.

## Controller split

- **Arduino Nano:** IR array, L298N, motors, RC522, line following, RFID stop logic.
- **ESP32:** validated RFID-event receiver, later DFPlayer/audio/LED/network duties.

The active allocation below supersedes the direct-ESP motor/RFID map in [`robot-wiring.md`](robot-wiring.md).

## Nano GPIO allocation

| Nano pin | Connection | Notes |
|---|---|---|
| A0, A1, A2, A3, A4 | IR far-left through far-right outputs | active-low; physical order is fixed |
| D5 | L298N ENA | left motor PWM; remove ENA jumper |
| D7, D8 | L298N IN1, IN2 | left motor direction |
| D6 | L298N ENB | right motor PWM; remove ENB jumper |
| D9, A5 | L298N IN3, IN4 | right motor direction; A5 used as digital output |
| D10 | RC522 SDA/SS | 5 V→3.3 V level shift required |
| D11 | RC522 MOSI | hardware SPI; 5 V→3.3 V level shift required |
| D12 | RC522 MISO | hardware SPI; direct RC522→Nano is acceptable |
| D13 | RC522 SCK | hardware SPI; 5 V→3.3 V level shift required |
| D4 | RC522 RST | 5 V→3.3 V level shift required |
| D2 | SoftwareSerial RX | optional ACK from ESP32 GPIO26 through 1 kΩ |
| D3 | SoftwareSerial TX | to ESP32 GPIO25 through 1 kΩ/2 kΩ divider |
| D0, D1 | USB hardware serial | diagnostics/upload; do not use for the inter-board link |

The supplied Arduino sketch cannot keep motors on D10/D11/D12 because those pins are the ATmega328P hardware-SPI bus required by the RC522. The motor remap above is mandatory.

## IR array and L298N

```text
IR VCC -> regulated 3.3 V
IR GND -> common ground
far-left -> Nano A0
left     -> Nano A1
centre   -> Nano A2
right    -> Nano A3
far-right-> Nano A4

Nano D5 -> ENA
Nano D7 -> IN1
Nano D8 -> IN2
OUT1/OUT2 -> physical left motor

Nano D6 -> ENB
Nano D9 -> IN3
Nano A5 -> IN4
OUT3/OUT4 -> physical right motor
```

The array was physically verified active-low: black produces logical `1` after inversion. If its outputs are open collector, fit one 10 kΩ pull-up from each output to **3.3 V**, never 5 V. Channel A remains the physical left motor and channel B remains the physical right motor. The measured initial trim remains left PWM 200 / right PWM 220, but Nano timer/PWM and on-floor load require fresh validation.

## RC522 SPI — 3.3 V only

```text
RC522 3.3V -> regulated 3.3 V
RC522 GND  -> common ground
Nano D10 -> 5 V-to-3.3 V buffer/level shifter -> RC522 SDA/SS
Nano D11 -> 5 V-to-3.3 V buffer/level shifter -> RC522 MOSI
RC522 MISO -> Nano D12
Nano D13 -> 5 V-to-3.3 V buffer/level shifter -> RC522 SCK
Nano D4  -> 5 V-to-3.3 V buffer/level shifter -> RC522 RST
RC522 IRQ -> unconnected
```

Use a push-pull-capable 3.3 V logic translator intended for SPI, such as a 74LVC125 powered at 3.3 V, with 100 nF decoupling at the IC. Do **not** power the RC522 from 5 V and do not connect Nano 5 V outputs directly to RC522 inputs. Keep SPI wiring short and away from motor leads.

## Nano SoftwareSerial ↔ ESP32 UART

```text
Nano D3 TX -> 1 kΩ -> junction -> ESP32 GPIO25 RX
                              -> 2 kΩ -> GND

ESP32 GPIO26 TX -> 1 kΩ -> Nano D2 RX
Nano GND -----------------> ESP32 GND
```

The 1 kΩ/2 kΩ divider reduces the Nano's 5 V UART HIGH to approximately 3.3 V. Never connect Nano D3 directly to an ESP32 GPIO. The ESP32's 3.3 V TX is accepted as HIGH by the Nano; the 1 kΩ series resistor limits fault current. Both controllers use 9600 baud, 8N1.

ESP32 GPIO16 RX and GPIO17 TX remain reserved for DFPlayer UART2. ESP32 GPIO1/3 remain USB serial. GPIO25/26 are dedicated to the Nano link now that the ESP32 no longer owns the L298N.

## RFID event behavior and protocol

When a newly presented card is read successfully, the Nano:

1. brakes both motors immediately;
2. formats the UID as uppercase hexadecimal without separators;
3. sends one newline-terminated event to the ESP32;
4. calls `PICC_HaltA()` and `PCD_StopCrypto1()` so the same continuously held card does not retrigger;
5. remains stopped until the ESP32 sends `RESUME,<sequence>`, with a 15-second Nano fail-safe maximum stop.

Removing and presenting the card again creates another event. Different cards each generate their own sequence number and event.

Every link message uses `$PAYLOAD*HH\n`, where `HH` is the two-digit XOR of all payload bytes. A card event is `$TAG,<sequence>,<UID_HEX>,<SAK_HEX>,<PICC_TYPE_NUMBER>*HH`. The ESP validates the checksum and fields, then replies with framed `ACK,<sequence>`. After its station work it sends framed `RESUME,<sequence>`; the Nano then replies with `RESUMED,<sequence>` and restarts line following.

The UID, SAK, and PICC type are card-identification metadata, not secret or cryptographically authenticated credentials.

## Power and first bring-up

- Power the Nano from regulated 5 V at its `5V` pin for the installed robot; do not apply regulated 5 V to `VIN`.
- Use USB alone while initially flashing unless the exact dual-supply path is verified. Never backfeed two unrelated 5 V supplies.
- Keep all Nano, ESP32, RC522, sensor, L298N logic, and buck grounds common.
- Keep the existing motor buck, L298N suppression, star ground, and 1000–2200 µF motor-rail bulk capacitor.
- Add 100 nF plus 10 µF at the RC522 and IR array, and 100 nF at the SPI level translator.

Acceptance order: build both targets; power Nano/RC522 with motors disconnected; prove card reads; prove framed UART reception; test each motor wheels-up; confirm five sensor patterns; then perform bounded floor tests. Do not assume the previous ESP32 PWM thresholds transfer exactly to Nano PWM timers.
