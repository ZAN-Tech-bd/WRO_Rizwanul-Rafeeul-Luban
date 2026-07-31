#include <Arduino.h>
#include "MuseumProtocol.h"   // ../common (shared ESP-NOW packet)

// =====================================================================
//  ESP32-CAM NODE — skeleton.
//  Captures a photo at a station and HTTP-POSTs the raw JPEG body to the Hub:
//      POST http://192.168.4.1/api/image/upload
//  The admin dashboard then shows it at GET /api/image.
//  To upload, this board joins the Hub's WiFi "MuseumHub" as a client (STA) on
//  channel 1. NOTE: the CAM is a CLIENT only — it never hosts. See README.md.
// =====================================================================

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("[cam] skeleton ready — implement Phase 6 (capture + upload).");
}

void loop() {}
