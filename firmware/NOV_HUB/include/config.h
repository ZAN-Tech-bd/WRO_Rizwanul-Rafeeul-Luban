#pragma once
#include <Arduino.h>
#include "StationCatalog.h"

// NOV_HUB: six stations with one eight-pixel LED board per station.
static const char* HUB_SSID = "MuseumHub";
static const char* HUB_PASS = "museum2026";
static const uint8_t HUB_CHANNEL = 1;
static const uint8_t HUB_IP[] = {192, 168, 4, 1};
static const uint8_t STATION_COUNT = 6;
static const uint8_t REAL_STATION_COUNT = 6;
static const uint8_t SIMULATED_STATION_COUNT = 0;
static const char* STATION_IDS[STATION_COUNT] = {
  "terracotta", "woodenboat", "rickshaw", "pottery", "flute", "basket"
};

static const uint8_t NEOPIXEL_PIN = 45;
static const uint8_t LED_PIXELS_PER_STATION = 8;
static const uint8_t LED_CHAIN_LENGTH = STATION_COUNT * LED_PIXELS_PER_STATION;
static const uint8_t LED_BRIGHTNESS = 110; // Bench-proven recovered LED demo.
static const uint8_t LED_EMERGENCY_BRIGHTNESS = 60; // Whole-chain power budget.
static_assert(LED_EMERGENCY_BRIGHTNESS <= LED_BRIGHTNESS, "emergency brightness must fit strip limit");
static const uint16_t LED_REFRESH_MS = 20; // Re-send both on and off frames throughout each pulse.
static const uint16_t LED_FLOW_STEP_MS = 55;
static const uint16_t LED_FLOW_HUE_MS = 400;
static const uint8_t LED_FLOW_TAIL = 8;
static const uint16_t LED_PROX_HALF_PERIOD_MS = 250;
static const uint16_t LED_EMERGENCY_ON_MS = 150;
static const uint16_t LED_EMERGENCY_GAP_MS = 200;
static const uint16_t LED_EMERGENCY_PAUSE_MS = 500;
static const uint8_t BUZZER_PIN = 47;
static const uint8_t EMERGENCY_BUTTON_PIN = 21; // J20, active HIGH/pull-down.
static const uint32_t ROBOT_MOVE_SETTLE_MS = 2500;
static const uint8_t HUB_NODE_ID = 10;
static const uint32_t EMERGENCY_BROADCAST_MS = 2000;
static const uint8_t EMERGENCY_BURST_COUNT = 6;
static const uint32_t EMERGENCY_BURST_GAP_MS = 60;

enum : uint8_t { ROLE_DHT11 = 0, ROLE_MQ_AO = 1 };
static constexpr uint8_t HUB_DHT_PINS[STATION_COUNT] = {15, 16, 17, 18, 38, 39};
static constexpr uint8_t HUB_MQ_PINS[STATION_COUNT] = {4, 5, 6, 7, 1, 2};
static constexpr uint8_t HUB_PROX_ZONE_PINS[] = {41, 42, 3};
static constexpr uint8_t HUB_PROX_ZONE_COUNT = sizeof(HUB_PROX_ZONE_PINS);
static const uint32_t HUB_PROX_RECENT_MS = 3000;
static constexpr uint8_t HUB_SENSOR_COUNT = 2 * STATION_COUNT;
static const uint32_t HUB_DHT_STEP_MS = 500; // One station per step; 3 s per DHT11.
static const uint32_t HUB_MQ_POLL_MS = 1000;
static const uint32_t HUB_HEALTH_STALE_MS = 12000;
static const uint32_t HUB_DHT_SETTLE_MS = 2500;
// MQ-5 AO is NOT calibrated to ppm or a safety threshold. Never force "good air".
static const bool HUB_GAS_CALIBRATED = false;
static const uint16_t HUB_GAS_THRESHOLD_RAW = 3000; // Used only if calibrated=true.
static const float HUB_MQ_DIVIDER_RATIO = 1.68f;   // Station 68k / 100k.

// Current S3 PCB SD port; legacy ESP32 pins conflict with DHT/MQ on this board.
static const bool HUB_SD_ENABLED = true;
static const uint8_t HUB_SD_SCK_PIN = 12;
static const uint8_t HUB_SD_MISO_PIN = 13;
static const uint8_t HUB_SD_MOSI_PIN = 11;
static const uint8_t HUB_SD_CS_PIN = 14;
static const uint32_t HUB_SD_FREQ_HZ = 4000000UL;
static const uint8_t GALLERY_MAX_PHOTOS = 12;
static const char* GALLERY_DIR = "/photos";
static const bool HUB_CAMERA_SIMULATED = false;

static_assert(STATION_COUNT == REAL_STATION_COUNT + SIMULATED_STATION_COUNT, "station counts");
static_assert(STATION_COUNT == 2 * HUB_PROX_ZONE_COUNT, "two stations per proximity zone");
