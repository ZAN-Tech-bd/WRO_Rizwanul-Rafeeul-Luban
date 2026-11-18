#pragma once
#include <stdint.h>
struct DemoCameraPhoto { const char* path; uint16_t bytes; uint8_t stationId; };
static const DemoCameraPhoto DEMO_CAMERA_PHOTOS[] = {
  {"/images/camera-demo/01.jpg", 59114, 0},
  {"/images/camera-demo/02.jpg", 57310, 0},
  {"/images/camera-demo/03.jpg", 55265, 0},
  {"/images/camera-demo/04.jpg", 55461, 0},
  {"/images/camera-demo/05.jpg", 56287, 0},
  {"/images/camera-demo/06.jpg", 56309, 0},
};
static const uint8_t DEMO_CAMERA_PHOTO_COUNT = sizeof(DEMO_CAMERA_PHOTOS) / sizeof(DEMO_CAMERA_PHOTOS[0]);
