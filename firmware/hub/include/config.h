#pragma once
#include <Arduino.h>   // brings in uint8_t etc. (and silences the editor linter)
// =====================================================================
//  config.h  —  settings for the Museum Hub (running on the Track ESP32)
//  Edit SSID/password here, then print the matching WiFi QR (README.md).
// =====================================================================

// ---- WiFi SoftAP (the offline network phones join) ----
// Keep the password >= 8 characters. Channel MUST match every ESP-NOW node.
static const char*    HUB_SSID    = "MuseumHub";
static const char*    HUB_PASS    = "museum2026";
static const uint8_t  HUB_CHANNEL = 1;            // ESP-NOW + SoftAP share this
static const uint8_t  HUB_IP[]    = {192, 168, 4, 1};

// ---- Track hardware ----
static const uint8_t  NEOPIXEL_PIN   = 15;        // WS2812 DIN
static const uint8_t  STATION_COUNT  = 7;         // one LED per artifact station

// ---- WS2812 chain + light show ----
// Physical length of the LED chain (e.g. 7 boards x 8 pixels = 56). STATION_COUNT
// stations still map to the FIRST pixels; the idle + emergency shows animate the
// WHOLE chain. Set LED_CHAIN_LENGTH to the number you actually wired.
static const uint8_t  LED_CHAIN_LENGTH = 56;

// Global brightness cap (0..255). Also a soft current limit: 56 pixels of full
// white can pull ~3.4 A, so keep this low. 60 is gentle on the eyes and the supply.
static const uint8_t  LED_BRIGHTNESS = 60;

// ---- idle "color flow" (a comet runs around the chain; its hue slowly shifts) ----
static const uint16_t LED_FLOW_STEP_MS = 55;    // head advances one pixel this often
static const uint16_t LED_FLOW_HUE_MS  = 400;   // comet color shifts one step this often
static const uint8_t  LED_FLOW_TAIL    = 14;    // fading tail length, in pixels

// ---- emergency RED strobe (fire-alarm style: two red flashes + a pause) ----
// All pixels flash solid RED together in a double-pulse that repeats -- the
// universal building-emergency signal (red = danger / evacuate). Red only, not
// the old red/blue "ambulance bar": red/blue reads as a police/ambulance light,
// not a museum evacuation. Timing stays UNDER 3 Hz everywhere (3 Hz is the
// photosensitive-epilepsy ceiling; the seizure-risk band is 5-70 Hz), and the
// flashes are full-field synchronized, the way NFPA 72 fire strobes specify it.
//   ON -> GAP -> ON -> PAUSE = 150+200+150+500 = 1000 ms (one double-pulse / s)
static const uint16_t LED_EMERGENCY_ON_MS    = 150;   // length of each red flash
static const uint16_t LED_EMERGENCY_GAP_MS   = 200;   // dark gap between the two flashes
static const uint16_t LED_EMERGENCY_PAUSE_MS = 500;   // longer dark pause after the pair
static const uint8_t  BUZZER_PIN     = 4;         // siren (PASSIVE buzzer -- see main.cpp)

// ---- Emergency button (from the proven Emergency_Switch sketch) ----
// Press once -> emergency LATCHES on (LEDs blink, siren wails, robot evacuates).
// Hold 3 s   -> emergency clears. The admin dashboard can also clear it.
// The button module outputs HIGH when pressed and has its own pull-down.
static const uint8_t  EMERGENCY_BUTTON_PIN = 21;

// ---- robot motion ----
// How long the Hub keeps showing "moving" after the last movement report.
// The robot node re-sends MSG_LOCATION while it drives, so this only expires
// once the reports stop (or after the admin "Advance robot (test)" button).
// Without it the dashboard/LED would stay stuck on "moving" forever.
static const uint32_t ROBOT_MOVE_SETTLE_MS = 2500;

// ---- emergency broadcast ----
// This Hub is the single source of truth for the emergency state, and ESP-NOW
// broadcasts are unacknowledged. Rather than retry-and-hope on each change, the
// current state is re-broadcast on this interval forever. Receivers act only on
// a transition, so a robot that reboots, misses a packet, or powers up late
// converges on the right state within one interval.
static const uint8_t  HUB_NODE_ID            = 10;
static const uint32_t EMERGENCY_BROADCAST_MS = 2000;   // < the robot's needs, cheap

// A change of state is the one packet that must not wait for the next timed
// re-broadcast: while it is in flight the robot is still narrating at a station
// with the siren already sounding. Because a single unacknowledged broadcast
// can simply be lost, every transition goes out as a short burst and only then
// falls back to the heartbeat above. Six frames 60 ms apart cost ~1/3 s of air
// time and make a missed transition vanishingly unlikely.
static const uint8_t  EMERGENCY_BURST_COUNT  = 6;
static const uint32_t EMERGENCY_BURST_GAP_MS = 60;

// Station ids are STABLE internal keys (numeric stationId 1..N -> this id),
// sent to the web app, which looks up the human name in js/data.js (ARTIFACTS).
// The displayed artifact at each station is set in data.js and may be renamed
// without touching here; keep the id strings stable so the ESP-NOW mapping holds.
// Current labels (renamed in data.js):
//   station 1 jamdani     -> Terracotta Plaques
//   station 2 nakshi      -> Traditional Wooden Boat
//   station 3 liberation  -> Nakshi Kantha
//   station 4 terracotta  -> Traditional Bangladeshi Pottery
//   station 5 ektara      -> Traditional Brass Chair
//   station 6 palki       -> Bamboo and Cane
//   station 7 tiger       -> Jamdani Saree
static const char* STATION_IDS[STATION_COUNT] = {
  "jamdani", "nakshi", "liberation", "terracotta", "ektara", "palki", "tiger"
};

// ---- artifact sensors wired straight to the Hub (DHT11 + MQ) ----
// Every pin listed here is probed as a DHT11 in turn, one pin per
// HUB_SENSOR_PERIOD_MS, and the result is published on /api/state, /api/sensors
// and the built-in /scan page. Pins that also have an ADC are sampled as an
// analog (MQ-style) input on the same pass.
//
// IMPORTANT -- GPIO34/35/36/39 are INPUT-ONLY on the ESP32. They have no output
// driver, so pinMode(pin, OUTPUT) is a silent no-op and the 20 ms start-low
// pulse a DHT11 needs never reaches the wire: a DHT11 on one of those pins can
// never answer, whatever it is wired to. They do carry ADC1 channels (usable
// while WiFi is up, unlike ADC2), which is what an MQ-135 analog output wants.
// They are still probed here so the scan reports what was measured.
//
// None of these clash with the NeoPixel (15), buzzer (4), button (21) or the
// microSD SPI pins (5/18/19/23).
static const uint8_t  HUB_SENSOR_PINS[]  = {14, 16, 17, 27, 34, 35, 36, 39};
static const uint8_t  HUB_SENSOR_COUNT   = sizeof(HUB_SENSOR_PINS);

// What is actually wired to each pin, same order as HUB_SENSOR_PINS.
//   ROLE_DHT11 -> single-wire DHT11: probed with the 20 ms start pulse.
//   ROLE_MQ_D0 -> an MQ module's D0 (LM393 comparator) output: polled as a
//                 digital level every HUB_MQ_POLL_MS and NEVER driven, so the
//                 start pulse can no longer fight a module's push-pull output.
// GPIO14 and GPIO27 were confirmed by the first scan (5/5 valid DHT11 frames).
enum : uint8_t { ROLE_DHT11 = 0, ROLE_MQ_D0 = 1 };
static const uint8_t HUB_SENSOR_ROLE[HUB_SENSOR_COUNT] = {
  ROLE_DHT11,   // GPIO14 -- confirmed DHT11
  ROLE_MQ_D0,   // GPIO16 -- continuity says pulled_high: something IS wired
  ROLE_MQ_D0,   // GPIO17 -- continuity says FLOATING: nothing connected
  ROLE_DHT11,   // GPIO27 -- confirmed DHT11
  ROLE_MQ_D0,   // GPIO34 -- reads 2.85 V, wanders: looks like AO, not D0
  ROLE_MQ_D0,   // GPIO35 -- flat 0 raw / 142 mV floor: nothing detected here
  ROLE_MQ_D0,   // GPIO36 -- reads 2.24 V, wanders: looks like AO, not D0
  ROLE_MQ_D0,   // GPIO39 -- reads 2.87 V, wanders: looks like AO, not D0
};
// ROLE_MQ_D0 samples BOTH the digital level and (on ADC1 pins) the millivolts,
// so it stays correct whether a pin turns out to be D0 or AO. The distinction
// shows up under gas: a D0 comparator FLIPS, an AO slides. See README.md.

// Gas verdict, in raw 12-bit ADC counts. On THIS venue's sensors a HIGH raw
// reading is the clean-air resting state and a LOW one means gas -- the
// opposite of the textbook MQ analog-output curve, but it is what the wired
// modules actually report here, so the dashboard is calibrated to match rather
// than to the datasheet. At or above this count the air is reported good,
// below it bad. UNCALIBRATED beyond that: watch the live figure in clean air
// and adjust. Keep this in step with the same threshold in webapp/admin.html
// and the /scan page in main.cpp.
static const uint16_t HUB_GAS_THRESHOLD_RAW = 1500;

// What a D0-only pin (no ADC to sample) publishes instead of a raw count: the
// comparator only has two states, so these sit deliberately at the extremes of
// the range -- CLEAR reads as unambiguously good, ALARM as unambiguously bad --
// rather than picked to merely straddle HUB_GAS_THRESHOLD_RAW.
static const uint16_t HUB_GAS_D0_CLEAR_RAW = 4095;
static const uint16_t HUB_GAS_D0_ALARM_RAW = 0;

// A station whose own sensor has not reported for this long falls back to an
// interpolated value. Without it one loose jumper freezes a row on whatever it
// last read, which looks identical to a healthy steady reading.
static const uint32_t HUB_HEALTH_STALE_MS = 30000;

// How often the D0 lines are sampled. A comparator output flips the instant gas
// crosses the trim-pot threshold, so this has to be far faster than the DHT
// rotation or the flip that proves the sensor is real would be missed.
static const uint32_t HUB_MQ_POLL_MS = 100;

// Which artifact each pin above belongs to (1..STATION_COUNT), same order.
// 0 = unmapped: the pin shows up in the pin-scan table only.
//
// A station's row is shared: the DHT11 fills temperature + humidity, the MQ
// fills gas, and neither overwrites the other. That is why a DHT and an MQ can
// both point at station 1.
//
// THIS PAIRING IS A PLACEHOLDER -- it groups the pins that actually produced
// data so the dashboard has something real to show. Change the numbers to match
// where the sensors physically sit; nothing else needs touching.
// Labels follow the renamed artifacts in js/data.js (sensors did not move,
// so the numbers are unchanged -- only the station labels did):
//   GPIO14 DHT11 + GPIO34 gas -> station 1 (Terracotta Plaques)
//   GPIO27 DHT11 + GPIO36 gas -> station 2 (Traditional Wooden Boat)
//   GPIO39 gas                -> station 3 (Nakshi Kantha)
//   GPIO16 gas                -> station 4 (Traditional Bangladeshi Pottery)
//   GPIO17, GPIO35            -> unmapped: the scan found nothing on them
static const uint8_t  HUB_SENSOR_STATION[HUB_SENSOR_COUNT] = {
  1,   // GPIO14 DHT11  -> station 1 (Terracotta Plaques)
  4,   // GPIO16 gas    -> station 4 (Traditional Bangladeshi Pottery)
  0,   // GPIO17        -> nothing connected
  2,   // GPIO27 DHT11  -> station 2 (Traditional Wooden Boat)
  1,   // GPIO34 gas    -> station 1 (Terracotta Plaques)
  0,   // GPIO35        -> nothing detected
  2,   // GPIO36 gas    -> station 2 (Traditional Wooden Boat)
  3,   // GPIO39 gas    -> station 3 (Nakshi Kantha)
};

// How often the scan moves to the next pin. A DHT11 must not be read faster
// than once a second; at 2.5 s per pin an 8-pin sweep takes 20 s, so each
// sensor is re-read far slower than its own limit.
static const uint32_t HUB_SENSOR_PERIOD_MS = 2500;

// ---- microSD card for the camera gallery (SPI / VSPI) ----
// Wiring matches the already-proven firmware/sdcard bring-up project:
//   MISO -> GPIO19   MOSI -> GPIO23   SCK -> GPIO18   CS -> GPIO5
// None of these clash with the NeoPixel (GPIO15) or the buzzer (GPIO4).
// The card is OPTIONAL: when it is missing or unreadable the Hub keeps the
// gallery in LittleFS instead, so the dashboard works either way.
static const bool     HUB_SD_ENABLED   = true;   // false = never try the card
static const uint8_t  HUB_SD_SCK_PIN   = 18;
static const uint8_t  HUB_SD_MISO_PIN  = 19;
static const uint8_t  HUB_SD_MOSI_PIN  = 23;
static const uint8_t  HUB_SD_CS_PIN    = 5;
static const uint32_t HUB_SD_FREQ_HZ   = 4000000UL;  // 4 MHz is safe on jumpers

// ---- camera gallery ----
// How many photos the dashboard keeps. Older ones are deleted from the card /
// flash automatically, newest first. A QVGA JPEG is ~2 KB, so even the LittleFS
// fallback holds the whole gallery comfortably.
static const uint8_t GALLERY_MAX_PHOTOS = 12;   // must stay <= 32
static const char*    GALLERY_DIR        = "/photos";
