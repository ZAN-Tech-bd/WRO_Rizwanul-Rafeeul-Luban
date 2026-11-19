# Museum HUB Carrier — Architecture & Wiring Plan v0.2 (ESP32-S3) — SUPERSEDED by WIRING-PLAN.md v0.3

> **Status: PROPOSAL — awaiting review (2026-10-11).** No schematic work until the open items in §10 are answered.
> Supersedes v0.1 (classic ESP32 DevKitC, jack-switch battery cut-off) → kept in [`WIRING-PLAN-v0.1.md`](WIRING-PLAN-v0.1.md).
> Firmware today: [`firmware/hub/include/config.h`](../../firmware/hub/include/config.h) still holds the classic-ESP32 pins; §11 lists the S3 changes.

---

## 0 · Decisions at a glance

| # | Decision | Why |
|---|---|---|
| D1 | MCU = **YD-ESP32-S3 N16R8** DevKit (VCC-GND style: 2 × 22 pins, 2 × USB-C), plugged into female headers | user choice; replaceable; no RF/USB design on our board |
| D2 | **External** 5 V buck. 12 V leaves the board on **XT60**, 5 V comes back on **XT30** | user choice; keeps a 10 A switching converter off a 2-layer, AI-routed board |
| D3 | **DC-priority selector**: back-to-back P-MOSFETs on the battery path, Schottky on the DC path, LM393 decides | battery is isolated **in both directions** whenever DC is present; switchover without an ESP reset |
| D4 | **One 2-pole (DPST) panel rocker** switches battery *and* DC *before* the selector | OFF = physically disconnected: zero drain, no plug-in spark, switch never carries inrush |
| D5 | **Battery under-voltage cut-off** (3S: off below 9.6 V, back on above 10.5 V) | a buck keeps pulling until a LiPo is destroyed; this stops it |
| D6 | Blade fuse at the battery, PTC at DC, blade fuse + TVS crowbar at 5 V in, PTC on every 5 V branch | LiPo fault energy; buck failure can't put 12 V on the 5 V rail; one bad cable can't reset the hub |
| D7 | Connector families + XT genders make wrong plugging **impossible or harmless** (§5) | the board gets re-wired by hand at every event |
| D8 | JLC places **SMD only**; the team hand-solders every through-hole connector | all connectors are LCSC *Extended* THT parts → avoids ~$3 per connector type + THT assembly fees |

---

## 1 · Inputs & assumptions (confirm in §10)

| Item | Assumed | Affects |
|---|---|---|
| Battery | 3S Li-ion/LiPo, 9.6–12.6 V, XT60 **female** lead (LiPo standard) | UVLO resistors. Everything is rated ≥ 17 V so a 4S pack also fits |
| DC adapter | 12 V, ≥ 3 A (5 A preferred), 5.5 × 2.1 mm, centre-positive | DC-detect threshold, F2 |
| Buck | **fixed** 5.0–5.2 V, ≥ 10 A, input range covering 8–17 V, short-circuit protected (potted "12 V→5 V 10 A" type) | J3/J5, F3. Avoid trim-pot bucks (XL4016): a pot knock = 12 V on the ESP |
| Loads | 56 × WS2812B (7 boards × 8), 4 × SG90, 4 × MQ-135, 4 × DHT11, passive buzzer, emergency button, microSD | current budget below |

### Current budget

| 5 V load | Typical | Worst case |
|---|---|---|
| 56 × WS2812B at firmware brightness 60/255 | ≤ 0.8 A | 3.4 A (full white at 255) |
| 4 × SG90 | 0.1 A idle | 2.6 A (all stalled) |
| 4 × MQ-135 heaters | 0.6 A | 0.6 A |
| ESP32-S3 + Wi-Fi AP, SD, DHT, buzzer, logic | 0.3 A | 0.7 A |
| **Total 5 V** | **~1.5 A** | **~7.3 A** |

12 V side ≈ 0.7 A typical, ≈ 3.5 A worst case (88 % buck efficiency). A ≥ 10 A buck leaves margin; the XT30 (15 A) and XT60 (30 A) are far from their limits.

---

## 2 · Power architecture

### 2.1 Block diagram

```
 J1 XT60-M (battery) ─F1 7.5 A─► [panel switch pole A] ─► VBAT_SW ─► Q1⇄Q2 back-to-back P-FETs ─┐
                                                              │                                  │
 panel DC jack ─J2─ F2 PTC ────► [panel switch pole B] ─► VDC_SW ─► D1 Schottky (≥ 5 A) ─────────┤
                                                              │                                  ▼
          U2 LM393 + U3 TL431:  Q1/Q2 ON only when  VBAT_SW > UVLO  AND  VDC_SW < 9 V        V12 rail
                                                                       TVS1 SMBJ18A · 1000 µF · LEDs
                                                                          │                 │
                                                             J3 XT60-F → buck IN    J4 XT60-F 12 V AUX (optional)

 buck OUT → J5 XT30-M → F3 10 A blade → TVS2 SMBJ5.0A → 5V_BUS (1000 µF, wide pours)
   5V_BUS ├─ F4 PTC → J6 XT30-F LED-A (chain start) + J7 XT30-F LED-B (far-end injection)
          ├─ F5 PTC → SERVO1–4 (XH-3) + 470 µF
          ├─ F6 PTC → MQ1–4 (XH-4)
          ├─ F7 PTC → J8 XT30-F 5 V AUX
          ├─ D2 SS34 → 470 µF reservoir → DevKit 5Vin  (its 1 A LDO makes 3V3 for ESP, SD, DHT, I²C)
          └─ U4 74HCT541 VCC, buzzer, V5 sense
```

The DC jack and both switch poles share **one 6-pin VH plug (J2 "PANEL")** — see §5.

### 2.2 Behaviour

| Switch | DC adapter | Battery | Q1/Q2 | Hub runs from | Status LEDs |
|---|---|---|---|---|---|
| OFF | any | any | off | nothing (**0 mA** drain) | none |
| ON | absent | OK (> 9.6 V) | **ON** | battery | 12V, BAT |
| ON | absent | low (< 9.6 V) | off | nothing | LOW-BAT |
| ON | present (> 9 V) | any / none | **off — blocks both ways** | DC adapter | 12V, DC |

- **DC plugged in while on battery** → comparator opens Q1/Q2 within microseconds; the rail falls to V_DC − 0.4 V and D1 takes over. No gap, no reset.
- **DC unplugged** → VDC_SW has almost no capacitance, so it collapses at once; Q1/Q2 close in ≈ 1 ms while the 1000 µF + buck input caps hold the rail.
- **Why a Schottky on the DC side and not a MOSFET:** the detector must only ever see the adapter. A switched-on MOSFET would back-feed the rail into the jack, the detector would keep reading "DC present" after unplugging, and the battery would never take over.
- **Why reverse charging is impossible:** the FETs are only on when V_DC < 9.0 V, so V_DC − V_F < 8.8 V, which is below the lowest battery voltage UVLO allows (9.6 V). Above 9 V the FETs block both directions.
- **Soft start:** a Miller capacitor on Q1/Q2 ramps the rail over a few ms, so neither the switch nor the XT60 ever sparks. It has no effect on the fast takeover, because the rail is already charged then.
- **Power-up glitch guard:** the UVLO enable has an RC delay (≈ 10 ms) so the comparators settle before the FETs can turn on. The DC-loss path has no delay.
- Thresholds are set by resistors (exact values fixed in the schematic step); 4S = scale the UVLO divider.

### 2.3 Protection matrix

| Threat | Protection |
|---|---|
| Short on board/wiring while on a LiPo (100 A+ available) | **F1** ATO blade 7.5 A directly behind J1 |
| Battery lead soldered reversed | back-to-back FET body diodes + diode-OR block; nothing conducts |
| Reversed / centre-negative adapter | D1 blocks; detector input current-limited |
| Battery drained to damage | UVLO + LOW-BAT LED; firmware warns earlier (§11) |
| Plug-in spark, switch contact wear | switch sits upstream of all bulk capacitance; FET soft-start |
| 12 V spikes (hot-plug, long leads) | TVS1 SMBJ18A on the 12 V rail |
| Buck fails high (12 V onto 5 V rail) | **F3** 10 A blade + TVS2 SMBJ5.0A crowbar: TVS clamps → fuse opens |
| One shorted sensor / LED / servo cable | per-branch PTC (F4–F7); ESP and emergency broadcast keep running |
| Servo stall / LED surge browning out the ESP | ESP fed through D2 + 470 µF reservoir |
| USB plugged in while hub is off | YD board's own 5Vin diode + our D2 → USB can't feed the LED/servo rail |

### 2.4 5 V distribution rules
- The high-current path is a short strip along one edge: J5 → F3 → J6/J7/J8 side by side. LED and servo return current never crosses the MCU area.
- 5V_BUS and V12 are ≥ 5 mm pours on both layers, stitched with vias (1 oz copper is fine at this length; 2 oz optional).
- Bulk: 1000 µF at the LED branch, 470 µF at the servo branch. Recommend 100–470 µF on each remote LED board too.

---

## 3 · MCU facts that shaped the board (YD-ESP32-S3 N16R8)

| Fact (source in §12) | Consequence for the PCB |
|---|---|
| Header rows look **25.4 mm (1.0")** apart on the YD clone (photo measurement + reports); Espressif's DevKitC-1 drawing/footprint uses **22.86 mm (0.9")** | **Measure centre-to-centre with calipers before layout.** Optional third 1 × 22 row at 22.86 mm so a genuine DevKitC-1 also fits |
| N16R8 = octal PSRAM → GPIO35/36/37 used internally | never connected |
| GPIO19/20 native USB, GPIO43/44 USB-UART (CH343P) | never connected |
| Strapping pins: GPIO0 (BOOT), **GPIO45** (flash voltage — must be LOW at reset), GPIO46 (boot mode), GPIO3 (JTAG select, inactive unless an eFuse is burned) | 0/45 never connected; 46 only as an output with an external pull-down; 3 only on the EXP header |
| ADC2 is unusable while Wi-Fi runs | every analog signal on **ADC1 = GPIO1–10** |
| S3 has **no input-only GPIOs** (ESP-IDF `soc_caps.h`: "No GPIO is input only") | any pin can run a DHT11 — the classic board's GPIO34–39 problem disappears |
| 5Vin pin enters through a diode (D3 on the YD schematic); LDO = CJ6107A33 (1 A, SOT-223) | USB can't back-feed us; DevKit 3V3 can supply ≈ 250 mA of carrier loads. **Leave the IN-OUT jumper open** |
| YD "USB-JTAG" jumper can put a 10 k pull-up on GPIO3 | GPIO3 not used for analog |
| Onboard RGB LED on GPIO48 (via "RGB" jumper) | GPIO48 left free |
| PCB antenna sticks ≈ 7 mm past the DevKit's top edge | antenna end flush with the carrier edge; copper keep-out; no metal within 15 mm |

---

## 4 · GPIO map (ESP32-S3)

| DevKit pin | GPIO | Function | Conditioning on carrier |
|---|---|---|---|
| J1-1, J1-2 | 3V3 | 3V3 rail for SD, DHT, I²C, EXP | from DevKit LDO |
| J1-4…7 | 4, 5, 6, 7 | MQ1…MQ4 (ADC1 CH3–CH6) | AO/DO jumper → 10 k / 15 k divider (5 V → 3.0 V) + 100 nF |
| J1-8…11 | 15, 16, 17, 18 | DHT1…DHT4 | 10 k pull-up to 3V3 (DNP if module has one), 470 Ω series |
| J1-12 | 8 | V5 sense (ADC1 CH7) | 10 k / 10 k + 100 nF |
| J1-13 | 3 | EXP-B | strapping pin, see §3 |
| J1-14 | 46 | buzzer driver | AO3400A gate, 100 k pull-down (keeps strap LOW) |
| J1-15 | 9 | EXP-A (ADC1 CH8) | — |
| J1-16…19 | 10, 11, 12, 13 | SD CS, MOSI/CMD, SCK/CLK, MISO/D0 | native FSPI pins = Arduino S3 defaults; 10 k pull-ups (SD-mode ready) |
| J1-20 | 14 | LED data | → 74HCT541 → 330 Ω → J10 |
| J1-21 | 5V | DevKit supply | ← D2 SS34 + 470 µF |
| J3-4 | 1 | VBAT sense (ADC1 CH0) | 100 k / 22 k (17 V → 3.0 V) + 100 nF |
| J3-5 | 2 | VDC sense (ADC1 CH1) | 100 k / 22 k + 100 nF (also tells firmware which source is live) |
| J3-6 | 42 | emergency button | 1 k series, 10 k pull-down, 100 nF |
| J3-7…10 | 41, 40, 39, 38 | SERVO4…SERVO1 | → 74HCT541 → 220 Ω |
| J3-17 | 47 | I²C SDA | 4.7 k pull-up |
| J3-18 | 21 | I²C SCL | 4.7 k pull-up |
| J1-3, J3-2/3, J3-11…16, J3-19/20 | RST, 43/44, 37/36/35, 0, 45, 48, 20/19 | **not connected** | see §3 |

74HCT541 inputs have 10 k pull-downs → no LED flash and no servo twitch while the S3 boots. OE is tied to GND; spare inputs go to GND.

---

## 5 · Connector map (every off-board wire)

**Rules**
1. **XT gender = LiPo/ESC convention:** the live (source) side carries the **female** sockets, the load side the **male** pins. The battery only fits J1, the buck input only fits J3, the buck output only fits J5, LED harnesses only fit J6–J8.
2. **One family per job:** XT60 = 12 V power, XT30 = 5 V power, VH 3.96 mm = front panel, XH 2.5 mm = sensors/signals, SH 1.0 mm = I²C (Qwiic standard). Families don't cross-mate.
3. **XH pin order everywhere: pin 1 = signal(s), next-to-last = supply, last = GND.** Any XH plug in any same-size XH port therefore never sees reversed polarity, only 3.3 V vs 5 V, which every module here tolerates. The hub end follows the rule; the module end is wired to the module's printed labels.
4. Front panel (DC jack + power switch) = **one VH-6 plug**. A loose DC plug can never land on a battery pin.

| Ref | Port | Board-side connector | Pins (1 → n) | Notes |
|---|---|---|---|---|
| J1 | Battery in | XT60PW-M (male, right-angle) | +, − | F1 directly behind it |
| J2 | PANEL (DC jack + switch) | JST VH 6-pin | DC_JACK+, DC_JACK−, DC→SW-B, SW-B→DC, BAT→SW-A, SW-A→BAT | 18 AWG; panel jack 5.5 × 2.1 mm metal, rocker = 2-pole ON-OFF ≥ 10 A |
| J3 | 12 V → buck | XT60PW-F (female) | +, − | buck input lead gets XT60 male |
| J4 | 12 V AUX *(optional)* | XT60PW-F | +, − | footprint only — future 12 V load or a second buck |
| J5 | 5 V ← buck | XT30PW-M (male) | +, − | buck output lead gets XT30 female |
| J6 | LED power A | XT30PW-F | +, − | feeds LED board 1 |
| J7 | LED power B | XT30PW-F | +, − | injection at board 7 (far end) |
| J8 | 5 V AUX | XT30PW-F | +, − | spare 5 V |
| J10 | LED data | XH-2 | DIN, GND | 5 V buffered |
| J11–J14 | DHT1–DHT4 | XH-3 | DATA, 3V3, GND | |
| J15–J18 | MQ1–MQ4 | XH-4 | AO, DO, 5V, GND | solder jumper picks AO (default) or DO into the ADC |
| J19–J22 | SERVO1–SERVO4 | XH-3 | SIG, 5V, GND | same order as a JR/Futaba servo plug |
| J23 | Emergency button | XH-3 | SIG, 3V3, GND | module (HIGH = pressed) or bare NO switch SIG↔3V3 |
| J24 | External buzzer | XH-2 | 5V, BUZ− | parallel to onboard BZ1 |
| J25 | EXP | XH-4 | GPIO9, GPIO3, 3V3, GND | 2 spare GPIO (GPIO9 is ADC-capable) |
| J26 | I²C | JST SH-4 (Qwiic) | GND, 3V3, SDA, SCL | plug-in OLED / SHT4x / BME280, no soldering |

Wire gauges: battery/buck 12 V 16 AWG · 5 V buck→board 16 AWG · LED power 18 AWG · panel VH 18 AWG · XH signals 22–24 AWG (XH max 22 AWG).

---

## 6 · On-board parts (besides connectors)

| Ref | Part | Why |
|---|---|---|
| U1 | 2 × 1 × 22 female headers 2.54 mm (+ optional 22.86 mm row) | socketed DevKit |
| Q1, Q2 | P-MOSFET −30 V SO-8 (AO4409 / AO4407A class) | battery path, back-to-back |
| U2, U3 | LM393 dual comparator + TL431 reference | DC-detect and UVLO with hysteresis |
| Q3, Q4 | 2N7002 | FET gate logic (AND of "battery OK" and "no DC") |
| D1 | Schottky ≥ 5 A (SS54; 10 A class if sustained 12 V current > 2 A) | DC path, blocks back-feed into the jack |
| D2 | SS34 + 470 µF | ESP supply isolation / brown-out ride-through |
| TVS1, TVS2 | SMBJ18A, SMBJ5.0A | 12 V spike clamp; 5 V crowbar |
| F1, F3 | ATO blade holders (7.5 A battery, 10 A 5 V in) | replaceable car fuses, sold everywhere |
| F2, F4–F7 | PTC resettable fuses | DC input; LED, servo, MQ, AUX branches |
| U4 | 74HCT541 octal buffer | 3.3 → 5 V push-pull: LED data + 4 servo signals (3 spare channels) |
| J27 | microSD push-push socket | no loose SD module/jumper wires; SPI now, SD_MMC 1-bit later on the same pins |
| BZ1, Q5 | passive buzzer + AO3400A + flyback diode | siren works out of the box (`tone()`) |
| — | 5 status LEDs: 12V, DC, BAT, LOW-BAT, 5V | see the power state without a meter |
| — | test points: V12, VBAT, VDC, 5V, 3V3, GND × 2 | bring-up and debugging |
| H1–H4 | M3 holes (3.2 mm), corners | standard standoffs |
| — | 2 cable-tie slots next to J1/J3/J5 | strain relief for the heavy leads |

---

## 7 · Board layout plan (indicative)

```
 ←──────────────────────────────── ~100 mm ────────────────────────────────→
┌H1───────────────┬─── antenna overhang ───┬──────────────────────┬───────H2┐
│ SENSOR I/O      │ (no copper, no metal   │ J19–J22 SERVO1–4     │ 12 V    │
│ J11–J14 DHT1–4  │  within 15 mm)         │ J23 E-STOP           │ ZONE    │
│ J15–J18 MQ1–4   ├────────────────────────┤ J26 I²C (Qwiic)      │ J1 BAT  │
│ J25 EXP         │                        │ U4 74HCT541          │ F1      │
│ J27 microSD     │   U1 YD-ESP32-S3       │ J10 LED DATA         │ J2 PANEL│
│ MQ dividers,    │   on 2 × 22 female     │ BZ1 + J24 BUZZER     │ Q1 Q2   │
│ DHT pull-ups    │   headers              │ VBAT / VDC dividers  │ U2 U3 D1│
│                 │   USB-C at this end ▼  │                      │ J3 BUCK │
│                 ├──── USB access lane ───┤                      │ J4 AUX  │
├─────────────────┴────────────────────────┴──────────────────────┴─────────┤
│ 5 V ZONE:  J5 XT30 IN · F3 · TVS2 · 1000 µF · F4–F7 · J6 LED-A · J7 LED-B · J8 AUX │
└H3────────────────────────────────────────────────────────────────────H4┘
```

- 2 layers, 1.6 mm FR-4, 1 oz. **Target 100 × 100 mm** (JLC cheapest tier); grow to ≈ 120 × 100 mm if routing is cramped.
- Bottom layer = solid GND; top layer = signals + power pours.
- DevKit antenna end flush with the top edge (module overhangs); copper keep-out on both layers under it; XT60s, electrolytics and the buzzer ≥ 15 mm away. The hub's Wi-Fi AP serves every visitor phone, so this matters.
- Keep a lane in front of the USB-C ports clear of tall parts (in-place flashing / serial monitor).
- Parts under the DevKit ≤ 6 mm tall (female headers give ≈ 8.5 mm).
- Every connector silkscreened with its name, pin functions and the station it serves.

---

## 8 · Parts shortlist — LCSC library check (2026-10-11, read-only via the EasyEDA bridge)

| Part | LCSC # | JLC class | Note |
|---|---|---|---|
| XT60PW-M / XT60PW-F | C98732 / C428722 | Extended | THT, hand-solder |
| XT30PW-M / XT30PW-F | C53373776 / C2913282 (pick variant in step 2) | Extended | THT, hand-solder |
| JST VH B2P-VH / B4P-VH (6-pin TBD) | C485345 / C485344 | Extended | THT, hand-solder |
| JST XH 3-pin (M2501V-03P) | C492457 | Extended | THT, hand-solder (2/4-pin same series) |
| JST SH-4 SM04B-SRSS-TB | C5306065 | — | SMD |
| ATO blade fuse holder Keystone 3557-2 | C352820 | Extended | THT, hand-solder |
| P-MOSFET AO4409 / AO4407A | C21480 / C3019374 | Extended | SO-8 |
| LM393DR2G | C7955 | **Basic** | |
| TL431AIDBZR | C23892 | Extended | |
| 2N7002 | C8545 | **Basic** | |
| AO3400A | C20917 | **Basic** | |
| SS54 (5 A Schottky) | C22452 | **Basic** | |
| SMBJ18A / SMBJ5.0A | C127556 / C127544 | Extended | |
| 74HCT541D | C547780 | Extended | SO-20 |
| microSD TF-015 | C410351 | Extended | |
| MLT-8530 passive buzzer | C94599 | Extended | or a THT buzzer, hand-soldered |
| PTCs (1812 family) | values TBD in step 2 | Extended | |

The search API returned no stock figures → **re-check stock at order time.** Cost drivers: ≈ $3 per unique Extended SMD part (≈ 8–10 here) + assembly setup. Assemble only 2 of the 5 boards to save part cost. Get the JLC quote before ordering.

---

## 9 · Bring-up sequence (with the board on the bench, no DevKit yet)

1. Bench supply 12 V, 0.5 A limit, into J1. Switch OFF → 0 mA. Switch ON → 12V + BAT LEDs, V12 present.
2. Lower the supply → LOW-BAT at ≈ 9.6 V; raise → back on at ≈ 10.5 V.
3. Add DC on the panel jack → DC LED on, BAT LED off. **Ammeter in series with the battery supply reads 0 in both directions.**
4. Plug/unplug DC under a dummy load → V12 never drops below ≈ 10 V (meter min/max or scope).
5. Buck alone first: verify 5.0–5.2 V and polarity **before** plugging into J5. Then 5V LED, every PTC branch.
6. Measure the DevKit row spacing again against the board, insert it, flash over USB, check 3V3.
7. Port by port: LED walk 0–55, each DHT, each MQ, each servo, E-STOP latch/hold-clear, buzzer, SD, I²C.
8. Full load 10 min (LEDs + servos + Wi-Fi clients): zero brown-out resets; touch-check FETs, D1, PTCs.

---

## 10 · Open items — answer before the schematic

1. **Battery:** exact type (3S LiPo? 18650 pack with BMS?), capacity, connector.
2. **DC adapter:** voltage and current rating.
3. **Buck module:** which one (photo/link)? Fixed 5 V, ≥ 10 A recommended.
4. **Servos:** still 4 × SG90? Independent control per servo is planned (one GPIO each).
5. **Calipers:** DevKit pin-row spacing, centre to centre (expect 25.4 mm).
6. **Panel:** OK to panel-mount the DC jack + a 2-pole rocker on one VH-6 harness? (Alternative: PCB-edge jack.)
7. **microSD:** keep it? With 16 MB flash, LittleFS can hold the gallery on its own.

---

## 11 · Firmware migration notes (ESP32-S3)

- `platformio.ini`: `board = esp32-s3-devkitc-1`, `board_build.arduino.memory_type = qio_opi`, 16 MB flash (`board_build.flash_size`/`board_upload.flash_size = 16MB`) + a 16 MB partition table, `-DBOARD_HAS_PSRAM`; set the USB-CDC-on-boot flag to match the port used for the serial monitor.
- `config.h`: new pins from §4 (LED 14, buzzer 46, button 42, SD 10/11/12/13, sensors DHT 15–18 + MQ 4–7).
- `HubSensors`: drop the GPIO34–39 input-only logic; roles become 4 × DHT11 + 4 × MQ (analog on ADC1, threshold calibrated in software).
- New: power telemetry (VBAT, VDC, V5) in `/api/state`; dashboard shows source + battery %, firmware chirps at 10.2 V (3S), well before UVLO.
- `SPI.begin(12, 13, 11, 10)` for SD (or SD_MMC 1-bit later); `Wire.begin(47, 21)`.
- Servos: LEDC 50 Hz on GPIO38–41 (ESP32Servo supports the S3).
- ESP-NOW with the classic-ESP32 robot is unchanged — both must stay on channel 1.

---

## 12 · Sources

- Espressif ESP32-S3-DevKitC-1 user guide v1.1 (pin headers, octal-PSRAM note, RGB GPIO): https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-devkitc-1/user_guide_v1.1.html
- Espressif DevKitC-1 PCB drawing (25.40 mm board width, 1.27 mm pin inset → 22.86 mm rows): https://dl.espressif.com/dl/PCB_ESP32-S3-DevKitC-1_V1_20210312CB.pdf
- KiCad forum, footprint 22.86 mm vs physical boards 25.4 mm: https://forum.kicad.info/t/pin-distance-in-esp32-s3-devkitc-1/71001
- YD-ESP32-S3 core board schematic V1.4 (5Vin diode, IN-OUT/USB-OTG/RGB jumpers, CJ6107 LDO): https://mischianti.org/wp-content/uploads/2023/08/YD-ESP32-S3-SCH-V1.4.pdf
- ESP-IDF `soc_caps.h` for ESP32-S3 ("No GPIO is input only"): https://github.com/espressif/esp-idf/blob/master/components/soc/esp32s3/include/soc/soc_caps.h
- CJ6107A33GW (1 A LDO) on JLCPCB: https://jlcpcb.com/partdetail/2979431-CJ6107A33GW/C2829350
- Hub firmware pin use: `firmware/hub/include/config.h`, `src/main.cpp`, `src/HubSensors.cpp`, `src/PhotoStore.cpp`; earlier load/fuse analysis: `plans/hub-sensor-wiring.md`
