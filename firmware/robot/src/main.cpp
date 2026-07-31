#include <Arduino.h>
#include "MuseumProtocol.h"   // ../common (shared ESP-NOW packet)

// =====================================================================
//  GUIDE ROBOT — skeleton.
//  Phase 1: chassis + 2 motors + L298N + line-follow
//  Phase 2: RC522 RFID station stop + DFPlayer Bangla/English narration + OLED
//  Phase 3: direction arrow (NeoPixel) + queue logic
//
//  On the network it BROADCASTS museum::MSG_LOCATION {stationId 1..7, moving}
//  whenever it reaches a station, and LISTENS for MSG_EMERGENCY to reroute.
//  See README.md for wiring + stationId map.
// =====================================================================

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("[robot] skeleton ready — implement Phase 1 (drive + line-follow).");
}

void loop() {}
