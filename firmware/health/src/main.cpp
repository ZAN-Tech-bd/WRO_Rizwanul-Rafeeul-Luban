#include <Arduino.h>
#include "MuseumProtocol.h"   // ../common (shared ESP-NOW packet)

// =====================================================================
//  HEALTH NODE — skeleton.
//  Reads DHT11 (temp/humidity) + MQ-135 (gas) at an artifact and broadcasts
//  museum::MSG_HEALTH { stationId, tempX10, humidX10, gas } every few seconds.
//  Set THIS_STATION_ID to the artifact this node watches (1..7), and run one
//  node per artifact (or one node wired to several nearby sensors).
//  See README.md for wiring + stationId map. Channel 1.
// =====================================================================

static const uint8_t THIS_STATION_ID = 1;   // TODO: set per artifact (1..7)

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.printf("[health] skeleton ready — station %u. Implement Phase 6.\n", THIS_STATION_ID);
}

void loop() {}
