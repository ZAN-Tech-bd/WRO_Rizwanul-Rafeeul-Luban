#pragma once
// =====================================================================
//  MuseumState.h  —  the shared state both cores read/write.
//  ALWAYS take gStateMutex before touching gState. The web server (Core 0)
//  and the LED loop (Core 1) both use it.
// =====================================================================

#include <Arduino.h>
#include "freertos/semphr.h"

// one check-in request from a visitor. status: 0=pending 1=approved 2=denied
struct CheckInReq  { char id[16]; uint8_t status; uint32_t ts; };
// one artifact's latest sensor reading (from ESP-NOW HEALTH messages)
struct HealthRow   { char id[12]; int16_t tempX10; int16_t humX10; uint16_t gas; uint32_t lastMs; };

struct MuseumState {
  char     robotStation[12] = "";   // station id the robot is at
  bool     robotMoving   = false;
  uint32_t robotMs       = 0;

  bool     emergency     = false;

  HealthRow   health[8];
  uint8_t     healthCount = 0;

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

// check-in helpers (call with the lock held)
void    addRequestLocked(const char* id);     // adds a new pending request
CheckInReq* findRequestLocked(const char* id); // returns NULL if not found
