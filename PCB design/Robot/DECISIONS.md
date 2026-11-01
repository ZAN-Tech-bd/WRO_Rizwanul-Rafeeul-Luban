# Robot Carrier PCB — Decision Log

> One entry per decision: **context → decision → why → rejected alternatives → source**. Newest last. A decision changes
> only with new facts; write a new entry that supersedes the old one (don't edit history).
> Spec = [`WIRING-PLAN.md`](WIRING-PLAN.md) · handoff = [`README.md`](README.md). D-001…D-012 back-fill the v0.1–v0.3 calls
> (2026-10-12/13) so the log is complete from the start.

### D-001 · MCU = YD-ESP32-S3 N16R8, socketed, the only controller (2026-10-12, user)
The rail build used an ESP32-WROOM DevKit (brain) + an Arduino Nano (L298N driver) on a 9600-baud link.
**Decision:** one YD-ESP32-S3 N16R8 in 2 × 1×22 female sockets, rows 25.40 mm (drawing `YD-ESP32-S3 N16R8_footprint.png`).
**Why:** same DevKit as the hub (one spare fits both), 26+ free GPIOs, no inter-MCU link, no Nano reflash tax.
**Rejected:** v0.1 dual sockets (WROOM + Nano) — two controllers and a UART protocol for 6 pins.

### D-002 · The S3 drives the L298N; boot-safe ENA/ENB pull-downs (2026-10-12)
**Decision:** ENA GPIO4, IN1–IN4 GPIO5/6/7/15, ENB GPIO16 on J6 (XH-8); LEDC 1 kHz; 10 k pull-downs on ENA/ENB.
**Why:** the pull-downs keep the bridge disabled while the S3 resets, flashes or crashes (the Nano's old job). ESP32-S3
datasheet v2.2 Table 2-2: GPIO4–7 only glitch LOW at power-up, GPIO15/16 don't glitch, none has a boot pull-up (Table 2-1).
A generic L298N ties ENA/ENB to +5 V only through its jumpers (teardown), so 10 k is enough once they are removed.

### D-003 · L298N module and both bucks stay off-board (2026-10-12, hub precedent)
Motor current runs buck → L298N screws directly; the carrier carries only logic and switched battery. A ≥ 5 A switcher
does not belong on a hand-routed 2-layer board.

### D-004 · Power entry: 3S LiPo on XT60, rocker on VH-2, no protection parts (2026-10-13, user; hub v0.3)
**Decision:** J1 XT60PW-M battery; rocker via J5 B2P-VH (one pole: COM + "I"); no fuses / MOSFETs / TVS.
**Why:** battery-side current ≤ ≈ 4 A (the bucks convert down), so one rocker pole and a VH pin are plenty; keyed
connectors prevent reversal; beginner build. **Rejected:** v0.2 VH-4 with both poles paralleled (based on output-side current).

### D-005 · Buck feeds on XT30 female (2026-10-15, user) — supersedes the XT60 feeds of v0.2/v0.3
**Decision:** J2/J3 = XT30PW-F (C2913282); 5 V returns on J4 XT30PW-M; battery stays XT60.
**Why:** each feed carries ≤ 2 A; XT30 saves ≈ 4 cm² and weight. Genders still block every wrong mate (buck input leads =
XT30 male, 5 V output lead = XT30 female, LiPo = XT60). The user owns both bucks and guarantees 5 V.
**Note:** library search hit C431092 "XT30PW-M30.G.Y" first — that is a male part; the female is C2913282.

### D-006 · Audio = two LM386 modules on a battery rail `VAMP` ≈ 11 V, JP1 fallback to 5 V (2026-10-13/15, user)
**Context:** the team owns two LC Technology LM386 modules (LM386M-1, SOIC) and two 3 W 4 Ω speakers; the legacy Cirkit diagram
fed the amps straight from the battery; the proven `esp32-rfid-audio-player` build ran an LM386 on 5 V.
**Decision:** BAT_SW → D1, D2 1N4007 → R1 1.1 Ω 1 W → `VAMP_BAT` → JP1 (1×3 header + cap) → `VAMP`, C9 1000 µF 35 V + C10;
JP1 pin 3 = +5V. Default cap on BAT. Firmware volume cap 24.
**Why:** the user asked for "a little less than 12 V at full charge": 2 diodes give ≈ 11.3 V idle at 12.6 V, inside the
LM386M-1's 4–12 V (TI SNAS545D §6.3); the diodes block reversal and hold `VAMP` up through motor dips; the RC filters motor
whine. **Trade-off shown to the user:** into 4 Ω the LM386 swing flattens above ~9 V (≈ 3.7 Vpp, Fig. 6-3), so ~11 V is only
≈ 1 dB louder than 7 V while dissipation reaches ≈ 1.2 W at full output (Fig. 6-7) vs the SOIC's 0.73 W (§6.1). The user kept
the battery rail; JP1 gives the proven 5 V supply without rework if the chips run hot.
**Rejected:** LM317 at ≈ 7 V (recommended: +2 dB vs 5 V, inside the heat rating); 5 V only (proven, ≈ 3 dB quieter);
PAM8403 class-D (louder, but the team keeps its modules); direct battery (12.6 V > 12 V rating).

### D-007 · LM386 modules re-headered pins-down, flat in 1×4 sockets (2026-10-15, user)
The module's header pins point up and it has no holes (photo `lm386_amplifier.jpg`, 40.9 × 13.9 mm). The user re-solders a
straight header from the top so the pins point down; U3/U4 = PM254V-11-04-H85, pin 1 VCC, 2 IN, 3 GND, 4 GND; the far end
rests on foam; outline on the silkscreen. **Rejected:** foam tape + Dupont jumper (loose wires); vertical right-angle socket
(module edge would hit the board).

### D-008 · DFPlayer wired like the proven build; BUSY added (2026-10-15)
**Context:** github.com/ZAN-Tech-bd/esp32-rfid-audio-player: DF TX straight into the ESP32 RX, ESP32 TX → 1 kΩ → DF RX,
`begin(serial, true, true)`, volume 24, DAC → LM386 on 5 V with a 4 Ω speaker (its diagram drove two speakers in parallel = 2 Ω).
**Decision:** GPIO14 → R7 1 k → DF RX; DF TX → R8 1 k → GPIO17; BUSY → R9 1 k → GPIO8; DAC_L → U3, DAC_R → U4 (one amp per
speaker). **Why:** the TX level is proven 3.3 V-safe on the team's module; v0.2's 10 k / 20 k divider put a 3.3 V TX at 2.2 V,
below the S3's VIH = 0.75 × VDD = 2.475 V (datasheet Table 5-4). BUSY gives a hardware "playback finished".
**Rejected:** divider + DNP pull-down footprints (v0.3) — not needed for this module.

### D-009 · RC522 on an XH-7 cable, RST wired, FSPI default pins (2026-10-13, user)
J7 = 3V3, GND, SS, SCK, MOSI, MISO, RST; SS/SCK/MOSI through 47 Ω; GPIO10/11/12/13 (Arduino S3 default SPI), RST GPIO9.
**Why:** read range is 2–3 cm, so the reader must sit near the floor cards; RST allows recovery from a hung reader (the proven
build also wires RST). **Rejected:** 1×8 socket on the board (v0.2).

### D-010 · Mega port XH-4 (power + optional data) with a 1N5819; I²C port protected with 1 kΩ (2026-10-15, user)
J9 = MEGA_5V (D3 1N5819 from +5V), GND, TX (GPIO41 → R11 1 k → Mega RX1), RX (Mega TX1 → R12 10 k / R13 20 k → GPIO42).
The user builds the Mega-end JST female and decides whether to use the data link. Because J9 and the I²C port J10 share the
XH-4 size, R16 / R17 1 kΩ sit between J10 and GPIO39 / GPIO40 (a Mega TX1 in J10 injects ≤ 1.4 mA; bus low = 0.58 V).
**Rejected:** XH-5 with a blank key pin (user prefers exactly 4 pins).

### D-011 · Expansion ports (2026-10-13, user)
Front obstacle J11 XH-3 (3V3, GND, SIG → GPIO2 ADC1); I²C J10 like the hub; MUTE (GPIO21) and spare (GPIO47) buttons = on-board
tact + XH-2 panel port each; battery sense 100 k / 20 k → GPIO1; EXP pads J14 (GPIO3, GPIO38). **Rejected:** a 5-sensor IR
line-array port (rail build is sensor-free; EXP + I²C cover future sensors).

### D-012 · No WS2812 / 74HCT125 on the robot (2026-10-12, user)
The hub owns all lighting.

### D-013 · 100 % through-hole, hand-soldered, bare-PCB order (2026-10-15, user)
Every footprint THT. `scripts/robot/07_parts_verify_v04.js` classifies each footprint name and fails on any SMD pattern:
2026-10-16 result 32 part types, 0 SMD, 0 missing (PASS). SMD results from searches (e.g. TS-1102S SMD tacts) are rejected.

### D-014 · Board ≤ 100 × 100 mm, target ≈ 90 × 85 mm, all parts on the top side (2026-10-15, user)
Top faces up on the robot. Low parts go under the DevKit, DFPlayer and LM386 modules to save area.

### D-015 · XT30PW-F polarity: pad 1 = +, pad 2 = − (2026-10-16, derived)
**Context:** both LCSC XT30PW-F parts (C2913282, C30170176) have pins named only 1–4.
**Derivation:** Amass XT30PW-F drawing — in the mating-face view (first-angle, PCB legs on the left) the chamfered end (= "−" by
the XT convention: contact at the straight side +, at the angled side −) is the bottom contact; mapped to the footprint (mating
face toward −y) the chamfered contact is +x = pad 2. Cross-checks: (1) the same method on XT60PW-F (library pins 3 = +, 4 = −)
agrees with its product photo; (2) XT30PW-M (library pins 1 = −, 2 = +) and XT30PW-F placed face to face mate + to + with this
mapping. **Safety net:** spec §8.3 (compare with the molded "+") and §9.1 (meter J2/J3 before plugging a buck).

### D-016 · Tact switches wired on diagonal pads 1 and 3 (2026-10-16)
K2-1102DP-J4SW-04 (C136680) symbol pins are only 1–4; the internal pairing isn't documented in the library. Diagonal pads are
always on opposite contacts whichever pairs are joined inside, so pad 1 = signal, pad 3 = GND, pads 2/4 No Connect.

### D-017 · Polarity map for the schematic (2026-10-16, from the library symbol / footprint files)
`09_symbol_fp_files.js` + `read_symbols.py` / `read_elibz.py`: 1N4007-T and 1N5819-T 1 = K, 2 = A; LED C9900102778 1 = +, 2 = −;
electrolytics C439782 / C2960416 / C109066 / C503219 have pins "1"/"2" only — the footprint silk "+" sits beside pad 1 and the
filled negative crescent beside pad 2 → 1 = +; XT60PW-M and XT30PW-M 1 = −, 2 = +.

### D-018 · Tact pairing confirmed from the symbol drawing (2026-10-16) — confirms D-016
The placed K2-1102DP symbol shows pins 1/3 on the left and 2/4 on the right, which looked like "1–3 = one contact". Its `.esym`
POLY records draw one bar joining pins **1–2** and one joining **3–4**, with the switch between the bars; the footprint
agrees (pads 1/2 straight across the 6.5 mm span). Pads 1 and 3 are on different contacts, so SW1/SW2 stay wired 1 = BTN,
3 = GND, 2/4 No Connect.

### D-019 · Board 95.0 × 97.5 mm and the floor plan (2026-10-16, user) — supersedes D-014's ≈ 90 × 85 target
**Context:** 75 through-hole parts, four raised modules (DevKit, DFPlayer, 2 × LM386 40.9 × 13.9 mm) and four XT connectors.
**Decision:** 3740 × 3840 mil with the `layout_plan.py` floor plan (spec §7): DevKit top-left with the antenna over the top
edge; power column on the right edge (J1 battery, J2, J3, J4, C1, top → bottom; J5 left of J1); DFPlayer 1 mm right of the
DevKit with both LM386 modules below it (screw terminals at the bottom edge) and the VAMP block on its right; 3.3 V ports
J11 / J10 at the top edge ≥ 15 mm from the antenna; buttons + Mega bottom-left, CAM + LEDs bottom-right; resistors under
the raised modules by height class (≤ 3 mm under the LM386 modules, ≤ 5.5 mm under the DevKit / DFPlayer).
**Why:** 95 × 90 left no room for the top-edge ports and the button cluster; 95 × 95 left C3 and both LEDs without a spot.
At 95 × 97.5 every check passes (0 overlaps / edge / hole / antenna / USB-lane / height / foam problems) and the 3 mm
`BAT_SW` corridor beside J2 / J3 stays free of pads. Still inside the 100 × 100 price tier.
**Rejected:** connectors in a second row behind the top-edge ports (cables over the antenna area); 100 × 100.

### D-020 · Upload and console over the DevKit's native USB port only (2026-10-16, user)
The 17 mm USB lane is in front of the left USB-C port ("USB", native USB/OTG on GPIO19/20; the right "COM" port sits next to
the CH343P in the YD drawing, and the user confirmed the left one is the USB/OTG port). C3, J9 and SW2 block the COM port
once the DevKit is seated. **Why:** the ESP32-S3 flashes through its built-in USB-Serial/JTAG (the upload tool resets it
into download mode; BOOT + RST by hand as a fallback), the firmware's console is already on that port
(`ARDUINO_USB_CDC_ON_BOOT=1`), and OTA works as on the hub. Clearing both lanes would have cost the bottom-left region.

### D-021 · No zip-tie holes; DFPlayer card swapped by lifting the module (2026-10-16, user)
Every port header sits at a board edge, so there was no room for a tie pair between a header and the edge; the user
dropped the anchors (spec §7 had listed them). The DFPlayer's micro-SD slot opens sideways toward a neighbouring part;
the card is changed with the module out of its socket.

### D-022 · Mega fed straight from +5V; D3 (1N5819) removed (2026-10-25, user) — supersedes D-010's diode
**Context:** part-count review before routing. D3 kept the Mega's USB 5 V and the robot's +5V apart so the Mega could be
flashed with J9 plugged in. The user flashes the Mega rarely and accepts unplugging J9 instead.
**Decision:** J9.1 = `+5V` directly; the `MEGA_5V` net is gone. Rule (spec §2.2.4, §9.7): **unplug J9 before any USB cable
touches the Mega** (flashing or serial monitor), and never power the Mega from its barrel jack while J9 is plugged.
**Why:** the Mega and its 3.2″ TFT get the full 5.1–5.2 V instead of ≈ 4.7 V (the shield's 3.3 V LDO was tight); one part
fewer; D3's spot in the USB lane is freed.
**What the rule protects against:** the Mega 2560 R3 joins USB VBUS to its 5V pin through a 500 mA PTC (MF-MSMF050) and a
P-FET (FDN340P) that an LMV358 turns ON whenever VIN < ≈ 6.6 V. With J9 plugged and the robot on, the buck pushes
≈ 0.1–0.3 A back into the laptop port (out of USB spec); with the robot off, the laptop tries to run the whole +5V rail
until the PTC trips. Damage risk is low (the PTC limits it); the symptom is flaky flashing. A Mega powered from its own
barrel jack would back-feed the robot's +5V through its NCP1117.
**Rejected:** keeping D3 for "flash while plugged" (the user won't flash repeatedly).

### D-023 · R1 (1.1 Ω 1 W) removed from the VAMP stage (2026-10-25, user) — supersedes part of D-006
**Decision:** D2's cathode feeds `VAMP_BAT` (JP1.1) directly; the `AMP_R` net is gone. C9 1000 µF stays.
**Why it isn't must-have:** the two 1N4007s' own dynamic resistance (≈ 2·nV_T/I: ≈ 3.5 Ω at 30 mA idle, ≈ 0.4 Ω at
0.25 A loud) already forms the low-pass with C9. 1 kHz motor-ripple attenuation: idle ≈ −29 dB with R1 → ≈ −27 dB without;
loud passages ≈ −19 dB → ≈ −9 dB. Switch-on inrush into C9 without R1: ≈ 40 A for ≈ 0.25 ms, I²t ≈ 0.2 A²s against the
1N4007's 30 A / 8.3 ms (≈ 3.7 A²s) — 15× margin; the rocker already took a similar inrush from the bucks' input caps.
VAMP idle is unchanged (≈ 11.3 V at 12.6 V); loud passages run ≈ 0.3 V higher, still ≤ 12 V.
**Lost:** ≈ 10 dB of filtering in loud passages, and R1's fusible behaviour if an LM386 module were plugged in reversed
(the silk outline and the single orientation remain the guard).
**Fallback if whine is audible (bring-up §9.8):** JP1 → 5V, or 2200 µF 25 V in C9's 13 mm footprint (≈ −34 / −16 dB).

### D-024 · No capacitor at J4: C1 (1000 µF) and C2 (100 nF) removed (2026-10-25, user)
**Decision:** +5V enters at J4 with no local capacitor (hub v0.3 §3.6 precedent: "the buck has its own").
**Why:** the loads keep their own bulk and HF capacitors — C3 470 µF + C4 at the DFPlayer, C5 470 µF + C6 at the CAM port,
C8 at the RC522, C10 on VAMP, the DevKit's 10 µF in/out — so C1 was a duplicate. The buck's capacitors sit behind ≈ 0.3 µH
of lead: with ≈ 950 µF of load bulk that is f₀ ≈ 9 kHz, Z₀ ≈ 0.02 Ω, over-damped by the electrolytics' own ESR
(0.05–0.3 Ω), so nothing rings. C2 decoupled nothing the loads need: 30–80 mm of +5V spine (≈ 30–80 nH) separates J4 from
them, and each has its own 100 nF.
**Fallback:** a 470–1000 µF on the 5 V buck's output lead (off-board).

### D-025 · Two FC-51 obstacle sensors, each on its own XH-3 port, wired-OR on the PCB (2026-10-25, user)
**Context:** two front FC-51 modules; one shared "obstacle" event (pause the rail) is enough.
**Decision:** new port **J15** (JST XH B3B-XH-A, C144394 — same part as J11): pin 1 +3V3, 2 GND, 3 `OBST`. J15.3 joins J11.3
on the PCB, so both feed the existing R18 10 k pull-up → R19 1 k → GPIO2 (+ C12). Placed at (3225, H − 105, rot 0) in
the free top-right corner between J10 and H4, above J1 (`layout_plan.py` check: 0 problems).
**Why it is safe:** the FC-51 output is an LM393 open collector with its own 10 k pull-up, so two outputs can never fight;
either pulling LOW = obstacle. Pull-up = 10 k ∥ 10 k ∥ 10 k = 3.3 k; the detecting LM393 sinks ≈ 4 mA (pull-ups + both
modules' indicator LEDs, which light together) → VOL ≤ 0.4 V (TI LM393, 4 mA) < VIL 0.825 V. +3V3 load ≈ +35 mA.
**Firmware:** no pin change (GPIO2, `INPUT`, active LOW, debounced).
**Rejected:** a Y-harness into J11 (the user wants each sensor plugged into the PCB); a second, independent input on J14
GPIO38 (unkeyed pin header, no series/filter, and "which side" isn't needed).

### D-026 · Test points TP1–TP6 deleted (2026-10-25, user)
**Decision:** the six 1-pin pads (pads only, never in the BOM) and their pins leave the schematic and PCB; designators
retired. This overrides the house "test pad on every rail" convention by the user's call.
**Why:** six fewer routing targets (TP1 sat in the 3 mm `BAT_SW` corridor, TP4 / TP6 in the USB lane) and no pads-only
exceptions. Every rail already has an exposed probe point: `BAT_SW` = D1 anode lead or the J2 / J3 "+" contact;
`VAMP_BAT` / `VAMP` / `+5V` = JP1 pins 1 / 2 / 3; `+3V3` / `GND` = J14 pads 1 / 2 (spec §3.9).
**Rejected:** keeping them as free DNP pads (they still need routing and space).

### D-027 · 3V3 stays on the DevKit's LDO; battery divider re-verified (2026-10-25)
**Context:** the user asked whether the DevKit's regulator can carry each board's 3V3 loads.
**Facts:** the YD-ESP32-S3 CoreBoard V1.4 schematic shows U3 = CJ6107A33GW (SOT-223, 1 A, 0.5 V dropout at 1 A, 6 V max
input; JLC C2829350), 10 µF in and out, fed from the board's 5 V net; 5Vin reaches that net through D3 (IN-OUT jumper
across it). Clones vary: read the marking (an AMS1117 needs ≥ 4.8 V at 5Vin for its 1.1–1.3 V dropout).
**Robot budget:** S3 + Wi-Fi AP ≈ 0.13 A, RC522 field ≈ 60 mA (100 mA max), 2 × FC-51 ≈ 35 mA, I²C ≤ 20 mA → ≈ 0.25 A
average, ≈ 0.55 A peak. ≈ 1.4 V across the LDO → ≈ 0.35 W → ≈ +32 °C (θJA ≈ 90 °C/W); Tj ≈ 72 °C in a 40 °C body.
**Decision:** no extra regulator (two regulators must never share the DevKit's 3V3 pins; a separate THT rail would add an
active part with no measured need). Bring-up §9.8: the LDO must still be touchable after a 10-min tour.
**Battery divider (ROBOT-6, verified, unchanged):** R24 100 k / R25 20 k = ÷6.00 → 12.6 / 10.8 / 10.5 V = 2.10 / 1.80 /
1.75 V at GPIO1 (ADC1, 12 dB, ≈ 0–3.1 V); 16.7 kΩ source with C11 100 nF (τ 1.7 ms); 0.1 mA only while switched on.
Spec §1, §1.4, §5, §6 and §10 agree; no firmware reads it yet. §10 gained an averaging / hysteresis note.

### D-028 · v0.5 PCB: silkscreen around J15, R14 / R15 nudged, outline recreated as a polyline (2026-10-25)
**Context:** J15 took the top-right corner where the I²C legend ("I2C 3.3V ONLY", "SDA SCL GND 3V3") sat. Separately,
EasyEDA had auto-updated to 3.2.206: `pcb_PrimitiveLine` now creates copper only, and the v0.4 board outline (four
layer-11 lines) and the 14 silkscreen lines were gone after the update (0 lines on the board; texts and parts intact).
**Decision:**
- J10's pin names go under its pads (as J11's), which needed R14 / R15 55 mil lower (`layout_plan.py` 0 problems);
  "I2C 3.3V ONLY" stands upright in the gap between J11 and J10, "OBST 2 3.3V" in the gap between J10 and J15, and J11's
  name becomes "OBST 1 3.3V ONLY".
- "UNPLUG J9 FOR MEGA USB" (D-022) stands upright in D3's old spot; J1's designator sits between its + / − marks so it
  doesn't read as "SIG J1" after J15's pin names; the DFPlayer outline is closed on all four sides now that R1 is gone.
- The outline is one closed layer-11 `pcb_PrimitivePolyline` (3740 × 3840), the silk lines are 2-point layer-3
  polylines; `21`, `22`, `24` and `silk_plan.py` (and the pcb-pipeline templates) read and write polylines.
**Checks:** `22_check_placement` 0 problems (board 3740 × 3840, 66 parts, all polarity nets); `silk_plan.py` 0 problems
(81 labels, 15 lines, 66 designators); `24_check_silk` 0 hits; `27_drc` only Connection Error (189 / 51 nets).

### D-029 · Motor rail: L298N fed straight from the battery; motor buck removed (2026-10-25, user)
**Context:** v0.1–v0.5 fed the L298N from an adjustable motor buck ("set to the motor rating", 6.0 V for TT motors) fed
on J2. The user reversed this: the motors get whatever the battery supplies.
**Decision:** J2 (XT30PW-F, unchanged net `BAT_SW`) now feeds the L298N 12V/GND screws **directly** through one XT30 male
lead (twisted 16–18 AWG). The adjustable motor buck is gone; the only off-board buck left is the fixed 5 V one (J3/J4).
**Why:** user call — the motors are rated for the pack; one converter fewer to own, adjust and fail.
**Consequences:**
- Motor terminal voltage = pack (9.6–12.6 V) − L298N Darlington drop (≈ 1.8–2.7 V) − lead drop ≈ **7–10.5 V**. No knob
  sets the motor voltage any more; firmware PWM caps are the only speed limit (spec §10 recalibration note).
- Stall current now rides the battery feed directly: budget §1.3 worst case ≈ 5 A (XT30 15 A, VH pin 10 A, rocker pole
  ≥ 6 A DC; the ≥ 3 mm `BAT_SW` corridor to J2 already exists).
- Motor stall dips land on `BAT_SW`: `VAMP` is already isolated by the D1/D2 hold-up valve + C9 (§1.1 unchanged); JP1 → 5 V
  stays the whine fallback.
- Nets, BOM, placement: **unchanged** (J2 already carried `BAT_SW`/GND). Silkscreen: J2's label → "L298N RAW BAT
  (OUT=STOP)", board title → v0.6 — regenerate with `silk_plan.py` → `23`/`25`/`24` before ordering.
- J2/J3 stay cross-mateable (both female `BAT_SW` sockets for XT30 male leads) — electrically identical, harmless
  (same situation as the two buck feeds in v0.4/v0.5).
**Rejected:** keeping the adjustable buck "set to the motor rating" (the v0.5 state) — the user wants raw battery.

### D-030 · Vertical XT connectors on J1–J4 (2026-10-25, user) — supersedes the right-angle parts of D-004 / D-005 / D-015
**Context:** v0.1–v0.5 used right-angle board connectors (XT60PW-M, XT30PW-F, XT30PW-M) whose mating faces stuck out of
the right edge. The team only has the normal vertical (top-entry) XT60 / XT30 PCB connectors.
**Decision:** J1 = XT60UPB-M (C2918078), J2 / J3 = XT30UPB-F (C108769), J4 = XT30UPB-M (C428721), all Amass, THT, found
with `08c_parts_find_xt_vertical.js`. Genders unchanged (D-005: battery lead XT60 female → J1; buck / motor leads XT30
male → J2 / J3; 5 V buck output XT30 female → J4), so no wrong power mate is possible. J2's silkscreen reads
"MOTOR POWER" (D-029, user wording).
**Polarity (from the library files, `09_symbol_fp_files.js` batch 4):**
- XT30UPB-F: pins named 1 = +, 2 = − → J2 / J3 pad 1 = `BAT_SW`, pad 2 = GND (as before).
- XT30UPB-M: pins named 1 = −, 2 = + → J4 pad 1 = GND, pad 2 = +5V (as before).
- XT60UPB-M: pins named only 1 / 2. Its footprint draws the pad-1 end rounded (the chamfered end) and the pad-2 end
  square; XT rule: chamfered end = −. Cross-check: the named XT30UPB-M draws its chamfered corners at its pad 1 (= −).
  → J1 pad 1 = GND, pad 2 = BAT+ (as before). Spec §8.3 + §9.1: compare with the molded marks and meter before plugging.
**Consequences:** 2-pin parts: the mounting-tab No-Connects (J1–J4 pins 3 / 4) are gone; nets per pin unchanged
(51 nets). The cables now plug in from above, so the power column no longer needs its faces flush with the right edge
(re-placed with `layout_plan.py`).
**Rejected:** keeping the right-angle footprints (the parts the team owns wouldn't fit).

### D-031 · XT drill holes: XT60 4.0 mm, XT30 2.5 mm (2026-10-26, user)
**Context:** the library footprints drill XT60UPB-M 4.6 mm, XT30UPB-F 2.2 mm and XT30UPB-M 1.8 mm. The swap was held
until the user gave the sizes for the connectors the team owns. Measured legs (user, 2026-10-26): XT60 male 3.7 mm,
female 3.8 mm; XT30 male 2.2 mm, female 2.1 mm. Holes chosen with margin: 4.0 mm (+0.2–0.3) and 2.5 mm (+0.3–0.4).
**Decision:** J1 hole 4.0 mm, pad 5.5 mm kept (0.75 mm ring); J2–J4 hole 2.5 mm, pad 3.2 / 2.5 → 3.5 mm (0.5 mm ring for
the high-current pins, 1.5 mm copper gap at the 5.0 mm pitch). Same sizes on the hub (hub spec v0.4).
**How:** `scripts/robot/44_xt_pads.js` edits the component pads (hole set in mil: getter/setter unit bug, playbook §7).
Import Changes resets them to the library sizes, so re-run it after every import and confirm in the drill file at P6.
**Placement:** the user places J1–J4 by hand (2026-10-26); after Import Changes they stay off-board until then.
**Rejected:** a 4.6 mm XT60 hole (the library's) — user's size wins; a 3.2 mm XT30 pad (0.35 mm ring on a 2.5 mm hole).

### D-032 · Routing v0.6 as built (2026-10-27, user: "route the robot PCB, ground pour, vias, place everything")
**Placement:** only J1–J4 were misplaced (off-board since the D-030 import). The v0.5 column spots fit the smaller
vertical footprints with 0 problems (`layout_plan.py` on the 2026-10-27 inventory), so J1 (3217, 3245, rot 180: + left,
− right), J2 (3278.5, 2604, 90), J3 (3278.5, 2002, 90), J4 (3414, 1404, 90) went back there via `21_place_pcb`
(4 placed, 62 already OK); `22_check_placement` 0 problems. Silk v0.6 regenerated (J1 +/− under the body, J2 "MOTOR
POWER", title v0.6): `silk_plan.py` 0 problems, `24_check_silk` 0 hits.
**Routing:** pcb-pipeline router (templates copied into `scripts/robot/`, robot config in `router.py`):
- classes: `PWR_HI` 3.0 mm (BAT+, BAT_SW; spec ≥ 3 mm), `PWR_5V` 2.0 mm (+5V spine), `PWR_AMP` 1.2 mm (AMP_D, VAMP_BAT,
  VAMP), `PWR_3V3` 0.8 mm, `SIG` 0.35 mm; necks only where nothing wider fits (0.3 mm between header pins on two low-current
  branches: +5V to the L298N logic pin past J14, +3V3 past U1B / J10);
- AUX branches at signal width: BAT_SW → R24 / R26, +5V → R27, +3V3 → pull-ups and J14; caps and J6.1 at 0.8 mm;
- power on the top layer (bottom only to cross); power pads inside the antenna strip may only escape it (`ZONE_ESCAPE`
  140 mil around their own pad; spec §7 "no power tracks under the antenna end");
- PathFinder converged in 7 passes (~4.5 min), 0 conflicts, 0 hard fails → `trim` 0 → `gnd_stage` stitching at a
  7.6 mm grid (the template's 3.8 mm gave 526 vias: needless on a board whose 44 THT GND pads already join the layers)
  → 153 stitching vias; U1B.1 (DevKit GND in the fill-free antenna strip) joined to the nearest stitching via by a
  0.4 mm stub.
**Result:** 394 segments, 175 vias (22 signal / power, 153 GND), 4.3 m of 57 m of track on the bottom (bottom = GND
plane). `route_check` and `check_eda_copper` on the written board: 0 clearance / edge / hole / antenna / connectivity
issues. GND fills (`pour_fill.py`, thermal reliefs; XT GND pads J1–J4 with 1.0 mm spokes for the battery / motor
return): 1922 hole-free pieces, every GND pad on the main network; read back 1922 / 1922 identical, 0 overlaps, min fill
gap 9.8 mil. Strict DRC after D-033: **passed, 0 errors**. Backups `WRO_Robot_v0.6_pre_route.eprj2`,
`WRO_Robot_v0.6_routed.eprj2`, saved-state export `WRO_Robot_v0.6_routed_export.epro2` (175 VIA records).
**Rejected:** hand-planned routes (station method) — 66 parts / 189 pins is grid-router territory.

### D-033 · DRC maximum track width raised to 5.08 mm (2026-10-27)
**Context:** the project's rule set ("JLCPCB Capability (Two Layers Board)", Physics → Track copperThickness1oz) caps
tracks at 2.54 mm and reported the 3.0 mm battery tracks as 9 "Physical Error"s.
**Decision:** raise the cap to 5.08 mm (`28_drc_track_max.js`, `pcb_Drc.overwriteCurrentRuleConfiguration`, read back).
JLC has no maximum track width; the spec asks for ≥ 3 mm on BAT_SW. Minimum width (0.127 mm) and all spacing rules unchanged.
**Rejected:** 2.5 mm battery tracks (below the spec's 3 mm).

### D-034 · XT drill holes don't persist through component-pad edits (2026-10-27) — open, user GUI step
**Finding:** after `44_xt_pads.js` + save, the saved project (`getProjectFile` export) still holds the library drills:
XT60 4.6 mm, XT30UPB-F 2.2 mm, XT30UPB-M 1.8 mm — the hub's H-001 lesson (playbook §7) applies to the robot too.
The copper is unaffected (routing, fills and DRC used the larger 3.5 / 5.5 mm pads, so a library pad only gains clearance).
**Decision:** make D-031 persistent in each part's own footprint (hub retry-2 method): the user opens PCB1 → right-click
J1 (then J2, J3, J4) → Edit Footprint → agent runs `45_xt_fp_editor.js` (keeps the pad ids and nets; XT60 5.5 / 4.0 mm,
XT30 3.5 / 2.5 mm) → user saves the tab. Then export (`51_export_project.js`) and confirm the drills in the saved file;
P6 confirms them in the drill file. Import Changes re-binds the library footprint: re-run after any import.

### D-035 · Remove Robot GND fills for user manual edits (2026-10-28)

**Source:** user opened/connected WRO_Robot and explicitly requested removal of its ground pours,
with a later request to refill after their edits. Verified projectUUID
7c9ddded879e0beabea718339ea291c0ca140c2c8d71e5fdf6645641934d5864, PCB643ce99542452c8f.

**Decision/execution:** full pre-change project backup plus exact native/live polygon dumps, then
guarded deletion of exactly1,922 GND Fill primitives on layers1/2 (325top/1597bottom) and save.
No source/library/netlist/pad/placement/routing/visibility trick was used. Export comparison across
110documents proves only thoseFILL records and331FILL allocator records removed; every other native
record body unchanged, including all66components,374coppertracks and175vias.

**Why:** the user wants an unobstructed manual-edit interval, not a new layout or fabrication release.
**Rejected:** automatically restoring old polygons after user edits, replaying historical builders,
or touching unrelated open drill/fit issues. A future repour must use new snapshots and edited geometry.

Backups and reproducible verification: [GND edit record](../scripts/robot/data/gnd_removal_20261028/README.md).
No order, commit or push. Ground connectivity/DRC must be rechecked only after the new pour is generated.

### D-036 · Edited Robot review and failed XT-writer investigation (2026-10-28)

**Source:** user completed manual edits and requested route/via/DFM review, new GND, documentation,
actual Gerber and board-specific JLC settings. Later reported an Associated Device popup and a
possible prior click. User then reported visible PCB1 and authorized continuation.

**Preservation:** use the user-edited10:59 archive as baseline, not the older routed layout.
The fresh13:24 export and complete comparison cover110documents. All PCB record bodies,
schematic record bodies, DEVICE metadata and footprint bindings are unchanged. The three XT
footprints contain six changed PAD records (centres/sizes rounded by <0.001mm, default/mask fields)
and three new empty NET records. Document headers differ only in client IDs; archive resources and
project metadata are unchanged. Do not attribute these changes to the user's click without evidence.
Evidence: [full comparison](../scripts/robot/data/association_check_20261028-1324.json).

**Confirmed tool defect:** builder emits holeAfter/padAfter while the original writer reads
hole/pad; it passed an undefined drill and omitted pad enlargement. Its getter readback also used
object fields on tuple values, so prior success claims were invalid. The old generated54_run is
disabled. Tests first reproduced the argument mismatch and missing preflight/readback guards;
the corrected writer and one-footprint builder pass4Robot Python and7Node tests.

**Decision:** keep D-031 sizes; inspect/open one project-footprint tab separately, then run a
snapshot-locked pad-only writer with full before-state validation and fresh dimensional readback.
Verify a new full export after each footprint before proceeding. Cancel association prompts rather
than selecting a catalog device. No library-device API, schematic ECO, full restore or database edits.
The replacement is not a claimed persistent fix until saved native geometry proves it.

**Remaining release gates:** actual RF projection, load-specific route widths/centreline lengths,
all hole/via/copper clearances, GND regeneration/connectivity/floating islands, silk bounds, fresh
ERC/strict DRC and pin membership, actual Gerber/drill inspection, user physical/visual review.
No current order authorization or fabrication-ready claim.

**As built13:51:** approved drills now persist in the three actual project-footprint documents.
J1 has157.48mil drill (3.999992mm), original216.5mil copper (5.4991mm); J2/J3/J4 have98.425mil
drill (2.499995mm) and137.795mil copper (3.499993mm). Each footprint was saved/exported/diffed
separately; only intended hole/pad widths/heights and its header timestamp/version changed versus
the previous verified export. Live PCB1 uses these sizes and original pin nets/positions. Archive:
[XT verified](WRO_Robot_xt_verified_20261028-1351.epro2). J2/J3 readback rounded pad width to137.8mil,
while native export retained137.795mil; regression tests now cover that exact representation and a
save-only interruption recovery (4Python +9Node tests). No association or ECO was applied.
Current geometric audit finds no shorts, minimum gap0.1569mm, no hole-gap violations, all non-GND
nets connected; this does not yet close RF, pour, ERC/DRC, silk, CAM or physical qualification gates.

### D-037 · +3V3 reroute out of the actual antenna overhang; orphan via removed (2026-10-28)

**Context:** the user's outline change (top edge 3840→3900 mil) put the DevKit module's radiating
element (module top edge = pin1 + 1.91 mm = y3840.2, +6.24 mm) on-board. The +3V3 spine ran at
y3850 directly beneath it for ~24 mm; the historical conservative strip (416,3640,1524,3840) never
covered that region, so the old audit missed it. Meanwhile the 1.5 mm +5V ×37.9 mm run proved to be
the L298N logic branch (tens of mA — auxiliary, no change), and the 1.0/1.2 mm BAT_SW branches are
the sense/LED and amplifier auxiliaries (within class minimums).

**Decision:** modify exactly three LINE segments (net/layer/endpoints (470,3760)/(1765,3850) kept,
width 1.0→0.8 mm class): dip the crossing to y3819.348, rise after x1540. Corridor derivation:
U1B.1 pad top 3796.5 + 6 mil + half-width vs antenna 3840.2 − 4 mil − half-width ⇒ only ≤0.856 mm
fits; 0.8 mm is the D-032 class width. Gates: ≥4 mil to the actual antenna box (pin1-derived, hub
test_repair_rf method), ≥6 mil to all foreign copper, +3V3 connectivity intact — 7 RED→GREEN tests.
GND stitching via 44628080eda6d090 (orphaned when the user's rework removed its stub; unreachable
inside the pour-excluded strip) was deleted with a guarded writer; one more via nudged (5,−20) mil
during the fill batches and was restored to its planned position (both spots ≥15 mil from foreign
copper). Persisted and diff-verified; [archive](WRO_Robot_rf_verified_20261028-1431.epro2).

### D-038 · Final GND regeneration and fabrication release (2026-10-28)

**Decision:** regenerate GND from the final edited geometry (never restore old polygons): 4,612
pieces (2,178 top / 2,434 bottom), 11 mil normal clearance, 16 mil edge, 8 mil margin around BOTH
the historical strip and the actual antenna overhang, 40/14 mil XT thermal spokes, islands removed
via FG.connectivity (real intersects predicate — the earlier bbox-only union-find was a defect).
Serialized with repair-split self-touches; saved export equals the plan piece-for-piece; exported
geometry passes 0 foreign overlaps / 0 clearance violations (min 10.898 mil) / edge 16.0 / antenna
margins 7.9 / 0 floating / full connectivity (`saved_fills_check.json`). 11 silk artifacts fixed
via a guarded relocation writer (labels off pads/texts, SW1 designator back on-board); 24_check_silk
0 hits. Strict DRC re-run inside the export script: **passed, 0 findings in every category**.

**Release:** [fabrication/20261028-final/](fabrication/20261028-final/README.md) — Gerber zip,
project, raw BOM, MFG inspection (actual drills 4.0×2 / 2.5×6 / 3.2×4 mm, plated flag, 156 via
hits, one closed 92.329×98.425 mm outline, pours on both copper layers), SHA256SUMS. JLC suggestion:
2-layer 1.6 mm FR-4 **1 oz**, HASL, tented vias, **no assembly** (D-013). Do not switch to 2 oz
without re-checking every gap (0.157 mm min gap passes 1 oz, not 2 oz). No order placed.
Open hardware gates unchanged: XT plug fit on a 1:1 print, switch contact action, full-load
voltage-drop/thermal, live Wi-Fi range test with the antenna projection clear.
