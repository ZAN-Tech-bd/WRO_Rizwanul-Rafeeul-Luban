#include <Arduino.h>
#include <ctype.h>
#include "esp_camera.h"          // camera driver (bundled with espressif32)
#include "img_converters.h"      // frame2jpg(): RGB565 -> JPEG for the GC2145
#include "SD_MMC.h"              // onboard microSD slot, 1-bit mode
#include "FS.h"

#include "config.h"              // local pin + tuning constants
#include "MuseumProtocol.h"      // ../common -- shared ESP-NOW packet

#include <WiFi.h>                // join the Hub SoftAP (STA mode, channel 1)
#include <HTTPClient.h>          // POST each JPEG to /api/image/upload
#include <esp_now.h>             // lightweight MSG_IMAGE_READY "ping"

// ===========================================================================
//  ESP32S-CAM (GC2145) camera node -- Smart Museum.
//
//  This firmware captures a still photo, converts it to JPEG in RAM, and sends
//  it directly to the Hub dashboard. A microSD card is optional: when mounted,
//  it also receives a backup copy of each capture.
//
//  GC2145 caveat: the sensor produces RGB565, not JPEG. Every capture therefore
//  grabs one RGB565 framebuffer and converts it to JPEG in software with
//  frame2jpg() before writing it to the card.  Settings (XCLK 10 MHz, RGB565,
//  QVGA 320x240, 1 fb in PSRAM) are the ones that previously produced a stable
//  PID 0x2145 detection on this exact board (see "ESP32 CAM.pdf").
//
//  Commands arrive from TWO sources, both line-oriented and case-insensitive:
//    * the USB console @115200, and
//    * the guide ESP32's trigger wire on UART1 (GPIO13, 9600, RX only), which
//      sends "SHOOT <station>" each time the robot reads a station RFID card.
//
//  Commands:
//     CAPTURE [station]   acquire a frame, convert to JPEG, upload to Hub
//     STATUS              print camera/SD/PSRAM/heap diagnostics
//     LIST                list saved photos in /photos (name + size)
//     INIT                re-initialise camera and SD (bring-up retry)
//     FLASH ON|OFF        toggle the onboard flash LED (optional, safe in 1-bit SD)
//     HELP                show this list
//
// ===========================================================================

// --- Module state ----------------------------------------------------------
static bool     g_cameraReady = false;
static bool     g_sdReady     = false;
static uint16_t g_sensorPid   = 0x0000;
static uint8_t  g_cardType    = CARD_NONE;
static bool     g_flashOn     = false;

// --- Hub link state (Wi-Fi STA + ESP-NOW image-ready ping) -----------------
static uint8_t *g_lastJpeg      = nullptr; // latest encoded JPEG, retained for SEND
static size_t   g_lastJpegLen   = 0;
static char     g_lastPhoto[48] = "";   // optional SD backup path (empty without SD)
static uint16_t g_lastStation   = 0;    // station id tagged on the last capture
static uint32_t g_autoMs        = 0;    // auto-capture interval in ms (0 = off)
static uint32_t g_autoLast      = 0;    // millis() of the last auto capture

// Forward declarations -- the Hub helpers live further down (after doCapture).
static bool  connectToHub();
static bool  ensureWifi();
static bool  initEspNow();
static void  notifyHubImageReady(uint8_t stationId);
static bool  uploadToHubJpeg(const uint8_t *jpeg, size_t jpegLen, uint16_t stationId);
static void  sendLastPhoto(uint16_t stationId);

// --- Bounded line-oriented command buffers (no dynamic String) -------------
// One per command source: the USB console and the robot link each need their
// own buffer, or a half-typed USB line would be spliced into a robot command.
static char  g_cmdBuf[CMD_BUF_LEN];
static int   g_cmdLen = 0;
static char  g_linkBuf[CMD_BUF_LEN];
static int   g_linkLen = 0;

// UART1: the guide ESP32's "SHOOT <station>" trigger (RX only, see config.h).
static HardwareSerial RobotLink(1);

// ---------------------------------------------------------------------------
//  Small helpers
// ---------------------------------------------------------------------------
static const char *cardTypeStr(uint8_t t) {
  switch (t) {
    case CARD_NONE: return "NONE";
    case CARD_MMC:  return "MMC";
    case CARD_SD:   return "SDSC";
    case CARD_SDHC: return "SDHC/SDXC";
    default:        return "unknown";
  }
}

static double mb(uint64_t bytes) { return (double)bytes / (1024.0 * 1024.0); }

// Parse the sequential index out of an "IMG_####.JPG" entry name. The name may
// be bare ("IMG_0003.JPG") or fully qualified ("/photos/IMG_0003.JPG").
static uint16_t parsePhotoIndex(const char *name) {
  if (!name) return 0;
  const char *p = strstr(name, PHOTO_PREFIX);
  if (!p) return 0;
  p += strlen(PHOTO_PREFIX);
  uint16_t v = 0;
  for (int i = 0; i < PHOTO_INDEX_WIDTH && isdigit((unsigned char)p[i]); ++i) {
    v = (uint16_t)(v * 10 + (p[i] - '0'));
  }
  return v;
}

// Scan /photos for the highest existing IMG_####.JPG index so the next capture
// never overwrites an earlier photo -- this makes the counter survive reboots
// without any persistent config file.
static uint16_t findMaxPhotoIndex() {
  File dir = SD_MMC.open(PHOTO_DIR);
  if (!dir || !dir.isDirectory()) return 0;
  uint16_t maxIdx = 0;
  File entry = dir.openNextFile();
  while (entry) {
    if (!entry.isDirectory()) {
      String nm = entry.name();          // bounded scan; one short-lived temp
      uint16_t idx = parsePhotoIndex(nm.c_str());
      if (idx > maxIdx) maxIdx = idx;
    }
    entry.close();
    entry = dir.openNextFile();
  }
  dir.close();
  return maxIdx;
}

// ---------------------------------------------------------------------------
//  Camera + SD initialisation (idempotent / non-crashing)
// ---------------------------------------------------------------------------
static bool initCamera() {
  camera_config_t cfg = {};
  cfg.ledc_channel  = LEDC_CHANNEL_0;
  cfg.ledc_timer    = LEDC_TIMER_0;
  cfg.pin_d0        = CAM_PIN_Y2;
  cfg.pin_d1        = CAM_PIN_Y3;
  cfg.pin_d2        = CAM_PIN_Y4;
  cfg.pin_d3        = CAM_PIN_Y5;
  cfg.pin_d4        = CAM_PIN_Y6;
  cfg.pin_d5        = CAM_PIN_Y7;
  cfg.pin_d6        = CAM_PIN_Y8;
  cfg.pin_d7        = CAM_PIN_Y9;
  cfg.pin_xclk      = CAM_PIN_XCLK;
  cfg.pin_pclk      = CAM_PIN_PCLK;
  cfg.pin_vsync     = CAM_PIN_VSYNC;
  cfg.pin_href      = CAM_PIN_HREF;
  cfg.pin_sccb_sda  = CAM_PIN_SIOD;
  cfg.pin_sccb_scl  = CAM_PIN_SIOC;
  cfg.pin_pwdn      = CAM_PIN_PWDN;
  cfg.pin_reset     = CAM_PIN_RESET;
  cfg.xclk_freq_hz  = CAM_XCLK_HZ;
  cfg.pixel_format  = PIXFORMAT_RGB565;     // GC2145: no native JPEG
  cfg.frame_size    = CAM_FRAME_SIZE;       // QVGA 320x240
  cfg.grab_mode     = CAMERA_GRAB_WHEN_EMPTY;
  cfg.fb_location   = psramFound() ? CAMERA_FB_IN_PSRAM : CAMERA_FB_IN_DRAM;
  cfg.jpeg_quality  = 12;                    // ignored for RGB565; frame2jpg uses CAM_JPEG_QUALITY
  cfg.fb_count      = CAM_FB_COUNT;

  esp_err_t err = esp_camera_init(&cfg);
  if (err != ESP_OK) {
    Serial.printf("[cam] esp_camera_init FAILED: 0x%x (check ribbon / GC2145 / PSRAM)\n", err);
    g_cameraReady = false;
    return false;
  }
  sensor_t *s = esp_camera_sensor_get();
  g_sensorPid = (s != nullptr) ? s->id.PID : 0x0000;
  g_cameraReady = true;
  Serial.printf("[cam] init OK. sensor PID=0x%04X %s  PSRAM=%s (%.0f KB)\n",
                g_sensorPid,
                (g_sensorPid == 0x2145) ? "(GC2145)" : "(UNEXPECTED sensor)",
                psramFound() ? "yes" : "NO",
                psramFound() ? mb(ESP.getPsramSize()) * 1024.0 : 0.0);
  return true;
}

static bool initSD() {
  // 1-bit mode, mount at /sdcard, never auto-format (we only report failures).
  if (!SD_MMC.begin(SD_MOUNT_POINT, true)) {
    Serial.println("[sd]  SD_MMC.begin() FAILED -- no card, wrong FS, or poor seating");
    g_sdReady = false;
    return false;
  }
  g_cardType = SD_MMC.cardType();
  if (g_cardType == CARD_NONE) {
    Serial.println("[sd]  no card detected (CARD_NONE)");
    g_sdReady = false;
    return false;
  }
  SD_MMC.mkdir(PHOTO_DIR);
  uint64_t fsTotal = SD_MMC.totalBytes();
  uint64_t fsFree  = (fsTotal > SD_MMC.usedBytes()) ? (fsTotal - SD_MMC.usedBytes()) : 0;
  Serial.printf("[sd]  mounted %s (1-bit)  type=%s  card=%.1f MB  fsTotal=%.1f MB  fsFree=%.1f MB  photos=%u\n",
                SD_MOUNT_POINT, cardTypeStr(g_cardType),
                mb(SD_MMC.cardSize()), mb(fsTotal), mb(fsFree),
                findMaxPhotoIndex());
  g_sdReady = true;
  return true;
}

// Save an optional backup copy. A failure here is non-fatal because the Hub
// upload uses the in-memory JPEG, not the SD file.
static bool saveJpegToSD(const uint8_t *jpg, size_t jpgLen,
                         char *outPath, size_t outPathLen) {
  if (!g_sdReady || !jpg || jpgLen == 0 || !outPath || outPathLen == 0) return false;
  if (!SD_MMC.exists(PHOTO_DIR) && !SD_MMC.mkdir(PHOTO_DIR)) {
    Serial.println("[sd] backup skipped: could not create " PHOTO_DIR);
    return false;
  }

  uint16_t nextIdx = (uint16_t)(findMaxPhotoIndex() + 1);
  snprintf(outPath, outPathLen, "%s/%s%0*u%s",
           PHOTO_DIR, PHOTO_PREFIX, PHOTO_INDEX_WIDTH, (unsigned)nextIdx, PHOTO_EXT);

  File file = SD_MMC.open(outPath, FILE_WRITE);
  if (!file) {
    Serial.printf("[sd] backup skipped: could not open %s\n", outPath);
    outPath[0] = 0;
    return false;
  }
  size_t written = file.write(jpg, jpgLen);
  file.flush();
  file.close();

  File chk = SD_MMC.open(outPath, FILE_READ);
  size_t verified = chk ? chk.size() : 0;
  if (chk) chk.close();
  if (written != jpgLen || verified != jpgLen) {
    Serial.printf("[sd] backup failed: wrote=%u verified=%u expected=%u\n",
                  (unsigned)written, (unsigned)verified, (unsigned)jpgLen);
    outPath[0] = 0;
    return false;
  }
  Serial.printf("[sd] backup OK %s (%u bytes)\n", outPath, (unsigned)jpgLen);
  return true;
}

// ---------------------------------------------------------------------------
//  Capture: RGB565 frame -> software JPEG in RAM -> Hub HTTP upload.
//  The newest JPEG stays cached so SEND can retry after a Hub/Wi-Fi outage.
// ---------------------------------------------------------------------------
static void doCapture(int stationId) {
  if (!g_cameraReady) {
    Serial.println("[capture] FAIL camera not initialised -- send INIT or reboot");
    return;
  }

  // Drop the frame the driver has been parking since the last capture, then let
  // the sensor produce a fresh one (see CAM_WARMUP_FRAMES in config.h). Without
  // this the Hub receives the *previous* shot on every CAPTURE.
  for (int i = 0; i < CAM_WARMUP_FRAMES; ++i) {
    camera_fb_t *stale = esp_camera_fb_get();
    if (!stale) break;                 // nothing parked; the next grab is live
    esp_camera_fb_return(stale);       // returning it restarts the capture
    if (CAM_WARMUP_DELAY_MS) delay(CAM_WARMUP_DELAY_MS);
  }

  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("[capture] FAIL esp_camera_fb_get() returned NULL");
    return;
  }
  int fw = fb->width;
  int fh = fb->height;

  uint8_t *jpg = nullptr;
  size_t   jpgLen = 0;
  bool ok = frame2jpg(fb, CAM_JPEG_QUALITY, &jpg, &jpgLen);   // GC2145: RGB565 -> JPEG
  esp_camera_fb_return(fb);                                   // framebuffer done
  fb = nullptr;
  if (!ok || jpg == nullptr || jpgLen == 0) {
    Serial.println("[capture] FAIL RGB565->JPEG conversion (frame2jpg) failed");
    if (jpg) free(jpg);
    return;
  }

  // Replace the previous cache only after this conversion succeeds. frame2jpg()
  // allocates the output; ownership moves to g_lastJpeg until the next capture.
  if (g_lastJpeg) free(g_lastJpeg);
  g_lastJpeg = jpg;
  g_lastJpegLen = jpgLen;
  jpg = nullptr;
  g_lastStation = (uint16_t)stationId;
  g_lastPhoto[0] = 0;

  if (g_sdReady) {
    saveJpegToSD(g_lastJpeg, g_lastJpegLen, g_lastPhoto, sizeof(g_lastPhoto));
  } else {
    Serial.println(F("[sd] no card; continuing with RAM -> Hub upload."));
  }

#if CAM_AUTO_UPLOAD
  if (WiFi.status() == WL_CONNECTED) {
    if (uploadToHubJpeg(g_lastJpeg, g_lastJpegLen, g_lastStation))
      notifyHubImageReady((uint8_t)stationId);
  } else {
    Serial.println(F("[capture] JPEG retained in RAM. Run WIFI, then SEND to retry."));
  }
#endif

  Serial.printf("[capture] OK RAM  %dx%d  %u bytes  station=%d%s%s\n",
                fw, fh, (unsigned)g_lastJpegLen, stationId,
                g_lastPhoto[0] ? "  SD=" : "",
                g_lastPhoto[0] ? g_lastPhoto : "");
}

// ---------------------------------------------------------------------------
//  Command handlers
// ---------------------------------------------------------------------------
static void printHelp() {
  Serial.println(F("Commands (case-insensitive, end with newline):"));
  Serial.println(F("  CAPTURE [station]  capture -> JPEG in RAM -> Hub (SD backup optional)"));
  Serial.println(F("  SHOOT   [station]  ensure Wi-Fi, then capture + push to Hub"));
  Serial.println(F("  SEND    [station]  re-upload the last RAM-cached JPEG"));
  Serial.println(F("  WIFI              join / show Hub Wi-Fi status"));
  Serial.println(F("  AUTO <ms>         auto CAPTURE+SEND every <ms> (min 1000); 'AUTO' alone = off"));
  Serial.println(F("  STATUS             camera/RAM/SD/PSRAM/Wi-Fi diagnostics"));
  Serial.println(F("  LIST               list optional SD backup photos"));
  Serial.println(F("  INIT               re-initialise camera and try mounting optional SD"));
  Serial.println(F("  FLASH ON | OFF     toggle onboard flash LED"));
  Serial.println(F("  HELP               show this list"));
}

static void printStatus() {
  Serial.println(F("---- STATUS ----"));
  Serial.printf("uptime      : %lu ms\n", (unsigned long)millis());
  Serial.printf("free heap   : %u bytes (%.0f KB)   free PSRAM: %.0f KB\n",
                (unsigned)ESP.getFreeHeap(), mb(ESP.getFreeHeap()) * 1024.0,
                psramFound() ? mb(ESP.getFreePsram()) * 1024.0 : 0.0);
  Serial.printf("camera      : %s", g_cameraReady ? "ready, " : "NOT ready\n");
  if (g_cameraReady)
    Serial.printf("sensor PID=0x%04X %s\n", g_sensorPid,
                  (g_sensorPid == 0x2145) ? "(GC2145)" : "(UNEXPECTED)");
  if (g_sdReady) {
    uint64_t fsTotal = SD_MMC.totalBytes();
    uint64_t fsFree  = (fsTotal > SD_MMC.usedBytes()) ? (fsTotal - SD_MMC.usedBytes()) : 0;
    Serial.printf("microSD     : mounted %s (1-bit)  type=%s  card=%.1f MB  fsFree=%.1f MB  photos=%u\n",
                  SD_MOUNT_POINT, cardTypeStr(g_cardType),
                  mb(SD_MMC.cardSize()), mb(fsFree), findMaxPhotoIndex());
  } else {
    Serial.println(F("microSD     : optional / not mounted (RAM capture still works)"));
  }
  Serial.printf("flash LED   : %s\n", g_flashOn ? "ON" : "off");
  Serial.printf("Wi-Fi       : %s",
                WiFi.status() == WL_CONNECTED ? "connected, IP " : "NOT connected\n");
  if (WiFi.status() == WL_CONNECTED)
    Serial.printf("%s  RSSI %d dBm  ch %d\n",
                  WiFi.localIP().toString().c_str(), WiFi.RSSI(), WiFi.channel());
  if (g_lastJpeg)
    Serial.printf("last JPEG   : RAM %u bytes  station=%u%s%s\n",
                  (unsigned)g_lastJpegLen, (unsigned)g_lastStation,
                  g_lastPhoto[0] ? "  SD=" : "",
                  g_lastPhoto[0] ? g_lastPhoto : "");
  else
    Serial.println(F("last JPEG   : none (send SHOOT)"));
  if (g_autoMs) Serial.printf("auto-capture: every %lu ms\n", (unsigned long)g_autoMs);
  Serial.println(F("----------------"));
}

static void listPhotos() {
  if (!g_sdReady) {
    Serial.println("[list] SD not mounted; send INIT after inserting a card");
    return;
  }
  File dir = SD_MMC.open(PHOTO_DIR);
  if (!dir || !dir.isDirectory()) {
    Serial.printf("[list] %s does not exist yet (no captures yet)\n", PHOTO_DIR);
    return;
  }
  Serial.printf("[list] %s:\n", PHOTO_DIR);
  int count = 0;
  File entry = dir.openNextFile();
  while (entry && count < LIST_MAX_ENTRIES) {
    if (!entry.isDirectory()) {
      String nm = entry.name();
      Serial.printf("  %-24s %10u bytes\n", nm.c_str(), (unsigned)entry.size());
      ++count;
    }
    entry.close();
    entry = dir.openNextFile();
  }
  while (entry) { entry.close(); entry = dir.openNextFile(); }  // drain remainder
  dir.close();
  Serial.printf("[list] %d entr%s shown (cap %d).\n",
                count, (count == 1) ? "y" : "ies", LIST_MAX_ENTRIES);
}

static void setFlash(bool on) {
  pinMode(FLASH_LED_GPIO, OUTPUT);
  digitalWrite(FLASH_LED_GPIO, on ? HIGH : LOW);
  g_flashOn = on;
  Serial.printf("[flash] LED %s\n", on ? "ON" : "OFF");
}

// Re-run bring-up without rebooting: (re)init camera and try optional SD.
static void doInit() {
  Serial.println(F("[init] re-initialising..."));
  if (!g_cameraReady) initCamera();
  initSD();
  Serial.printf("[init] camera=%s optional-sd=%s\n",
                g_cameraReady ? "OK" : "FAIL", g_sdReady ? "OK" : "not mounted");
}

// ---------------------------------------------------------------------------
//  Command dispatch (line already stripped of CR/LF)
// ---------------------------------------------------------------------------
static void handleCommand(char *line) {
  // Uppercase only the first whitespace-delimited token (the verb).
  char *arg = line;
  while (*arg && !isspace((unsigned char)*arg)) {
    *arg = (char)toupper((unsigned char)*arg);
    ++arg;
  }
  if (*arg) { *arg = 0; ++arg; }     // split verb / remainder
  while (*arg && isspace((unsigned char)*arg)) ++arg;

  if (strcmp(line, "CAPTURE") == 0) {
    int station = (arg && *arg) ? atoi(arg) : 0;
    doCapture(station);
  } else if (strcmp(line, "STATUS") == 0) {
    printStatus();
  } else if (strcmp(line, "LIST") == 0) {
    listPhotos();
  } else if (strcmp(line, "INIT") == 0) {
    doInit();
  } else if (strcmp(line, "FLASH") == 0) {
    if (strcmp(arg, "ON") == 0)       setFlash(true);
    else if (strcmp(arg, "OFF") == 0) setFlash(false);
    else Serial.println(F("[flash] usage: FLASH ON | OFF"));
  } else if (strcmp(line, "SHOOT") == 0) {
    int station = (arg && *arg) ? atoi(arg) : (int)g_lastStation;
    if (ensureWifi()) doCapture(station);
    else Serial.println(F("[shoot] no Wi-Fi -- not captured. Run WIFI, or use CAPTURE for local-only."));
  } else if (strcmp(line, "SEND") == 0) {
    int station = (arg && *arg) ? atoi(arg) : (int)g_lastStation;
    sendLastPhoto((uint16_t)station);
  } else if (strcmp(line, "WIFI") == 0) {
    if (WiFi.status() == WL_CONNECTED)
      Serial.printf("[wifi] connected. IP %s  RSSI %d dBm  ch %d\n",
                    WiFi.localIP().toString().c_str(), WiFi.RSSI(), WiFi.channel());
    else connectToHub();
  } else if (strcmp(line, "AUTO") == 0) {
    if (!arg || !*arg) { g_autoMs = 0; Serial.println(F("[auto] OFF")); }
    else {
      long ms = atol(arg);
      if (ms < 1000) ms = 1000;            // floor at 1 s
      g_autoMs = (uint32_t)ms;
      g_autoLast = millis();
      Serial.printf("[auto] every %lu ms (CAPTURE+SEND). Send AUTO alone to stop.\n", (unsigned long)g_autoMs);
    }
  } else if (strcmp(line, "HELP") == 0 || strcmp(line, "?") == 0) {
    printHelp();
  } else if (line[0] != 0) {
    Serial.printf("[cmd] unknown \"%s\" -- send HELP\n", line);
  }
}

// Drain one command source into its own line buffer and dispatch complete
// lines. `tag` non-null echoes each accepted line to the USB console, so a
// robot-triggered capture is visible while you watch the monitor.
static void pumpCommandStream(Stream &io, char *buf, int &len, const char *tag) {
  while (io.available()) {
    char c = (char)io.read();
    if (c == '\n' || c == '\r') {
      if (len > 0) {
        buf[len] = '\0';
        if (tag) Serial.printf("[%s] <- \"%s\"\n", tag, buf);
        handleCommand(buf);
      }
      len = 0;
    } else if (len < (int)(CMD_BUF_LEN - 1)) {
      buf[len++] = c;   // silently drop chars beyond the buffer
    }
  }
}

// ===========================================================================
//  Hub integration: join the Hub SoftAP, then POST each JPEG to the dashboard.
// ===========================================================================
// connectToHub(): join "MuseumHub" (channel 1, same as ESP-NOW) as a STA.
//   Non-fatal -- the node still captures to SD even if the Hub is offline.
static bool connectToHub() {
  if (WiFi.status() == WL_CONNECTED) return true;
  Serial.printf("[wifi] joining '%s' on ch %d ...\n", HUB_SSID, museum::kEspNowChannel);
  WiFi.mode(WIFI_STA);
  WiFi.setHostname("esp32-cam");
  WiFi.begin(HUB_SSID, HUB_PASS);
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - t0) < WIFI_CONNECT_TIMEOUT_MS) {
    delay(250);
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[wifi] joined. IP %s  RSSI %d dBm  ch %d\n",
                  WiFi.localIP().toString().c_str(), WiFi.RSSI(), WiFi.channel());
    return true;
  }
  Serial.println(F("[wifi] could not join Hub -- captures stay in RAM (run WIFI, then SEND)."));
  return false;
}

static bool ensureWifi() {
  return (WiFi.status() == WL_CONNECTED) ? true : connectToHub();
}

// initEspNow(): add the broadcast peer so we can send a 1-packet "image ready"
// ping on the same channel. The JPEG itself travels over HTTP; this just makes
// the Hub dashboard refresh near-instantly.
static bool initEspNow() {
  if (esp_now_init() != ESP_OK) { Serial.println(F("[espnow] init FAILED")); return false; }
  esp_now_peer_info_t peer{};
  memcpy(peer.peer_addr, museum::kBroadcastAddress, 6);
  peer.channel = 0;            // 0 = use the current Wi-Fi channel
  peer.ifidx   = WIFI_IF_STA;  // we send from the STA interface
  peer.encrypt = false;
  if (esp_now_add_peer(&peer) != ESP_OK) { Serial.println(F("[espnow] add_peer FAILED")); return false; }
  Serial.println(F("[espnow] ready (broadcast peer added)."));
  return true;
}

static void notifyHubImageReady(uint8_t stationId) {
  museum::MuseumMessage m = museum::makeMessage(museum::MSG_IMAGE_READY, CAM_NODE_ID);
  m.stationId = stationId;
  museum::finalizeMessage(m);
  esp_now_send(museum::kBroadcastAddress, (const uint8_t*)&m, sizeof(m));
}

// uploadToHubJpeg(): POST the in-memory JPEG to /api/image/upload using
// multipart/form-data (field "image"). The Hub uses WebServer's standard
// HTTPUpload callback for this -- the same path used by OTA/file uploads --
// which is watchdog-safe on this ESP32 core (the raw-body HTTPRaw callback is
// not). Returns true only on HTTP 200.
static bool uploadToHubJpeg(const uint8_t *jpeg, size_t jpegLen, uint16_t stationId) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("[hub] not connected; skipping upload (run WIFI first)."));
    return false;
  }
  if (!jpeg || jpegLen == 0) {
    Serial.println(F("[hub] no photo to upload (CAPTURE first)."));
    return false;
  }

  static const char kBoundary[] = "----MuseumCamBoundary";
  String head = String("--") + kBoundary + "\r\n"
                "Content-Disposition: form-data; name=\"image\"; filename=\"cam.jpg\"\r\n"
                "Content-Type: image/jpeg\r\n\r\n";
  String tail = String("\r\n--") + kBoundary + "--\r\n";

  const size_t bodyLen = head.length() + jpegLen + tail.length();
  uint8_t *body = (uint8_t *)ps_malloc(bodyLen);
  if (!body) body = (uint8_t *)malloc(bodyLen);
  if (!body) {
    Serial.println(F("[hub] multipart body alloc failed"));
    return false;
  }
  memcpy(body, head.c_str(), head.length());
  memcpy(body + head.length(), jpeg, jpegLen);
  memcpy(body + head.length() + jpegLen, tail.c_str(), tail.length());

  WiFiClient client;
  HTTPClient http;
  char url[100];
  snprintf(url, sizeof(url), "%s?station=%u", HUB_UPLOAD_URL, (unsigned)stationId);
  if (!http.begin(client, url)) {
    Serial.println(F("[hub] http.begin failed"));
    free(body);
    return false;
  }
  http.addHeader("Content-Type",
                 String("multipart/form-data; boundary=") + kBoundary);
  http.setTimeout(HUB_HTTP_TIMEOUT_MS);
  int code = http.sendRequest("POST", body, bodyLen);
  http.end();
  free(body);

  bool ok = (code == 200);
  Serial.printf("[hub] POST multipart JPEG -> HTTP %d  %u bytes  %s\n",
                code, (unsigned)jpegLen, ok ? "OK" : "FAILED");
  return ok;
}

// sendLastPhoto(): re-push the most recent capture (e.g. after a Hub reboot).
static void sendLastPhoto(uint16_t stationId) {
  if (!g_lastJpeg || g_lastJpegLen == 0) {
    Serial.println(F("[send] no RAM-cached JPEG yet -- CAPTURE first."));
    return;
  }
  if (!ensureWifi()) return;
  if (uploadToHubJpeg(g_lastJpeg, g_lastJpegLen, stationId))
    notifyHubImageReady((uint8_t)stationId);
}

// ===========================================================================
//  Arduino entry points
// ===========================================================================
void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(300);

  Serial.println();
  Serial.println(F("========================================"));
  Serial.println(F(" Smart Museum -- ESP32S-CAM (GC2145) node"));
  Serial.println(F(" capture -> RAM JPEG -> Hub dashboard (SD optional)"));
  Serial.println(F(" upload transport: multipart/form-data field 'image'"));
  Serial.println(F("========================================"));

  Serial.printf("[boot] PSRAM %s (%.1f MB), free heap %.0f KB\n",
                psramFound() ? "present" : "MISSING",
                psramFound() ? mb(ESP.getPsramSize()) : 0.0,
                mb(ESP.getFreeHeap()) * 1024.0);

  initCamera();
  Serial.println(F("[sd] optional backup not mounted at boot; send INIT to try an SD card."));

  // ---- Robot trigger link: the guide ESP32 sends "SHOOT <station>". ----
  RobotLink.begin(ROBOT_LINK_BAUD, SERIAL_8N1,
                  ROBOT_LINK_RX_GPIO, ROBOT_LINK_TX_GPIO);
  Serial.printf("[link] robot trigger ready on GPIO%d @ %d baud (RX only)\n",
                ROBOT_LINK_RX_GPIO, ROBOT_LINK_BAUD);

  // Flash LED off by default (reliability first; brownout-prone rail).
  pinMode(FLASH_LED_GPIO, OUTPUT);
  digitalWrite(FLASH_LED_GPIO, LOW);

  // ---- Join the Hub Wi-Fi so captures can be pushed to the dashboard. ----
  // Non-fatal: if the Hub is offline the latest capture stays cached in RAM.
  WiFi.mode(WIFI_STA);
  WiFi.setHostname("esp32-cam");
  connectToHub();
  initEspNow();   // best-effort: instant MSG_IMAGE_READY ping on each upload

  Serial.println(F("[boot] ready. Commands: SHOOT/CAPTURE [station] | SEND | WIFI | AUTO <ms> | STATUS | LIST | INIT | FLASH ON/OFF | HELP"));
  Serial.println(F("[boot] send HELP for details."));
}

void loop() {
  // Two independent command sources: the USB console and the robot's trigger
  // wire. Both speak the same line protocol and share one dispatcher.
  pumpCommandStream(Serial,    g_cmdBuf,  g_cmdLen,  nullptr);
  pumpCommandStream(RobotLink, g_linkBuf, g_linkLen, "link");

  // Auto-capture timer (CAPTURE + push to Hub). Skipped when g_autoMs == 0.
  if (g_autoMs && (millis() - g_autoLast) >= g_autoMs) {
    g_autoLast = millis();
    Serial.println(F("[auto] tick"));
    doCapture((int)g_lastStation);
  }
}
