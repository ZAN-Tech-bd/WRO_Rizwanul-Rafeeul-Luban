#pragma once
// =====================================================================
//  MuseumState.h  —  the shared state both cores read/write.
//  ALWAYS take gStateMutex before touching gState. The web server (Core 0)
//  and the LED loop (Core 1) both use it.
// =====================================================================

#include <Arduino.h>
#include "freertos/semphr.h"
#include "config.h"        // HUB_SENSOR_COUNT sizes the scan table below
#include "HubSensors.h"

// one check-in request from a visitor. status: 0=pending 1=approved 2=denied
struct CheckInReq  { char id[16]; uint8_t status; uint32_t ts; };
// Each station has independent DHT11, MQ ADC and shared proximity inputs.
struct HealthRow {
  char     id[12];
  int16_t  tempX10 = 0;
  int16_t  humX10  = 0;
  uint16_t gas     = 0;
  MqStatus gasStatus = MqStatus::Learning;
  bool     hasTH   = false;
  bool     hasGas  = false;
  uint32_t realTHMs = 0;
  uint32_t realGasMs = 0;
  bool hasFreshTemperature(uint32_t now) const { return hasTH && now - realTHMs < HUB_HEALTH_STALE_MS; }
  bool hasFreshGas(uint32_t now) const { return hasGas && now - realGasMs < HUB_HEALTH_STALE_MS; }
  bool hasProx = false;
  bool prox = false;
  bool proxShared = false;
  uint32_t lastProxDetectionMs = 0;
};

struct StationTemperature {
  int16_t tempX10 = 0;
  int16_t humX10 = 0;
  const char* source = "unavailable";
  bool available = false;
  bool estimated = false;
};
StationTemperature stationTemperatureLocked(uint8_t station, uint32_t now);

// One GPIO on the Hub itself, probed as a DHT11 and (where it has an ADC)
// sampled as an analog MQ input. This is the pin-scan table: it exists so an
// unknown wiring loom can be identified without reflashing per guess.
struct SensorRow {
  uint8_t  pin        = 0;
  uint8_t  dht        = 0;      // DhtResult (see HubSensors.h)
  uint8_t  attach     = 0;      // PinAttach: is anything wired here at all?
  uint8_t  role       = 0;      // ROLE_DHT11 / ROLE_MQ_AO (config.h)
  int8_t   d0         = -1;     // Legacy layout field; DO is not wired in this profile.
  uint16_t d0Changes  = 0;      // debounced D0 flips -- the proof an MQ is real
  int16_t  mvMin      = -1;     // analog envelope seen on an MQ line
  int16_t  mvMax      = -1;
  uint16_t okCount    = 0;      // valid DHT11 frames since boot
  uint16_t tryCount   = 0;      // probes attempted
  int16_t  tempX10    = 0;      // last good reading only
  int16_t  humX10     = 0;
  int16_t  adcRaw     = -1;     // -1 = not sampled (no ADC1 on this pin)
  int16_t  adcMv      = -1;
  uint32_t lastOkMs   = 0;
};

struct MuseumState {
  char     robotStation[12] = "";   // station id the robot is at
  bool     robotMoving   = false;
  bool     robotReportsLive = false;
  uint32_t robotMs       = 0;

  bool     emergency     = false;
  // Admin dashboard mute switch. Like the emergency it is LATCHED here and
  // re-broadcast, so the Hub -- not the robot -- is the source of truth: a robot
  // that reboots mid-tour comes back muted if that is still the admin's choice.
  bool     audioMuted    = false;

  HealthRow   health[STATION_COUNT];
  uint8_t     healthCount = 0;

  SensorRow   sensors[HUB_SENSOR_COUNT];   // Hub-local pin scan (HubSensors.cpp)

  CheckInReq  requests[24];
  uint8_t     reqCount    = 0;
  uint16_t    approvedCount = 0;

  // Latest photo pushed by the ESP-CAM over HTTP (/api/image/upload -> /last.jpg)
  uint32_t    imageMs      = 0;   // millis() of the last received image
  uint16_t    imageBytes   = 0;   // JPEG byte count
  uint8_t     imageStation = 0;   // 1..N (0 = unknown)
  uint16_t    imageSeq     = 0;   // bumped on every upload (dashboard reload cue)
};

extern MuseumState      gState;
extern SemaphoreHandle_t gStateMutex;

void    stateInit();        // create the mutex (call once in setup)
void    stateLock();        // blocks until the mutex is free
void    stateUnlock();

// helper used while the lock is held
uint8_t pendingCountLocked();

// station id (1..STATION_COUNT) -> the name the web app uses; "" if out of range
const char* stationName(uint8_t stationId);

// The artifact-health row for a station, created on first use. Returns nullptr
// for an unknown station or a full table. Call with the lock held, then write
// only the fields you actually measured: a DHT11 has no gas channel and an MQ
// has no temperature, so one source must not clobber the other's fields.
HealthRow* healthRowLocked(uint8_t stationId);

// check-in helpers (call with the lock held)
void    addRequestLocked(const char* id);     // adds a new pending request
CheckInReq* findRequestLocked(const char* id); // returns NULL if not found
