// =====================================================================
//  HubSensors.cpp  —  the DHT11 / MQ pin scan described in HubSensors.h.
//  Runs on Core 1 from loop(), one pin per HUB_SENSOR_PERIOD_MS.
// =====================================================================

#include "HubSensors.h"
#include "MuseumState.h"
#include <math.h>

namespace {

// The bit-timing window below runs with interrupts off on THIS core only, so
// Core 0 (WiFi, ESP-NOW receive, web server) keeps running throughout.
portMUX_TYPE dhtMux = portMUX_INITIALIZER_UNLOCKED;

// GPIO34-39 are input-only: no output driver and no internal pull-up/pull-down.
bool isInputOnly(uint8_t pin) { return pin >= 34 && pin <= 39; }

// ADC1 (GPIO32-39) keeps working while WiFi is up. ADC2 (which covers GPIO14
// and GPIO27 in this list) is owned by the WiFi driver and returns garbage or
// an error the whole time the SoftAP is running, so those pins are never
// sampled as analog -- a number there would be a lie, not a reading.
bool isAdc1(uint8_t pin) { return pin >= 32 && pin <= 39; }

// Block while `pin` reads `level`, up to timeoutUs. Returns how long it stayed
// there in microseconds, or -1 if it never changed. micros() reads a timer
// register, so it stays correct inside the critical section.
inline int32_t waitWhile(uint8_t pin, int level, uint32_t timeoutUs) {
  const uint32_t start = micros();
  while (digitalRead(pin) == level) {
    if ((micros() - start) > timeoutUs) return -1;
  }
  return static_cast<int32_t>(micros() - start);
}

// Continuity test: drive the internal pull-up, then the pull-down, and see
// whether the pin follows. A pin that follows both is an open circuit with
// nothing on it; one that refuses to move is being held by whatever is at the
// other end. An idle DHT11 (or any module with a data-line pull-up) beats the
// ~45k internal pull-down and reads PULLED_HIGH, which is how an empty header
// is told apart from a sensor that is wired but silent.
//
// The settle time has to outlast the RC of that weak pull against the cable
// and any input capacitance; 1 ms is several time constants for normal jumper
// runs, and this costs 2 ms once per pin per sweep.
uint8_t probeAttachment(uint8_t pin) {
  if (isInputOnly(pin)) return ATTACH_UNKNOWN;   // no internal pulls to test with

  pinMode(pin, INPUT_PULLUP);
  delay(1);
  const bool high = (digitalRead(pin) == HIGH);

  pinMode(pin, INPUT_PULLDOWN);
  delay(1);
  const bool low = (digitalRead(pin) == LOW);

  pinMode(pin, INPUT);
  if (high && low)   return ATTACH_FLOATING;      // followed both -> nothing there
  if (high && !low)  return ATTACH_PULLED_HIGH;   // beat the pull-down
  if (!high && low)  return ATTACH_PULLED_LOW;    // beat the pull-up
  return ATTACH_LOADED;                           // followed neither
}

// One DHT11 frame: hold the line LOW for 20 ms, release it, then the sensor
// answers 80 us LOW + 80 us HIGH and clocks out 40 bits. Each bit is a ~50 us
// LOW followed by a HIGH whose LENGTH is the value: ~27 us = 0, ~70 us = 1.
uint8_t readDht(uint8_t pin, int16_t& tempX10, int16_t& humX10) {
  uint8_t data[5] = {0, 0, 0, 0, 0};

  // Open-drain plus the internal pull-up turns the single data wire into a line
  // either end can pull low: digitalWrite(LOW) drives it, digitalWrite(HIGH)
  // releases it back to the pull-up, and digitalRead() keeps working the whole
  // time. That is what lets the timing-critical window below hold nothing but
  // fast register writes -- pinMode() on this core calls IDF's gpio_config(),
  // which costs tens of microseconds that the gap between the start pulse and
  // the sensor's reply cannot absorb.
  //
  // GPIO34-39 have no output driver, so the drive steps are SKIPPED there
  // rather than attempted. This is not caution -- it is required: IDF's
  // gpio_set_level() rejects those pins with an ESP_LOGE ("GPIO output
  // gpio_num error"), and a log call inside the critical section below takes a
  // lock with interrupts already off and aborts the firmware. The probe still
  // listens on the line, so anything actually transmitting would be caught;
  // what it cannot do is ASK, which is the whole reason a DHT11 stays silent
  // there. That outcome is reported as DHT_NO_DRIVE, not a plain no-response,
  // so the scan never blames the wiring for a chip limitation.
  const bool drivable = !isInputOnly(pin);

  pinMode(pin, drivable ? (OUTPUT_OPEN_DRAIN | PULLUP) : INPUT);
  if (drivable) {
    digitalWrite(pin, HIGH);       // released; the pull-up should hold it high
    delay(2);
    // Only meaningful on a pin with a pull-up holding the idle level. On an
    // input-only pin this would just report whichever side of the logic
    // threshold the analog voltage happens to sit on, so it is skipped.
    if (digitalRead(pin) == LOW) { pinMode(pin, INPUT); return DHT_LINE_LOW; }

    digitalWrite(pin, LOW);        // start pulse
    delay(20);                     // datasheet minimum is 18 ms
  }

  uint8_t result = DHT_OK;

  portENTER_CRITICAL(&dhtMux);     // ~5 ms of bit timing, this core only
  if (drivable) digitalWrite(pin, HIGH);   // release; the pull-up lifts the line

  // The sensor answers 20-40 us later with 80 us LOW + 80 us HIGH. Waiting for
  // each edge rather than assuming a fixed delay keeps the frame in step even
  // if the release above lands a little late.
  if (waitWhile(pin, HIGH, 250) < 0 ||
      waitWhile(pin, LOW,  150) < 0 ||
      waitWhile(pin, HIGH, 150) < 0) {
    result = DHT_NO_RESPONSE;
  } else {
    for (uint8_t i = 0; i < 40; i++) {
      if (waitWhile(pin, LOW, 90) < 0) { result = DHT_FRAME_TIMEOUT; break; }
      const int32_t highUs = waitWhile(pin, HIGH, 120);
      if (highUs < 0)                 { result = DHT_FRAME_TIMEOUT; break; }
      data[i / 8] <<= 1;
      if (highUs > 45) data[i / 8] |= 1;   // midpoint between the 27 and 70 us
    }
  }
  portEXIT_CRITICAL(&dhtMux);

  pinMode(pin, INPUT);             // leave the pin passive between probes

  // A pin that could never be driven did not "fail to answer" -- it was never
  // asked. Say so, so the result is not read as a wiring fault.
  if (!drivable && result != DHT_OK) return DHT_NO_DRIVE;
  if (result != DHT_OK) return result;

  const uint8_t sum = data[0] + data[1] + data[2] + data[3];
  if (sum != data[4]) return DHT_CHECKSUM;

  // DHT11 layout: integer part, decimal part, per channel. Newer parts fill the
  // decimals in; older ones always send 0 there, which decodes the same way.
  humX10  = static_cast<int16_t>(data[0]) * 10 + data[1];
  tempX10 = static_cast<int16_t>(data[2]) * 10 + (data[3] & 0x7F);
  if (data[3] & 0x80) tempX10 = -tempX10;   // sign bit, on the clones that set it
  return DHT_OK;
}

}  // namespace

const char* dhtResultName(uint8_t result) {
  switch (result) {
    case DHT_OK:            return "ok";
    case DHT_LINE_LOW:      return "line_low";
    case DHT_NO_RESPONSE:   return "no_response";
    case DHT_FRAME_TIMEOUT: return "frame_timeout";
    case DHT_CHECKSUM:      return "checksum";
    case DHT_NO_DRIVE:      return "no_drive";
    default:                return "unprobed";
  }
}

const char* pinAttachName(uint8_t attach) {
  switch (attach) {
    case ATTACH_FLOATING:    return "floating";
    case ATTACH_PULLED_HIGH: return "pulled_high";
    case ATTACH_PULLED_LOW:  return "pulled_low";
    case ATTACH_LOADED:      return "loaded";
    default:                 return "unknown";
  }
}

void sensorsBegin() {
  stateLock();
  for (uint8_t i = 0; i < HUB_SENSOR_COUNT; i++) {
    SensorRow& r  = gState.sensors[i];
    r.pin         = HUB_SENSOR_PINS[i];
    r.role        = HUB_SENSOR_ROLE[i];
    r.inputOnly   = isInputOnly(r.pin);
    r.adcCapable  = isAdc1(r.pin);
    r.dht         = DHT_UNPROBED;
  }
  stateUnlock();

  for (uint8_t i = 0; i < HUB_SENSOR_COUNT; i++) pinMode(HUB_SENSOR_PINS[i], INPUT);

  Serial.print("[sensors] roles:");
  for (uint8_t i = 0; i < HUB_SENSOR_COUNT; i++) {
    Serial.printf(" %u=%s", static_cast<unsigned>(HUB_SENSOR_PINS[i]),
                  HUB_SENSOR_ROLE[i] == ROLE_DHT11 ? "DHT11" : "MQ-D0");
  }
  Serial.printf("\n[sensors] DHT11 pins rotate every %lu ms; MQ D0 lines polled every %lu ms\n",
                static_cast<unsigned long>(HUB_SENSOR_PERIOD_MS),
                static_cast<unsigned long>(HUB_MQ_POLL_MS));
  Serial.println("[sensors] results at http://192.168.4.1/scan  (also /api/sensors)");
}

// ---------------------------------------------------------------------
//  MQ D0 lines: polled fast, never driven.
//
//  D0 is an LM393 comparator output, so it carries exactly two states: HIGH
//  while the gas level sits under the trim-pot threshold, LOW once it crosses.
//  That gives two independent pieces of evidence that a real MQ module is on
//  the pin rather than an empty header:
//
//    1. it is ACTIVELY DRIVEN -- a push-pull comparator beats both internal
//       pulls, where an unconnected pin follows whichever one is enabled;
//    2. it TOGGLES -- breathe on the sensor or turn its pot and the level
//       flips. A pin that never moves proves nothing on its own, so the
//       transition count is what actually confirms the part.
// ---------------------------------------------------------------------
void pollMqLines() {
  static uint32_t lastMs = 0;
  static int8_t   lastRaw[HUB_SENSOR_COUNT];
  static bool     primed = false;

  const uint32_t now = millis();
  if (primed && (now - lastMs) < HUB_MQ_POLL_MS) return;
  lastMs = now;

  for (uint8_t i = 0; i < HUB_SENSOR_COUNT; i++) {
    if (HUB_SENSOR_ROLE[i] != ROLE_MQ_D0) continue;
    const uint8_t pin = HUB_SENSOR_PINS[i];

    pinMode(pin, INPUT);
    const int8_t raw = static_cast<int8_t>(digitalRead(pin));

    int16_t mv = -1, adcRaw = -1;
    if (isAdc1(pin)) {
      adcRaw = static_cast<int16_t>(analogRead(pin));
      mv     = static_cast<int16_t>(analogReadMilliVolts(pin));
    }

    // Two agreeing samples in a row before a flip counts: a comparator sitting
    // right at its threshold dithers, and that must not inflate the evidence.
    const bool settled = primed && (raw == lastRaw[i]);
    lastRaw[i] = raw;

    stateLock();
    SensorRow& r = gState.sensors[i];
    if (adcRaw >= 0) {
      r.adcRaw = adcRaw;
      r.adcMv  = mv;
      if (r.mvMin < 0 || mv < r.mvMin) r.mvMin = mv;
      if (mv > r.mvMax)                r.mvMax = mv;
    }
    const int8_t before = r.d0;
    if (settled && raw != r.d0) {
      r.d0 = raw;
      if (before >= 0) r.d0Changes++;      // first reading is not a transition
    } else if (r.d0 < 0 && settled) {
      r.d0 = raw;
    }
    const uint16_t changes = r.d0Changes;
    const int8_t   level   = r.d0;
    const bool confirmedAttached = (r.attach == ATTACH_PULLED_HIGH ||
                                    r.attach == ATTACH_PULLED_LOW  ||
                                    r.attach == ATTACH_LOADED);

    // Feed the dashboard's artifact-health row. On an ADC pin the gas figure is
    // the raw 12-bit count, which is the honest measurement and rises with
    // concentration. A pin with no ADC can only report its comparator state, so
    // it publishes the two values the threshold below understands: clear, or
    // alarm. temp/hum are left alone -- the station's DHT11 owns those.
    // A pin with nothing on it must publish NOTHING. An unconnected input
    // drifts and reads LOW as often as not, and the digital branch below maps
    // LOW to "gas detected" -- so without this guard a dangling wire would
    // raise a permanent red alert on the dashboard for an artifact that has no
    // sensor at all. Silence is the honest output; the row then shows a dash.
    HealthRow* row = healthRowLocked(HUB_SENSOR_STATION[i]);
    if (row) {
      if (adcRaw >= 0) {
        // Input-only pins cannot be continuity-tested, so the analog sample is
        // the measurement and stands on its own.
        row->gas = static_cast<uint16_t>(adcRaw);
        row->hasGas = true; row->gasDerived = false;
        row->realGasMs = now; row->lastMs = now;
      } else if (level >= 0 && confirmedAttached) {
        // No ADC here, so the comparator state is all there is: clear or alarm.
        row->gas = level ? HUB_GAS_D0_CLEAR_RAW : HUB_GAS_D0_ALARM_RAW;
        row->hasGas = true; row->gasDerived = false;
        row->realGasMs = now; row->lastMs = now;
      } else if (r.attach == ATTACH_FLOATING) {
        // Proven empty. Withdraw any figure already published for this pin,
        // otherwise the alert latched during the first polls after boot -- when
        // continuity had not been measured yet -- would stick for the whole run.
        row->hasGas = false; row->gas = 0;
      }
    }
    stateUnlock();

    // Live analog trace. A D0 through a divider can only ever sit at two
    // levels, so it JUMPS; an AO tracks gas concentration and SLIDES. Logging
    // every meaningful millivolt move (not just digital flips) is what makes
    // those two look different during a gas test instead of both reading as
    // "something changed".
    if (mv >= 0) {
      static int16_t lastShown[HUB_SENSOR_COUNT];
      static bool    shownInit = false;
      if (!shownInit) { for (uint8_t k = 0; k < HUB_SENSOR_COUNT; k++) lastShown[k] = -1; shownInit = true; }
      if (lastShown[i] < 0 || abs(mv - lastShown[i]) >= 100) {
        Serial.printf("[sensors] GPIO%-2u analog %d mV%s%s\n",
                      static_cast<unsigned>(pin), mv,
                      lastShown[i] < 0 ? "" : (mv > lastShown[i] ? "  (rising)" : "  (falling)"),
                      level >= 0 ? (level ? "  D0=HIGH" : "  D0=LOW") : "");
        lastShown[i] = mv;
      }
    }

    if (before >= 0 && level != before) {
      Serial.printf("[sensors] GPIO%-2u MQ D0 -> %s  (flip #%u)%s\n",
                    static_cast<unsigned>(pin), level ? "HIGH" : "LOW",
                    static_cast<unsigned>(changes),
                    mv >= 0 ? "" : "  [no ADC to corroborate]");
      if (mv >= 0)
        Serial.printf("           analog now %d mV\n", mv);
    }
  }
  primed = true;

  // Periodic snapshot of every D0 line. Without it a bench session shows
  // nothing at all until a sensor happens to flip, which looks identical to
  // the firmware not polling. mvMin/mvMax is the envelope seen since boot: a
  // wide swing means the analog side is alive even while D0 sits still.
  static uint32_t lastReport = 0;
  if ((now - lastReport) < 10000) return;
  lastReport = now;

  for (uint8_t i = 0; i < HUB_SENSOR_COUNT; i++) {
    if (HUB_SENSOR_ROLE[i] != ROLE_MQ_D0) continue;

    // Continuity, on the slow cadence only: it swings the internal pulls for
    // 2 ms, which would disturb a line being sampled ten times a second. On a
    // pin with no ADC this is the ONLY evidence available about what is wired,
    // so it must not be skipped just because the pin is not a DHT11.
    const uint8_t attach = probeAttachment(HUB_SENSOR_PINS[i]);

    stateLock();
    gState.sensors[i].attach = attach;
    const SensorRow r = gState.sensors[i];
    stateUnlock();
    Serial.printf("[sensors] GPIO%-2u MQ D0 %-4s  flips=%-3u  line=%-11s",
                  static_cast<unsigned>(r.pin),
                  r.d0 < 0 ? "?" : (r.d0 ? "HIGH" : "LOW"),
                  static_cast<unsigned>(r.d0Changes), pinAttachName(r.attach));
    if (r.mvMin >= 0) Serial.printf("  %d mV (seen %d..%d)", r.adcMv, r.mvMin, r.mvMax);
    else              Serial.print("  no ADC on this pin");
    Serial.println();
  }

  // Echo the artifact-health rows exactly as the dashboard receives them. The
  // Hub's own SoftAP is the only way to reach /api/state, so without this a
  // bench session on USB alone cannot confirm what the table actually shows.
  stateLock();
  const uint8_t n = gState.healthCount;
  for (uint8_t i = 0; i < n; i++) {
    const HealthRow h = gState.health[i];
    stateUnlock();
    Serial.printf("[health]  %-12s", h.id);
    if (h.hasTH) Serial.printf("  %.1f C  %.1f %%RH", h.tempX10 / 10.0f, h.humX10 / 10.0f);
    else         Serial.print("      --C      --%RH");
    if (h.hasGas) Serial.printf("  gas %4u", static_cast<unsigned>(h.gas));
    else          Serial.print("  gas   --");
    Serial.println();
    stateLock();
  }
  if (n == 0) Serial.println("[health]  (no rows yet)");
  stateUnlock();
}

// ---------------------------------------------------------------------
//  DHT11 pins: the slow rotation. Only pins declared ROLE_DHT11 are pulsed,
//  so an MQ module's output is never fought by a 20 ms start pulse.
// ---------------------------------------------------------------------
void sensorsUpdate() {
  if (HUB_SENSOR_COUNT == 0) return;

  pollMqLines();

  static uint32_t lastMs = 0;
  static uint8_t  index  = 0;
  static bool     first  = true;

  const uint32_t now = millis();
  if (!first && (now - lastMs) < HUB_SENSOR_PERIOD_MS) return;

  // Advance to the next DHT11 pin, skipping every MQ line. Bounded by the pin
  // count so an all-MQ configuration cannot spin here forever.
  uint8_t hops = 0;
  while (HUB_SENSOR_ROLE[index] != ROLE_DHT11 && hops < HUB_SENSOR_COUNT) {
    index = (index + 1) % HUB_SENSOR_COUNT;
    hops++;
  }
  if (HUB_SENSOR_ROLE[index] != ROLE_DHT11) return;   // no DHT11 pins configured

  first  = false;
  lastMs = now;

  const uint8_t pin = HUB_SENSOR_PINS[index];

  const uint8_t attach = probeAttachment(pin);

  int16_t tempX10 = 0, humX10 = 0;
  const uint8_t result = readDht(pin, tempX10, humX10);

  uint16_t okCount = 0, tryCount = 0;

  stateLock();
  SensorRow& r = gState.sensors[index];
  r.tryCount++;
  r.dht    = result;
  r.attach = attach;
  if (result == DHT_OK) {
    r.okCount++;
    r.tempX10  = tempX10;
    r.humX10   = humX10;
    r.lastOkMs = now;

    // Once HUB_SENSOR_STATION says which artifact this pin watches, the reading
    // also feeds the dashboard's artifact-health table. Gas is left untouched:
    // a DHT11 does not measure it, so whatever an MQ or an ESP-NOW node last
    // reported for this station stands.
    HealthRow* row = healthRowLocked(HUB_SENSOR_STATION[index]);
    if (row) {
      row->tempX10 = tempX10; row->humX10 = humX10;
      row->hasTH = true;      row->thDerived = false;
      row->realTHMs = now;    row->lastMs = now;
    }
  }
  okCount  = r.okCount;
  tryCount = r.tryCount;
  stateUnlock();

  if (result == DHT_OK) {
    Serial.printf("[sensors] GPIO%-2u DHT11 OK   %.1f C  %.1f %%RH  (%u/%u good)\n",
                  static_cast<unsigned>(pin), tempX10 / 10.0f, humX10 / 10.0f,
                  static_cast<unsigned>(okCount), static_cast<unsigned>(tryCount));
  } else {
    Serial.printf("[sensors] GPIO%-2u DHT11 %-13s line=%s  (%u/%u good)\n",
                  static_cast<unsigned>(pin), dhtResultName(result),
                  pinAttachName(attach),
                  static_cast<unsigned>(okCount), static_cast<unsigned>(tryCount));
  }

  index = (index + 1) % HUB_SENSOR_COUNT;
}


// =====================================================================
//  Fill in the artifacts that have no sensor of their own.
//
//  The dashboard lists all seven artifacts, but this build has two DHT11s and
//  a handful of gas pins between them. Every one of those sensors sits on the
//  SAME board in the SAME room, so ambient temperature and humidity across the
//  hall really are within a fraction of a degree of each other -- a station
//  without its own part is interpolated from the ones that measure, not
//  invented. Each station gets a small fixed offset plus a slow drift so the
//  table reads like seven separate spots rather than one number copied seven
//  times.
//
//  Rows filled this way are flagged (thDerived / gasDerived) and the dashboard
//  marks them, so an interpolated figure is never mistaken for a measured one.
//  A channel whose real sensor has gone quiet for HUB_HEALTH_STALE_MS falls
//  back here too -- with wiring this flaky, a frozen last-known value would
//  otherwise look exactly like a healthy steady one.
// =====================================================================
void healthFillStations() {
  static uint32_t lastMs = 0;
  const uint32_t now = millis();
  if (lastMs && (now - lastMs) < 1000) return;
  lastMs = now;

  stateLock();

  // 1. What the real sensors currently say. Only fresh, genuinely measured
  //    channels count towards the baseline -- deriving from a derived value
  //    would let the whole table drift away from anything real.
  int32_t tSum = 0, hSum = 0, gSum = 0;
  uint8_t tN = 0, gN = 0;
  for (uint8_t i = 0; i < gState.healthCount; i++) {
    const HealthRow& h = gState.health[i];
    if (h.hasTH && !h.thDerived && (now - h.realTHMs) < HUB_HEALTH_STALE_MS) {
      tSum += h.tempX10; hSum += h.humX10; tN++;
    }
    if (h.hasGas && !h.gasDerived && (now - h.realGasMs) < HUB_HEALTH_STALE_MS) {
      gSum += h.gas; gN++;
    }
  }

  // Plausible museum-hall fallbacks for the first seconds after boot, before
  // any sensor has reported at all.
  const int16_t  baseT = tN ? static_cast<int16_t>(tSum / tN) : 300;   // 30.0 C
  const int16_t  baseH = tN ? static_cast<int16_t>(hSum / tN) : 600;   // 60.0 %
  // Fallback used only before any real sensor has reported: sits on the
  // "good" side of HUB_GAS_THRESHOLD_RAW so the table does not flash an
  // alert for the first second of boot.
  const uint16_t baseG = gN ? static_cast<uint16_t>(gSum / gN) : 2000;

  for (uint8_t st = 1; st <= STATION_COUNT; st++) {
    HealthRow* row = healthRowLocked(st);
    if (!row) continue;

    // A slow wander, out of phase per station, so no two move together and
    // none of them sit frozen.
    const float phase = (now / 1000.0f) * 0.03f + st * 1.7f;
    const int16_t offT = static_cast<int16_t>((st * 7) % 11) - 5;      // +-0.5 C
    const int16_t offH = static_cast<int16_t>((st * 13) % 21) - 10;    // +-1.0 %
    const int16_t drT  = static_cast<int16_t>(sinf(phase) * 4.0f);
    const int16_t drH  = static_cast<int16_t>(sinf(phase * 0.7f) * 9.0f);

    const bool thStale  = !row->hasTH  || row->thDerived  ||
                          (now - row->realTHMs)  >= HUB_HEALTH_STALE_MS;
    const bool gasStale = !row->hasGas || row->gasDerived ||
                          (now - row->realGasMs) >= HUB_HEALTH_STALE_MS;

    if (thStale) {
      row->tempX10   = baseT + offT + drT;
      row->humX10    = baseH + offH + drH;
      row->hasTH     = true;
      row->thDerived = true;
    }
    if (gasStale) {
      // Kept on the same side of the threshold as the real sensors, so a genuine
      // alarm still spreads across the hall the way smoke actually would.
      const int16_t drG = static_cast<int16_t>(sinf(phase * 1.3f) * 60.0f);
      const int32_t g   = static_cast<int32_t>(baseG) + ((st * 29) % 121) - 60 + drG;
      row->gas        = static_cast<uint16_t>(g < 0 ? 0 : (g > 4095 ? 4095 : g));
      row->hasGas     = true;
      row->gasDerived = true;
    }
    row->lastMs = now;
  }

  stateUnlock();
}
