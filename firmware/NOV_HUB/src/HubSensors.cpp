#include "HubSensors.h"
#include "MuseumState.h"
#include <DHT.h>
#include <math.h>

namespace {
DHT dhtSensors[STATION_COUNT] = {
  DHT(HUB_DHT_PINS[0], DHT11), DHT(HUB_DHT_PINS[1], DHT11), DHT(HUB_DHT_PINS[2], DHT11),
  DHT(HUB_DHT_PINS[3], DHT11), DHT(HUB_DHT_PINS[4], DHT11), DHT(HUB_DHT_PINS[5], DHT11),
};

struct ProxZone {
  int level = HIGH;
  uint32_t changedAt = 0;
  bool published = false;
};
ProxZone proxZones[HUB_PROX_ZONE_COUNT];
uint32_t nextDhtAt = 0, lastMq = 0;
uint8_t nextDht = 0;
bool stationInputsAvailable = false;
constexpr uint16_t MQ_NO_SIGNAL_MAX = 8;

struct MqMonitor {
  uint16_t baseline = 0;
  uint16_t minimum = 0, maximum = 0;
  uint32_t sum = 0;
  uint8_t samples = 0;
  MqStatus status = MqStatus::Learning;

  MqStatus sample(uint16_t raw) {
    constexpr uint16_t lowLimit = 750, highLimit = 2600;
    constexpr uint16_t lowEvent = 100, highEvent = 3000;
    if (status == MqStatus::Learning) {
      if (raw <= MQ_NO_SIGNAL_MAX || (raw > lowLimit && raw < highLimit)) {
        samples = 0;
        return status;
      }
      if (!samples || (raw > minimum && raw - minimum > 250) || (raw < maximum && maximum - raw > 250)) {
        minimum = maximum = raw;
        sum = 0;
        samples = 0;
      }
      if (raw < minimum) minimum = raw;
      if (raw > maximum) maximum = raw;
      sum += raw;
      if (++samples == 12) {
        baseline = sum / samples;
        status = MqStatus::Normal;
      }
      return status;
    }

    const bool falls = baseline >= highLimit;
    const int travel = falls ? baseline - lowEvent : highEvent - baseline;
    const int excursion = falls ? baseline - raw : raw - baseline;
    if (excursion >= travel || (status == MqStatus::High && excursion >= travel * 9 / 10))
      status = MqStatus::High;
    else if (excursion >= (status == MqStatus::Normal ? travel / 4 : travel / 6))
      status = MqStatus::Elevated;
    else
      status = MqStatus::Normal;
    return status;
  }
};
MqMonitor mqMonitors[STATION_COUNT];

void readDht(uint8_t station) {
  auto& sensor = dhtSensors[station];
  const float temperature = sensor.readTemperature();
  const float humidity = sensor.readHumidity();
  const bool valid = !isnan(temperature) && !isnan(humidity) && humidity >= 0 && humidity <= 100;
  const int level = digitalRead(HUB_DHT_PINS[station]);
  const uint32_t now = millis();

  stateLock();
  auto& scan = gState.sensors[station];
  scan.tryCount++;
  scan.dht = valid ? DHT_OK : DHT_NO_RESPONSE;
  scan.attach = level == HIGH ? ATTACH_PULLED_HIGH : ATTACH_PULLED_LOW;
  auto& row = gState.health[station];
  row.hasTH = valid;
  if (valid) {
    scan.okCount++;
    scan.tempX10 = lroundf(temperature * 10);
    scan.humX10 = lroundf(humidity * 10);
    scan.lastOkMs = now;
    row.tempX10 = scan.tempX10;
    row.humX10 = scan.humX10;
    row.realTHMs = now;
  }
  stateUnlock();
}

struct MqSample { uint16_t raw; uint16_t mv; };

MqSample readMq(uint8_t station) {
  constexpr uint8_t samples = 4;
  const uint8_t pin = HUB_MQ_PINS[station];
  uint32_t raw = 0, mv = 0;
  for (uint8_t sample = 0; sample < samples; sample++) {
    raw += analogRead(pin);
    mv += analogReadMilliVolts(pin);
  }
  return {static_cast<uint16_t>(raw / samples), static_cast<uint16_t>(mv / samples)};
}

void updateStationInputs(uint32_t now) {
  MqSample samples[STATION_COUNT];
  bool anySignal = false;
  for (uint8_t station = 0; station < STATION_COUNT; ++station) {
    samples[station] = readMq(station);
    anySignal |= samples[station].raw > MQ_NO_SIGNAL_MAX;
  }

  // USB may power the ESP and DHTs while the MQ and IR modules have no 5 V.
  // A high proximity input still permits IR-only operation with the MQs absent.
  bool proximityAvailable = anySignal;
  for (uint8_t pin : HUB_PROX_ZONE_PINS) proximityAvailable |= digitalRead(pin) == HIGH;
  if (stationInputsAvailable != proximityAvailable) {
    stationInputsAvailable = proximityAvailable;
    for (auto& zone : proxZones) zone = ProxZone{};
  }

  stateLock();
  for (uint8_t station = 0; station < STATION_COUNT; ++station) {
    const auto& sample = samples[station];
    auto& scan = gState.sensors[STATION_COUNT + station];
    scan.adcRaw = sample.raw;
    scan.adcMv = sample.mv;
    if (scan.mvMin < 0 || sample.mv < scan.mvMin) scan.mvMin = sample.mv;
    if (scan.mvMax < 0 || sample.mv > scan.mvMax) scan.mvMax = sample.mv;
    auto& row = gState.health[station];
    row.gas = sample.raw;
    row.realGasMs = now;
    row.hasGas = anySignal;
    if (anySignal) {
      row.gasStatus = mqMonitors[station].sample(sample.raw);
    } else {
      mqMonitors[station] = MqMonitor{};
      row.gasStatus = MqStatus::Learning;
    }
    if (!proximityAvailable) {
      row.hasProx = false;
      row.prox = false;
      row.lastProxDetectionMs = 0;
    }
  }
  stateUnlock();
}

void pollProx(uint32_t now) {
  for (uint8_t zone = 0; zone < HUB_PROX_ZONE_COUNT; zone++) {
    auto& prox = proxZones[zone];
    const int level = digitalRead(HUB_PROX_ZONE_PINS[zone]);
    if (level != prox.level) {
      prox.level = level;
      prox.changedAt = now;
      prox.published = false;
    }
    if (prox.published || now - prox.changedAt < 30) continue;

    stateLock();
    for (uint8_t station = zone * 2; station < zone * 2 + 2; station++) {
      auto& row = gState.health[station];
      if (level == LOW || row.prox) row.lastProxDetectionMs = now;
      row.prox = level == LOW;
      row.hasProx = true;
      row.proxShared = true;
    }
    stateUnlock();
    prox.published = true;
  }
}
} // namespace

const char* mqStatusName(MqStatus status) {
  switch (status) {
    case MqStatus::Normal: return "normal";
    case MqStatus::Elevated: return "elevated";
    case MqStatus::High: return "high";
    default: return "learning";
  }
}

StationTemperature stationTemperatureLocked(uint8_t station, uint32_t now) {
  StationTemperature result;
  const auto* sample = &gState.health[station];
  if (sample->hasFreshTemperature(now)) {
    result.source = "measured";
  } else {
    int before = station - 1, after = station + 1;
    while (before >= 0 && !gState.health[before].hasFreshTemperature(now)) --before;
    while (after < gState.healthCount && !gState.health[after].hasFreshTemperature(now)) ++after;
    const bool hasBefore = before >= 0, hasAfter = after < gState.healthCount;
    if (!hasBefore && !hasAfter) return result;
    result.source = "estimated";
    result.estimated = true;
    if (hasBefore && hasAfter) {
      const auto& left = gState.health[before];
      const auto& right = gState.health[after];
      const int fromLeft = station - before, fromRight = after - station;
      const int distance = after - before;
      result.tempX10 = (left.tempX10 * fromRight + right.tempX10 * fromLeft) / distance;
      result.humX10 = (left.humX10 * fromRight + right.humX10 * fromLeft) / distance;
      result.available = true;
      return result;
    }
    sample = &gState.health[hasBefore ? before : after];
  }
  result.tempX10 = sample->tempX10;
  result.humX10 = sample->humX10;
  result.available = true;
  return result;
}

const char* dhtResultName(uint8_t result) {
  switch (result) {
    case DHT_OK: return "ok";
    case DHT_LINE_LOW: return "line_low";
    case DHT_NO_RESPONSE: return "no_response";
    case DHT_FRAME_TIMEOUT: return "frame_timeout";
    case DHT_CHECKSUM: return "checksum";
    case DHT_NO_DRIVE: return "no_drive";
    default: return "unprobed";
  }
}
const char* pinAttachName(uint8_t attach) {
  switch (attach) {
    case ATTACH_FLOATING: return "floating";
    case ATTACH_PULLED_HIGH: return "pulled_high";
    case ATTACH_PULLED_LOW: return "pulled_low";
    case ATTACH_LOADED: return "loaded";
    default: return "unknown";
  }
}

void sensorsBegin() {
  stateLock();
  for (uint8_t station = 0; station < STATION_COUNT; station++) {
    healthRowLocked(station + 1);
    gState.sensors[station].pin = HUB_DHT_PINS[station];
    auto& mq = gState.sensors[STATION_COUNT + station];
    mq.pin = HUB_MQ_PINS[station];
    mq.role = ROLE_MQ_AO;
  }
  stateUnlock();

  analogReadResolution(12);
  for (uint8_t station = 0; station < STATION_COUNT; station++) {
    dhtSensors[station].begin();
    analogSetPinAttenuation(HUB_MQ_PINS[station], ADC_11db);
  }
  for (uint8_t pin : HUB_PROX_ZONE_PINS) pinMode(pin, INPUT_PULLUP);

  const uint32_t now = millis();
  nextDhtAt = now + HUB_DHT_SETTLE_MS;
  lastMq = now;
}

void sensorsUpdate() {
  const uint32_t now = millis();
  if (now - lastMq >= HUB_MQ_POLL_MS) {
    lastMq = now;
    updateStationInputs(now);
  }
  if (stationInputsAvailable) pollProx(now);
  if (static_cast<int32_t>(now - nextDhtAt) >= 0) {
    nextDhtAt = now + HUB_DHT_STEP_MS;
    readDht(nextDht);
    nextDht = (nextDht + 1) % STATION_COUNT;
  }
}
