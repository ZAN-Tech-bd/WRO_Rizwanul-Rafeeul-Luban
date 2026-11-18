#pragma once
#include <stdint.h>
#include "CameraDemoCatalog.h"

static const uint32_t CAMERA_DEMO_INTERVAL_MS = 20000;

// One camera arrival every20s, with bounded history and wrap-safe elapsed time.
// Sequence always maps to the same photograph; restart begins with photo1.
class DemoCameraClock {
 public:
  uint32_t startedMs = 0;
  uint32_t firstSequence = 1;

  uint32_t sequenceAt(uint32_t now) const {
    return firstSequence + (uint32_t(now - startedMs) / CAMERA_DEMO_INTERVAL_MS);
  }
  uint8_t countAt(uint32_t now) const {
    const uint32_t count = uint32_t(now - startedMs) / CAMERA_DEMO_INTERVAL_MS + 1;
    return count < DEMO_CAMERA_PHOTO_COUNT ? count : DEMO_CAMERA_PHOTO_COUNT;
  }
  static uint8_t photoIndex(uint32_t sequence) {
    return (sequence - 1) % DEMO_CAMERA_PHOTO_COUNT;
  }
  uint32_t ageAt(uint32_t now, uint32_t sequence) const {
    return uint32_t(now - startedMs) - (sequence - firstSequence) * CAMERA_DEMO_INTERVAL_MS;
  }
  void restart(uint32_t now) {
    firstSequence = (sequenceAt(now) / DEMO_CAMERA_PHOTO_COUNT + 1) * DEMO_CAMERA_PHOTO_COUNT + 1;
    startedMs = now;
  }
};
