# Station DHT11 bench test — Hub PCB (S1 + S2 only, for now)

Reads the two DHT11 sensors (station S1 + S2) every 3 seconds with
SUCCESS/FAIL counters, using the **Adafruit DHT library code path that the
user verified working on this exact board** (2026-11-11). This replaces the
earlier hand-written bit-bang reader and the multi-sensor menu version.

## Scope

- **S1 DHT11:** hub J10.4 → **GPIO15** (U1A.8), module on station J3 (3V3/GND/DHT)
- **S2 DHT11:** hub J11.4 → **GPIO16** (U1A.9), same wiring
- **S1 MQ-5:** hub J10.5 → **GPIO4** (ADC, C3 100 nF at pin), module on station J4 (5V/GND/AO)
- **S2 MQ-5:** hub J11.5 → **GPIO5** (ADC, C4), same wiring
- MQ lines carry **no series resistor on the hub** (spec §5) — the station's
  68k/100k divider already limits the cable to ≤3.0 V, so the R31–R37 removal
  cannot affect them. The sketch prints raw counts, ADC volts, and the
  un-divided sensor volts (Vadc × 1.68). Fresh MQ-5 needs minutes of warm-up:
  readings drift down while stabilising and rise with gas near the sensor.
- PROX zone 1 (GPIO41, shared S1+S2, LOW = detected, R51 1 k series + R41 10 k
  pull-up) and the LED sanity command (GPIO45) are **not in this sketch**.

## Hardware change on the PCB (user decision, 2026-11-11)

The hub's DHT series resistors **R31–R37 (1 kΩ by design) were removed** —
the DHT11 only answered when the resistor in its data path was bypassed.
The data lines now run **direct** from J10.4/J11.4 to GPIO15/GPIO16.

Facts worth keeping:

- The symptom (dead with resistor fitted, alive when shorted) does **not**
  match a correctly-valued 1 kΩ (or even 220 Ω) series part; a 1 kΩ series
  with a 10 k module pull-up leaves ≈0.3 V at the module — well inside DHT11
  LOW limits, RC ≈ 0.2 µs vs 26–70 µs pulses. The pattern **does** match a
  **220 kΩ** (red-red-yellow) accidentally stuffed instead of 1 kΩ: the 18 ms
  LOW start pulse would divide to ≈3.16 V and the sensor never sees it.
  The actual stuffed value was never measured before removal, so the root
  cause stays *suspected, not proven*.
- Consequence of removal: GPIO15/16 lose their series protection. A
  mis-crimped cable putting 5 V on pin 4 now reaches the S3 pin directly.
  If these stations are rebuilt or the resistors are re-fitted, use **1 kΩ**.
- **R54 was also removed by accident** — it is **not** a DHT resistor. It is
  the PROX **zone 4** (S7+S8) series resistor (PROX_Z4 → GPIO48). Re-stuff it
  with 1 kΩ; otherwise zone 4 floats when those stations are built.
- Boot output prints each data line's **idle state** — HIGH is expected
  (module pull-up). A LOW idle means wiring/power/module trouble before any
  read is attempted.

## Wiring reminder

| Module | Station port | Order |
|---|---|---|
| DHT11 | **J3** (3-pin XH) | 3V3, GND, DHT(data) — module runs on **3V3** |

## Build / flash / monitor

Close any open serial monitor first. The board enumerates as
`/dev/serial/by-id/usb-1a86_USB_Single_Serial_5C83468930-if00` (port number
has bounced between ttyACM0/ttyACM1 after replugs — the by-id path is stable):

```sh
/home/user/.platformio/penv/bin/pio run -d firmware/tests/station_sensor_test -t upload \
  --upload-port /dev/serial/by-id/usb-1a86_USB_Single_Serial_5C83468930-if00
/home/user/.platformio/penv/bin/pio device monitor \
  --port /dev/serial/by-id/usb-1a86_USB_Single_Serial_5C83468930-if00 \
  --baud 115200 --dtr 0 --rts 0
```

Expected serial output per cycle:

```text
S1 GPIO15 SUCCESS #3: 26.4 C | 61.0 % RH
S2 GPIO16 SUCCESS #3: 26.7 C | 63.0 % RH
totals: S1 3 ok / 0 fail | S2 3 ok / 0 fail
```

`FAIL` lines mean no valid frame: check module power (3V3), the J3 crimp, and
the station-to-hub cable. DHT11 needs ≥2 s between reads; the 3 s loop is fine.
MQ/PROX behaviour and previous diagnostic commands are documented in the git
history of this workspace and in `firmware/tests/station_led_test/README.md`.
