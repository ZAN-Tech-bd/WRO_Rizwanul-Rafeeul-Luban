# Project Audit Findings

## 2026-08-10 — RFID rail narration stop/resume behavior

- The standalone target for the requested change is `ESP32_RFID_MP3_RAIL/ESP32_RFID_MP3.ino`; it currently starts the Nano immediately for the start card, but station cards do not explicitly stop the Nano before audio and there is no playback-finished/resume state machine.
- Required behavior is: mapped audio RFID -> send Nano `STOP` before playback -> send `START` only after DFPlayer reports the track finished; the end UID `A3 C0 38 15` sends `STOP`, plays no audio, and latches the rail stopped.
- The stop latch must prevent station audio completion, manual playback, or timeout fallback from starting the rail. Only the start UID `18 C4 0F BA` clears the latch and can resume the rail.
- The existing integrated `ESP32_RFID_MP3_LFR` controller already demonstrates the needed DFPlayer finished-event guard (`NARRATION_MIN_MS`) and timeout fallback (`NARRATION_MAX_MS`), but the standalone rail target needs a smaller equivalent implementation.
- The DFPlayer error path must not auto-start after a failed/no-audio playback; when audio is unavailable, the rail remains stopped until the explicit start card is scanned.

## 2026-08-10 — ESP32 RFID rail start/end cards

- The requested target is `ESP32_RFID_MP3_RAIL/ESP32_RFID_MP3.ino`, not the already-integrated `ESP32_RFID_MP3_LFR` target.
- The rail target currently maps only station UIDs to tracks 2–8 and has no Nano UART. Its serial monitor commands control only DFPlayer playback and volume.
- The requested start UID is `18 C4 0F BA` and should map to `0001.mp3`; the requested end/stop UID is `A3 C0 38 15` and should produce no new audio.
- Because this target is paired with `NANO_LFR_RAIL`, the useful interpretation of the start/end cards is to add the existing 9600-baud newline UART contract: start card sends `START`, end card sends `STOP`. Use ESP32 UART1 GPIO25 RX/GPIO26 TX, matching the other RFID/LFR controller and its documented Nano wiring.
- The end card must be distinguishable from an unknown UID even though it has no track. A mapped action/entry-index result is safer than using track `0` as both “no audio” and “unknown.”
- Implementation/build result: `ESP32_RFID_MP3_RAIL/ESP32_RFID_MP3.ino` now uses mapped track `1` for START and track `0` for the STOP/no-audio action, sends newline-terminated Nano commands over UART1 GPIO25/26, and builds successfully for `esp32dev` at 21,864 B RAM / 282,981 B application flash.
- Hardware upload and physical RFID/Nano verification were not performed for this request.
- Follow-up hardware flash succeeded after the user connected the Nano: `/dev/ttyUSB0` was the CH340 at USB path `3-2`; `nanoatmega328new` upload wrote and verified 4,092 bytes, and boot capture returned `READY:STOPPED`.
- ESP32 follow-up flash succeeded to `/dev/ttyUSB1` after the second CH340 appeared at USB path `3-1`; esptool identified ESP32-D0WD-V3 rev 3.1 / MAC `1c:c3:ab:3c:8c:38`, wrote 283,344 application bytes, and verified all flash hashes. Passive post-reset capture was empty because it started too late; interactive PlatformIO monitor cannot run in the non-TTY tool terminal, so an explicit reset plus pyserial capture remains the runtime verification method.
- Explicit reset plus pyserial capture verified the ESP32 runtime markers, RFID initialization, Nano UART initialization, and boot-time `STOP` transmission. The same capture reported `DFPlayer ERROR! Check wiring / SD card.` and audio disabled, so the firmware path is verified but DFPlayer/SD hardware is not.

## 2026-08-10 — Nano rail controller ↔ ESP32 RFID/MP3 integration

- User locked a Nano-only adaptation: `ESP32_RFID_MP3_LFR/` must not be modified. Compatibility must therefore be achieved entirely by implementing the companion's existing UART contract on the Nano.
- `NANO_LFR/NANO_LFR.ino` is not a simple motor sketch. It is a five-sensor, active-low line follower with rule-based straight/curve/sharp/search/junction steering, UART START/STOP control, sensor/motor telemetry, live `SET` tuning, EEPROM persistence, a minimum-drive ramp, and a manual rail-test mode.
- The companion `ESP32_RFID_MP3_LFR` depends only on a small subset of that Nano behavior for the tour: newline-delimited `START` and `STOP` at 9600 baud, plus `READY:STOPPED`, `STATE:RUNNING`, and `STATE:STOPPED` replies. It sends STOP before station narration, START after narration, START for the silent tour-start card/emergency evacuation, and STOP at the evacuation exit.
- The calibrated `NANO_RAIL` behavior is physically different from line following: fixed forward PWM left=150/right=210 and active braking. Its current `F`/`S` protocol and human-oriented status output are not directly compatible with the ESP32's deterministic `START`/`STOP` protocol and state parser.
- Existing `NANO_LFR` logical motor mapping is crossed: `wheelRaw(lm, rm)` sends `lm` through IN3/IN4 + ENB and `rm` through IN1/IN2 + ENA. The calibrated `NANO_RAIL` sketch names ENA/IN1/IN2 as left and ENB/IN3/IN4 as right. Copying 150/210 without reconciling this mapping can silently swap the tuned wheel PWMs.
- Existing `NANO_LFR` zero command sets both direction inputs HIGH but then writes enable PWM 0. That disables the bridge and does not provide the active HIGH/HIGH, enable-ON braking that physically stopped the rail robot. A dedicated brake path must write both direction inputs HIGH and ENA/ENB=255.
- If the final robot is rail-guided rather than line-guided, retaining the sensor steering, `MIN/BASE/CO/CI/SO/SI/...` tuning table, EEPROM image, RAMP, and junction/search logic adds failure modes without adding useful behavior. The clean design is a small rail state machine that starts braked, drives only forward at 150/210 on START, and hard-brakes on STOP.
- For robust Nano memory use, the final UART parser should reuse the existing fixed-size, newline-delimited character buffer rather than `String::readStringUntil()`. Commands should be case-insensitive but deterministic; unknown input must report an error and must never toggle motion.
- The ESP32 currently sets `lfrRunning` optimistically as soon as it transmits START/STOP, then corrects it from Nano replies. Basic compatibility requires no ESP32 change if the Nano preserves the expected replies. Stronger integration would add command acknowledgement timeout/retry and avoid beginning narration/photo work until STOP is confirmed or a bounded fail-safe expires.
- `NANO_LFR/platformio.ini` still exposes both old- and new-bootloader environments. The attached Nano has already been proven as `nanoatmega328new`; the adapted target should retain only the new-bootloader environment to remove ambiguity.

## 2026-08-08 — Active Nano LFR correction

- User reports the current robot mostly drives straight and does not slow/steer adequately when the line bends left or right. Requested master speed is 38%.
- Required Nano diagnostic is one serial line containing all five normalized logical sensor values (`s1` through `s5`) so the physical pattern can be observed while testing.
- The Nano hardware UART is simultaneously the ESP32 command link at 9600 baud, so printing on every unrestricted control-loop iteration would saturate the link and can delay `START`/`STOP`. The implementation must rate-limit complete sensor lines while still printing continuously enough for diagnosis.
- Current repository status contains substantial pre-existing modifications, including both `NANO_LFR/NANO_LFR.ino` and `ESP32_RFID_MP3_LFR/ESP32_RFID_MP3_LFR.ino`. Those edits must be inspected and preserved rather than replaced wholesale.
- Audio suppression belongs on the active ESP32 RFID/DFPlayer mapping path, not in the Nano line sensor algorithm. The exact configured start UID and active companion target still need source inspection.
- Active Nano source is `NANO_LFR/NANO_LFR.ino`; it currently uses `SPEED_PERCENT=40`, active-low normalized sensors on A0–A4, and a 9600-baud D0/D1 ESP32 link.
- Root cause of weak steering is visible in source: speed and both PD gains are scaled together. At 40%, `kp=kd=0.40`; a steady far-edge error of ±20 therefore changes each wheel by only about 8 PWM counts. Both wheels remain forward and close in speed, so the chassis mostly continues straight.
- Current bend logic is inverted from the requested behavior: straight base is PWM 60, while a non-straight path uses the higher PWM 64 base. It therefore speeds up slightly on bends rather than slowing.
- The current all-five pattern (`11111`) always performs a blocking 500 ms right pivot. It cannot select a left versus right branch from an ambiguous all-five reading and it blocks serial stop processing for half a second. Revised steering should avoid long blocking decisions and use the measured error/edge pattern for ordinary bends.
- Active merged ESP32 source maps start UID `18 C4 0F BA` to track 1 and currently calls `playTrack(1)`, then waits for narration completion before sending Nano `START`. The requested behavior should special-case that mapped start entry: skip DFPlayer playback and start the Nano immediately, while leaving tracks 2–8 unchanged.
- Nano telemetry currently has no sensor output. The ESP32 forwards complete Nano lines to USB, but its inbound `nanoBuf[32]` is too small for a readable `S1=... S5=...` line plus steering state; it must be enlarged if detailed telemetry is emitted.
- Implemented telemetry format is `S1=<0|1> S2=<0|1> S3=<0|1> S4=<0|1> S5=<0|1> E=<value|LOST> M=<mode> L=<signed PWM> R=<signed PWM>`. It is emitted every 125 ms (~8 Hz), not every unbounded loop pass, because the UART is only 9600 baud and also carries safety-critical START/STOP commands.
- Implemented 38% commands: straight 57/57; ordinary left 24/57; ordinary right 57/24; sharp left 5/62; sharp right 62/5; lost-line search ±41; all-five junction pivot ±38. Values remain within the existing signed `wheel()`/PWM model.
- Sensor direction history now updates only when exactly one outer edge is active. This prevents `11111` from always overwriting history as left merely because S1 was checked before S5.
- All-five junction steering is now non-blocking and follows the last unambiguous edge (default right when no history), replacing the unconditional blocking 500 ms right pivot. This keeps UART STOP processing responsive.
- Both affected targets compile: Nano `nano_new` uses 330/2048 B RAM (16.1%) and 4302/30720 B flash (14.0%); merged ESP32 uses 46,328/327,680 B RAM (14.1%) and 815,625/1,310,720 B app flash (62.2%).
- Fresh hardware inventory found no `/dev/ttyUSB*` or `/dev/ttyACM*`, no `/dev/serial/by-id` entries, and no USB serial-class adapter in `lsusb`. Previous `/dev/ttyUSB0` identities are stale and cannot safely be reused for flashing.
- After the ESP32 was plugged in, `/dev/ttyUSB0` reappeared as an unowned CH340; a read-only `esptool chip_id` positively identified ESP32-D0WD-V3 rev v3.1, MAC `1c:c3:ab:3c:8c:38` before any write.
- After the Nano was plugged in, a second CH340 appeared at `/dev/ttyUSB1` (USB path `04:00.4:2`, distinct from the ESP32 at `04:00.4:1`); avrdude read signature `0x1e950f` (ATmega328P). Both CH340s share VID:PID `1a86:7523`, so they were distinguished by USB path and the esptool/avrdude probe, not by name alone.
- ESP32 flash and runtime verified: all region hashes verified; boot log showed `RFID-MP3-LFR-v3-silent-start`, `-> Nano: STOP`, and `<- Nano: STATE:STOPPED`.
- Nano flash and runtime verified: 4302 bytes written and verified; boot/telemetry showed `READY:STOPPED` then `S1=0 S2=1 S3=1 S4=0 S5=0 E=-5 M=STOP L=0 R=0`, repeated at ~8 Hz (rate-limited by `SENSOR_REPORT_MS=125`). Live sensors (S2/S3 active) confirm polarity is correct and telemetry is flowing.

## 2026-08-08 — ESP32 MP3/LFR Browser-Control Diagnosis

- The physical sequence LFR ON → LFR OFF → track 1 was generated by the laptop's direct HTTP `curl` requests, not by clicking the rendered page. It proves the HTTP handlers and hardware command paths, but not the page's JavaScript.
- The user reproduced the no-op buttons from both a phone and a laptop while viewing `http://192.168.4.1`. This disproves the prior phone/mobile-data-only explanation.
- Current isolation: `GET /lfr?state=on`, `GET /lfr?state=off`, and `GET /play?track=1` work when requested directly; the rendered page's JavaScript-only `onclick` → `fetch()` path is the unverified/failing layer.
- Acceptance now requires a real browser click test, not URL-level HTTP checks. The corrective UI should function without JavaScript so browser policy, script errors, or stale script caching cannot disable controls.
- The first Firefox/WebDriver reproduction was inconclusive: the expected ESP32 device node `/dev/ttyUSB1` no longer existed when serial capture opened, and WebDriver navigation to the AP timed out. No button was clicked and this is not evidence for or against the page. USB identities and AP association must be freshly established before the next run.
- Browser-safe corrective design is now implemented: every control is a real HTML GET form and therefore does not require JavaScript; successful actions use HTTP 303 back to `/` and render the action status; root/action responses disable caching; the page and serial banner expose `WEB-FORMS-v2`; AP mode is explicit and Wi-Fi power saving is disabled. The replacement build passes at 45,304 B RAM and 782,097 B flash.
- The corrected image is physically installed on the same ESP32 identity (MAC `1c:c3:ab:3c:8c:38`), with 788,672 application bytes written and hash verified. Runtime identity and service startup are proven by `WEB-FORMS-v2` in the boot banner.
- Post-flash network gate succeeded from the laptop: `ESP32-MP3` association, DHCP `192.168.4.2/24`, route via `192.168.4.1`, HTTP 200 root response, 2,886-byte body, no-cache headers, and the new page marker all passed. This removes the prior transient AP-association failure before browser automation.

## 2026-08-06 — Nano LFR/RFID Split Findings

- During implementation, an untracked [`RobotLinkProtocol.h`](firmware/common/RobotLinkProtocol.h) appeared at 21:38:33 with a competing `$TAG...*HH` run/stop protocol, one minute before the newly added [`RobotUartProtocol.h`](firmware/common/RobotUartProtocol.h). Neither file is tracked in Git and the former was absent from the initial workspace inventory. Reconcile the two designs before either controller includes one; do not ship duplicate wire protocols.
- The Arduino Nano cannot retain the supplied D10/D11/D12 motor allocation while using an RC522 over hardware SPI: ATmega328P SPI is fixed to D11 MOSI, D12 MISO, D13 SCK, and D10 should remain the RC522 SS/hardware-SS output. Motor control therefore needs a deliberate remap rather than a textual ESP32-to-Nano port.
- A conflict-free full Nano allocation is available: IR A0–A4; RC522 D10/D11/D12/D13 plus RST D4; SoftwareSerial D2 RX/D3 TX; L298N left D5 PWM + D7/D8 and right D6 PWM + D9/A5. D0/D1 remain available for USB hardware-serial diagnostics.
- RC522 modules are 3.3 V devices. Supplying 3.3 V is not enough protection when a 5 V Nano drives SS, MOSI, SCK, and RST; those four outputs need a proper 3.3 V logic buffer/level shifter. RC522 MISO at 3.3 V is high enough for a 5 V ATmega328P input in this application.
- The Nano UART output is also 5 V. Use Nano D3 -> 1 kΩ -> ESP RX node, with 2 kΩ from that node to GND, yielding approximately 3.3 V at ESP GPIO25. ESP GPIO26 -> 1 kΩ -> Nano D2 is safe for optional ACKs. Common ground is mandatory.
- ESP GPIO25/26 are selected for the Nano link because direct ESP motor ownership is ending and this preserves GPIO16/17 for the already-planned DFPlayer UART.
- RC522 UID/SAK/type can identify a presented card for station routing, but UID alone is cloneable and must not be described as cryptographic authentication.

## 2026-08-06 — ESP32 Line-Follower Port Findings

- New split architecture requested: Arduino Nano ATmega328P becomes the real-time motion/RFID controller; ESP32 becomes the application-side receiver for framed RFID credentials and later museum functions.
- RC522 requires the Nano hardware SPI pins D10 SS, D11 MOSI, D12 MISO, and D13 SCK. The original Arduino sketch already consumes D10 as ENA, D11 as IN3, and D12 as IN4, so those motor signals must move. The IR array on A0–A4 can remain unchanged.
- PlatformIO board IDs available locally are `nanoatmega328` (classic bootloader) and `nanoatmega328new` (new bootloader); MFRC522 package search resolves `miguelbalboa/MFRC522` version 1.4.12.
- A Nano-to-ESP UART needs voltage protection in the Nano TX → ESP32 RX direction because ESP32 GPIOs are not 5 V tolerant. Common ground is required. Keeping USB hardware serial free for diagnostics favors Nano `SoftwareSerial` and an ESP32 hardware UART.
- Pin-complete split is feasible without another module: Nano uses A0–A4 IR, A5 RC522 reset, D10–D13 hardware SPI, D4/D5/D9 for L298N channel A (physical left), D7/D8/D6 for channel B (physical right), and D2 RX/D3 TX for the ESP link. ESP uses UART1 GPIO21 RX/GPIO22 TX for Nano and UART2 GPIO16 RX/GPIO17 TX for the existing DFPlayer.
- Required rewiring is substantial: IR and all six L298N control signals move from ESP32 to Nano; RC522 connects to Nano with 5 V-to-3.3 V protection on Nano-driven SS/MOSI/SCK/RST; Nano D3 TX needs a divider before ESP GPIO21. DFPlayer stays on ESP GPIO16/17 exactly as most recently planned.
- The ESP cannot infer station identity from an unknown UID. The receiver will send/log every UID and support an explicit UID→station map; mapped tags play station tracks while unknown tags still produce a safe stop and visible diagnostic rather than being silently assigned nondeterministically.

- The correct implementation target already exists at `firmware/robot/`; it is a valid PlatformIO `esp32dev` project but its source is still a 21-line skeleton and its README still contains a blank planned-wiring table.
- The confirmed 30-pin ESP32-WROOM-32 allocation has a clean Phase-1 subset: IR far-left→far-right = GPIO36/39/34/35/32; L298N channel A = GPIO25 ENA, GPIO26 IN1, GPIO27 IN2; channel B = GPIO13 ENB, GPIO14 IN3, GPIO33 IN4. None of these Phase-1 signals uses USB UART or ESP32 flash pins.
- The user's working Arduino `wheel(lm, rm)` controls the **left** motor through L298N channel B (`inC`, `inD`, `enb`) and the **right** motor through channel A (`inA`, `inB`, `ena`). The existing robot plan used the opposite channel labels even though its GPIO numbers were usable. Preserve the proven physical convention by relabeling A=right and B=left in both wiring and firmware.
- The original sensor algorithm reads five digital channels and then inverts each bit, so its default electrical interpretation is active-low: sensor output LOW means line detected. This should be a named compile-time setting because actual polarity must be verified over black and white before wheels-down testing.
- GPIO34/35/36/39 are input-only and have no internal pull resistors. This is valid for actively driven comparator outputs; open-collector outputs require external 10 kΩ pull-ups to 3.3 V. Powering the sensor array from 5 V is not acceptable unless every output is independently level-shifted.
- The weighted error must retain the original positive-weight arithmetic `(10,20,30,40,50)/sum - 30`, rather than changing to direct signed-weight averaging, because C++ integer truncation differs for some asymmetric multi-sensor patterns.
- The ESP32 port needs explicit PWM setup rather than AVR timer assumptions. An 8-bit, 20 kHz LEDC output preserves the original 0–254 command range while moving motor PWM above normal audible range.
- Safe electrical bring-up requires external regulated motor and logic rails, common ground, ENA/ENB jumper removal, motor suppression capacitors, and wheels raised for the first direction test. Positive left/right commands must both move the chassis forward; if one does not, swap that motor's two L298N output leads rather than changing the sensor order.
- The implementation uses 20 kHz, 8-bit ESP32 LEDC PWM and clamps signed commands to ±254, matching the working sketch's practical range. Zero command preserves its HIGH/HIGH active-brake direction state with PWM duty zero.
- A compile-time motor gate allows first sensor-polarity diagnostics with all PWM output forced off. Normal autonomous operation remains the default after a 1.5 s startup stop; if no line has ever been observed, the ESP32 port safely stops instead of retaining an undefined/previous motor command.
- Build verification passed on Espressif32 platform 6.5.0, which this environment resolved to Arduino-ESP32 2.0.14 (not the 2.0.17 stated in an older audio-project comment). Firmware usage: 21,480 B RAM (6.6%) and 270,233 B application flash (20.6%). cppcheck passed with no high/medium findings; its low-only `setup`/`loop` reports are expected because it does not model Arduino framework entry points.
- Physical-calibration connection: the newly plugged robot presents one CH340 (`1a86:7523`) at `/dev/ttyUSB0`, stable by-id `usb-1a86_USB_Serial-if00-port0`, USB path `3-2`. It connected at 17:11:52–17:11:53 Asia/Dhaka and no serial-monitor process owns it. This is the only currently enumerated serial adapter, so it is the positively identified target for the sensor-only flash.
- Physical sensor result: the array requires no firmware polarity change. On the real light background all outputs settle inactive (`00000`); black is detected as active `1`. Isolated physical detector tests exactly match GPIO order 36/39/34/35/32 from far-left through far-right, and all-black is stable `11111`. External 10 kΩ pull-ups and current mounting height therefore provide valid digital levels under stationary test conditions.
- Physical motor isolation disproved a global motor-power fault: both channels drive a wheel at higher PWM. Actual chassis mapping is channel A (ENA25/IN1=26/IN2=27/OUT1-2) → physical left wheel, and channel B (ENB13/IN3=14/OUT3-4) → physical right wheel. IN4 used GPIO16 during these tests but was later reassigned to GPIO33 at the user's request. Both positive commands rotate forward. PWM 80 produced no visible motion, channel B/right moved slowly at 160, and channel A/left moved at 220; exact per-wheel minimums still require calibration after reflashing the corrected semantic map.
- Final wheels-up motor calibration after the semantic-map reflash: left start threshold lies in `(160,180]`; right start threshold lies in `(180,190]`. Both reverse correctly at magnitude 200. Approximate open-loop speed trim is left=200, right=220 (right needs about 10% more command). A right pivot is left=+200/right=-220. These no-load observations are starting values; on-floor traction may require higher minimum commands.
- The first floor-calibration image keeps autonomous-at-boot false and now accepts only `AUTO <milliseconds>` in the range 1–5000. It records the start time, stops, and disarms automatically at timeout. This removes dependence on a host `STOP` command if the robot leaves the line or the serial link is interrupted.
- The latest guarded image with 200/220 drive baseline and 250 ms provisional pivot builds successfully on the pinned toolchain: RAM 21,552 B (6.6%), application flash 273,109 B (20.8%). cppcheck has zero high/medium findings and only the same three expected low Arduino-entry-point/compile-time-folding reports; targeted whitespace validation passes.
- User-requested post-calibration pin swap: GPIO33 now drives L298N IN4, freeing GPIO16 for future DFPlayer TX -> UART2 RX; GPIO17 remains UART2 TX -> 1 kΩ -> DFPlayer RX. This changes only the ESP32 signal pins, not the L298N output-pair/physical-wheel mapping. Prior motor evidence used IN4 on GPIO16, so right-wheel forward/reverse must be rechecked wheels-up after rewiring and flashing the new map.
- The GPIO33-IN4 image passes PlatformIO build, cppcheck, and whitespace validation with unchanged size (21,552 B RAM / 273,109 B application). It was not uploaded because the physical move from GPIO16 to GPIO33 has not yet been confirmed; the currently installed guarded firmware therefore remains the prior GPIO16-IN4 image.
- After the user confirmed the physical IN4 move, the GPIO33 image was flashed through the freshly revalidated, unowned CH340 by-id path. Esptool again identified the same ESP32-D0WD-V3 rev 3.1 and MAC `1c:c3:ab:3c:8c:38`; every region hash verified. Live boot output uniquely confirms `IN4=33`, startup autonomous output disabled, and calibration mode with motors disabled. GPIO16 remains physically disconnected pending future DFPlayer integration.
- **Direction preserved after swap.** A guarded `MOTOR 0 200 700` right-wheel test through GPIO33→IN4 drove the physical right wheel forward, matching the pre-swap measurement. The full motor calibration (left starts by 180, right starts by 190, matched pair left=200/right=220, pivot left=+200/right=-220) remains valid under the new pin map without any firmware direction inversion.

## 2026-08-05 — Current Track + Robot Wiring Scope

- The track has seven straight WS2812B boards with eight addressable pixels each. They will be wired as one 56-pixel DIN/DOUT chain; only the first board consumes an ESP32 data GPIO. All LED power taps remain parallel rather than flowing board-to-board through small traces/connectors.
- The final track target is four real DHT11 plus four real MQ-135 digital channels, distributed at stations 1, 3, 5, and 7. The existing Hub microSD allocation is retained; stations 2, 4, and 6 use explicitly identified generated values derived from neighboring valid measurements.
- The corrected track supply design must account for 56 WS2812B pixels, four MQ heater modules, four SG90 servos, ESP32/Wi-Fi/microSD, buzzer, and logic. A regulated 5 V / 10 A class supply remains the conservative single-supply baseline, with separately fused/distributed branches and local bulk capacitance.
- The user expanded planning scope after track finalization. The robot has three logical controllers: a separately powered Arduino + display shield, a separately powered ESP32-CAM, and a main ESP32 for DFPlayer/audio, five IR channels, L298N, RC522 SPI, and one or two eight-pixel WS2812B boards.
- A provisional main-ESP32 allocation is feasible without using USB UART or boot-strapping pins: four input-only pins plus one regular input for the IR array; VSPI plus two control pins for RC522; six outputs for L298N direction and dual PWM; UART2 for DFPlayer; and one WS2812B output. It must not be finalized until the exact IR array output voltage/type, ESP32 board, battery/motor ratings, amplifier topology, speaker load, and display communication requirement are known.
- Critical unresolved electrical questions for the robot are not cosmetic: battery and motor ratings determine buck and motor-rail design; IR output voltage determines whether direct ESP32 input is safe; and amplifier model determines whether it should receive DFPlayer DAC-level audio or a speaker-level signal.
- TI's current SN74HCT165 product page confirms that an eight-bit parallel-in/serial-out register could concentrate multiple 5 V digital MQ outputs, but the final four-real-pair architecture has enough direct GPIOs while retaining SD. The shift-register alternative is therefore not required and is not part of the locked BOM.
- Robot details confirmed by the user: the main controller is another 30-pin ESP32-WROOM-32 DevKit; the five-channel line array exposes five digital outputs and is operated at 3.3 V; drivetrain uses two motors through an L298N; source battery is a 3S LiPo; audio is DFPlayer Mini feeding an LM386 amplifier module; lighting is one eight-pixel WS2812B board; the Arduino Mega + TFT remains a separately powered fixed-screen assembly and needs no ESP32 UART/data link. ESP32-CAM likewise remains functionally unchanged and only needs an appropriate power branch.
- Still required before assigning final robot power hardware: the two motors' rated voltage and stall current, speaker impedance/wattage/count, 3S battery capacity/C rating, and exact LM386 module terminals. A pin map can be finalized independently, but a specific buck current rating or safe amplifier interconnect cannot honestly be locked without those values.
- User then confirmed two 6 V TT gearmotors and one 4-ohm, 3 W speaker, and requested that the architecture begin at the buck-converter outputs rather than sizing the 3S battery itself. The motor branch can therefore be specified as a regulated 6 V high-current buck into the L298N motor-supply input, accepting the L298N's significant voltage loss and reduced motor voltage/torque. A genuine 5 A class converter is a conservative starting point, but the final fuse and acceptance test still depend on measured two-motor stall current.
- The robot logic/audio branch can be specified as regulated 5 V. A 4-ohm/3 W speaker rating does not mean an LM386 can produce 3 W; a typical LM386 operated from 5 V will under-drive that speaker. This is electrically safe but may be too quiet. The audio plan must use a ground-referenced DFPlayer DAC_L or DAC_R line output into the LM386 input, never the DFPlayer's bridged SPK1/SPK2 output into a ground-referenced amplifier input.
- Final Hub trade-off selected by the user: retain the existing microSD interface unchanged on GPIO18 SCK, GPIO19 MISO, GPIO23 MOSI, and GPIO5 CS; install four real DHT11 and four real MQ-135 modules; generate clearly marked simulated values for the other three station rows. A distributed real-sensor layout at stations 1, 3, 5, and 7 permits bounded interpolation for simulated stations 2, 4, and 6.
- The pictured HW-209 automatic bidirectional level converter is a discrete BSS138/passive-pull-up design, not a push-pull AHCT/HCT buffer. It is appropriate for open-drain buses and may pass a very short WS2812B bench test, but its resistor-produced HIGH transition is not the reliability choice for an 800 kHz signal over a long track cable.
- The locally available pictured 74HCT125 DIP-14 is accepted and locked for both WS2812B interfaces. When powered at 5 V, its TTL-compatible input threshold accepts an ESP32 3.3 V HIGH and its push-pull output supplies a strong 5 V waveform. Use one enabled channel plus a 330–470 ohm series resistor; place a 100 nF decoupling capacitor at the IC; tie unused buffer inputs to a defined level and disable their active-low output-enable pins HIGH.

## 2026-08-05 — pskill9 Web Search MCP Diagnosis (FIXED)

- A known-good query returned an empty JSON array from the pskill9 server, while an independent DuckDuckGo MCP returned five normal results for the identical query. MCP tool discovery, invocation, argument validation, and response transport were therefore working; the defect was provider/parser-specific.
- The installed server (`/home/user/.pskill9-web-search/build/index.js`, launched via the Zoo Code `mcp_settings.json` entry `pskill9-web-search`) was a minimal Google HTML scraper. It sent a direct request with an old Chrome 91 user agent, then accepted only result containers matching one hard-coded Google layout (`div.g` + `h3` + `.VwiC3b`) and silently returned an empty array when none matched.
- A direct request to the same Google search endpoint from this environment returned a ~91 KB localized consent/interstitial-style page with no `div.g` result containers. It behaved like a successful HTTP 200, so Axios did not throw; the hard-coded selector found zero result cards and the server serialized that condition as an apparently successful empty array.
- Root cause: Google scraping is being served a layout the stale parser does not recognize, and the server had no interstitial/bot-block/layout-change detection, so the upstream failure was misreported as zero search results.
- REMEDIATION APPLIED: patched the installed `build/index.js` (v0.1.0 -> v0.2.0). The patch adds a modern Chrome UA with `Accept-*` headers and `hl=en`/`num`, broader Google result selectors, explicit blocked-page detection, a DuckDuckGo HTML (`html.duckduckgo.com/html/`) fallback with `uddg` URL decoding and per-URL dedup, a descriptive thrown error when both providers return zero, and stderr diagnostics. The original artifact was backed up to `build/index.js.bak-20260805-134628`.
- VALIDATION: drove the patched server over MCP stdio with a JSON-RPC `initialize` -> `notifications/initialized` -> `tools/call(search)` sequence. "OpenAI official website" -> 5 unique results; "ESP32 camera frame2jpeg" -> 3 unique results. `node --check` passes. The earlier-suspected possibilities (MCP transport failure, wrong tool args, Axios exception) were all ruled out by evidence.
- Caveat: unauthenticated Google/DuckDuckGo HTML scraping is inherently fragile (layout/bot-detection changes can re-break it). If reliability matters, migrate to a maintained search API/provider. The DuckDuckGo MCP remains available as a second independent provider.

## Repository State

### 2026-08-04 direct-RAM camera integration

- Repeated-upload evidence supersedes the earlier preference for raw `image/jpeg`: although `HTTPRaw` avoids NUL truncation and once committed a complete 1,981-byte JPEG, successive requests triggered the Hub web-task watchdog. Camera HTTP outcomes degraded from 200 to `HTTPC_ERROR_READ_TIMEOUT` (`-11`) and then connection refused (`-1`). On this physical Hub and Arduino-ESP32 core, raw callback stability—not JPEG encoding or Wi-Fi signal—is the remaining transport defect.
- Selected replacement: standard `multipart/form-data` with JPEG field `image`, camera-side contiguous multipart body, and Hub-side `HTTPUpload` chunks. This still preserves exact binary JPEG bytes and avoids Arduino `String`; the small multipart framing overhead is immaterial for ~1.9 KB QVGA images.
- Current endpoint mismatch: the Hub multipart build is physically installed, while the camera still runs the old raw sender. The camera flash never started because broad `pkill -f serial-monitor` matched and killed its own shell. Future port cleanup must use `fuser /dev/ttyUSB0` and terminate only a positively identified exact PID.
- Current Hub multipart source would reject every valid upload: `UPLOAD_FILE_START` leaves `imageUpload.error` non-null (`"incomplete_body"`), and the completion condition treats any non-null value as failure. Initialize the fresh transaction with no error, set errors only on actual failures, guard cumulative length before writing each chunk, and reflash Hub before testing.
- Framework-source confirmation for multipart semantics: this core calls `UPLOAD_FILE_START` once, switches status to `UPLOAD_FILE_WRITE`, invokes the write callback for each full buffer and once more for the final partial buffer, increments `HTTPUpload.totalSize` after each callback, then calls `UPLOAD_FILE_END`. Therefore the application must increment its own `received` count only in `UPLOAD_FILE_WRITE`; adding `currentSize` again in `UPLOAD_FILE_END` would double-count the final chunk.
- The installed FS API is `FS::open(path, mode, create=false)`. A new `/last.tmp` must be opened with the explicit third argument `true`; relying on `FILE_WRITE` alone previously failed physically on this core.
- Corrected Hub multipart runtime is physically installed and identified by its unique boot text `Image receiver: multipart/form-data field 'image'.`. The normal camera upload also released `/dev/ttyUSB0` and produced a new 973,104-byte firmware image without any IO0/reset ritual or broad process kill.
- Host serial caution: opening the Hub CP2102 from a pyserial probe while DTR/RTS transitions occur can repeatedly reset the ESP32, producing many boot banners and unrelated captive-route `not found` lines. This evidence must not be mistaken for a firmware watchdog. Avoid simultaneous ad-hoc port opens during final transfer testing; use deasserted control lines from open or a stable monitor.
- First synchronized `SHOOT 1` attempt proved the camera was still running the old raw sender: its runtime string was `[hub] POST RAM JPEG`, not the new `[hub] POST multipart JPEG`. The preceding camera upload must not be counted as successful merely because the port was later released; capture and retain the full uploader result and verify the unique boot/runtime marker.
- That old raw request exposed a framework/API trap in the Hub route registration. This core's four-argument `server.on(..., completeFn, uploadFn)` installs the same second callback as both `RequestHandler::upload()` and `RequestHandler::raw()`. For a non-multipart POST, `canRaw()` returns true and invokes `handleImageUploadForm()`, but `server.upload()` dereferences a null `_currentUpload`; decoded panic: `LoadProhibited` at `WebServer::upload()` → `handleImageUploadForm()` → `FunctionRequestHandler::raw()` → `_parseRequest()`. A robust multipart endpoint must use a custom `RequestHandler` whose `canUpload()` is true and `canRaw()` is false, and whose `upload()` override receives the valid `HTTPUpload&` directly. It should reject old/raw senders cleanly instead of crashing.
- The custom request handler is now implemented and the 853,584-byte Hub image is physically installed with hash verification. Its raw fallback returns HTTP 415 instead of touching `server.upload()`.
- A fully captured camera upload attempt failed to connect to the bootloader before any flash write. This is the known CAM-MB entry-state issue, not a compile or image defect. The camera must be normally unplugged/replugged before one upload retry; no IO0/reset ritual should be introduced unless the user's standard workflow itself fails after replug.
- Laptop network is out of scope for further changes: keep `Lovly` connected and both `MuseumHub` profiles at `autoconnect=no`. Physical evidence comes from USB serial; the user can verify the dashboard from a phone joined to the Hub AP.

- User boot output proves the ESP-CAM application, GC2145 camera, PSRAM, Hub SoftAP association, and ESP-NOW setup all work. The camera joined `MuseumHub` as `192.168.4.3` at RSSI −44 dBm on channel 1.
- Onboard SD_MMC failed initialization with ESP-IDF error `0x107`. This is no longer a blocker: capture now converts RGB565 to JPEG and keeps the latest JPEG in RAM/PSRAM for direct HTTP upload. A mounted SD card only adds an optional backup.
- The revised no-SD image compiles successfully. The first upload attempt failed before any write because the IDE `serial-monitor` PID 64068 exclusively held `/dev/ttyUSB0`; this is a host port-ownership issue, not a code/build/device failure.
- Camera-side transfer is physically proven: `SHOOT 1` produced a 320×240, 1,981-byte JPEG and the Hub returned HTTP 200. The dashboard then displayed `last 4 bytes @jamdani` with a broken image.
- The 4-byte result is deterministic binary truncation in the Hub, not a wrong image protocol. JPEG begins `FF D8 FF E0 00...`; the installed Arduino ESP32 `WebServer` ordinary POST parser does `arg.value = String(plainBuf)`, so the first embedded NUL terminates the value after exactly four bytes. The current Hub handler then writes that truncated `String` and returns success.
- HTTP POST with `Content-Type: image/jpeg` is appropriate and simpler than base64 or multipart for this local link. Base64 would add about 33% size plus encoding/decoding memory and cannot repair a binary-unsafe receiver. The correct receiver is the framework's four-argument route with its `HTTPRaw` callback, which exposes exact binary chunks.
- Receiver design decision: stream `RAW_WRITE` chunks into `/last.tmp`; cap total size; capture station metadata; on `RAW_END`, close and verify received length plus JPEG SOI (`FF D8`) and EOI (`FF D9`); replace `/last.jpg` only after all checks; update `MuseumState` and return HTTP 200 only for a committed image. `RAW_ABORTED`, filesystem errors, invalid JPEG, and oversize input remove only the temporary file and preserve the previous valid photo.
- Lessons to prevent repetition: never put arbitrary binary in Arduino `String`; a transport-level HTTP 200 is not payload verification; test binary APIs with embedded NUL data; compare sender count, receiver count, persisted-file size, and served `Content-Length`; validate file signatures/end markers and actual decode/dimensions before declaring an end-to-end image path complete.
- Hub build target remains PlatformIO `esp32dev` with Arduino ESP32, LittleFS, and upload speed 921600. Only Hub application firmware needs rebuilding/reflashing for this parser correction; the already-deployed dashboard filesystem and already-proven camera firmware do not need to be rewritten unless verification exposes a separate issue.
- First binary-fix build succeeded (RAM 46,752 B, flash 846,477 B; 853,056-byte firmware image) and was sent to the Hub CP2102 (`10c4:ea60`, `/dev/ttyUSB1`); the camera is the distinct CH340 (`1a86:7523`, `/dev/ttyUSB0`). After a Hub reset, it printed the expected normal Hub boot banner and the camera joined at `192.168.4.2`.
- That first retest still served the previous 4-byte `/last.jpg`. The Hub serial port subsequently returned hundreds of kilobytes of repeated ASCII `1`, with no `[img] committed` log. This invalidates the physical runtime result despite the camera's HTTP 200. A successful compile/upload and a 200 response are each necessary evidence but neither proves that the expected handler committed a body. Runtime identity, logs, API byte count, JPEG markers, and decode all remain required.
- A clean reset followed by simultaneous serial capture removed ambiguity: the Hub emitted `[img] upload station=1  bytes=4  seq=1`, the exact old-handler format, while the camera emitted HTTP 200 for 1,981 bytes. The device is definitively executing the old application. Reflash must be repeated with complete captured uploader output and a preserved true exit code, then runtime identity should be confirmed via the new `[img] committed ...` log.
- The fully captured reflash succeeded and the next simultaneous trace proved binary-safe storage: camera sent 1,981 bytes and Hub emitted `[img] committed ... bytes=1981`. The remaining metadata discrepancy (`station=0` despite `?station=1`) proves that this Arduino `WebServer` raw lifecycle invokes `RAW_START` before query arguments are available through `server.arg()`. Body transport is fixed; station metadata must travel through a request header collected by `server.collectHeaders()` (or be parsed outside raw start). A small explicit header is preferable to wrapping JPEG in multipart/base64.
- Header-capable Hub/CAM builds both compile. The Hub flashed successfully; the CAM-MB did not enter the bootloader for the camera update (`No serial data received`, no flash write). Since the current camera still sends `?station=1&bytes=1981`, the lower-risk compatibility path is to check and apply those query args in `handleImageUploadComplete()` after raw parsing; if available there, no camera reflash is needed and headers remain a forward-compatible option.
- The installed Arduino FS layer has an explicit third `create` parameter on `FS::open()`. A physical trace showed a nonexistent temp path rejected with `no permits for creation`, then a web-task watchdog while the client awaited completion. The Hub receiver must use `LittleFS.open(path, FILE_WRITE, true)` for its new temporary file and must remain responsive after every upload outcome. This is distinct from the original NUL/String bug.

- The working tree was clean before this audit except for the three newly created planning files; there are no pre-existing uncommitted product-code changes.
- At the start of the 2026-08-02 SD-card inspection, the main working tree contains the previously documented Hub/web/display work plus generated/dependency directories and the three planning files. These are existing project changes and must not be altered during the read-only storage inspection.
- The repository is organized into a static browser app, five PlatformIO firmware targets (`robot`, `hub`, `health`, `emergency`, and `cam`), one shared protocol header, a QR utility, plans, and rule/reference documents.
- The repository contains 2025 WRO PDFs, while the canonical build plan says the target is WRO 2026 and explicitly marks those PDFs as outdated. The official 2026 Season Challenge and team/coach material still need to be acquired and checked.

## Intended System

- The canonical design is an offline Smart Museum Guidance System for WRO 2026 Elementary, centered on a guide robot and Bangladeshi cultural exhibits.
- Planned critical path: robot line following, RFID station stops, bilingual narration, location/arrow display, obstacle response, emergency evacuation mode, responsive track lights, per-artifact DHT11/MQ-135 monitoring, gate counting, and visitor/admin dashboards.
- Communications are intended to use ESP-NOW on fixed Wi-Fi channel 1. A dedicated Hub ESP32 is the sole SoftAP/HTTP host and state aggregator at `192.168.4.1`; browsers do not communicate directly with nodes.
- Camera capture and restricted-area access are stretch features. Cloud/Firebase and main-system face recognition are intentionally excluded.
- The build plan defines browser prototype work as Phase 0, then robot motion, station detection, audio/UI, responsive track, emergency handling, live health/gate integration, and finally build freeze/rehearsal.
- Reliability gate: each hardware phase should pass three consecutive runs before later work starts; the final robot should complete five unattended laps and the whole presentation should be rehearsed repeatedly.
- The original concept document describes 11 broad functions and an older Firebase/Firebase Storage architecture. The canonical build plan supersedes that cloud design with an offline Hub, so code must be judged against the build plan rather than the original platform section.
- The recommendation log confirms locked decisions: standalone LEGO AI Guard, ESP-NOW, English-only TFT, one MQ-135 per artifact, separate visitor/admin views backed by one Hub, and a dedicated Hub host.
- Still-open product decisions include the exact AI Guard product, TFT placement, number of physical stations (three recommended), health-node topology, and team membership/roles. These affect final physical layout and competition documentation but do not block most software prototyping.

## Implementation Inventory

### Browser application

- The web app claims to be a complete Phase 0 mock and already has gate, visitor, and admin pages plus shared styling, bilingual artifact data, a Hub adapter, and QR handling.
- `hub.js` contains both a `localStorage` mock backend and an HTTP backend automatically selected only when the hostname is exactly `192.168.4.1`.
- The adapter implements state polling, pending/approved check-ins, request lookup, approval/denial, emergency control, and a robot-advance test command. Live mode expects Hub endpoints under `/api/*`.
- The mock seeds seven artifacts with synthetic health data and robot state. This is useful for UI work but is not sensor integration.
- Live mode catches polling/command failures silently and has no exposed online/error state. Also, every `subscribe()` call creates another interval; that is harmless with one subscriber per page but is not robust as a general adapter.
- The README says offline QR decoding requires a vendored `jsQR.js`; no such vendor path appeared in the repository listing, so real QR decoding is likely not bundled yet and the demo depends on the simulation fallback.
- Health thresholds are explicitly placeholders pending real sensor observations.
- The gate page implements simulated/photo-QR check-in, approval polling, and redirect to the visitor view. It does not validate QR contents: any successfully decoded QR starts check-in, and if the decoder library is absent, selecting any image also starts check-in. This is acceptable as a demonstration trigger but not actual access validation.
- Gate network/API failures are not handled. A failed check-in can leave the page waiting with an undefined request ID, and request polling retries indefinitely without backoff or an offline message.
- The admin page renders pending approval controls, counts, robot state, health rows, and emergency controls. It directly interpolates request IDs into HTML/inline JavaScript and health values into HTML. Hub-generated IDs are constrained in the mock, but live API values should be escaped or rendered with DOM APIs before this is treated as secure.
- The admin page has no PIN/login gate despite the canonical plan describing a PIN-gated admin view and the original concept requiring secure login.
- Admin health classification catches high temperature/humidity/gas and low humidity only in the warning tier; unusually low temperature is not represented. More importantly, thresholds and units are not shared with firmware, so status could disagree across layers once live.
- Unknown/missing station IDs are displayed as the first artifact rather than an explicit offline/unknown state, which can make absent live data look valid.
- The visitor page displays current and next artifacts, narration label, queue guidance, emergency banner, visitor ID/status, and all seven bilingual artifact cards. It does not include the museum map promised by the build plan, direction/route visualization, device online/offline state, or a help-request action.
- Direct navigation to `visitor.html` is unrestricted; an absent, denied, unknown, or forged request ID does not lock the dashboard. Check-in approval is currently a presentation flow rather than authorization.
- Changing language re-renders artifact cards but does not re-render the dynamic robot/emergency content until the next Hub poll. In mock/live mode that resolves within roughly one second, but it is an avoidable UI delay.
- Artifact content is substantive and bilingual, though at least two Bangla descriptions contain likely typos (`ঐতিতাসিক` and `একটানা` in contexts likely intended as `ঐতিহাসিক` and `একতারা`). Cultural wording should be reviewed by the team before judging.
- The seven dashboard artifacts are modeled as a circular physical robot route in UI logic, while the canonical physical recommendation is only three stations. A station/route configuration layer is needed to avoid presenting all content cards as actual stops.
- `qr.js` performs image-based offline QR decoding appropriately for plain HTTP, but has no `FileReader`, image decode, canvas context, or oversized-image error handling. Its simulated QR value is discarded by the gate flow, reinforcing that the current QR is only a trigger.

### Shared firmware protocol

- A shared packed `MuseumMessage` protocol exists with magic/version/checksum validation, fixed-point health values, visitor count, station/node IDs, flags, sequence, uptime, and message types for location, emergency, health, count, image readiness, commands, and heartbeats.
- The fixed-size packet and explicit protocol validation are a good base for ESP-NOW integration. Fixed channel 1 is shared as intended.
- The checksum is only an additive corruption check, which is reasonable for accidental packet errors but provides no authenticity. Any ESP-NOW peer able to send a correctly shaped packet can issue data/command messages. For a booth demo this may be acceptable, but command authorization and sender allowlisting still matter.
- There are no explicit node-ID, station-ID, command-code, robot-state, or language enums in the shared header. Multiple firmware targets may therefore encode meanings independently, creating integration drift; this will be checked in each target.

### Robot firmware

- Robot firmware is an explicit skeleton: setup starts Serial and prints a message; the main loop is empty.
- None of the robot's core physical functions are implemented yet: motor control, line sensing/PID or rule-based steering, calibration, obstacle pause/resume, RFID station detection/debouncing, station timing, OLED, DFPlayer/audio selection, language button, direction LEDs, queue behavior, ESP-NOW setup/transmit/receive, emergency routing, heartbeat, or fail-safe stop.
- Planned wiring is still blank, so the current code cannot be safely connected to hardware without choosing and documenting pins. This is the largest critical-path gap because phases 1–3 and part of phase 5 depend on it.
- The robot README says narration is Phase 1–3 while source comments assign RFID/audio/OLED to Phase 2, slightly diverging from the canonical build plan where narration is Phase 3. This is minor but should be normalized when implementation begins.

### Hub/track firmware

- Contrary to the canonical dedicated-Hub architecture, the existing Hub target intentionally combines Hub web/API work and track LEDs/buzzer on one ESP32, split between tasks/cores. Therefore, a separate `track` target is not missing by accident; the implementation chose the build plan's documented weaker board-saving fallback.
- Hub documentation advertises a broad working slice: SoftAP, captive DNS, ESP-NOW aggregation, LittleFS static files, check-in APIs, state API, test controls, image upload, station LEDs, and emergency buzzer behavior. Source inspection/build testing must verify those claims.
- Hub configuration is concrete: SSID `MuseumHub`, password `museum2026`, channel 1, seven stations, NeoPixel GPIO15, and buzzer GPIO4.
- The hard-coded shared demo password is visible in firmware, documentation, and generated QR by design. That is operationally simple but means anyone nearby can join. More importantly, admin URLs/actions are not separated by authentication, so Wi-Fi access currently implies admin control unless source has an unadvertised guard.
- The Hub is fixed at seven LEDs/stations even though three physical stations are recommended. This is easy to configure, but currently duplicates station semantics outside the shared protocol and web data.
- Hub source substantially implements its documented prototype: mutex-protected RAM state, queued ESP-NOW receive, location/health/emergency aggregation, JSON state, check-in/approval APIs, captive DNS, LittleFS serving, NeoPixel station indication, and emergency buzzer/blink behavior.
- It is receiver-only despite registering a broadcast peer. Admin emergency and robot-advance endpoints change Hub RAM only; they do not send `MSG_COMMAND` or `MSG_EMERGENCY` to the robot or other nodes. Thus web controls can animate the dashboard/track but cannot currently control the physical system.
- `MSG_VISITOR_COUNT`, `MSG_IMAGE_READY`, `MSG_COMMAND`, and `MSG_HEARTBEAT` are ignored. Sequence numbers are ignored, queue overflow is silent, sender MAC/node identity is not checked, and stale timestamps are stored but never used to mark robot/sensors offline. Several protocol/planned dashboard capabilities therefore remain unwired.
- The state API reports a non-moving robot as `narrating` even before any robot packet arrives. Combined with UI fallback to artifact 1, a disconnected robot appears to be narrating at Jamdani rather than offline.
- `approvedCount` is cumulative approvals, not actual visitors inside: there is no checkout/exit decrement, denying a previously approved request does not decrement it, and physical IR count messages are ignored. The label “Visitors inside” is currently inaccurate.
- Check-in state is RAM-only and capped at 24 records. Overflow drops the oldest request, potentially stranding a waiting visitor; reset/reboot loses all state. IDs are short, not checked for collisions, and every action returns success even when a request does not exist.
- No HTTP endpoint is authenticated or rate-limited. Anyone on the AP can read state, approve/deny visitors, toggle emergency, advance mock state, or replace the latest image. This confirms that the advertised PIN-gated admin view is absent in both frontend and firmware.
- The image upload handler treats the raw body as a `String` and always returns success even if the body is absent or the file cannot be opened/written. Its binary/runtime behavior and size limits need an ESP32 integration test; it is not production-ready storage logic.
- `/api/state` omits last-seen/device-online details, health alert flags/status, restricted access attempts, images, language/narration detail, and visitor help requests promised in the design. It provides enough for the current mock-style pages, not the full planned dashboard.

### Health firmware

- Health firmware is an explicit skeleton with only a station-ID constant and Serial message; its loop is empty.
- DHT11 and MQ-135 pin selection, sensor libraries, sampling cadence, invalid-reading handling, filtering/averaging, ADC attenuation/range, baseline/threshold calibration, fixed-point conversion, sequence/uptime values, ESP-NOW channel/peer setup, sends/retries, heartbeat, and alert flags are all missing.
- The README describes one artifact per board while source comments allow one board wired to multiple nearby sensors. The canonical BOM appears to budget one health ESP32 for three artifact sensor pairs, so topology and analog pin allocation must be decided before implementation.
- A single-node/multiple-station design is implementable with the current protocol by sending one message per station, but it needs explicit arrays/configuration and enough ADC1 GPIOs. Three MQ-135 analog inputs plus DHT pins are feasible on a typical ESP32, subject to chosen board/pin conflicts.

### Emergency firmware

- Emergency firmware is an explicit skeleton with no pin configuration, button input, debounce/state machine, status output, Wi-Fi channel setup, ESP-NOW peer/init, broadcast, sequence/retry, or delivery feedback.
- A toggle-on-second-press design is planned. For safety and demo clarity, reset semantics need a deliberate choice: a protected reset/long press or admin acknowledgement is less error-prone than allowing any second accidental press to clear emergency.
- Broadcast is the right low-dependency topology, but ESP-NOW broadcast is not guaranteed delivery. A robust implementation should retransmit state several times and periodically while active; each receiver should make emergency processing idempotent.
- Because the Hub currently receives emergency but never sends it, implementing this small node plus receivers is essential to turn emergency from a dashboard simulation into a real system feature.

### Camera firmware

- Camera firmware is an explicit stretch-goal skeleton. It has no camera pin model/configuration, frame-buffer initialization/capture/release, Wi-Fi credentials/connection/reconnection, HTTP client/upload, station trigger, retry, status indication, or error handling.
- The trigger mechanism is not designed. The camera includes the shared ESP-NOW protocol but documentation says it joins the Hub AP as an HTTP client; source does not state whether station visits arrive over ESP-NOW, a Hub HTTP command, GPIO, or timing. This must be decided before implementation.
- Hub stores only `/last.jpg`, while the original concept/admin requirements describe latest and all images per exhibit. Current design can at most support one global latest image unless filenames/metadata/state API/UI are expanded.
- No current admin UI displays `/api/image`, despite Hub routes already existing. Camera remains genuinely optional and should not be attempted before the core robot/health/emergency path is reliable.

### QR tooling and repository-level firmware status

- The QR generator is implemented and produces Wi-Fi join, in-page token, and native `/api/enter` QR images. Its network constants duplicate Hub configuration manually, so changing one file can silently create invalid printed QRs.
- The generated in-page token is `MUSEUM-ENTRY-2026`, but the browser never compares decoded text with that token. Therefore the tooling implies validation that the app does not perform.
- QR output is generated into `tools/qr_output/`; no generated images are currently listed in the repository, which is sensible for derived assets but means generation remains a setup step.
- Repository firmware status is accurately described at a high level: Hub/track is implemented as a prototype; robot, health, emergency, and camera are skeletons. “Hub built” should not be interpreted as fully integrated or hardware-validated.

### Build configuration observations

- PlatformIO projects reference the shared header with `-I ../common`. From each project directory, `../common` correctly resolves to `firmware/common`.
- Hub dependencies declare NeoPixel and LittleFS, with ESP32 Arduino platform/core version unconstrained. The source contains compatibility handling for ESP-NOW receive callbacks across core 2.x/3.x, but an unpinned platform can still introduce future reproducibility drift.
- Robot dependencies already include RFID, DFPlayer, NeoPixel, and SSD1306, but no explicit Adafruit GFX dependency (normally transitively pulled by SSD1306) and no dedicated line-sensor library is needed. Actual implementation can begin without restructuring the PlatformIO target.
- Health dependencies already include the DHT and unified sensor libraries, so DHT11 implementation can start. MQ-135 can be read directly with the ESP32 ADC and needs no library for the planned relative trend.
- Emergency needs only built-in Wi-Fi/ESP-NOW APIs, so its empty dependency list is appropriate.
- Camera is configured for the generic `esp32cam` board and can use built-in `esp_camera`, Wi-Fi, and HTTP APIs, but a specific module pin map (commonly AI Thinker) still has to be selected in source.
- The Hub LittleFS `data/` directory contains only instructions, not the actual web assets. Therefore `pio run -t uploadfs` right now would deploy no app. The documented copy path in `data/README.txt` appears one directory too high (`../../../../webapp` from `firmware/hub/data` resolves outside the repository); the Hub README's manual copy description is clearer.
- Keeping duplicate copies of web assets under `webapp/` and `firmware/hub/data/` would create drift. An automated sync/copy target should be used before upload, or PlatformIO's data directory should be configured/generated from the canonical webapp directory.

## Verification Results

- PlatformIO Core 6.1.19 is installed on the laptop.
- A connected ESP32-compatible board was detected at `/dev/ttyUSB0` through a CH340 USB-serial adapter (`VID:PID 1A86:7523`).
- The Hub target compiled successfully and produced an approximately 831 KiB firmware image.
- The firmware was flashed to the connected board. A forced normal-mode reset produced a complete 115200-baud startup log: LittleFS mounted, SoftAP `MuseumHub` started on channel 1 at `192.168.4.1`, ESP-NOW initialized, and the Hub reported its visitor/admin URLs. This is direct evidence that the Hub application is now running on the board.
- The canonical web assets were staged and uploaded as a LittleFS image. A post-upload reset again mounted LittleFS and started all Hub services, so the board now contains both firmware and the visitor/admin application.
- QR dependencies were installed in an isolated `tools/.venv` environment. All three project QR images were generated under `tools/qr_output/`; the recommended immediate flow uses `01_wifi_join.png` followed by `03_checkin_native.png`, because `02_checkin_inpage.png` requires the absent vendored `jsQR` decoder.
- A laptop Wi-Fi scan sees `MuseumHub` on channel 1 at 100% signal with WPA2, independently confirming that the connected ESP32 is broadcasting its configured network.
- Source verification of the deployed native-QR path: scanning `/api/enter` creates a pending ID and redirects the phone to `index.html?autoid=<id>`; the gate page polls that ID every 800 ms. The admin page polls Hub state, displays pending IDs, and exposes approve/deny controls. Approval causes the phone to redirect automatically to `visitor.html?id=<id>` after 800 ms.
- Both visitor phone and admin laptop must join the ESP32's `MuseumHub` Wi-Fi. Internet is intentionally unavailable on that network; users should accept “stay connected/use without internet.” The laptop cannot remain on a different Wi-Fi through the same radio while directly opening `192.168.4.1`.
- A live HTTP smoke test was completed while the laptop was temporarily joined to `MuseumHub`, then its original Wi-Fi was restored automatically. `/`, `/admin.html`, `/visitor.html`, and `/css/style.css` all returned the expected deployed content.
- The live native-camera flow passed end to end: `/api/enter` returned a redirect with visitor ID `V7962628`; `/api/request` reported `pending`; `/api/approve` succeeded; and the next request lookup reported `approved`.
- Both live admin demo controls passed: robot advance changed the state from `nakshi` to `liberation`, and emergency changed from false to true and back to false. The final test left emergency safely off.
- Health data remained empty during the live test, which is expected because no implemented health node is transmitting. Robot movement is simulated Hub state; it does not prove a physical robot is connected.

## Immediate Implementation Scope

### Current ESP32 test baseline

- The Hub is the only target documented as an implemented hardware-testable prototype. With one ordinary ESP32, it can expose the `MuseumHub` SoftAP and HTTP API, receive ESP-NOW packets, drive a seven-pixel WS2812 strip on GPIO15, and drive an active buzzer on GPIO4.
- The Hub README defines a no-other-node smoke test: flash Hub firmware, upload the web app to LittleFS, join `MuseumHub` / `museum2026`, open the visitor and admin pages, then exercise robot-advance and emergency simulation controls. This can validate the Hub, browser flow, LED indication, and buzzer, but not a physical robot or sensors.
- The Hub's `data/` directory does not currently contain the web application, and the documented offline `jsQR` dependency is absent. A raw `uploadfs` from the repository will therefore not produce the documented complete browser test without first staging the canonical `webapp/` files; native `/api/enter` check-in does not depend on `jsQR`.
- Robot, health, emergency, and camera targets currently provide only boot/serial skeletons. They can be compiled and flashed as board/toolchain smoke tests, but their advertised peripherals and ESP-NOW behavior cannot yet be functionally tested from current source.
- The Hub target is configured for a generic `esp32dev` board at 115200 baud and a fast 921600 upload rate. It will fit common classic ESP32 DevKit boards, but a different board definition or lower upload speed may be needed if the user's exact board fails to flash.
- Safe Hub wiring from current configuration is WS2812 DIN to GPIO15 with shared ground and appropriate 5 V power, plus an active buzzer on GPIO4. The README explicitly warns to use a transistor rather than sourcing it directly when buzzer current exceeds 20 mA. Testing Wi-Fi/API behavior requires neither peripheral, so the first flash should be done with both disconnected.

### Mobile QR capture follow-up

- The user's phone confirms that the current `Scan QR` button can fail to open anything. The implementation calls `.click()` on an `<input type="file">` carrying the `hidden` attribute. Mobile browser behavior around synthetic activation of fully hidden file inputs is inconsistent.
- The compatible plain-HTTP approach remains a camera-backed file input (`accept="image/*" capture="environment"`), because live `getUserMedia()` camera streaming generally requires HTTPS. The repair should make the real file input itself cover the visible button so the tap is a direct trusted activation rather than JavaScript delegation.
- The page references `js/vendor/jsQR.js`, but that file is absent from both the canonical web app and deployed LittleFS. A repaired camera picker would otherwise take a photo and then silently treat it as a simulated check-in. The offline decoder must be bundled and the decoded value should be checked against `MUSEUM-ENTRY-2026`.
- The Hub's static-file handler streamed browser assets without cache headers. Existing phones could therefore retain the old broken page after a filesystem update. The repair adds `Cache-Control: no-store, no-cache, must-revalidate` plus legacy no-cache headers to static responses.
- The repaired control uses the real `accept="image/*" capture="environment"` file input as a transparent layer over the visible button. The user tap therefore reaches the input directly instead of going through `.click()` on a `hidden` element. This is the most compatible camera-capture mechanism available on the Hub's plain-HTTP origin.
- Official jsQR 1.4.0 was vendored from the npm package. Local functional validation decoded the generated 290×290 in-page entry QR as `MUSEUM-ENTRY-2026`; syntax checks passed for the app scripts, inline gate script, and decoder bundle.
- Image handling now uses object URLs, scales phone photos down to at most 1600 pixels on the longest side, handles image/canvas/decoder failures, and reports bilingual status. Only the project token or native `/api/enter` URL is accepted; arbitrary photographed QRs no longer initiate check-in.
- Updated Hub firmware and LittleFS images built successfully and were flashed to `/dev/ttyUSB0`. Post-flash boot again confirmed LittleFS, `MuseumHub`, ESP-NOW, and HTTP startup.
- Live deployment inspection over `MuseumHub` returned the repaired 4553-byte gate page and full 256885-byte decoder. The decoder SHA-256 matched the canonical vendored file (`bc40c8a15196236b2314db0856f72ca0b49980cd5413b8c852a7349f5fee0859`), the broken scripted trigger was absent, no-cache headers were present, and `/api/state` returned HTTP 200-equivalent valid state data.
- A phone opened inside an OS captive-portal mini-browser may still restrict file/camera inputs at the WebView level. If so, the user must open `http://192.168.4.1/` explicitly in Chrome, Safari, or Firefox; JavaScript cannot override an OS WebView restriction.

## Standalone Repository Publication Result

- The published repository is `https://github.com/ThZihan/3.2-inch-TFT-LCD-ILI9481-480X320-display`.
- The published commit on `main` is `941acb7` (full SHA `941acb7250cec9b324ba5395eb5145623110d73b`), authored by `Tahfizul Hasan Zihan`.
- The remote root now contains exactly: `LICENSE`, `README.md`, `SmilingRobotFace/`, and `THIRD_PARTY_LICENSES.md`.
- The published sketch is a single self-contained `SmilingRobotFace/SmilingRobotFace.ino` with no library dependencies. It compiles for `arduino:avr:mega` at 6,980 bytes of flash and 82 bytes of static RAM.
- The standalone sketch was uploaded to the connected Mega over `/dev/ttyACM0` and the user confirmed on-screen that the same smiling face, smooth blink, and smooth gaze matched the project firmware.
- ILI9481 initialization and the Mega 16-bit shield pin map are adapted from MCUFRIEND_kbv (FreeBSD). The standalone source includes MIT `LICENSE` and a `THIRD_PARTY_LICENSES.md` attribution file rather than copying the whole library.

## Risks and Blockers

- Rules risk: current local source documents are for 2025, not the stated 2026 season.
- Hardware validation will require the actual board set, sensors, motors, RFID tags, audio files/SD card, power rails, and track.
- There is no dedicated `track` firmware target because track LEDs/buzzer were deliberately folded into the Hub implementation. This saves a board but conflicts with the canonical dedicated-Hub reliability recommendation and should be treated as an explicit architecture deviation.
- Documentation drift is present: the concept file still says Firebase and places RFID tags under the track, while the canonical plan replaces Firebase and says RFID tags must be on top. Team members should use the build plan as source of truth and later reconcile the public-facing report.
- The recommendation log contains a stale 2025 elementary birth-year statement in one open-question section, although its later 2026 update and the build plan say 2014–2018. This should be cleaned up to prevent eligibility confusion.
- A repository-wide placeholder search confirms that the main unfinished implementation areas are openly marked rather than hidden: robot, health, emergency, and camera targets are skeletons; health thresholds/mock values and the Hub advance endpoint are test placeholders.

## Arduino Mega Display Follow-up

- The connected Mega 2560 is available at `/dev/ttyACM0`. The final project build without the voice-language badge was already rebuilt, uploaded, flash-verified, and left on the happy face.
- The physical 480x320 shield is a write-only 16-bit MCUFRIEND-style Mega shield. It reports `0xD3D3`; the proven project maps that unreadable ID to ILI9481 (`0x9481`).
- The shield wiring matches MCUFRIEND's `USE_MEGA_16BIT_SHIELD` path: LCD D15..D8 on Mega 29..22, D7..D0 on 30..37, RD 43, WR 39, RS/CD 38, CS 40, and reset 41.
- The user corrected the GitHub repository name to `3.2-inch-TFT-LCD-ILI9481-480X320-display`; the new remote URL is `https://github.com/ThZihan/3.2-inch-TFT-LCD-ILI9481-480X320-display`. This now matches the tested controller and screen size.
- The physical shield's rear label reads `HVGA 480x320 3.2\" TFT LCD Shield for Arduino Mega 2560 ILI9481 16bit bus`. The standalone sketch must retain this tested ILI9481 implementation and exact 16-bit Mega shield wiring.
- The least risky Arduino IDE conversion is to keep the proven MCUFRIEND_kbv and Adafruit GFX rendering path, place `USE_SPECIAL` and `USE_MEGA_16BIT_SHIELD` before the MCUFRIEND include in the sketch, and copy only the locked happy-face renderer into one `.ino`. This preserves the exact bus driver and rendering behavior while avoiding PlatformIO build flags.
- The standalone project should contain only the animated smiling face. It does not need the project display state machine, language argument, voice badge, station content, emergency screens, or UART command parser.
- Further Arduino-build inspection found that sketch-local preprocessor definitions do not reliably apply while the Arduino builder compiles a third-party library's separate `.cpp` translation unit. Requiring users to edit `MCUFRIEND_kbv/utility/mcufriend_shield.h` would make setup fragile. The publication will therefore keep the proven ILI9481 register sequence and exact Mega 16-bit port mapping in a small self-contained sketch, while preserving the locked face renderer unchanged in behavior.
- MCUFRIEND_kbv is FreeBSD-licensed (copyright David Prentice, 2018), and its bundled Adafruit GFX notice is BSD. The standalone source will retain attribution for the adapted ILI9481 initialization and Mega shield bus mapping; it will not claim ILI9486 support.

## Full Project Display Restoration

- Publishing and physically validating the standalone sketch temporarily replaced the Mega's full project firmware. The user now wants the robot-oriented application restored on the board.
- The main project's `firmware/display/src/main.cpp` still contains all intended screen renderers and commands: `BOOT`, `MENU`, `LANG 0`, `LANG 1`, `STATION <id> <moving>`, `EMERGENCY`, and `FACE <emotion>`.
- The locked renderer remains in `firmware/display/include/robot_face.h`. Its language argument is intentionally ignored, so restoring the full application will not restore the removed `VOICE ENGLISH` badge on the face.
- The standalone clone is clean and synchronized with `origin/main`. No standalone repository files need modification during this restoration.
- The full project target rebuilt successfully at 27,200 bytes flash and 718 bytes static RAM, then uploaded to the connected Mega on `/dev/ttyACM0`.
- USB command verification covered all major display states: `MENU`, `LANG 1`, `STATION 3 0`, `STATION 3 1`, `EMERGENCY`, `BOOT`, and `FACE HAPPY`. Each command was parsed and echoed without corruption.
- The Mega's final state is the normal smooth happy face. `currentLanguage` remains Bangla after the test, ready for later station/audio integration, but no language text appears on the face itself.
- The standalone repository remained untouched and clean during restoration.

## DFPlayer SD-card Read-only Preflight

### Device inventory at 2026-08-02 16:21 Asia/Dhaka

- The internal operating-system disk is `/dev/nvme0n1`: Samsung `MZAL4256HBJD-00BL2`, serial `S67PNF0W935415`, 256,060,514,304 bytes, NVMe transport, `RM=0`, `HOTPLUG=0`. Its GPT partitions are `/dev/nvme0n1p1` (FAT32 EFI, mounted at `/boot/efi`) and `/dev/nvme0n1p2` (ext4, mounted as `/`). It must never be selected for DFPlayer preparation.
- The only removable/USB block-device candidate is `/dev/sda`, stable identity `/dev/disk/by-id/usb-Mass_Storage_Device_121220160204-0:0`, USB VID:PID `14cd:1212`, serial `121220160204`, model `Mass Storage Device`, USB transport, `RM=1`, and `HOTPLUG=1`.
- `/dev/sda` currently reports **zero bytes** (`size=0`, sysfs sectors `0`). It has no child partition, partition table, filesystem signature, label, UUID, or mountpoint. The kernel has detected the USB mass-storage reader, but no readable card medium is exposed through it.
- Consequently, `/dev/sda` is identified with high confidence as the removable **reader path**, but the inserted SD card itself cannot yet be identified or validated by capacity/filesystem/content. It may be absent, incompletely seated, locked/faulty, incompatible with the reader, or not yet detected.
- No SD-card volume is currently mounted, so no visible content was read and there is no currently mounted volume to unmount. Existing card-content risk remains **unknown**, not zero, because the reader exposes no media from which to inventory files.
- No destructive or media-changing action was performed: no format, partition edit, erase, mount, unmount, or block-device write.

### Preparation-tool inventory

- FAT/partition inspection and preparation tools are installed: `mkfs.fat`/`mkfs.vfat` and `fsck.fat` from dosfstools 4.2; `sfdisk`, `fdisk`, `wipefs`, `lsblk`, `blkid`, and `findmnt` from util-linux 2.39.3; GNU Parted 3.6; plus `partprobe`, `udisksctl`, `mount`, `umount`, `eject`, and `sync`.
- Audio inspection/transcoding is ready: FFmpeg/ffprobe 6.1.1 is installed and exposes both `libmp3lame` and `libshine` MP3 encoders plus MP3 decoders. `mediainfo` and `sha256sum` are also installed.
- FFmpeg includes `sine`, `anoisesrc`, and the English-only Flite speech source. Standalone `espeak`, `espeak-ng`, Pico TTS, and Flite executables are absent. The bundled Flite path is suitable for original English placeholder speech but does not provide a Bangla voice.
- Download clients `curl` 8.5.0 and Wget 1.21.4 are installed. No content was downloaded in this subtask.
- Optional standalone audio/tag utilities `lame`, `mpg123`, SoX, `id3v2`, Kid3 CLI, and ExifTool are absent, but they are not required because FFmpeg/ffprobe can perform and validate the planned MP3 conversion.

### Project audio scope recovered from source

- The canonical seven-station order is consistent across the Hub, health-node documentation, display firmware, and browser data: 1 Jamdani Saree; 2 Nakshi Kantha; 3 Liberation War Artifacts; 4 Terracotta Plaques; 5 Ektara; 6 Palki (Palanquin); 7 Royal Bengal Tiger.
- The project explicitly requires two narration languages, English and Bangla. The implementation convention already uses language value `0` for English and `1` for Bangla.
- Existing browser descriptions provide a short draft narration for each artifact in both languages, but they are content references rather than approved scripts. The Bangla source contains at least two known likely errors (`ঐতিতাসিক` and `একটানা`), so the team must proofread and culturally review all 14 scripts before recording.
- The canonical build plan prefers the children/team recording their own bilingual narration. This is both a strong provenance strategy and better aligned with the competition presentation than scraping third-party speech clips.

### Official DFPlayer constraints and conservative interpretation

- DFRobot's official DFR0299 page states that the DFPlayer Mini fully supports FAT16/FAT32 and TF cards only up to 32 GB. It lists sample rates from 8 through 48 kHz, up to 100 folders, and up to 255 songs per folder. It advises against hot-plugging because of malfunction/corruption risk.
- DFRobot's official full-function example requires a root-level folder named exactly `mp3` and four-digit numeric filename prefixes such as `/mp3/0001.mp3`. The library's `playMp3Folder(4)` example resolves to `/MP3/0004.mp3`.
- The same official example warns that generic `play(1)` follows filesystem **copy order**, not necessarily lexical filename order, and that macOS `._` metadata files can be mistaken for tracks. For this project, firmware should call `playMp3Folder(trackNumber)` and the card should contain only the intended files—no desktop metadata, playlists, artwork, or unrelated audio.
- The most conservative target is therefore a genuine 8 or 16 GB microSD card, MBR/DOS partition table, one aligned primary FAT32 partition, and no exFAT/GPT. A 32 GB card is within DFRobot's published maximum but offers no project benefit for 14 short narrations and is less conservative than the build plan's 8/16 GB recommendation.
- Source provenance for these hardware facts: DFRobot DFR0299 product/wiki page and its official “Example Code for Arduino-FullFunction,” both inspected read-only on 2026-08-02. No instructions embedded in external content were executed.

### Defensible audio-rights strategy

- First choice for the final 14 narrations: scripts written/proofread by the team and voices recorded by the team. Record script authorship, speaker, recording date, recorder/operator, edits, and source/final SHA-256 hashes. Because likely speakers are children, obtain documented parent/guardian consent and permission covering recording, editing, exhibition playback, and any repository/public distribution.
- Keep ownership/permission claims precise. Team-created recordings may be described as “original Smart Museum recordings used with contributor/guardian permission.” Do **not** call them public domain or CC0 unless every relevant rightsholder has validly made that dedication; voice/personality and performer-related rights may remain even when copyright is addressed.
- For hardware-only placeholder tests, generate simple, distinguishable tones locally with FFmpeg. Preserve the exact generation recipe, tool version, creation date, author/operator, and hashes. Describe these as “project-generated test tones,” not automatically as “license-free” or “public domain.” The project can intentionally apply CC0 to its own generated assets if the actual rightsholder chooses.
- If an external sound is truly needed, prefer an asset explicitly offered by its rightsholder under CC0 1.0, or a work whose worldwide public-domain status is documented by a reliable primary institution. Preserve creator/title, canonical source URL, download date, exact rights statement/license version, a local copy of the license/rights evidence, original hash, edits/conversion recipe, and final hash.
- Creative Commons explains that CC0 is a rightsholder-applied legal tool intended to waive copyright/related rights as fully as possible; only a rightsholder or authorized party may apply it. A Public Domain Mark is informational and should be used only where worldwide public-domain status is reliably established. These distinctions matter for Bangladeshi/international competition use.
- “Royalty-free” normally describes a payment/licensing arrangement; it does not mean no copyright, no conditions, CC0, or public domain. Never relabel stock-library, YouTube, or TTS-provider audio as “license-free.” Review and preserve the actual terms, including attribution, redistribution, derivative, noncommercial, and voice-model/output restrictions.
- Local FFmpeg Flite can produce English placeholder speech, but no Bangla voice is present and the engine/voice-data/output rights would need a separate documented review before distribution. Do not use an online TTS service until its current output and voice terms have been checked and archived.

### Proposed deterministic track map

- Use exactly 14 narration tracks in one root-level `/mp3` directory. Keep filenames ASCII and exactly four decimal digits plus `.mp3`; do not append descriptive suffixes on the deployed card even though DFRobot permits them. Keep a human-readable manifest in the project workspace rather than on the card.
- Reserve `0001`–`0007` for English and `0008`–`0014` for Bangla. Firmware mapping is simple and heap-free: for station IDs 1–7, English track = `stationId`; Bangla track = `stationId + 7`; call the library's deterministic MP3-folder playback API rather than global copy-order playback.

| Track | Language | Station | Artifact |
|---:|---|---:|---|
| `0001.mp3` | English | 1 | Jamdani Saree |
| `0002.mp3` | English | 2 | Nakshi Kantha |
| `0003.mp3` | English | 3 | Liberation War Artifacts |
| `0004.mp3` | English | 4 | Terracotta Plaques |
| `0005.mp3` | English | 5 | Ektara |
| `0006.mp3` | English | 6 | Palki (Palanquin) |
| `0007.mp3` | English | 7 | Royal Bengal Tiger |
| `0008.mp3` | Bangla | 1 | Jamdani Saree |
| `0009.mp3` | Bangla | 2 | Nakshi Kantha |
| `0010.mp3` | Bangla | 3 | Liberation War Artifacts |
| `0011.mp3` | Bangla | 4 | Terracotta Plaques |
| `0012.mp3` | Bangla | 5 | Ektara |
| `0013.mp3` | Bangla | 6 | Palki (Palanquin) |
| `0014.mp3` | Bangla | 7 | Royal Bengal Tiger |

- Normalize final narration files to a deliberately plain profile: MP3, MPEG-1 Layer III, constant 128 kb/s, 44.1 kHz, mono, no cover art, no chapters, and preferably no metadata tags. DFRobot advertises 44.1 kHz support; CBR/mono minimizes decoder and level surprises. Use FFmpeg's `libmp3lame`, then verify every output with ffprobe for codec, sample rate, channel count, bit rate/duration, and absence of attached pictures.
- Before card copying, stage all 14 files together, verify names are unique/contiguous, record SHA-256 hashes and durations, and review loudness/listenability. Copy the empty `mp3` directory's files in numeric order in one operation; use `playMp3Folder()` so mapping does not rely on directory-entry order.
- Do not place provenance or license files on the deployed DFPlayer card because strict “audio only” contents reduce indexing hazards. Store the scripts, consent records (privately), rights manifest, source licenses/evidence, conversion log, and hashes in the project/archive outside the card.

### Destructive preparation plan — commands documented, not executed

- This plan remains conditional on a new read-only inventory showing a nonzero-capacity removable card and the user explicitly confirming its exact path, stable identity, capacity/model, and acceptance that all content will be erased.
- Re-identify immediately before acting with `lsblk`, `udevadm`, `findmnt`, and `blkid`; abort if the expected stable identity/capacity changed, if the target is zero bytes, if any target partition is mounted unexpectedly, or if the target is the NVMe system disk.
- Inventory the mounted card's existing top-level content and summarize backup risk before requesting final erase confirmation. Never inspect or write through the whole-disk reader while it reports zero media.
- After confirmation only: unmount target partitions cleanly if mounted; create a new DOS/MBR partition table and one primary FAT32-type partition aligned at 1 MiB and spanning the card; ask the kernel to reread the table; format the **partition** (expected `/dev/sda1`, not `/dev/sda`) as FAT32 with volume label `MUSEUM_AUDIO`; run a read-only FAT check; mount through the desktop-safe mechanism; create `/mp3`; copy only the 14 validated files; sync; verify file names, hashes and ffprobe properties from the mounted card; cleanly unmount/eject; then test all 14 tracks on the actual DFPlayer with power off before card insertion/removal.
- Recommended implementation tools after confirmation: scripted noninteractive `sfdisk` for an auditable single MBR partition, `mkfs.fat -F 32 -n MUSEUM_AUDIO` on the resulting partition, `fsck.fat -n` for read-only validation, and `udisksctl` for user-level mount/unmount. Exact sector values must be derived from the card's then-current nonzero size; no fixed endpoint should be guessed now.

### Exact confirmation gate for the parent task

- Current status does **not** permit an erase-confirmation request yet: `/dev/sda` is only a zero-byte reader endpoint. The user should first reseat/insert the intended 8 or 16 GB microSD card in the reader. The parent must then repeat the full read-only identity, filesystem, mount, and content-risk inventory.
- Once the reader exposes real media, the confirmation must quote the freshly observed values and request an unambiguous response in this form: **“ERASE AND PREPARE `<exact device path>` — `<stable by-id identity>` — `<exact decimal and human-readable capacity>` — `<model/serial if exposed>`. I understand every partition and file on that device will be permanently erased. Do not touch `/dev/nvme0n1`.”**
- The parent must not accept a generic “yes,” a confirmation naming only `/dev/sda` without the refreshed stable identity/capacity, or any confirmation issued while size remains zero. Device paths can be reassigned after unplug/replug; recheck immediately after confirmation and abort on any mismatch.
- At this inspection's final read-only recheck, facts were unchanged: `/dev/sda` remained USB/removable/hot-pluggable but zero bytes, with stable by-id `usb-Mass_Storage_Device_121220160204-0:0`, no children/signatures/mounts; `/dev/nvme0n1` remained the mounted 256,060,514,304-byte Samsung OS disk.

## DFPlayer SD-card Re-inspection After Reader Swap (2026-08-02 16:35–16:56 Asia/Dhaka)

### Scope and method
- Read-only only. No format, partition, unmount, mount, erase, download, or write was performed on any block device. The SD card was not mounted because it is not mountable (see below).
- Device names were treated as potentially reassigned after the reader swap; nothing was assumed from the prior session. The inventory was rebuilt from scratch with `lsblk`, `/dev/disk/by-*`, `udevadm`, `findmnt`, `/proc/mounts`, `journalctl`, and (via a one-time polkit elevation the user approved) `parted`, `fdisk`, `sfdisk`, `blkid -p`, `wipefs`, and `file -s`.

### Removable medium now present but physically unreadable
- The only removable/USB block device is `/dev/sda`, stable by-id `/dev/disk/by-id/usb-Mass_Storage_Device_121220160204-0:0`, by-path `pci-0000:04:00.4-usb-0:1:1.0-scsi-0:0:0:0`. USB VID:PID `14cd:1212` (lsusb: `Super Top microSD card reader (SY-T18)`), USB serial `121220160204`, model `Mass Storage Device` (udev `ID_MODEL=Storage_Device`), revision `1.00`, `RM=1`, `HOTPLUG=1`, `RO=0`. Reader unchanged; the swap exposed a card where there was none before.
- `/dev/sda` now reports **nonzero capacity**: `lsblk --bytes` size `15634268160`; sysfs `/sys/block/sda/size` = `30535680` sectors × 512 B hw/logical sector size = 15,634,268,160 bytes. Human: ~14.6 GiB (lsblk) / ~15.6 GB (decimal). parted corroborates: `Disk /dev/sda: 30535680s`, `Sector size (logical/physical): 512B/512B`.
- **No valid partition table, filesystem, label, UUID, partition, or mountpoint is exposed.** `parted` reports `Partition Table: unknown`, `unrecognised disk label`, no disk flags, no partitions. `lsblk` shows no children, no FSTYPE/LABEL/UUID. `blkid -p` failed (exit 2). `/dev/disk/by-partlabel`, `by-label`, and `by-uuid` contain only the NVMe entries; no `sda` symlink appears there. `findmnt` and `/proc/mounts` show no `sda` mount anywhere.
- **Block reads fail at sector 0 with a hardware error.** Even as root: `fdisk`/`sfdisk` return `cannot open /dev/sda: Input/output error`. `journalctl` shows repeated `critical medium error, dev sda, sector 0 op 0x0:(READ)`, `Sense Key : Medium Error [current]`, `Add. Sense: Unrecovered read error`, `CDB: Read(10) 28 00 00 00 00 00 00 00 08 00`, `Buffer I/O error on dev sda, logical block 0, async page read`, `sda: unable to read partition table`, and `Dev sda: unable to read RDB block 0`. A read at logical block 3 (sector 24) failed the same way. The reader successfully answers SCSI READ CAPACITY (hence the reported size) but every actual data read returns a Medium Error.

### Distinction from the internal OS disk
- `/dev/nvme0n1` is unchanged and is **not** the SD card: Samsung `MZAL4256HBJD-00BL2`, serial `S67PNF0W935415`, NVMe transport, 256,060,514,304 bytes, `RM=0`, `HOTPLUG=0`, GPT with `/dev/nvme0n1p1` (vfat/FAT32, UUID `7CCD-F1DB`, mounted `/boot/efi`) and `/dev/nvme0n1p2` (ext4, UUID `8677ea8e-af72-46a7-bc9e-491e6eb2a78d`, mounted `/`). It must never be selected for DFPlayer preparation.
- `/dev/sda` is USB-attached, removable, hot-pluggable, ~15.6 GB decimal, no partitions/filesystem/mount, and physically unreadable. The two share no stable identifier and are trivially distinguished by transport (`usb` vs `nvme`), size, model, serial, and mount state.

### Existing-data / backup risk assessment
- **Existing card content cannot be inventoried and its recoverability is doubtful.** Because sector 0 (and at least sector 24) return unrecovered read errors, no partition table, filesystem superblock, file listing, or file content can be read from this card through this reader on this host. There is no mounted volume and therefore no visible top-level contents to report.
- Interpretation: the kernel sees a card of ~15.6 GB, but the flash is returning hard read errors starting at the very first sector. This is consistent with a damaged/failed/faulty SD card (bad flash, controller failure, or severe corruption at the metadata sectors), not merely an unformatted card. A truly blank-but-healthy card would still let the host read sector 0 (it would just be zeroes/garbage without a signature); here the read itself fails at the hardware level.
- Backup recommendation: if the user believes this card ever held valuable data, **do not attempt writes/erase/format** on it (writes cannot rescue read-failed media and may stress it further). Before any destructive step, try to recover readable regions with a sector-by-sector imaging tool (e.g., `ddrescue`) to a file on the NVMe — but only after the user explicitly authorizes it, and only if a different reader or cleaned contacts first demonstrate that reads can succeed. If no valuable data is believed to exist, the safe and correct action is to discard this card and use a known-good 8 or 16 GB microSD for the DFPlayer.

### Erase-confirmation status — still NOT permitted
- The confirmation gate from the prior session remains unsatisfied and is now blocked for a different reason: the medium is physically unreadable. A nonzero reported size is **not** sufficient evidence that a card is real, healthy, or the intended target.
- Hard preconditions that must all hold before an erase-confirmation request can even be formulated: (a) the device is removable/USB and not `/dev/nvme0n1`; (b) `parted`/`fdisk`/`blkid -p` can read it without I/O errors; (c) sector 0 is readable; (d) a partition table and/or filesystem signature is observable; and (e) capacity/model/serial/partition layout are freshly re-read immediately before confirmation.
- Because `/dev/sda` currently fails (b) and (c) and has no (d), **no exact erase-confirmation sentence should be issued yet.** Proceeding to erase a Medium-Error card would at best fail and at worst target the wrong device after a path reassignment.

### Exact erase-confirmation sentence template (to be used ONLY once the card reads cleanly)
- When a subsequent read-only inventory shows a healthy, readable, non-NVMe removable card, the confirmation must quote the then-fresh values in this exact form and require an unambiguous user reply: **"ERASE AND PREPARE `<exact device path>` — `<stable by-id identity>` — `<exact decimal and human-readable capacity>` — `<model / serial if exposed>` — `<observed partition table / partitions / filesystem / label>` . I understand every partition and file on that device will be permanently erased. Do not touch `/dev/nvme0n1`."**
- Drafted with current (failing) values for reference only — **must not be used as-is** because reads fail: *"ERASE AND PREPARE `/dev/sda` — `usb-Mass_Storage_Device_121220160204-0:0` — `15,634,268,160 bytes (~14.6 GiB / ~15.6 GB)` — `Mass Storage Device / serial 121220160204` — `no readable partition table or filesystem (Medium Error at sector 0)` . …"* This is deliberately invalid because the preconditions above are not met.

### Recommendation for the user (read-only conclusion)
- The SD card currently in the `Super Top microSD card reader (SY-T18)` is **not OK**: it announces ~15.6 GB to the host but returns SCSI Medium Errors on every block read from sector 0, so it has no readable partition table, filesystem, label, UUID, or files. Treat it as damaged/unreadable until proven otherwise. Try, in order: (1) remove and reseat the card, clean its contacts, and try a known-good microSD in the same reader to isolate card vs reader; (2) if a different card reads fine, this card is faulty — replace it with a genuine 8 or 16 GB card; (3) only once a card reads cleanly (sector 0 readable, partition table/filesystem visible) should the parent request exact erase confirmation and proceed with DFPlayer FAT32 preparation. No destructive or write action was taken in this subtask.

## DFPlayer SD-card Re-inspection After Replacement Card (2026-08-02 16:45–16:47 Asia/Dhaka)

### Scope and method
- Strictly read-only. No format, partition, unmount, mount, erase, download, or write was performed on any block device. Only the three root planning Markdown files may be changed.
- Device names were treated as potentially reassigned after the card swap; nothing was assumed from the prior session. The inventory was rebuilt from scratch with `lsblk`, `udevadm`, `/dev/disk/by-*`, `findmnt`, `/proc/mounts`, `lsusb`, sysfs, and `journalctl`. The kernel log is the authoritative evidence for the failure mode and the announced-but-collapsed capacity.

### A different card was inserted, then proved unreadable
- The user removed the prior 15.6 GB Medium-Error card and connected a replacement 16 GB microSD in the same `Super Top microSD card reader (SY-T18)` reader. This is a **genuinely different card**, proven by the kernel log line `sda: detected capacity change from 31207424 to 0`: the new card briefly announced **31,207,424 sectors × 512 B = 15,978,204,928 bytes (~14.9 GiB / ~16.0 GB decimal)** at enumeration, which is distinct from the prior failed card's 30,535,680 sectors (15,634,268,160 bytes). The reader itself (USB VID:PID `14cd:1212`, serial `121220160204`, model `Mass Storage Device`) is unchanged.
- After enumeration, the kernel attempted reads and they all failed with the **same hardware failure mode** as the prior card: repeated `critical medium error, dev sda, sector 0 op 0x0:(READ)`, `Sense Key : Medium Error [current]`, `Add. Sense: Unrecovered read error`, `CDB: Read(10) 28 00 ...`, `Buffer I/O error on dev sda, logical block 0, async page read`, a read at sector 24 (logical block 3) failing the same way, `Dev sda: unable to read RDB block 0`, and `sda: unable to read partition table`. The kernel then collapsed the reported size to 0 (`detected capacity change from 31207424 to 0`).
- Current authoritative state: `/sys/block/sda/size` = **0**; `lsblk --bytes` size **0**; logical/physical/hw sector size 512/512/512; `RM=1`, `HOTPLUG=1`, `RO=0`; no child partitions (`holders`/`slaves` empty); no FSTYPE/LABEL/UUID/MOUNTPOINT. `/dev/disk/by-partlabel` and `/dev/disk/by-label` do not exist; `/dev/disk/by-uuid` and `/dev/disk/by-id` contain only NVMe entries plus the reader's `usb-Mass_Storage_Device_121220160204-0:0` whole-disk node (no `sda1`/`-part1` child). No `sda` mount exists in `findmnt` or `/proc/mounts`.
- A single non-looping, non-retrying userspace read of sector 0 (`dd if=/dev/sda of=/dev/null bs=512 count=1 iflag=direct`) returned `Permission denied` because this run was non-root. This is moot: the kernel's own block-layer reads already failed the same sector 0 with `Sense Key : Medium Error` / `Unrecovered read error` dozens of times, which is strictly more authoritative than any userspace `dd`. Root escalation would only reproduce the same hardware Medium Error and adds no new information, so it was deliberately not requested.

### Distinction from the internal OS disk (unchanged, decisive)
- `/dev/nvme0n1` is **not** the SD card and is unchanged: Samsung `MZAL4256HBJD-00BL2`, serial `S67PNF0W935415`, NVMe transport, sysfs size 500,118,192 sectors × 512 = 256,060,514,304 bytes, `RM=0`, `HOTPLUG=0`, GPT with `/dev/nvme0n1p1` (vfat/FAT32, UUID `7CCD-F1DB`, mounted `/boot/efi`) and `/dev/nvme0n1p2` (ext4, UUID `8677ea8e-af72-46a7-bc9e-491e6eb2a78d`, mounted `/`). It must never be selected for DFPlayer preparation.
- `/dev/sda` is USB-attached, removable, hot-pluggable, same Super Top SY-T18 reader, currently 0 bytes after the Medium-Error collapse, no partitions/filesystem/mount. The two share no stable identifier and are trivially distinguished by transport (`usb` vs `nvme`), model, serial, size, partition layout, and mount state.

### Existing-data / backup risk assessment
- **Existing card content cannot be inventoried and recoverability is doubtful.** Because sector 0 (and sector 24) return unrecovered read errors, no partition table, filesystem superblock, file listing, or file content can be read from this card through this reader on this host. There is no mounted volume and therefore no visible top-level contents to report (and the card was not mounted — it is not mountable).
- Two consecutive distinct cards failing with the identical sector-0 SCSI Medium Error in the same reader raises the probability that the **reader** (Super Top SY-T18, VID:PID `14cd:1212`) — not just the cards — is the common-cause fault, or that both cards are damaged. This cannot be isolated further without trying a known-good microSD in this reader or trying the same cards in a different reader.
- Backup recommendation unchanged from the prior session: do not attempt writes/erase/format on read-failed media (writes cannot rescue read-failed flash and may stress it further). If valuable data is believed to exist, sector imaging (e.g., `ddrescue`) to the NVMe is the only recovery option, and only after the user explicitly authorizes it and after reads can succeed in a known-good configuration. If no valuable data exists, discard/replace the faulty medium.

### Erase-confirmation status — still NOT permitted
- The destructive confirmation gate remains **closed**. The preconditions documented in the prior session all still fail: `/dev/sda` is not cleanly readable by `parted`/`fdisk`/`blkid`/`sfdisk` (they would return Input/output error), sector 0 is not readable (kernel Medium Error), no partition table or filesystem signature is observable, and the size has collapsed to 0. A nonzero announced size alone was never sufficient, and the current size is not even nonzero.
- No exact erase-confirmation sentence is issued. The card must read cleanly (sector 0 readable, partition table/filesystem visible, fresh nonzero capacity) before any confirmation request can be formulated.

### Recommendation for the user (read-only conclusion)
- The replacement 16 GB microSD is **not readable** in the current setup: it briefly announced ~16.0 GB at insertion but every block read fails at sector 0 with a SCSI Medium Error, and the kernel has collapsed the size to 0. Because this is the **second distinct card** failing with the identical sector-0 error in the same reader, the reader is now a strong suspect. Recommended next steps, in order: (1) try a **known-good** microSD in this same `Super Top SY-T18` reader — if it also fails sector-0, the reader is faulty and must be replaced; (2) if a known-good card reads fine here, both project cards are damaged and a genuine new 8 or 16 GB microSD should be used; (3) alternatively, retry the same cards in a **different** USB SD reader to isolate card vs reader; (4) only once a card reads cleanly (sector 0 readable, partition table/filesystem visible, fresh nonzero capacity) should the parent request exact erase confirmation and proceed with DFPlayer FAT32 preparation. No destructive or write action was taken in this subtask.

## DFPlayer SD-card Re-inspection: Two Cards Connected Simultaneously (2026-08-02 16:54–17:00 Asia/Dhaka)

### Scope and method
- Strictly read-only. No format, partition, unmount, mount, erase, download, or write was performed on any block device. The two cards were already auto-mounted by the desktop when this session began; this subtask did not mount or unmount anything. Only the three root planning Markdown files were changed.
- Device names were treated as potentially reassigned; nothing was assumed from prior sessions. The inventory was rebuilt from scratch with `lsblk`, `lsusb`/`lsusb -t`, `/dev/disk/by-*`, `udevadm`, sysfs, `/proc/partitions`, `findmnt`, `/proc/mounts`, and `journalctl`.
- Raw userspace block reads (`dd`, `fdisk`, `file -s`, `fsck.fat`, `fatlabel` directly on the device node) returned `Permission denied` because this run is non-root and the device nodes are root:`disk` (mode 0660). Passwordless `sudo` was unavailable (`sudo: a password is required`). This is categorically **different** from the prior sessions' `Input/output error`/`Medium Error`: here it is a userspace permission gate, not a hardware read failure. The user offered to enable passwordless sudo; the evidence was already conclusive without it, so it was declined.

### Inventory: THREE block disks, two of them removable
- `/dev/nvme0n1` — nvme (PCIe 01:00.0), SAMSUNG MZAL4256HBJD-00BL2 / serial S67PNF0W935415, 256,060,514,304 bytes (500,118,192 × 512), RM=0, RO=0, removable=0, GPT, `nvme0n1p1` FAT32 `/boot/efi` + `nvme0n1p2` ext4 `/`. **Internal OS disk, never touch.**
- `/dev/sda` — usb (04:00.3 port 1.2), Super Top SY-T18 "Mass Storage Device" / serial 121220160204, VID:PID `14cd:1212`, 15,978,201,088 bytes (31,207,424 × 512), RM=1, RO=0, removable=1, DOS/MBR table, one partition `sda1` vfat UUID `0403-0201` mounted at `/media/user/0403-0201`.
- `/dev/sdb` — usb (04:00.4 port 2.1), Generic "Mass-Storage" / no serial, VID:PID `1908:0226`, 15,634,268,160 bytes (30,535,680 × 512), RM=1, RO=0, removable=1, GPT, four partitions: `sdb1` iso9660 `Ubuntu_24.04.4_LTS_amd64` → `/media/user/Ubuntu 24.04.4 LTS amd64`; `sdb2` vfat FAT12 `ESP` UUID `73B7-988D`; `sdb3` gap; `sdb4` ext4 `writable` UUID `05c2d1ff-d3b3-4602-b3d4-d2d10a22a5a5` → `/media/user/writable`.
- The two removable devices are **genuinely distinct readers** (different VID:PID, vendor/model strings, USB ports/controllers, by-id stable names, partition schemes, and capacities) — not the same reader twice.
- The internal NVMe is unchanged and trivially distinguished by transport/size/model/serial/`removable=0`/mount-state.

### `/dev/sda` — former-suspect Super Top SY-T18 reader: NOW HEALTHY/READABLE
- Kernel log for the current enumeration (16:54:43–16:54:44) is **clean**: `[sda] 31207424 512-byte logical blocks (16.0 GB/14.9 GiB)`, `Write Protect is off`, `Assuming drive cache: write through`, `sda: sda1`, `Attached SCSI removable disk`. There are **no** `Medium Error`/`Unrecovered read error`/`Buffer I/O error`/`unable to read partition table` lines after 16:54:40.
- The block/FS layers confirm readability: udev reports `ID_PART_TABLE_TYPE=dos`; `sda1` = `ID_FS_TYPE=vfat`, `ID_FS_UUID=0403-0201`, `ID_PART_ENTRY_TYPE=0xc` (W95 FAT32 LBA), start sector 8192 (4 MiB aligned), size 31,199,232 sectors (15,974,006,784 bytes). The vfat driver parsed the FAT and auto-mounted it; `ls`, `stat`, and `df` succeed (size 15 GiB, used 32 KiB, avail 15 GiB).
- Top-level contents (read-only): the vfat root contains only `.` and `..` — i.e., effectively **empty** (32 KiB used by the empty root directory). No files at risk.
- Classification: **healthy/readable**. The earlier failures in this same reader were therefore caused by the *cards* in it at those times (or their seating/contacts), not by the reader — the reader-suspect hypothesis from Phase 16 is **disproven** for this reader.

### `/dev/sdb` — distinct Generic reader (1908:0226): HEALTHY/READABLE, holds an Ubuntu live USB
- Transient USB instability preceded this session: the `1908:0226` device failed enumeration several times with `error -71` (EPROTO) on 16:07–16:08 and 16:33, and at 16:53:59–16:54:37 it connected/disconnected twice (devices 26→27) without stabilizing. It finally enumerated stably as device 28 at **16:58:02** and has stayed clean since.
- Kernel log at 16:58:02–16:58:04 is **clean**: `[sdb] 30535680 512-byte logical blocks (15.6 GB/14.6 GiB)`, `sdb: sdb1 sdb2 sdb3 sdb4`, then `EXT4-fs (sdb4): recovery complete` and `mounted filesystem 05c2d1ff-… r/w with ordered data mode`. Successful ext4 journal recovery is only possible if **every block of that filesystem reads without error** — stronger proof of readability than any single-sector `dd`.
- Partition layout (GPT): `sdb1` iso9660 "Ubuntu_24.04.4_LTS_amd64" (preparer XORRISO-1.5.4), start 64, 6.65 GB, 100% full; `sdb2` vfat FAT12 "ESP", start 12,988,432; `sdb3` gap (600 sectors); `sdb4` ext4 "writable", start 13,000,704, 8.98 GB, 18 MiB used.
- Top-level contents (read-only): `sdb1` is a complete **Ubuntu 24.04.4 LTS "Noble Numbat" Release amd64 (20260210)** live ISO (`.disk/info`, `boot.catalog`, `casper/`, `dists/`, `pool/`, `md5sum.txt`, `EFI/`). `sdb4` ("writable") contains `install-logs-2026-06-25.0/.1/.2/.3/` and `lost+found` — a **persistent Ubuntu installer USB** that has been booted/installed from. Meaningful, not-easily-replaceable content.
- Classification: **healthy/readable** (filesystem intact, not damaged). But its content is an OS installer + install logs, making it a **poor and risky** DFPlayer erase candidate.

### Reader-suspect resolution
- The Phase 15/16 hypothesis that the `Super Top SY-T18` (`14cd:1212`) reader was a common-cause fault is now **disproven**: that exact reader, with a different (healthy) card, enumerates and reads cleanly with zero Medium Errors.
- The `1908:0226` Generic reader is a **different** reader that had USB-physical-layer enumeration flakiness (`error -71`/connect-disconnect) but eventually stabilized and also reads cleanly. Its earlier failures were EPROTO-class, not SCSI Medium Errors.
- Most likely explanation for the earlier sessions: the two earlier cards were themselves faulty/damaged or poorly seated; today's two cards read cleanly in their respective readers.

### Destructive-confirmation-gate status
- `/dev/sda`: **preconditions satisfied** — removable/USB, not the NVMe, cleanly readable by the kernel/FS, sector 0 readable (kernel parsed MBR), valid partition table/filesystem observable, fresh nonzero capacity, empty (no content at risk). Valid DFPlayer FAT32 erase/prepare candidate, pending the user's exact confirmation.
- `/dev/sdb`: preconditions technically satisfied (readable, valid FS) but it is a **poor** candidate because it holds a complete Ubuntu 24.04.4 installer + persistent `writable` ext4 with install logs. Should not be chosen unless the user explicitly confirms they do not need the Ubuntu image.
- `/dev/nvme0n1`: never a candidate.
- **Recommended candidate: `/dev/sda`** (the empty 16 GB vfat in the SY-T18 reader).

### Exact confirmation sentence (ready to issue for `/dev/sda` only, once the user approves)
- "ERASE AND PREPARE `/dev/sda` — `usb-Mass_Storage_Device_121220160204-0:0` — `15,978,201,088 bytes (31,207,424 × 512 B; ~14.9 GiB / ~16.0 GB)` — `Super Top SY-T18 / Mass Storage Device, serial 121220160204` — `DOS/MBR table, one partition sda1 vfat UUID 0403-0201 (empty, 32 KiB used)` — currently mounted at `/media/user/0403-0201`. I understand every partition and file on that device will be permanently erased. Do not touch `/dev/nvme0n1` or `/dev/sdb`." — To be issued only after the user explicitly confirms; device path/identity must be re-read immediately before acting.

## DFPlayer SD-card Fresh Read-only Preflight: "erase and prepare both" (2026-08-02 ~19:48 Asia/Dhaka)

### Scope, method, and what the user asked
- The user said "erase and prepare **both**", referring to both removable cards including the Ubuntu installer card. This was issued several hours after the Phase 17 inventory. It is a generic, unscoped phrase and is **not** a valid destructive confirmation (see the confirmation-gate analysis below). No destructive action was taken; this subtask is strictly read-only.
- No block device was formatted, partitioned, erased, unmounted, mounted (the desktop had already auto-mounted the cards before this session; nothing was mounted or unmounted by this subtask), repaired, downloaded to, or written. Only the three root planning Markdown files were changed.
- All prior device identities were treated as stale (several hours elapsed; paths/readers/mounts/content may change). The inventory was rebuilt from scratch with `lsblk --bytes`, `/proc/partitions`, `/dev/disk/by-id`, `/dev/disk/by-path`, `lsusb`/`lsusb -t`, `udevadm`, sysfs (`/sys/block/*`), `findmnt`, `df`, `ls`/`du`/`stat` on mountpoints, `blkid` (non-root), and `journalctl`/`dmesg`. Raw userspace reads on the 0660 device nodes returned `Permission denied` (non-root; passwordless sudo unavailable and not requested) — the kernel/FS-layer evidence is authoritative and conclusive, exactly as in prior sessions.

### Fresh inventory: THREE disks, two removable, both readers/cards UNCHANGED from Phase 17

| Device | Kernel path | Stable by-id | Stable by-path | Transport / VID:PID / serial / model / rev | Bytes (sysfs sectors × 512) | RM/RO/removable | Partition table | Partitions (type/label/uuid/mount) |
|---|---|---|---|---|---|---|---|---|
| Internal OS disk | `/dev/nvme0n1` | `nvme-SAMSUNG_MZAL4256HBJD-00BL2_S67PNF0W935415` | `pci-0000:01:00.0-nvme-1` | nvme (PCI 01:00.0) / SAMSUNG MZAL4256HBJD-00BL2 / serial S67PNF0W935415 | 256,060,514,304 (500,118,192 × 512) | RM=0, RO=0, removable=0 | GPT | `nvme0n1p1` vfat FAT32 UUID `7CCD-F1DB` → `/boot/efi`; `nvme0n1p2` ext4 UUID `8677ea8e-af72-46a7-bc9e-491e6eb2a78d` → `/` |
| Card 1 (former suspect) | `/dev/sda` | `usb-Mass_Storage_Device_121220160204-0:0` | `pci-0000:04:00.3-usb-0:1.2:1.0-scsi-0:0:0:0` | usb `14cd:1212` Super Top SY-T18 / model `Mass Storage Device` / serial `121220160204` / rev `1.00` | 15,978,201,088 (31,207,424 × 512) | RM=1, RO=0, removable=1 | DOS/MBR (`ID_PART_TABLE_TYPE=dos`) | `sda1`: vfat FAT32 UUID `0403-0201`, PARTTYPE `0xc` (W95 FAT32 LBA), start 8192 (1 MiB aligned... actually 4 MiB), size 31,199,232 sect → `/media/user/0403-0201` |
| Card 2 (Ubuntu installer) | `/dev/sdb` | `usb-Generic_Mass-Storage-0:0` | `pci-0000:04:00.4-usb-0:2:1.0-scsi-0:0:0:0` | usb `1908:0226` GEMBIRD "MicroSD Card Reader/Writer" / model `Mass-Storage` / no serial / rev `1.11` | 15,634,268,160 (30,535,680 × 512) | RM=1, RO=0, removable=1 | GPT (`ID_PART_TABLE_TYPE=gpt`) | `sdb1` iso9660 `Ubuntu 24.04.4 LTS amd64` (vol creation 2026-02-10 01:39:48) PARTTYPE ISO9660 → `/media/user/Ubuntu 24.04.4 LTS amd64`; `sdb2` vfat FAT12 `ESP` UUID `73B7-988D` (not mounted); `sdb3` gap (300 sect, not mounted); `sdb4` ext4 `writable` UUID `05c2d1ff-d3b3-4602-b3d4-d2d10a22a5a5` → `/media/user/writable` |

- All Phase 17 identities held: same two readers (distinct VID:PID, vendor/model, USB port/controller `04:00.3:1.2` vs `04:00.4:2.1`, by-id names), same capacities, same partition schemes, same UUIDs/labels. The by-id/by-path symlinks carry mtime `16:54` (sda) and `16:58` (sdb), i.e. **both readers have stayed continuously enumerated since Phase 17 with no replug** in the elapsed hours.

### Clean-readability verification (current-session gate)
- Kernel log since boot shows the sector-0 `Medium Error` / `Unrecovered read error` / `Buffer I/O error on dev sda` / `unable to read partition table` / `Dev sda: unable to read RDB block 0` bursts, but **all of them are timestamped 16:41–16:45** — those belong to the **earlier failed cards** (Phase 15 `30,535,680`-sector card and Phase 16 `31,207,424`-sector card before it was reseated), not the current cards.
- After the user inserted the current healthy cards, the kernel log is **clean**: `16:53:41 sda: sda1 Attached SCSI removable disk`; `16:54:44 sda: sda1 Attached SCSI removable disk` (the `detected capacity change ... to 0` at 16:54:10 was the transient removal of the prior card; it re-enumerated healthy at 16:54:44); `16:58:04 sdb: sdb1 sdb2 sdb3 sdb4 Attached SCSI removable disk` and `EXT4-fs (sdb4): recovery complete / mounted filesystem 05c2d1ff-... r/w with ordered data mode`. Successful ext4 journal recovery is only possible if every block of that filesystem reads without error.
- A grep for block-layer errors **strictly after 16:54:44** returned exactly one line: `Aug 02 19:24:52 usb 1-2: reset full-speed USB device number 2 using xhci_hcd`. This is the **Bluetooth radio** (`0bda:4853` Realtek, Bus 1 Port 2, dev 002), **not** an SD reader (SY-T18 is `1-1.2` dev 006 via hub; GEMBIRD is `3-2` dev 028). It is a USB reset, **not** a SCSI Medium Error or read failure. **Zero** block-layer read errors exist for either card in the current session.
- Live-mount evidence independently confirms readability: `sda1` mounted rw, `ls`/`stat`/`df` succeed (15 GiB, 32 KiB used); `sdb1` iso9660 mounted ro, full Ubuntu tree readable; `sdb4` ext4 mounted rw, `ls`/`du` succeed. None of these is possible if sector 0 returns Medium Errors.

### Distinction from the internal OS disk (decisive, unchanged)
- `/dev/nvme0n1` is the mounted Samsung NVMe OS disk: transport `nvme` (PCI `01:00.0`), 256 GB, `removable=0`, GPT, EFI + root. Its stable by-id `nvme-SAMSUNG_MZAL4256HBJD-00BL2_S67PNF0W935415` shares no component with either USB by-id name. `readlink` confirms `sda`→`usb1`, `sdb`→`usb3`, `nvme0n1`→`pci0000:00`. **Never touch `/dev/nvme0n1`.**

### DFPlayer FAT32 suitability for each current card
- DFRobot DFR0299 (authoritative, inspected read-only in Phase 14) supports FAT16/FAT32 TF cards up to 32 GB, sample rates 8–48 kHz, up to 100 folders / 255 songs per folder, deterministic `/mp3/NNNN.mp3` playback via `playMp3Folder()`, no hot-plugging. Conservative target: 8 or 16 GB, MBR/DOS, one primary FAT32, no exFAT/GPT.
- **`/dev/sda` (16 GB, MBR, single vfat): SUITABLE.** Within the 32 GB limit, already MBR + single FAT32-type partition, correct capacity for 14 short narrations. Erase would re-lay a clean MBR + one aligned primary FAT32 labeled `MUSEUM_AUDIO` — no functional conflict with current content.
- **`/sdb` (15.6 GB, GPT, 4 partitions incl. iso9660 + ext4): TECHNICALLY reformattable to FAT32 but POOR choice.** GPT must be destroyed and replaced by MBR; the Ubuntu ISO, ESP, and persistent ext4 are all destroyed. DFRobot does not forbid it on capacity grounds, but it throws away a complete reusable OS installer + install logs to gain one more 16 GB audio card that is redundant with `/dev/sda`. Suitable only if the user has confirmed they no longer need the Ubuntu image.

### Exact data loss on each current device (what "erase and prepare" will destroy)
- **`/dev/sda` — data loss: NONE of value.** Top-level of `sda1` (`/media/user/0403-0201`) contains only `.` and `..` (32 KiB = the empty FAT root directory). The `ls -laA` hidden-file check also returned nothing. Erasing destroys only an already-empty FAT32 volume. Risk: effectively zero.
- **`/dev/sdb` — data loss: SIGNIFICANT and not easily replaceable.**
  - `sdb1` (iso9660, 6.2 GiB, 100% full): a complete **Ubuntu 24.04.4 LTS "Noble Numbat" Release amd64** live ISO built 2026-02-10 (`.disk/info`, `boot.catalog`, `casper/`, `dists/`, `pool/`, `install/`, `preseed/`, `EFI/`, `md5sum.txt`). Redownloadable from Ubuntu mirrors, but only with internet and at the cost of re-verifying the image.
  - `sdb2` (vfat `ESP`, 5,201,920 B): UEFI boot files for the live USB.
  - `sdb3` (gap, 307,200 B): empty alignment gap.
  - `sdb4` (ext4 `writable`, 8.98 GiB, 18 MiB used): persistent casper-rw/home data — contains `install-logs-2026-06-25.0/.1/.2/.3/` (936K + 8.4M + 6.3M + 2.0M) and `lost+found`. These are **installation logs from 2026-06-25**, not redownloadable — they document how this machine's Ubuntu was installed.
  - Net: erasing `/dev/sdb` permanently loses the install logs and requires rebuilding the live USB from scratch to use it as an installer again.

### Why the user's "erase and prepare both" is NOT a valid destructive confirmation
- A bound confirmation must (a) name the **exact current** device path, (b) quote the **freshly observed** stable by-id/by-path, capacity, model/serial, and partition/filesystem layout, (c) state acceptance that **all** content will be permanently erased, and (d) explicitly exclude `/dev/nvme0n1`. Device paths are reassigned on unplug/replug and the elapsed hours could have swapped cards/readers; a generic "both" does not bind any specific current identity and would be unsafe to act on.
- Additionally, the phrase covers `/dev/sdb`, which holds irreplaceable-looking install logs. Erasing it requires a content-level loss acceptance that the generic phrase does not evidence. Therefore this subtask stops at the preflight and issues two **populated, exact** confirmation statements (below) for the user to affirm or decline individually.

### TWO populated exact confirmation statements (NOT executed; awaiting user affirmation)
These are populated from this session's fresh observations. They must be re-verified immediately before any future erase (re-run `lsblk`/`udevadm`/`findmnt` and abort on any mismatch).

- **Statement A — `/dev/sda` (recommended; empty):** "ERASE AND PREPARE `/dev/sda` — `usb-Mass_Storage_Device_121220160204-0:0` — `15,978,201,088 bytes (31,207,424 × 512 B; ~14.9 GiB / ~16.0 GB)` — `Super Top SY-T18 / model 'Mass Storage Device', serial 121220160204, rev 1.00, USB 14cd:1212, port 04:00.3:1.2` — `DOS/MBR table, one partition sda1 vfat FAT32 UUID 0403-0201 (empty: only '.' and '..', 32 KiB used), currently mounted at /media/user/0403-0201`. I understand every partition and file on that device will be permanently erased. **Do not touch `/dev/nvme0n1` or `/dev/sdb`.**"
- **Statement B — `/dev/sdb` (NOT recommended; holds Ubuntu installer + install logs):** "ERASE AND PREPARE `/dev/sdb` — `usb-Generic_Mass-Storage-0:0` — `15,634,268,160 bytes (30,535,680 × 512 B; ~14.6 GiB / ~15.6 GB)` — `GEMBIRD / model 'Mass-Storage', no serial, rev 1.11, USB 1908:0226, port 04:00.4:2.1` — `GPT, four partitions: sdb1 iso9660 'Ubuntu 24.04.4 LTS amd64' (6.2 GiB, 100% full); sdb2 vfat FAT12 'ESP' UUID 73B7-988D (5,201,920 B); sdb3 gap (307,200 B); sdb4 ext4 'writable' UUID 05c2d1ff-d3b3-4602-b3d4-d2d10a22a5a5 (8.98 GiB, 18 MiB used, contains install-logs-2026-06-25.0/.1/.2/.3)`; sdb1 mounted at /media/user/Ubuntu 24.04.4 LTS amd64 and sdb4 at /media/user/writable`. I understand that erasing this device will **permanently destroy the complete Ubuntu 24.04.4 installer image AND the non-redownloadable installation logs dated 2026-06-25**, and that the GPT layout will be replaced by MBR + one FAT32 partition. I confirm I do not need the Ubuntu image or the install logs. **Do not touch `/dev/nvme0n1` or `/dev/sda`.**"

### Read-only conclusion
- Both removable cards are freshly verified healthy/readable, distinct from each other and from `/dev/nvme0n1`, and stable since Phase 17. `/dev/sda` is the safe DFPlayer candidate (empty). `/dev/sdb` is readable but erasing it sacrifices a reusable Ubuntu installer + install logs and is not recommended unless the user explicitly accepts that loss. The user's generic "erase and prepare both" is insufficient as a destructive confirmation; it must be replaced by the two exact statements above before any block-device write occurs. No device was formatted, partitioned, erased, mounted, unmounted, repaired, downloaded to, or written.

## DFPlayer SD-card Destructive Single-Card Preparation: PRE-OPERATION EVIDENCE (2026-08-02 ~20:17 Asia/Dhaka)

### Authorization
- User instruction (current session): "erase any of the 16GB cards. just don't mess with my ssd!" Authorizes erasing exactly ONE removable 16 GB card, explicitly forbids touching the internal SSD/NVMe. Combined with prior context this selects the empty `/dev/sda` (not the `/dev/sdb` Ubuntu installer) as the safest single 16 GB card to erase.
- Scope: single-card formatting/verification ONLY. Do not source/copy audio. Do not modify product code or the standalone repository. Do not erase `/dev/sdb`. Do not touch `/dev/nvme0n1` under any circumstances.

### Fresh read-only identity gate — ALL criteria MATCH (zero mismatch)
- Kernel path: required `/dev/sda`; observed `sda` MAJ:MIN 8:0. MATCH.
- Stable by-id: required `usb-Mass_Storage_Device_121220160204-0:0`; observed identical (symlink mtime 16:54:44). MATCH.
- by-path: `pci-0000:04:00.3-usb-0:1.2:1.0-scsi-0:0:0:0` (same as Phase 17/18). MATCH.
- Exact capacity: required 15,978,201,088 bytes / 31,207,424 sectors; observed lsblk 15978201088 B, sysfs /sys/block/sda/size=31207424 x512 = 15,978,201,088. MATCH.
- Removable/hotplug USB: RM=1, tran=usb, ID_BUS=usb, ID_USB_DRIVER=usb-storage. MATCH.
- Reader: required Super Top SY-T18 / Mass Storage Device, serial 121220160204; observed lsusb `14cd:1212 Super Top microSD card reader (SY-T18)`, ID_MODEL=Storage_Device, ID_SERIAL_SHORT=121220160204, ID_REVISION=1.00. MATCH.
- Current table consistent with empty card: ID_PART_TABLE_TYPE=dos; sda1 vfat FAT32 UUID 0403-0201 PARTTYPE 0xc. MATCH.
- Card content empty: `/media/user/0403-0201/` contains only `.` and `..` (32 KiB). MATCH.

### Clean-readability gate — PASS
- Kernel log's Medium Error / Unrecovered read error / unable-to-read-partition-table bursts are ALL timestamped 16:45:28–16:45:36 (earlier failed Phase 15/16 cards). The current card enumerated CLEANLY at `Aug 02 16:54:44 kernel: sda: sda1` with no errors after 16:45:36.
- Live readability proof: vfat mounted rw at `/media/user/0403-0201`; ls/stat/df succeed; root empty.

### Do-not-touch devices — verified distinct and unchanged
- `/dev/sdb` (Ubuntu installer, MUST NOT touch): GEMBIRD 1908:0226, by-id `usb-Generic_Mass-Storage-0:0`, by-path `pci-0000:04:00.4-usb-0:2:1.0-scsi-0:0:0:0` (mtime 16:58:04), 15,634,268,160 B, GPT, 4 partitions (iso9660 Ubuntu 24.04.4 LTS amd64 + ESP + gap + ext4 writable). Mounted: sdb1 to `/media/user/Ubuntu 24.04.4 LTS amd64`, sdb4 to `/media/user/writable`. Distinct VID:PID/port/by-id/scheme/capacity — no ambiguity.
- `/dev/nvme0n1` (internal SSD, MUST NEVER touch): Samsung MZAL4256HBJD-00BL2, serial S67PNF0W935415, NVMe transport (PCI 01:00.0), by-id `nvme-SAMSUNG_MZAL4256HBJD-00BL2_S67PNF0W935415`, 256,060,514,304 B, RM=0/removable=0, GPT, nvme0n1p1 FAT32 to `/boot/efi`, nvme0n1p2 ext4 to `/`. readlink nvme0n1 to `pci0000:00` (vs sda to `usb1`). No shared identifier.

### Pre-operation state recorded (immutable baseline for later diff)
- `/dev/sda`: MBR/dos, one partition sda1, start sector 8192 (4 MiB), size 31,199,232 sectors, PARTTYPE 0xc, vfat FAT32 blocksize 32768, UUID 0403-0201, fs-size 15,974,006,784 B, mounted rw at `/media/user/0403-0201`. Content empty.
- `/dev/sdb`: GPT, 4 partitions, ISO label UUID 2026-02-10-01-39-48-00; sdb2 vfat ESP UUID 73B7-988D; sdb4 ext4 writable UUID 05c2d1ff-d3b3-4602-b3d4-d2d10a22a5a5. Will be re-diffed post-op to prove untouched.
- `/dev/nvme0n1`: nvme0n1p1 vfat UUID 7CCD-F1DB to /boot/efi; nvme0n1p2 ext4 UUID 8677ea8e-af72-46a7-bc9e-491e6eb2a78d to /. Will be re-diffed post-op to prove untouched.

### Privilege mechanism
- Passwordless sudo unavailable (`sudo: a password is required`). Will use normal interactive `sudo` (user enters password in terminal). sudoers NOT altered; no password stored; no system security weakened.

### Planned destructive commands (target `/dev/sda` ONLY, audio not copied)
1. `sudo umount /dev/sda1` (only the one sda1 mount; sdb/nvme mounts untouched).
2. Re-verify `/dev/sda` parent identity after unmount.
3. `sudo sfdisk /dev/sda` with a DOS/MBR layout: one primary partition start=2048, type=0x0c (W95 FAT32 LBA), spanning to end.
4. `sudo partprobe /dev/sda`; confirm child is `/dev/sda1`.
5. `sudo mkfs.fat -F 32 -n MUSEUM_AUDIO /dev/sda1` (partition only, not whole disk; no exFAT).
6. `sudo fsck.fat -n /dev/sda1` (read-only check); inspect lsblk/blkid.
7. Desktop auto-mount acceptable after formatting; do not place audio yet.

## DFPlayer SD-card Destructive Single-Card Preparation: COMPLETED (2026-08-02 ~20:40 Asia/Dhaka)

### Outcome — SUCCESS
- Exactly ONE removable 16 GB card (`/dev/sda`, the empty one) was erased and formatted. The Ubuntu installer `/dev/sdb` and the internal Samsung SSD `/dev/nvme0n1` were NOT touched.

### Resulting filesystem / layout on `/dev/sda`
- Partition table: DOS/MBR, `label-id 0xb61ac6ec`, sector-size 512.
- `sfdisk -d /dev/sda`: `sda1 : start=2048, size=31205376, type=c` (one primary partition, 1 MiB aligned at sector 2048, type `0x0c` W95 FAT32 LBA, spanning to sector 31207423).
- Filesystem on `/dev/sda1`: vfat FAT32, **label `MSMAUDIO`** (user chose `MSMAUDIO` because the originally-planned `MUSEUM_AUDIO` is 12 chars and exceeds the FAT32 11-char volume-label limit), UUID `6831-01A4`, blocksize 8192, fs-size 15,977,152,512 bytes (≈14.9 GiB).
- Parent device identity unchanged after operation: `/dev/sda`, by-id `usb-Mass_Storage_Device_121220160204-0:0`, by-path `pci-0000:04:00.3-usb-0:1.2:1.0-scsi-0:0:0:0`, Super Top SY-T18 `14cd:1212`, serial `121220160204`, 31,207,424 sectors = 15,978,201,088 bytes.

### Verification (read-only)
- `fsck.fat -n /dev/sda1`: exit 0; `fsck.fat 4.2` reported `/dev/sda1: 1 files, 1/1948430 clusters` — clean, zero errors. (The "1 file" is the FAT root volume-label entry; no audio placed.)
- `lsblk`/`udevadm`: FSTYPE=vfat, ID_FS_VERSION=FAT32, LABEL=MSMAUDIO, UUID=6831-01A4, PARTTYPE=0xc, PART_ENTRY_OFFSET=2048. Matches DFPlayer-compatible conservative target (FAT32, MBR, one primary partition, no exFAT).

### Proof other disks were untouched (post-op state vs pre-op baseline — all unchanged)
- `/dev/sdb` (Ubuntu installer): still GPT, 4 partitions, sdb1 iso9660 `Ubuntu_24.04.4_LTS_amd64` UUID `2026-02-10-01-39-48-00` (mounted `/media/user/Ubuntu 24.04.4 LTS amd64`); sdb2 vfat `ESP` UUID `73B7-988D`; sdb4 ext4 `writable` UUID `05c2d1ff-d3b3-4602-b3d4-d2d10a22a5a5` (mounted `/media/user/writable`). All UUIDs match pre-op baseline — NOT formatted, NOT partitioned, mounts intact.
- `/dev/nvme0n1` (internal Samsung SSD): nvme0n1p1 vfat UUID `7CCD-F1DB` mounted `/boot/efi`; nvme0n1p2 ext4 UUID `8677ea8e-af72-46a7-bc9e-491e6eb2a78d` mounted `/` (system still booting). NOT touched in any way.

### Privilege handling (security-preserving)
- Passwordless sudo unavailable; used a temporary `zenity`-based `SUDO_ASKPASS` helper (`/tmp/sudo_askpass_museum.sh`, mode 0700, owned by user) so `sudo -A` could prompt graphically. The helper was deleted after use. No sudoers file was altered, no password was stored/logged, and no system security setting was weakened.

### Errors during operation and resolution
- Initial `mkfs.fat -F 32 -n MUSEUM_AUDIO` failed: "Label can be no longer than 11 characters" (MUSEUM_AUDIO = 12 chars). Resolved by user choosing `MSMAUDIO` (8 chars). No data was lost from this failure; the partition was already created and simply re-formatted with the valid label.
- `sudo` credential cache did not persist across separate terminal invocations (VSCode runs each `execute_command` in a fresh shell), so destructive steps were grouped into single shells using `sudo -A -v` to cache creds once per shell.
- A spurious `partprobe /dev/sda1` "unable to inform the kernel... in use" warning appeared when partprobe was pointed at the partition node rather than the disk; it was harmless (no FS existed yet at that moment) and the subsequent `partprobe /dev/sda` + `lsblk` confirmed the correct layout.

### Scope adherence
- Single-card formatting and verification ONLY. No audio sourced/copied. Product code and the standalone repository not modified. `/dev/sdb` not erased. `/dev/nvme0n1` not touched. The card is ready for a future audio-copy subtask (separate confirmation).

## DFPlayer Example Audio Asset Staging (2026-08-02 ~20:45 Asia/Dhaka)

### Scope and method
- Asset-staging subtask only. Created files under a new repository-local directory `tools/dfplayer_audio/`. Did NOT format, repartition, erase, mount, unmount, write to, or copy audio onto any block device. No network downloads; everything synthesized locally. Only the staging directory and the three root planning Markdown files were modified.
- Restored planning context; confirmed the seven-station order and the 14-track map (`0001`–`0007` English, `0008`–`0014` Bangla) match [`webapp/js/data.js`](webapp/js/data.js) `ARTIFACTS` and the prior DFPlayer preflight findings.

### Toolchain confirmed
- FFmpeg/ffprobe 6.1.1-3ubuntu5, built `--enable-libflite --enable-libmp3lame`; gcc 13 (Ubuntu 13.2.0-23ubuntu3); host Linux 7.0.0-28-generic x86_64.
- Flite lavfi source available with voices `awb`, `kal`, `kal16`, `rms`, `slt` (English only — no Bangla voice). `slt` selected for clarity/consistency.
- Tone synthesis via `aevalsrc`; MP3 encode via `libmp3lame`.

### Cue design (14 distinct, originally synthesized)
- English tracks 0001–0007: damped-sinusoid bell (0.6 s) on the station's pentatonic root → 0.15 s silence → Flite `slt` speaking `"Station N. <Name>."` → closing bell (0.6 s). Roots (Hz): C4 261.63, D4 293.66, E4 329.63, G4 392.00, A4 440.00, C5 523.25, D5 587.33.
- Bangla tracks 0008–0014: deliberately NON-VERBAL ascending chime motif (root → 1.4983× fifth → 2.0× octave, played twice with a 0.30 s gap). Reason: no local Bangla TTS; an English voice mispronouncing Bangla would be culturally inappropriate and misleading. Station identity is encoded via the root pitch (same pentatonic sequence), so all seven are audibly distinct.
- All segments gain-staged (0.85) and passed through `alimiter` (ceiling 0.80) before encode.

### Files created
- `tools/dfplayer_audio/generate_tracks.sh` — deterministic, re-runnable generator.
- `tools/dfplayer_audio/mp3/0001.mp3`–`0014.mp3` — the 14 staged tracks (the ONLY contents of `mp3/`).
- `tools/dfplayer_audio/README.md` — human-readable map + encoding profile + regenerate instructions.
- `tools/dfplayer_audio/TRACK_MAP.csv` — machine-readable per-track map (track, language, station, artifact EN/BN, kind, root Hz, spoken text, duration, peak, size, sha256).
- `tools/dfplayer_audio/PROVENANCE.md` — recipe, tool versions, creation date, CC0 1.0 dedication rationale, engine≠asset boundary, replacement policy, DFPlayer constraints.
- `tools/dfplayer_audio/SHA256SUMS` — 14-line hash list (bare names, for card-side verification).
- `tools/dfplayer_audio/.gitignore` — excludes `.gen_tmp/`.

### Validation results (ffprobe + volumedetect, all 14 PASS)
- Codec: mp3 (MPEG-1 Layer III); sample_rate: 44100; channels: 1 (mono); bit_rate: 128000 CBR on every track.
- Duration: 3.32–4.44 s (short enough for rapid hardware testing); Bangla tracks all 3.944438 s by construction.
- Peak level: −2.1 dB to −4.4 dB — no clipping on any track.
- Metadata: none (`-map_metadata -1`); no video stream / no attached pictures.
- Purity: `mp3/` contains exactly 14 files, all match `[0-9]{4}\.mp3`, contiguous `0001`–`0014`, no subdirs/hidden/temp files.
- SHA-256: all 14 hashes distinct (the 7 Bangla tracks genuinely differ by pitch despite identical byte-size/duration); `sha256sum -c SHA256SUMS` from `mp3/` → 14/14 OK.

### Per-track SHA-256
- 0001 `66e5c45486fcad00e103e91cedeb4480a060bcbcf238e66e7e79b9c0f673ae7f`
- 0002 `ec4a6bc999f30070d8a5201ed232effe6582b6e99c64506a2f31b3c39edc2062`
- 0003 `9efca9259947f711f5ee279a4bb7994271b54ebcd15a9b3d269d0d43c156eb48`
- 0004 `0a95604d8549067ad0f1d146834a7a8371eb559d4a19e03c6d83ea240cff4412`
- 0005 `74530632f262dc8451e3c1002bc20412f5efc37fa33c43157297d408412957cf`
- 0006 `ccb258fa8c921faea19e9c14a132551e71fdccdc4c15b4a95882c7c6877b1acb`
- 0007 `e1a5386117fb1591da90905da38ecec162711837afb3fe267ee4bef97fec9b40`
- 0008 `dffab9a182560f47d3ca9ca8bebdeef0adb17a2f1a89488d4bbf2364144b7b9c`
- 0009 `ee505ecad34e5095a2aef30ee9e97ed30c634b38bac280144562166932c1ef19`
- 0010 `f66a7d6f4195d94b26e85b265c174f13e7e53bb3a34ea14281e3bd4f37558e8e`
- 0011 `03380c731d00dd5e5a3b6319705cd70a41aaffdb05c5fe501e339a3e0a28f735`
- 0012 `c3f6fdad4be69d664b4c690e56e2ba0481b120e4ec9468d74f13dde83ffb488b`
- 0013 `da50df1b4419b4e185882c39b84e91fe8cfbbb3fc5669faf8cff8389684ca01b`
- 0014 `54729d45b812c7d784e5aa563a3e1012fac3ee358fad51106e5315656d71e7e0`

### License / provenance approach (documented in PROVENANCE.md)
- Assets are original project-generated synthesis. Dedicated to the public domain via **CC0 1.0 Universal** — chosen over MIT (a software-source license, unsuitable for audio) and over Public Domain Mark (informational only; here the project is the rightsholder actively dedicating, so CC0 is the correct mechanism).
- Engine ≠ asset boundary stated explicitly: FFmpeg (GPL/LGPL) and CMU Flite (BSD-like) are third-party software whose licenses cover the software, not the audio output produced by running it on original parameters. The CC0 dedication applies to the generated audio files only and does not relicense FFmpeg/Flite.
- TTS-output note: English Flite speech is short, factual, non-expressive placeholder text ("Station N. <Name>.") and is documented as a temporary cue to be replaced; CC0 applied in good faith by the project as the operator that ran the synthesis. If any synthesized voice is retained in the final build, the team should re-confirm Flite's current output terms.

### Replacement policy (placeholders, not final narration)
- These cues are functional test cues for hardware/software bring-up only. They MUST be replaced with team-written, team-recorded bilingual narration before public/competition use.
- When recording real tracks, capture: script author + proofreader (Bangla script needs cultural review — note known typos in draft descriptions), speaker name + parent/guardian consent for child speakers, recording date/operator/equipment, edits + conversion recipe, source WAV SHA-256 + final MP3 SHA-256, and the chosen rights statement. Team voice recordings should be described as "original Smart Museum recordings used with contributor/guardian permission" — NOT automatically "CC0"/"public domain" unless every rightsholder (including the speaker/guardian) validly makes that dedication.
- Firmware must use `playMp3Folder(trackNumber)` (deterministic `/mp3/NNNN.mp3` folder mode), not global copy-order `play(n)`, per DFRobot DFR0299. The card must contain only the 14 numeric files (no desktop metadata/playlists/artwork) — hence documentation lives outside `mp3/`.

### Scope adherence / no-block-device proof
- Post-subtask read-only check: `/dev/sda1` remained label `MSMAUDIO`, UUID `6831-01A4`, unmounted and unwritten (no audio copied). `/dev/nvme0n1` unchanged (`7CCD-F1DB` `/boot/efi`, `8677ea8e…` `/`). `/dev/sdb` was not present during this subtask (Ubuntu installer card disconnected); the single `journalctl` line at 20:43:19 was its old session-start enumeration, not an action by this subtask. No new mounts; no kernel block-layer errors in the last 10 min attributable to this subtask.

### Concerns / follow-ups
- Copying the staged `mp3/` contents onto the `MSMAUDIO` card is a separate future subtask requiring explicit confirmation; copy ONLY the contents of `mp3/` into a `/mp3/` directory at the card root, in ascending order, and re-verify on-card hashes against `SHA256SUMS`.
- Final Bangla narration cannot be synthesized locally (no Bangla TTS); it must be team-recorded.
- Re-confirm Flite output terms if any synthesized English voice is retained for the final build.

## DFPlayer SD-card Copy + Verify + Eject (2026-08-02 ~20:52 Asia/Dhaka)

### Authorization and scope
- User-authorized scope: copy and verify the 14 staged DFPlayer test tracks onto the exact prepared `MSMAUDIO` card, then safely eject it; do not touch the Ubuntu card `/dev/sdb` or the internal SSD `/dev/nvme0n1`; document the future firmware command mapping; update planning files. Privileges via normal sudo/polkit/askpass only; never alter sudoers or store credentials.
- Scope of writes: only `/dev/sda1` (mount, mkdir `/mp3`, copy 14 files, sync, unmount) and the parent `/dev/sda` (power-off/eject). No firmware implementation; no product-code change.

### Fresh read-only identity gate BEFORE any write — ALL criteria MATCH (zero mismatch)
- Target `/dev/sda`: by-id `usb-Mass_Storage_Device_121220160204-0:0`, by-path `pci-0000:04:00.3-usb-0:1.2:1.0-scsi-0:0:0:0`, USB `14cd:1212` (lsusb "Super Top microSD card reader (SY-T18)"), ID_MODEL=Storage_Device, ID_SERIAL_SHORT=121220160204, ID_REVISION=1.00, ID_BUS=usb, ID_USB_DRIVER=usb-storage, ID_TYPE=disk, ID_PART_TABLE_TYPE=dos. Exact match to Phase 19/20.
- Capacity: sysfs `/sys/block/sda/size` = 31,207,424 sectors × 512 = **15,978,201,088 bytes** (exact match). removable=1.
- `/dev/sda1`: ID_FS_TYPE=vfat, ID_FS_VERSION=FAT32, ID_FS_LABEL=MSMAUDIO, ID_FS_UUID=6831-01A4, PARTTYPE 0xc (W95 FAT32 LBA), single partition under DOS/MBR. Exact match.
- Target was NOT mounted at gate time (clean, ready for mount). No `Medium Error`/`Unrecovered read error`/I/O error on sda in the kernel log (those bursts are timestamped ≤16:45 and belong to earlier failed cards).
- Excluded `/dev/nvme0n1` (SSD): Samsung MZAL4256HBJD-00BL2, NVMe, 256,060,514,304 B, removable=0; nvme0n1p1 vfat `7CCD-F1DB`→`/boot/efi`, nvme0n1p2 ext4 `8677ea8e-af72-46a7-bc9e-491e6eb2a78d`→`/`. Distinct transport/by-id/size/removable.
- Excluded `/dev/sdb` (Ubuntu installer): at the initial identity-gate probe it returned `lsblk: /dev/sdb: not a block device` because the GEMBIRD `1908:0226` reader was in one of its transient offline states (documented `device offline error`/`error -71` history). It re-enumerated as `/dev/sdb` during the session. It was NEVER referenced by any write command — every write this task named `/dev/sda1`, `/dev/sda`, or `/media/user/MSMAUDIO` explicitly. Its post-task state (below) proves it is intact.

### Source re-validation (pre-copy) — PASS
- `tools/dfplayer_audio/mp3/`: exactly 14 files, all `????.mp3`, contiguous `0001`–`0014`, no subdirs/hidden/temp. `sha256sum -c SHA256SUMS` → 14/14 OK; 14 distinct hashes; SHA256SUMS = 14 sorted bare-name lines. ffprobe all 14: codec mp3, 44100 Hz, 1 channel, 128000 b/s, durations 3.32–4.44 s; spot-check 0001/0014 → no metadata, no video stream, format_name=mp3.

### Mount — safe, exact target only
- `udisksctl mount --block-device /dev/sda1` → `Mounted /dev/sda1 at /media/user/MSMAUDIO` (rw, nosuid/nodev, uid/gid=1000, fmask/dmask=0022, errors=remount-ro). Mount options used `shortname=mixed,showexec,utf8,flush` (standard desktop-safe vfat). Target root was empty before copy.

### Copy — 14 numeric files, ascending order, no desktop metadata
- Created root `/mp3` (8192-byte FAT dir). Copied `0001.mp3`…`0014.mp3` in explicit numeric order via plain `cp` (the vfat mount does not preserve owner/mode/xattrs/extended attributes, so no macOS `._`/`.DS_Store` or Windows/desktop metadata is generated). Sizes match source exactly: 0001 64365, 0002 55588, 0003 71053, 0004 58514, 0005 53080, 0006 65619, 0007 70217, 0008–0014 63111 each. `sync` flushed all writes.

### Purity verification on the card — PASS
- Target root contains ONLY `mp3/` (zero normal files, zero other directories).
- Card-wide scan for desktop metadata returned NOTHING: no `.DS_Store`, no `._*`, no `.Trash*`, no `Thumbs.db`, no `desktop.ini`, no `System Volume Information`, no `__MACOSX`, no `.fseventsd`, no `.Spotlight-*`, no hidden files/dirs at any depth.
- `/mp3` contains exactly 14 entries, all `????.mp3`, no subdirs, no hidden files.

### On-card content verification — 14/14 PASS
- SHA-256: `sha256sum -c SHA256SUMS` run from the on-card `/mp3` → 14/14 OK.
- Byte-for-byte: `cmp` source vs on-card → all 14 IDENTICAL.
- ffprobe on every on-card file → all: codec_name=mp3, sample_rate=44100, channels=1, bit_rate=128000; durations 0001 4.022813, 0002 3.474250, 0003 4.440813, 0004 3.657125, 0005 3.317500, 0006 4.101188, 0007 4.388563, 0008–0014 3.944438 each.
- Names contiguous: on-card sorted listing = `0001.mp3 … 0014.mp3`.

### Firmware command-map documentation (reference only, no implementation)
- Created `tools/dfplayer_audio/FIRMWARE_COMMAND_MAP.md`: SD-card on-card layout; `(station,language)→track` formula (EN=`s`, BN=`s+7`); DFRobotDFPlayerMini API surface to use (`begin(Serial1,true,false)`, `volume`, `playMp3Folder(track)` [primary], `stop`, `pause/start`, `readState/readType/read`); hardware/level-shifter caution; planned serial command protocol (`LANG 0/1`, `STATION <n>`, `PLAY <t>`, `STOP`, `VOL <0..30>`, `STATE`) at 115200 baud; consistency with `MuseumProtocol.h` station id and `webapp/js/data.js` artifact order; bring-up verification steps. Stated explicitly that no firmware is implemented.
- Updated `tools/dfplayer_audio/README.md`: replaced the "future subtask — NOT done here" copy section with a "DONE (2026-08-02)" status block summarizing the copy + 14/14 verification, and added a pointer to `FIRMWARE_COMMAND_MAP.md`.

### Safe eject — clean
- `sync` (final). `udisksctl unmount --block-device /dev/sda1` → `Unmounted /dev/sda1.`; no mount remains for sda1 or sda (findmnt empty). `udisksctl power-off --block-device /dev/sda` → rc=0. Post-eject: by-id symlink `usb-Mass_Storage_Device_121220160204-0:0` gone, `/dev/sda` node gone. Card is safely powered off and may be physically removed by the user (not removed by this task).

### Excluded-disk untouched proof (post-op vs pre-op baseline — all unchanged)
- `/dev/nvme0n1` (SSD): nvme0n1p1 vfat UUID `7CCD-F1DB` mounted `/boot/efi`; nvme0n1p2 ext4 UUID `8677ea8e-af72-46a7-bc9e-491e6eb2a78d` mounted `/`. System still booting. NOT touched in any way.
- `/dev/sdb` (Ubuntu installer): sdb1 iso9660 `Ubuntu_24.04.4 LTS amd64` UUID `2026-02-10-01-39-48-00` (mounted); sdb2 vfat `ESP` UUID `73B7-988D`; sdb4 ext4 `writable` UUID `05c2d1ff-d3b3-4602-b3d4-d2d10a22a5a5` (mounted). `.disk/info` reads `Ubuntu 24.04.4 LTS "Noble Numbat" - Release amd64 (20260210)` — ISO intact. All UUIDs/labels match the Phase 18/19 baseline. It was never named by any write command.

### Privilege handling (security-preserving)
- No `sudo` was required or used. All mount/unmount/eject operations went through polkit via `udisksctl` (normal desktop mechanism). No sudoers file altered, no password stored/logged, no `root` shell, no askpass helper created. System security unchanged.

### Final card state
- The `MSMAUDIO` card now contains exactly `/mp3/0001.mp3`…`/mp3/0014.mp3` (14 verified MP3/44.1 kHz/mono/128 kb/s CBR cues), nothing else, on a clean FAT32/MBR single-partition volume (label `MSMAUDIO`, UUID `6831-01A4`). It was cleanly unmounted and power-off/ejected. It is ready to insert into a DFPlayer Mini (with DFPlayer power off, per DFRobot's hot-plug warning) for the next firmware bring-up task.

### Follow-ups
- Next task: implement the ESP32 DFPlayer firmware per `tools/dfplayer_audio/FIRMWARE_COMMAND_MAP.md` (DFRobotDFPlayerMini, hardware Serial1, `playMp3Folder()`, serial command protocol). Confirm DFPlayer module VCC and add a level shifter on DFPlayer TX→ESP32 RX if the module is powered at 5 V.
- Replace the 14 placeholder cues with team-recorded bilingual narration before public/competition use (the `(station,language)→track` mapping and `playMp3Folder()` call will not change — only file contents).

## Fresh Read-only Inventory: Which Card Is Plugged In Now? (2026-08-02 ~21:20 Asia/Dhaka)

### Scope and method
- User confusion to resolve: they unplugged one card and are unsure whether the prepared `MSMAUDIO` card is the one still plugged in or the one they removed. This subtask performs a fresh read-only inventory to answer that.
- Strictly read-only. No format, partition, unmount, mount, erase, download, or write was performed on any block device. Only the three root planning Markdown files may be changed.
- Inventory rebuilt from scratch (no assumptions from prior sessions) with `lsblk --bytes --all`, `/proc/partitions`, `/dev/disk/by-id`, `/dev/disk/by-path`, `lsusb`/`lsusb -t`, sysfs (`/sys/block/*`), `findmnt`, and `journalctl -k` for the disconnect timeline. Raw device-node reads were unnecessary: the absence of any `/dev/sd*` node and any USB mass-storage VID:PID is itself decisive.

### Current connected state — NO card reader of any kind is connected
- `lsblk --all`: the only non-loop/non-zram disk is `/dev/nvme0n1` (the internal Samsung OS SSD). There is **no `/dev/sda`, no `/dev/sdb`, no `/dev/mmcblk*`, no `/dev/sr*`**.
- `/dev/disk/by-id/` contains **only `nvme-*` symlinks**; the two expected USB names — `usb-Mass_Storage_Device_121220160204-0:0` (Super Top SY-T18) and `usb-Generic_Mass-Storage-0:0` (GEMBIRD) — are **both absent**.
- `/dev/disk/by-path/` has **no `usb` entries** at all.
- sysfs `/sys/block/sd*`, `/sys/block/mmcblk*`, `/sys/block/sr*` all return "No such file or directory".
- `lsusb`: only **Bus 001 Dev 002 `0bda:4853` Realtek Bluetooth Radio** and **Bus 005 Dev 002 `04f2:b78d` Chicony Integrated Camera** are present (plus root hubs). **Neither `14cd:1212` (Super Top SY-T18) nor `1908:0226` (GEMBIRD) appears.**
- `findmnt --real`: only the NVMe mounts (`/`, `/boot/efi`) and snap loops. `/media/user/` is empty. No SD-card volume is mounted.
- Conclusion: **right now there are ZERO card readers/cards attached.** The prepared `MSMAUDIO` card is NOT currently plugged in, and the Ubuntu installer card is NOT currently plugged in either.

### Disconnect timeline (kernel log, read-only) — establishes which card was unplugged when
- **Prepared `MSMAUDIO` card — Super Top SY-T18 reader (`14cd:1212`, serial `121220160204`, on the USB hub at `usb 1-1.2`):** last enumerated `16:54:43` (device 6); **disconnected at `20:54:07`** (`usb 1-1.2: USB disconnect, device number 6`). The hub it sat on (`usb 1-1`) disconnected at `21:17:29`. No `14cd:1212` enumeration occurs after `20:54:07`. This ~20:54 disconnect is consistent with the user physically removing the reader ~2 minutes after the clean software `udisksctl power-off` that ended the ~20:52 copy/eject session.
- **Ubuntu installer card — GEMBIRD reader (`1908:0226`, on `usb 3-2`):** disconnected/reconnected repeatedly (`21:17:53` dev 30, `21:18:18` dev 31, `21:18:22` dev 32, `21:18:23` dev 33, with an `error -71` at `21:18:19` — its known transient flakiness), and the **final disconnect was at `21:19:39`** (`usb 3-2: USB disconnect, device number 33`). No `1908:0226` enumeration occurs after `21:19:39`.
- Therefore, if the user "just" unplugged a card, that final removal was the **Ubuntu installer card in the GEMBIRD reader**. The **prepared `MSMAUDIO` card was unplugged earlier** (~20:54), not just now.

### Which physical card was prepared, and how to identify it when reinserted
- The prepared DFPlayer card is the **microSD inside the Super Top SY-T18 reader** — `lsusb` string `Super Top microSD card reader (SY-T18)`, VID:PID `14cd:1212`, serial `121220160160204`→ `121220160204`, by-id `usb-Mass_Storage_Device_121220160204-0:0`. It is the reader that sits on the small USB2.0 hub (not the GEMBIRD).
- On-card identity it will show when re-plugged: FAT32, **volume label `MSMAUDIO`**, **UUID `6831-01A4`**, MBR/DOS, one partition (`/dev/sda1` or whichever letter is reassigned). The desktop typically auto-mounts it at `/media/user/MSMAUDIO`.
- Its content fingerprint: the volume root contains exactly one directory `/mp3/` holding `0001.mp3`–`0014.mp3` (14 files) and nothing else. On-card SHA-256s match `tools/dfplayer_audio/SHA256SUMS`.
- By contrast, the other card (the one just unplugged) is the **Ubuntu 24.04.4 LTS installer** in the **GEMBIRD reader** (`1908:0226`); it shows a GPT with an iso9660 partition labelled `Ubuntu 24.04.4 LTS amd64` and an ext4 `writable` partition — clearly not an audio card.

### Read-only conclusion
- **Neither card is currently connected.** The prepared `MSMAUDIO` card (Super Top SY-T18 reader) was unplugged at ~20:54 Asia/Dhaka; the Ubuntu installer card (GEMBIRD reader) was the last one removed, at ~21:19:39. To use the prepared DFPlayer card, reinsert the **Super Top SY-T18** reader; the volume `MSMAUDIO` (UUID `6831-01A4`) with a single `/mp3/0001.mp3`–`/mp3/0014.mp3` tree is the prepared card. No device was formatted, partitioned, erased, mounted, unmounted, repaired, downloaded to, or written in this subtask.

## DFPlayer Re-preparation of Current 8 GB Card — Read-only Preflight (2026-08-03 ~20:25 Asia/Dhaka)

### Project context recovered
- `tools/dfplayer_audio/` already contains the intended sample set: 14 locally generated cues, generator, track map, provenance, checksums, and firmware command map. No unknown external download is needed.
- Required deployment is root `/mp3/` containing only `0001.mp3`–`0014.mp3`; English tracks are 1–7 and Bangla non-verbal placeholder cues are 8–14. Firmware uses `playMp3Folder(track)`.
- Conservative DFPlayer target: DOS/MBR + one primary FAT32-LBA partition (not exFAT/GPT), card no larger than 32 GB. The current 8 GB card is suitable by capacity.

### Fresh identity and current layout
- Candidate `/dev/sda`: removable USB, model `Storage Device`, serial `121220160204`, stable by-id `usb-Mass_Storage_Device_121220160204-0:0`, driver `usb-storage`, exact size **7,990,149,120 bytes**.
- `/dev/sda1`: 7,989,100,544 bytes, DOS parent table, type `0x83`, exFAT 1.0, label `SDCARD`, UUID `EAF5-E2A2`, mounted rw at `/media/user/SDCARD`; depth-2 content inventory is empty.
- Excluded system disk `/dev/nvme0n1`: Samsung `MZAL4256HBJD-00BL2`, serial `S67PNF0W935415`, non-removable NVMe, 256,060,514,304 bytes, mounted `/` and `/boot/efi`. It is decisively distinct and must never be targeted.
- The current exFAT filesystem is incompatible with the documented conservative DFPlayer setup and must be replaced with FAT32.

### Caution requiring validation
- Kernel log contains one recent line at 20:19:58: `Buffer I/O error on dev sda1, logical block 0, lost sync page write`. Inspect the surrounding sequence before destructive work. After preparation require FAT check, hashes/byte comparison, sync, clean unmount, and persistence verification.
- No block-device write, format, partition edit, mount, unmount, or audio copy occurred in this phase before this note.

### Targeted log and source validation
- The full kernel sequence explains the isolated warning: the card first enumerated cleanly at 20:18:25 (`15,605,760` sectors, write protection off, `sda: sda1`, attached removable disk), then logged the lost sync-page write at 20:19:58 during the prior setup/removal workflow. It re-enumerated cleanly at 20:21:23 with the same capacity and partition, followed by `exFAT-fs (sda1): Volume was not properly unmounted`. There are no Medium Errors, unrecovered reads, capacity collapses, or current read errors. This is consistent with an earlier unsafe removal/stale write, not the counterfeit-card signature seen previously.
- Source package revalidation PASS: `tools/dfplayer_audio/mp3/` contains exactly 14 entries, all regular files named contiguously `0001.mp3`–`0014.mp3`; `sha256sum -c SHA256SUMS` passed 14/14; ffprobe reports MP3, 44,100 Hz, mono, 128,000 b/s for every file. `TRACK_MAP.csv` starts at English station 1/Jamdani and ends at Bangla station 7/Royal Bengal Tiger, matching the documented mapping.

## ESP32S-CAM (GC2145) Node — Implementation & Hardware Verification (2026-08-03)

### Hardware identity
- The connected camera board is an **ESP32S-CAM** seated on an **ESP32-CAM-MB Type-C programmer**. The CAM-MB carries a built-in **CH340** USB-serial chip (`1a86:7523`); on this host it enumerates at `/dev/ttyUSB0` (by-id `usb-1a86_USB_Serial-if00-port0`). No FTDI/UNO is needed.
- **Chip identity (esptool):** `ESP32-D0WD-V3` revision **v3.1**, MAC `68:09:47:9b:38:d8`, 40 MHz crystal, features WiFi/BT/Dual Core/240 MHz. This is a genuine classic ESP32-CAM silicon (the "ESP32S-CAM" module label refers to the carrier/module, not a different SoC).
- Camera sensor: **GC2145** (ribbon labelled `GC2145`). Proven sensor **PID `0x2145`**, confirmed again at boot: `camera : ready, sensor PID=0x2145 (GC2145)`. This matches the working detection recorded in `ESP32 CAM.pdf`.

### GC2145 caveat (critical)
- The GC2145 **cannot output native JPEG**. Standard `PIXFORMAT_JPEG` / OV2640 CameraWebServer examples fail with `Camera probe failed with error 0x106` / `Detected camera not supported`. The implemented firmware therefore captures **RGB565** and converts each frame to JPEG **in software** with `frame2jpg()` (`img_converters.h`).
- Proven-stable settings (matching the PDF): **XCLK 10 MHz**, `PIXFORMAT_RGB565`, **QVGA 320×240**, **1 framebuffer in PSRAM**. At JPEG quality 15 each QVGA frame encodes to ~1.8 KB.

### Pin map (AI-Thinker / ESP32S-CAM, verified working)
- PWDN 32, RESET -1, XCLK 0, SIOD 26, SIOC 27, Y9 35, Y8 34, Y7 39, Y6 36, Y5 21, Y4 19, Y3 18, Y2 5, VSYNC 25, HREF 23, PCLK 22. Identical to the AI-Thinker pin map that already produced PID 0x2145 in the PDF.

### SD_MMC 1-bit mode (no pin conflicts)
- SD_MMC runs in **1-bit mode** → uses only **GPIO2 (D0) / GPIO14 (CLK) / GPIO15 (CMD)**. The camera DVP bus (5/18/19/21/22/23/25/26/27/32/34/35/36/39) is completely disjoint, and **GPIO4 (onboard flash LED)** stays free. 4-bit SD mode (which would also take GPIO4/12/13) is intentionally NOT used, so the flash LED does not conflict with the card.
- Verified on an 8 GB FAT32 SDHC card: `microSD : mounted /sdcard (1-bit) type=SDHC/SDXC card=7620.0 MB fsTotal=7604.1 MB fsFree=7602.0 MB`.

### Firmware behavior (verified on hardware)
- Commands (USB @115200, case-insensitive, newline-terminated): `CAPTURE [station]`, `STATUS`, `LIST`, `INIT`, `FLASH ON|OFF`, `HELP`. Line buffer is hard-bounded (no dynamic `String`).
- `CAPTURE` → grab RGB565 frame → `frame2jpg()` → `/photos/IMG_####.JPG` (sequential, collision-safe via post-reboot max-index scan) → write + close + read-back-verify. Releases fb and JPEG buffer on every path.
- Verified captures: `IMG_0001.JPG` 1829 B, `IMG_0002.JPG` 1883 B, `IMG_0003.JPG` 1883 B (station=3), all 320×240, all "(verified)". Sequential counter increments correctly with no overwrites.
- `STATUS` reports uptime, free heap/PSRAM, camera readiness + PID, SD type/capacity/free/photo-count, flash state. `LIST` lists `/photos` entries with sizes. `INIT` re-mounts SD after card insertion. All non-crashing and idempotent.

### CAM-MB reset reliability (key operational finding)
- The ESP32-CAM-MB's auto-reset is **not reliably software-controllable from this host**. After flashing, the board produced zero serial output and ignored all commands. esptool repeatedly reported **"Stub is already running"** and its RTS hard-reset, `esptool run`, `--after soft_reset` (ESP8266-only), and several pyserial DTR/RTS pulse patterns all failed to reboot the chip.
- This is the **same** "Connecting… No serial data received" failure documented in `ESP32 CAM.pdf` on the Mac, which there required manual IO0/RST handling. On this host the reliable way to enter run mode is a **physical RST press**, or — after the CH340 dropped off USB when the onboard microSD was inserted (the ESP32-CAM was nudged in its socket) — a **USB-C replug**. Bring-up scripts must not rely on software reset of the CAM-MB.
- Flashing itself worked first try with no manual IO0 needed (the CAM-MB did auto-enter download mode for upload at 115200); only the post-flash reset to run mode is unreliable.

### Build / flash result
- Build: **SUCCESS**. RAM 10.7% (35 048 / 327 680 B); Flash 14.3% (450 545 / 3 145 728 B) on `huge_app.csv` (3 MB app, no SPIFFS — photos live on the SD, not flash). No app-code errors/warnings.
- Flash: **SUCCESS** to `/dev/ttyUSB0` at 115200; all regions hash-verified; hard-reset via RTS.

### Future Hub integration seam
- Left as a single deliberate no-op stub `uploadToHubJpeg()` in `src/main.cpp`. When enabled (after the core demo is solid), the node will join `MuseumHub` on channel 1 as a client (never host), `POST http://192.168.4.1/api/image/upload` with the raw JPEG body, and send `museum::MSG_IMAGE_READY` over ESP-NOW. The shared packet is `../common/MuseumProtocol.h`. Nothing in this phase joins Wi-Fi or performs HTTP.

### No host block device touched
- `/dev/sda`, `/dev/sdb`, and `/dev/nvme0n1` were never formatted, repartitioned, erased, mounted, unmounted, or written. All photo storage is on the ESP32-CAM onboard microSD via SD_MMC firmware only. The 8 GB FAT32 card was inserted by the user and not modified by this task. `.standalone-display-repo/` untouched.

### Final user-side verification (2026-08-04 ~00:49 Asia/Dhaka)
- Record-only step: no code, firmware, `.standalone-display-repo/`, audio files, or storage devices were changed; it captures the user's final serial evidence for the already-flashed/verified node.
- Final `STATUS`: uptime **953720 ms**; free heap **281648 bytes**; free PSRAM **3918 KB**; camera ready **PID 0x2145 (GC2145)**; onboard microSD mounted `/sdcard` in **1-bit mode**, type **SDHC/SDXC**, **7620.0 MB** total with **7602.0 MB free**; `photos=3` immediately before the final captures; flash LED **off**.
- Final captures (both **320×240**, `verified`, `station=2`): `/photos/IMG_0004.JPG` **1879 bytes** and `/photos/IMG_0005.JPG` **1880 bytes**.
- Final `LIST`: `/photos/IMG_0001.JPG`…`IMG_0005.JPG` with sizes **1829, 1883, 1883, 1879, 1880** bytes — the sequential counter advanced IMG_0003→IMG_0005 with no overwrites, confirming the post-reboot max-index scan and counter persistence.
- Net result: the GC2145 RGB565→`frame2jpg()`→SD_MMC 1-bit capture pipeline is fully user-verified across 5 saved JPEGs (IMG_0001–IMG_0005). The camera phase and user verification are complete; the only remaining camera work is the future Hub upload seam (`uploadToHubJpeg()`).
