#pragma once
// ===========================================================================
//  config.h -- ESP32S-CAM (GC2145) Smart Museum camera-node configuration.
//
//  Hardware facts (verified against "ESP32 CAM.pdf" + project findings.md):
//   * Board    : ESP32S-CAM seated on an ESP32-CAM-MB Type-C programmer
//                (the CAM-MB is a CH340 USB-serial carrier -- no FTDI needed).
//   * Sensor   : GC2145 (prior test detected PID 0x2145).  *** GC2145 has NO
//                native JPEG output *** -- frames are grabbed as RGB565 and
//                converted to JPEG in software with frame2jpg() (img_converters.h).
//   * Pin map  : AI-Thinker-style DVP map that already worked for this board.
//
//  Stability rules that made the GC2145 reliable in prior tests:
//   * XCLK 10 MHz, PIXFORMAT_RGB565, QVGA 320x240, 1 framebuffer in PSRAM.
//
//  SD card: SD_MMC in 1-BIT mode uses ONLY GPIO2 (D0) / GPIO14 (CLK) /
//  GPIO15 (CMD).  That leaves GPIO4 (the onboard flash LED) and EVERY camera
//  pin free, so the camera and SD never fight over pins.  4-bit mode (which
//  would also take GPIO4/12/13) is intentionally NOT used.
// ===========================================================================

#include "esp_camera.h"   // framesize_t / PIXFORMAT_* enums used below

// --- Camera DVP pin map (AI-Thinker / ESP32S-CAM) -------------------------
#define CAM_PIN_PWDN    32
#define CAM_PIN_RESET   (-1)   // tied to the CAM-MB reset, not a free GPIO
#define CAM_PIN_XCLK     0
#define CAM_PIN_SIOD    26
#define CAM_PIN_SIOC    27
#define CAM_PIN_Y9      35
#define CAM_PIN_Y8      34
#define CAM_PIN_Y7      39
#define CAM_PIN_Y6      36
#define CAM_PIN_Y5      21
#define CAM_PIN_Y4      19
#define CAM_PIN_Y3      18
#define CAM_PIN_Y2       5
#define CAM_PIN_VSYNC   25
#define CAM_PIN_HREF    23
#define CAM_PIN_PCLK    22

// --- Proven-stable GC2145 camera settings ---------------------------------
#define CAM_XCLK_HZ       10000000UL   // 10 MHz = the stable GC2145 clock
#define CAM_FRAME_SIZE    FRAMESIZE_QVGA   // 320x240  (width*height*2 bytes RGB565)
#define CAM_FB_COUNT      1

// --- Stale-frame flush (fixes "the photo is always one shot behind") ------
// With CAM_FB_COUNT == 1 and CAMERA_GRAB_WHEN_EMPTY the driver starts filling
// the single framebuffer the moment the previous one is returned, then parks
// that frame until the next esp_camera_fb_get().  So an un-flushed grab hands
// back an image shot right after the PREVIOUS capture -- minutes old if that is
// when the last CAPTURE ran.  Grabbing and immediately returning this many
// frames first restarts the pipeline so the frame we keep is taken now.  The
// extra frames also give the GC2145's auto-exposure/gain time to settle.
#define CAM_WARMUP_FRAMES     2        // 0 disables the flush (not recommended)
#define CAM_WARMUP_DELAY_MS   40       // settle time between flushed frames

// Software-JPEG quality passed to frame2jpg().  GC2145 cannot emit JPEG, so the
// RGB565 framebuffer is converted here.  Lower = smaller file / more compression.
// 15 keeps each QVGA JPEG only a few KB while staying clearly readable; tune up
// toward ~80 for best quality.  The actual byte count is printed on every capture.
#define CAM_JPEG_QUALITY  15

// --- Onboard flash LED (optional; SAFE because SD runs in 1-bit mode) -----
#define FLASH_LED_GPIO    4            // AI-Thinker ESP32-CAM white flash LED

// --- microSD storage layout ----------------------------------------------
#define SD_MOUNT_POINT    "/sdcard"
#define PHOTO_DIR         "/photos"
#define PHOTO_PREFIX      "IMG_"       // /photos/IMG_0001.JPG, IMG_0002.JPG, ...
#define PHOTO_INDEX_WIDTH 4            // zero-padded width of the index field
#define PHOTO_EXT         ".JPG"

// --- USB serial console ---------------------------------------------------
#define SERIAL_BAUD       115200
#define CMD_BUF_LEN       96           // hard-bounded line buffer (no String)
#define LIST_MAX_ENTRIES  50           // safety cap for the LIST command

// --- Hub Wi-Fi (the ESP-CAM joins the Hub SoftAP to push photos) ----------
// MUST match firmware/hub/include/config.h (HUB_SSID / HUB_PASS / HUB_CHANNEL).
#define HUB_SSID                  "MuseumHub"
#define HUB_PASS                  "museum2026"
#define HUB_UPLOAD_URL            "http://192.168.4.1/api/image/upload"
#define HUB_HOST_IP               "192.168.4.1"
#define WIFI_CONNECT_TIMEOUT_MS   15000UL   // give up joining the Hub after this
#define HUB_HTTP_TIMEOUT_MS        8000UL   // per-upload HTTP timeout
#define CAM_AUTO_UPLOAD           1          // 1 = CAPTURE auto-pushes to Hub when Wi-Fi is up
#define CAM_NODE_ID               10         // ESP-NOW nodeId for this camera
