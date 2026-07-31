#include <Arduino.h>
#include "MuseumProtocol.h"   // ../common (shared ESP-NOW packet)

// =====================================================================
//  EMERGENCY NODE — skeleton.
//  A big push button. On press it BROADCASTS museum::MSG_EMERGENCY with
//  FLAG_EMERGENCY_ACTIVE to FF:FF:FF:FF:FF:FF so EVERY node reacts instantly
//  (robot reroutes to exit, track LEDs blink, buzzer sounds), even if the Hub
//  is rebooting. This is the autonomous-decision centerpiece. Channel 1.
//  See README.md for wiring.
// =====================================================================

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("[emergency] skeleton ready — implement Phase 5.");
}

void loop() {}
