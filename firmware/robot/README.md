# Robot — ESP32 five-sensor line follower

Phase 1 ports the team's working Arduino five-digital-sensor/L298N line follower to a DOIT-style 30-pin ESP32-WROOM-32 DevKit. Later RC522, DFPlayer, WS2812B, camera, display, and ESP-NOW work is intentionally outside this first motion build.

## Status

- Phase 1 wiring: locked.
- Phase 1 line-following firmware: implemented and build-verified.
- Later museum integration: not active in this target yet.

The full robot electrical and power plan is in [`plans/robot-wiring.md`](../../plans/robot-wiring.md:1).

## Exact Phase 1 pinout

### Five-channel digital IR array

Power the confirmed 3.3 V array from the ESP32 `3V3` pin. Wire by the sensor's **physical position when the robot faces forward**, even if the module's printed channel numbering is reversed by mounting orientation.

| Physical detector | Array pin | ESP32 pin | Firmware index |
|---|---|---:|---:|
| far left | corresponding digital output | GPIO36 / VP | 0 |
| left | corresponding digital output | GPIO39 / VN | 1 |
| centre | corresponding digital output | GPIO34 | 2 |
| right | corresponding digital output | GPIO35 | 3 |
| far right | corresponding digital output | GPIO32 | 4 |
| power | VCC | 3V3 | — |
| ground | GND | GND/common star ground | — |

GPIO34, 35, 36, and 39 have no internal pull-ups. The array must actively drive them. If testing proves the outputs are open collector, fit one external 10 kΩ pull-up from each output to **3.3 V**, never 5 V.

Default polarity matches the original Arduino sketch: sensor output LOW means line detected. Check all five channels over black and white before installing the motor jumpers.

### L298N

Physical calibration of the installed chassis confirmed: **channel A drives the robot's left motor; channel B drives the robot's right motor**. This differs from the original Arduino bench wiring, so the ESP32 pin map was corrected to match the actual robot.

| ESP32 / supply | L298N | Destination / note |
|---|---|---|
| GPIO25 | ENA | left PWM; remove ENA jumper |
| GPIO26 | IN1 | left direction |
| GPIO27 | IN2 | left direction |
| — | OUT1 + OUT2 | robot's physical left TT motor |
| GPIO13 | ENB | right PWM; remove ENB jumper |
| GPIO14 | IN3 | right direction |
| GPIO33 | IN4 | right direction |
| — | OUT3 + OUT4 | robot's physical right TT motor |
| regulated motor rail, initially 6.0 V | `+12V` / VS terminal | motor supply; the common board label does not require 12 V |
| regulated 5 V logic rail | `5V` terminal | remove/disable `5V_EN` first |
| common star ground | GND | join motor buck, logic buck, ESP32, and sensor grounds |

For final standalone power, feed the regulated 5 V logic rail to the ESP32 `5V`/`VIN` pin. During initial USB programming, keep the motor rail off and avoid paralleling USB 5 V with an external 5 V source unless the exact DevKit power path has been verified.

Do not power either motor from the ESP32. Add a 100 nF ceramic directly across each motor, twist each motor pair, and place 1000–2200 µF plus 100 nF at the L298N motor-supply input.

## Direction and sensor checks before wheels-down testing

1. Remove both ENA/ENB jumpers and raise the chassis so the wheels cannot drive away.
2. During calibration, keep `kEnableMotorsAfterStartup = false` in [`main.cpp`](src/main.cpp:62). Power the ESP32 and sensor array only, confirm serial output at 115200 baud, and verify the five reported bits left-to-right.
3. Confirm black produces active bit `1`. If it produces `0`, change the single sensor-polarity constant in the firmware; do not reorder the sensors.
4. Keep autonomous-at-boot disabled, raise the chassis, and then enable the motor rail. Send `ARM`, followed by a bounded command such as `MOTOR 200 220 500`. A positive command on both sides must propel both wheels forward.
5. If one wheel runs backward, switch off power and swap only that motor's two L298N output leads.
6. For the first wheels-down test, put the robot on a wide, straight, high-contrast line, send `ARM`, then `AUTO 1000`. Calibration `AUTO` requires a 1–5000 ms duration and automatically stops and disarms at timeout.
7. Keep a hand at the power switch and use progressively longer bounded runs only after each shorter run behaves safely. Tune `Kp`, `Kd`, base speed, recovery, and turn time before enabling autonomous operation at boot.

## Control behavior preserved from the Arduino sketch

- weighted five-sensor position error from -20 through +20;
- proportional + derivative correction;
- remembered left/right edge recovery when all sensors lose the line;
- all-five-active T-junction detection followed by a timed right pivot;
- separate straight and curve base-speed branches;
- signed wheel commands with active braking at zero.

## Measured motor calibration

Wheels-up tests on the physical chassis established:

- channel A (`ENA` GPIO25) is the physical left motor;
- channel B (`ENB` GPIO13) is the physical right motor;
- both positive commands are forward and both negative commands are reverse;
- left starts continuously by PWM 180; right starts continuously by PWM 190;
- left=200/right=220 gives approximately matched no-load wheel speeds;
- left=+200/right=-220 gives a correct right pivot.

The initial on-floor tuning baseline therefore uses left/right 200/220. These are no-load measurements, so final traction and PID tests may require adjustment.

## Build and upload

From the repository root:

```bash
pio run -d firmware/robot
pio run -d firmware/robot -t upload
pio device monitor -b 115200
```

Calibration serial commands at 115200 baud:

```text
HELP
SENSORS
ARM
MOTOR <left -254..254> <right -254..254> [milliseconds]
AUTO <milliseconds 1..5000>
STOP
DISARM
```

The board boots stopped and disarmed while `kEnableMotorsAfterStartup` is false. `AUTO` is deliberately time-bounded in this calibration image and disarms itself after the requested duration.

If 921600-baud upload is unreliable on a clone board, lower `upload_speed` in [`platformio.ini`](platformio.ini:1) to 460800 or 115200. Keep wheels raised and the motor rail off while flashing.

Verified local build: Espressif32 platform 6.5.0, Arduino-ESP32 2.0.14, 21,480 bytes RAM (6.6%), and 270,233 bytes application flash (20.6%). This is compile/static-analysis proof only; sensor polarity, motor direction, recovery direction, junction timing, and PD tuning still require the physical robot.

## Reserved later GPIO map

The full plan reserves RC522 SPI on GPIO18/19/23 with SS=21 and RST=22, DFPlayer UART2 on RX=16/TX=17, and WS2812B data on GPIO4. USB serial stays on GPIO1/3. GPIO2/5/12/15 remain intentionally unused reset-strapping pins.

When station networking is implemented, station IDs must match [`config.h`](../hub/include/config.h:1): 1=jamdani, 2=nakshi, 3=liberation, 4=terracotta, 5=ektara, 6=palki, 7=tiger.
