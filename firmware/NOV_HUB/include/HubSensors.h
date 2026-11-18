#pragma once
#include <Arduino.h>
#include "config.h"

// Public names retained so the original Hub JSON/API remains compatible.
enum DhtResult : uint8_t {
  DHT_UNPROBED = 0, DHT_OK, DHT_LINE_LOW, DHT_NO_RESPONSE,
  DHT_FRAME_TIMEOUT, DHT_CHECKSUM, DHT_NO_DRIVE
};
enum PinAttach : uint8_t {
  ATTACH_UNKNOWN = 0, ATTACH_FLOATING, ATTACH_PULLED_HIGH, ATTACH_PULLED_LOW, ATTACH_LOADED
};
enum class MqStatus : uint8_t { Learning, Normal, Elevated, High };
const char* mqStatusName(MqStatus status);
const char* dhtResultName(uint8_t result);
const char* pinAttachName(uint8_t attach);
void sensorsBegin();
void sensorsUpdate();
