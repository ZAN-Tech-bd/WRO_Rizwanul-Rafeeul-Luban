# ESP32_LFR — Five-Sensor Line Following Robot

A line-following robot built on the **ESP32-WROOM-32** DevKit, using a five-channel digital IR sensor array and an L298N dual H-bridge motor driver. This sketch is a direct port of an Arduino UNO line-follower to the ESP32, preserving the original PD control algorithm while adapting the I/O for ESP32 GPIO pins and LEDC PWM.

## Hardware

| Component | Details |
|---|---|
| Controller | ESP32-WROOM-32 DevKit (30-pin, DOIT-style) |
| Sensor array | 5-channel digital IR, active-low, powered at 3.3 V |
| Motor driver | L298N dual H-bridge (ENA/ENB jumpers removed) |
| Motors | 2 × 6 V TT gearmotors |

## Wiring

### IR sensors (physical far-left → far-right)

| Sensor position | ESP32 GPIO | Notes |
|---|---|---|
| Far left | GPIO36 (VP) | Input-only, no internal pull-up |
| Left | GPIO39 (VN) | Input-only, no internal pull-up |
| Centre | GPIO34 | Input-only, no internal pull-up |
| Right | GPIO35 | Input-only, no internal pull-up |
| Far right | GPIO32 | Digital input |
| VCC | 3V3 | **Do not use 5 V** |
| GND | Common ground | |

If the sensor outputs are open-collector, add a 10 kΩ pull-up from each signal to 3.3 V.

### L298N motor driver

Physical calibration confirmed: **channel A drives the left motor, channel B drives the right motor** on this chassis.

| ESP32 GPIO | L298N | Motor |
|---|---|---|
| GPIO25 | ENA | Left PWM |
| GPIO26 | IN1 | Left direction |
| GPIO27 | IN2 | Left direction |
| GPIO13 | ENB | Right PWM |
| GPIO14 | IN3 | Right direction |
| GPIO33 | IN4 | Right direction |

### Power

- **Motor supply**: regulated 6 V → L298N VS/motor terminal (start low, increase only if torque is inadequate).
- **Logic supply**: regulated 5 V → L298N 5 V terminal (disable the onboard regulator jumper first).
- **ESP32**: 5 V via USB during programming, or regulated 5 V to VIN/5V pin for standalone operation.
- **Common ground**: join ESP32 GND, sensor GND, L298N GND, motor buck GND, and logic buck GND at one star point.
- **Never power motors from the ESP32.**

## Algorithm

The line-following logic is identical to the original Arduino sketch:

1. **Read five digital IR sensors** and invert each (active-low: LOW = line detected).
2. **Compute weighted position error**: `(10·s[0] + 20·s[1] + 30·s[2] + 40·s[3] + 50·s[4]) / count − 30`, giving a range of −20 (far left) to +20 (far right).
3. **If no line is detected** (error = 420): recover using the last known edge direction.
4. **If all five sensors are active**: T-junction detected — pivot right for a fixed duration.
5. **Otherwise**: apply PD correction (`kp` and `kd`), with a faster straight-line speed when the error is small.

## Calibrated motor values

Wheels-up testing on this chassis measured:

| Parameter | Value |
|---|---|
| Left motor start threshold | PWM 180 |
| Right motor start threshold | PWM 190 |
| Matched pair (approx.) | Left 200 / Right 220 |
| Right pivot | Left +200 / Right −220 |
| Pivot duration (provisional) | 250 ms |

These are no-load values. On-floor traction and PID tuning may require adjustment.

## Arduino IDE setup

1. Install **ESP32 board support** by Espressif Systems via Boards Manager.
2. Select board: **ESP32 Dev Module**.
3. Open `ESP32_LFR.ino` and click Upload.
4. (Optional) Open Serial Monitor at 115200 baud for debug output.

## Tuning guide

All tunable parameters are at the top of the sketch:

```cpp
float kp = 1;        // Proportional gain
float kd = 1;        // Derivative gain
int base_L = 200;    // Curve base speed, left
int base_R = 220;    // Curve base speed, right
int turnDelay = 250; // T-junction pivot duration (ms)
```

- If the robot **oscillates** on straights: reduce `kp`.
- If the robot **overshoots** curves: increase `kd`.
- If the robot **drifts** to one side on straights: adjust `base_L` / `base_R` individually.
- If the T-junction turn is **too far or too short**: adjust `turnDelay`.
