// =====================================================================
//  track_hub.ino  —  Museum Hub running on the TRACK ESP32 (dual-core)
//
//  Core 0 (webTask): WiFi SoftAP "MuseumHub" + ESP-NOW + WebServer +
//                    captive-portal DNS  -> serves the web app + /api
//  Core 1 (loop)   : drains the ESP-NOW queue + NeoPixel LED show +
//                    buzzer (emergency blink)
//  Shared state gState is guarded by gStateMutex so both cores are safe.
//
//  Libraries (Arduino IDE -> Library Manager):
//    - Adafruit NeoPixel   (for WS2812 LEDs)
//    WiFi / WebServer / DNSServer / LittleFS / esp_now are built into the
//    ESP32 board package.
// =====================================================================

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <LittleFS.h>
#include <esp_now.h>
#include <Adafruit_NeoPixel.h>

#include "config.h"
#include "MuseumProtocol.h"
#include "MuseumState.h"
#include "PhotoStore.h"

WebServer        server(80);
DNSServer        dnsServer;
Adafruit_NeoPixel pixels(STATION_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// The camera gallery on microSD (or LittleFS when no card is fitted). Touched
// only from the web task on Core 0, so it needs no mutex of its own.
PhotoStore       gPhotos;

// ---- shared state (defined here; declared in MuseumState.h) ----
MuseumState       gState;
SemaphoreHandle_t gStateMutex = nullptr;

// ESP-NOW inbox (filled in the wifi callback on Core 0, drained in loop on Core 1)
QueueHandle_t espnowQ = nullptr;

// JPEG uploads must never pass through Arduino String: JPEG data contains
// embedded NUL bytes. WebServer's multipart HTTPUpload callback delivers each
// file chunk unchanged and follows the core's stable, standard upload path.
// Each finished JPEG is committed into gPhotos as its own numbered file, so the
// dashboard can show the newest photo large and the previous ones as thumbnails.
constexpr size_t      IMAGE_MAX_BYTES   = 65535;  // matches MuseumState::imageBytes

// Single-image paths used before the gallery existed. Removed at boot so an
// already-flashed Hub does not keep a stale copy wasting LittleFS space.
constexpr const char* LEGACY_IMAGE_PATHS[] = {"/last.jpg", "/last.tmp", "/last.bak"};

void removeLegacyImages() {
  File root = LittleFS.open("/");
  if (!root || !root.isDirectory()) { if (root) root.close(); return; }

  char victims[3][16];
  uint8_t n = 0;
  bool isDir = false;
  while (n < 3) {
    String path = root.getNextFileName(&isDir);
    if (!path.length()) break;
    if (isDir) continue;
    for (const char* legacy : LEGACY_IMAGE_PATHS) {
      if (path == legacy) { snprintf(victims[n], sizeof(victims[0]), "%s", legacy); n++; break; }
    }
  }
  root.close();

  for (uint8_t i = 0; i < n; i++)
    if (LittleFS.remove(victims[i])) Serial.printf("[img] removed legacy %s\n", victims[i]);
}

struct ImageUploadContext {
  File        file;
  size_t      received   = 0;
  uint8_t     station    = 0;
  uint16_t    sequence   = 0;
  int         statusCode = 400;
  const char* error      = "no_upload";
  bool        started    = false;
};

ImageUploadContext imageUpload;

// This Arduino-ESP32 core's four-argument WebServer::on() handler dispatches
// its upload callback for BOTH multipart uploads and non-multipart raw bodies.
// Calling server.upload() from that raw path dereferences a null HTTPUpload and
// crashes. This handler deliberately supports multipart upload only; old/raw
// senders fall through to handle(), which returns a clean 415 response.
class MultipartImageHandler final : public RequestHandler {
 public:
  bool canHandle(HTTPMethod method, String uri) override {
    return method == HTTP_POST && uri == "/api/image/upload";
  }

  bool canUpload(String uri) override {
    return uri == "/api/image/upload";
  }

  bool canRaw(String uri) override {
    (void)uri;
    return false;
  }

  bool handle(WebServer& web, HTTPMethod method, String uri) override;
  void upload(WebServer& web, String uri, HTTPUpload& upload) override;
};

MultipartImageHandler imageHandler;

// =====================================================================
//  State helpers
// =====================================================================
void stateInit()                                  { gStateMutex = xSemaphoreCreateMutex(); }
void stateLock()                                  { if (gStateMutex) xSemaphoreTake(gStateMutex, portMAX_DELAY); }
void stateUnlock()                                { if (gStateMutex) xSemaphoreGive(gStateMutex); }

uint8_t pendingCountLocked() {
  uint8_t n = 0;
  for (uint8_t i = 0; i < gState.reqCount; i++) if (gState.requests[i].status == 0) n++;
  return n;
}
void addRequestLocked(const char* id) {
  if (gState.reqCount >= 24) {                       // ring buffer: drop oldest
    for (uint8_t i = 0; i < 23; i++) gState.requests[i] = gState.requests[i + 1];
    gState.reqCount = 23;
  }
  CheckInReq& r = gState.requests[gState.reqCount];
  strncpy(r.id, id, sizeof(r.id) - 1); r.id[sizeof(r.id) - 1] = 0;
  r.status = 0; r.ts = millis();
  gState.reqCount++;
}
CheckInReq* findRequestLocked(const char* id) {
  for (uint8_t i = 0; i < gState.reqCount; i++)
    if (strcmp(gState.requests[i].id, id) == 0) return &gState.requests[i];
  return nullptr;
}
void makeId(char* out, size_t n) {
  snprintf(out, n, "V%lu%02d", (unsigned long)(millis() % 100000), (int)random(10, 99));
}

// =====================================================================
//  ESP-NOW receive -> queue  (runs in the wifi task on Core 0: keep tiny)
// =====================================================================
void enqueueMessage(const uint8_t* data, int len) {
  if (!espnowQ || len != (int)sizeof(museum::MuseumMessage)) return;
  museum::MuseumMessage m;
  memcpy(&m, data, sizeof(m));
  if (!museum::isValidMessage(m)) return;
  xQueueSend(espnowQ, &m, 0);
}
#if ESP_ARDUINO_VERSION_MAJOR >= 3
void onRecv(const esp_now_recv_info_t* info, const uint8_t* data, int len) { enqueueMessage(data, len); }
#else
void onRecv(const uint8_t* mac, const uint8_t* data, int len)             { enqueueMessage(data, len); }
#endif

const char* stationName(uint8_t stationId) {
  if (stationId >= 1 && stationId <= STATION_COUNT) return STATION_IDS[stationId - 1];
  return "";
}

void processMessage(const museum::MuseumMessage& m) {
  stateLock();
  switch (m.messageType) {
    case museum::MSG_LOCATION: {
      const char* nm = stationName(m.stationId);
      if (nm[0]) { strncpy(gState.robotStation, nm, sizeof(gState.robotStation) - 1); gState.robotStation[sizeof(gState.robotStation) - 1] = 0; }
      gState.robotMoving = (m.flags & museum::FLAG_ROBOT_MOVING);
      gState.robotMs = millis();
      break;
    }
    case museum::MSG_EMERGENCY:
      gState.emergency = (m.flags & museum::FLAG_EMERGENCY_ACTIVE);
      break;
    case museum::MSG_HEALTH: {
      const char* nm = stationName(m.stationId);
      if (!nm[0]) break;
      HealthRow* row = nullptr;
      for (uint8_t i = 0; i < gState.healthCount; i++) if (strcmp(gState.health[i].id, nm) == 0) { row = &gState.health[i]; break; }
      if (!row && gState.healthCount < 8) { row = &gState.health[gState.healthCount++]; strncpy(row->id, nm, sizeof(row->id) - 1); row->id[sizeof(row->id) - 1] = 0; }
      if (row) { row->tempX10 = m.tempX10; row->humX10 = m.humidX10; row->gas = m.gasRaw; row->lastMs = millis(); }
      break;
    }
    default: break;  // HEARTBEAT etc. ignored for now
  }
  stateUnlock();
}

// =====================================================================
//  JSON for /api/state
// =====================================================================
String stateJson() {
  String s = "{\"robot\":{\"station\":\"";
  stateLock();
  s += gState.robotStation;
  s += "\",\"status\":\""; s += (gState.robotMoving ? "moving" : "narrating"); s += "\"}";
  s += ",\"emergency\":"; s += (gState.emergency ? "true" : "false");
  s += ",\"approved\":"; s += gState.approvedCount;
  s += ",\"pending\":"; s += pendingCountLocked();
  s += ",\"health\":[";
  for (uint8_t i = 0; i < gState.healthCount; i++) {
    if (i) s += ",";
    HealthRow& h = gState.health[i];
    s += "{\"id\":\""; s += h.id; s += "\",\"temp\":";
    s += String((float)h.tempX10 / 10.0f, 1); s += ",\"hum\":";
    s += String((float)h.humX10 / 10.0f, 1); s += ",\"gas\":";
    s += h.gas; s += "}";
  }
  s += "],\"requests\":[";
  for (uint8_t i = 0; i < gState.reqCount; i++) {
    if (i) s += ",";
    CheckInReq& r = gState.requests[i];
    const char* st = (r.status == 0 ? "pending" : (r.status == 1 ? "approved" : "denied"));
    s += "{\"id\":\""; s += r.id; s += "\",\"status\":\""; s += st; s += "\"}";
  }
  s += "],\"image\":{\"bytes\":";
  s += gState.imageBytes;
  s += ",\"station\":\"";
  s += stationName(gState.imageStation);
  s += "\",\"seq\":";
  s += gState.imageSeq;
  stateUnlock();
  // gPhotos lives on this task only, so it is read outside the state lock.
  s += ",\"count\":";
  s += gPhotos.count();
  s += ",\"store\":\"";
  s += gPhotos.backend();
  s += "\"}}";
  return s;
}

// =====================================================================
//  HTTP handlers
// =====================================================================
void handleState() {
  // Polled every 1.5 s by every open dashboard: never let a phone browser
  // answer a poll from its own cache.
  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate");
  server.send(200, "application/json", stateJson());
}

void handleCheckIn() {
  char id[16]; makeId(id, sizeof(id));
  stateLock(); addRequestLocked(id); stateUnlock();
  server.send(200, "application/json", String("{\"id\":\"") + id + "\"}");
}

// Native-camera scan: phone scans a QR of  http://192.168.4.1/api/enter  ->
// creates the request here, then bounces to the gate page's waiting screen.
void handleEnter() {
  char id[16]; makeId(id, sizeof(id));
  stateLock(); addRequestLocked(id); stateUnlock();
  server.sendHeader("Location", String("http://192.168.4.1/index.html?autoid=") + id);
  server.send(302, "text/plain", "");
}
void handleRequest() {
  String id = server.arg("id"), status = "unknown";
  stateLock();
  CheckInReq* r = findRequestLocked(id.c_str());
  if (r) status = (r->status == 0 ? "pending" : (r->status == 1 ? "approved" : "denied"));
  stateUnlock();
  server.send(200, "application/json", String("{\"id\":\"") + id + "\",\"status\":\"" + status + "\"}");
}
void handleApprove() {
  String id = server.arg("id");
  stateLock(); CheckInReq* r = findRequestLocked(id.c_str());
  if (r && r->status == 0) { r->status = 1; gState.approvedCount++; }
  stateUnlock(); server.send(200, "application/json", "{\"ok\":true}");
}
void handleDeny() {
  String id = server.arg("id");
  stateLock(); CheckInReq* r = findRequestLocked(id.c_str());
  if (r) r->status = 2;
  stateUnlock(); server.send(200, "application/json", "{\"ok\":true}");
}
void handleEmergency() {
  int on = server.arg("on").toInt();
  stateLock(); gState.emergency = (on != 0); stateUnlock();
  server.send(200, "application/json", "{\"ok\":true}");
}
void handleAdvance() {  // test button: nudge the mock robot to the next station
  stateLock();
  // -1 = the robot has no station yet, so the first press lands on station 1
  // instead of skipping it.
  int idx = -1;
  for (int i = 0; i < STATION_COUNT; i++) if (strcmp(gState.robotStation, STATION_IDS[i]) == 0) { idx = i; break; }
  strncpy(gState.robotStation, STATION_IDS[(idx + 1) % STATION_COUNT], sizeof(gState.robotStation) - 1);
  gState.robotStation[sizeof(gState.robotStation) - 1] = 0;
  gState.robotMoving = true; gState.robotMs = millis();
  stateUnlock(); server.send(200, "application/json", "{\"ok\":true}");
}
void failImageUpload(int statusCode, const char* error) {
  if (imageUpload.file) imageUpload.file.close();
  gPhotos.discardUploadTemp();
  imageUpload.statusCode = statusCode;
  imageUpload.error      = error;
}

// A truncated transfer still produces a file, so check the JPEG markers and the
// byte count before this upload is allowed to replace the newest photo.
bool validateTemporaryJpeg() {
  File f = gPhotos.openUploadTempForRead();
  if (!f) return false;

  const size_t savedBytes = f.size();
  uint8_t soi[2] = {0, 0};
  uint8_t eoi[2] = {0, 0};
  const bool startRead = savedBytes >= 4 && f.read(soi, sizeof(soi)) == sizeof(soi);
  const bool endRead = startRead && f.seek(savedBytes - sizeof(eoi), SeekSet) &&
                       f.read(eoi, sizeof(eoi)) == sizeof(eoi);
  f.close();

  const bool sizeOk    = (savedBytes == imageUpload.received);
  const bool markersOk = startRead && endRead &&
                         soi[0] == 0xFF && soi[1] == 0xD8 &&
                         eoi[0] == 0xFF && eoi[1] == 0xD9;
  if (!sizeOk || !markersOk) {
    // Say exactly which check failed: a size mismatch means the transfer was
    // cut short, bad markers mean the bytes themselves arrived mangled.
    Serial.printf("[img] REJECT saved=%u received=%u SOI=%02X%02X EOI=%02X%02X\n",
                  static_cast<unsigned>(savedBytes),
                  static_cast<unsigned>(imageUpload.received),
                  soi[0], soi[1], eoi[0], eoi[1]);
  }
  return sizeOk && markersOk;
}

// Multipart form-data upload handler (the standard, watchdog-safe WebServer
// upload path -- the same one used by file/OTA uploads). The ESP-CAM POSTs the
// JPEG as a multipart part named "image". This callback fires per chunk.
void handleImageUploadForm(HTTPUpload& upload) {
  switch (upload.status) {
    case UPLOAD_FILE_START:
      if (imageUpload.file) imageUpload.file.close();
      gPhotos.discardUploadTemp();

      imageUpload.received   = 0;
      imageUpload.sequence   = 0;
      imageUpload.station    = 0;
      imageUpload.statusCode = 200;
      imageUpload.error      = nullptr;
      imageUpload.started    = true;

      if (!gPhotos.ready()) {
        failImageUpload(507, "no_storage");
        break;
      }

      if (upload.name != "image" || upload.type != "image/jpeg") {
        failImageUpload(415, "invalid_image_part");
        break;
      }

      imageUpload.file = gPhotos.openUploadTemp();
      if (!imageUpload.file) {
        Serial.printf("[img] fs_open failed on %s: used=%llu/%llu\n",
                      gPhotos.backend(),
                      static_cast<unsigned long long>(gPhotos.usedBytes()),
                      static_cast<unsigned long long>(gPhotos.capacityBytes()));
        failImageUpload(500, "fs_open");
      }
      break;

    case UPLOAD_FILE_WRITE:
      if (imageUpload.error) break;

      if (upload.currentSize > IMAGE_MAX_BYTES - imageUpload.received) {
        failImageUpload(413, "image_too_large");
        break;
      }

      if (!imageUpload.file ||
          imageUpload.file.write(upload.buf, upload.currentSize) != upload.currentSize) {
        failImageUpload(500, "fs_write");
        break;
      }

      imageUpload.received += upload.currentSize;
      break;

    case UPLOAD_FILE_END:
      if (imageUpload.file) { imageUpload.file.flush(); imageUpload.file.close(); }
      break;

    case UPLOAD_FILE_ABORTED:
      failImageUpload(400, "aborted");
      Serial.println("[img] upload aborted; previous image preserved");
      break;
  }
}

void MultipartImageHandler::upload(WebServer& web, String uri, HTTPUpload& upload) {
  (void)web;
  (void)uri;
  handleImageUploadForm(upload);
}

// Called once after the multipart body is fully received. Query args are
// available here, so station is read from ?station=N. Validate + commit + bump
// state only on a clean, complete, valid JPEG; otherwise reject and preserve
// the previous image.
void handleImageUploadComplete() {
  if (server.hasArg("station")) {
    // Clamped to a real station: the id is one digit of the photo's 8.3 file
    // name, and anything outside 1..STATION_COUNT has no name to show anyway.
    const long station = server.arg("station").toInt();
    if (station >= 0 && station <= STATION_COUNT)
      imageUpload.station = static_cast<uint8_t>(station);
  }

  char response[128];
  bool ok = false;
  PhotoMeta committed;

  if (!imageUpload.started) {
    imageUpload.statusCode = 400; imageUpload.error = "missing_image_part";
  } else if (imageUpload.error) {
    // The upload callback already selected the precise status and error.
  } else if (imageUpload.received == 0) {
    imageUpload.statusCode = 400; imageUpload.error = "empty_image";
  } else if (!validateTemporaryJpeg()) {
    imageUpload.statusCode = 422; imageUpload.error = "invalid_jpeg";
  } else if (!gPhotos.commitUploadTemp(imageUpload.received, imageUpload.station,
                                       &committed)) {
    imageUpload.statusCode = 500; imageUpload.error = "fs_commit";
  } else {
    // The gallery owns the sequence number now, so /api/image?seq=N and the
    // seq the dashboard polls for always name the same file.
    stateLock();
    gState.imageBytes   = committed.bytes;
    gState.imageStation = committed.station;
    gState.imageMs      = committed.ms;
    gState.imageSeq     = committed.seq;
    stateUnlock();

    imageUpload.sequence   = committed.seq;
    imageUpload.statusCode = 200;
    imageUpload.error      = nullptr;
    ok = true;
    Serial.printf("[img] committed station=%u  bytes=%u  seq=%u  gallery=%u/%u on %s\n",
                  static_cast<unsigned>(committed.station),
                  static_cast<unsigned>(committed.bytes),
                  static_cast<unsigned>(committed.seq),
                  static_cast<unsigned>(gPhotos.count()),
                  static_cast<unsigned>(GALLERY_MAX_PHOTOS),
                  gPhotos.backend());
  }

  // Always remove a leftover temp file on the non-commit paths.
  if (!ok) gPhotos.discardUploadTemp();

  if (ok) {
    snprintf(response, sizeof(response),
             "{\"ok\":true,\"bytes\":%u,\"station\":%u,\"seq\":%u}",
             static_cast<unsigned>(imageUpload.received),
             static_cast<unsigned>(imageUpload.station),
             static_cast<unsigned>(imageUpload.sequence));
    server.send(200, "application/json", response);
  } else {
    snprintf(response, sizeof(response),
             "{\"ok\":false,\"err\":\"%s\",\"bytes\":%u}",
             imageUpload.error ? imageUpload.error : "upload_failed",
             static_cast<unsigned>(imageUpload.received));
    server.send(imageUpload.statusCode ? imageUpload.statusCode : 400,
                "application/json", response);
  }

  imageUpload.started    = false;
  imageUpload.received   = 0;
  imageUpload.statusCode = 400;
  imageUpload.error      = "no_upload";
}

bool MultipartImageHandler::handle(WebServer& web, HTTPMethod method, String uri) {
  (void)method;
  (void)uri;

  if (imageUpload.started) {
    handleImageUploadComplete();
  } else {
    web.send(415, "application/json",
             "{\"ok\":false,\"err\":\"multipart_image_required\"}");
  }

  return true;
}
// GET /api/image           -> the newest photo (never cached)
// GET /api/image?seq=N     -> that exact photo. A given seq is written once and
//                             never rewritten, so the browser may cache it
//                             forever -- that is what keeps the thumbnail strip
//                             from re-downloading every photo on each poll.
void handleImageGet() {
  const bool bySeq = server.hasArg("seq");
  uint16_t seq = gPhotos.newestSeq();
  if (bySeq) {
    const long asked = server.arg("seq").toInt();
    if (asked < 1 || asked > 65535) { server.send(400, "text/plain", "bad seq"); return; }
    seq = static_cast<uint16_t>(asked);
  }

  if (seq == 0) { server.send(404, "text/plain", "no image"); return; }

  File f = gPhotos.openSeq(seq);
  if (!f) { server.send(404, "text/plain", "no image"); return; }

  server.sendHeader("Cache-Control", bySeq ? "public, max-age=31536000, immutable"
                                           : "no-store, no-cache, must-revalidate");
  server.streamFile(f, "image/jpeg");
  f.close();
}

// GET /api/gallery -> newest-first list the admin dashboard renders.
// "age" is milliseconds since the photo arrived, or -1 for photos recovered
// from storage after a Hub reboot (their arrival time is not known).
void handleGallery() {
  const uint32_t now = millis();
  String s = "{\"backend\":\"";
  s += gPhotos.backend();
  s += "\",\"count\":";
  s += gPhotos.count();
  s += ",\"max\":";
  s += GALLERY_MAX_PHOTOS;
  s += ",\"newest\":";
  s += gPhotos.newestSeq();
  s += ",\"photos\":[";
  for (uint8_t i = 0; i < gPhotos.count(); i++) {
    const PhotoMeta* p = gPhotos.at(i);
    if (!p) break;
    if (i) s += ",";
    s += "{\"seq\":";        s += p->seq;
    s += ",\"bytes\":";      s += p->bytes;
    s += ",\"stationId\":";  s += p->station;
    s += ",\"station\":\"";  s += stationName(p->station);
    s += "\",\"age\":";      s += (p->ms ? String(now - p->ms) : String("-1"));
    s += "}";
  }
  s += "]}";
  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate");
  server.send(200, "application/json", s);
}

// POST /api/gallery/clear -> wipe every stored photo (admin "clear" button).
void handleGalleryClear() {
  gPhotos.clear();
  stateLock();
  gState.imageBytes = 0; gState.imageStation = 0; gState.imageMs = 0; gState.imageSeq = 0;
  stateUnlock();
  server.send(200, "application/json", "{\"ok\":true}");
}

// ---- static files + captive portal ----
String mimeOf(const String& path) {
  if (path.endsWith(".html")) return "text/html; charset=utf-8";
  if (path.endsWith(".css"))  return "text/css";
  if (path.endsWith(".js"))   return "application/javascript";
  if (path.endsWith(".json")) return "application/json";
  if (path.endsWith(".png"))  return "image/png";
  if (path.endsWith(".jpg") || path.endsWith(".jpeg")) return "image/jpeg";
  if (path.endsWith(".svg"))  return "image/svg+xml";
  return "text/plain";
}
bool isCaptiveProbe(const String& uri) {
  return uri == "/generate_204" || uri == "/gen_204" || uri == "/hotspot-detect.html" ||
         uri == "/library/test/signal.html" || uri == "/connecttest.txt" || uri == "/ncsi.txt" ||
         uri == "/fwlink" || uri == "/redirect" || uri == "/canonical.html" || uri == "/success.txt";
}
void handleFile() {
  String path = server.uri();
  if (path == "/") path = "/index.html";
  if (LittleFS.exists(path)) {
    // Open first: streamFile() on an invalid handle (or a directory, which
    // exists() also reports) would dereference a null file implementation.
    File f = LittleFS.open(path, "r");
    if (f && !f.isDirectory()) {
      // During development/demo setup the web app is updated independently of
      // firmware. Prevent phone captive-portal browsers from retaining an old UI.
      server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate");
      server.sendHeader("Pragma", "no-cache");
      server.sendHeader("Expires", "0");
      server.streamFile(f, mimeOf(path));
      f.close();
      return;
    }
    if (f) f.close();
  }
  if (isCaptiveProbe(path)) { server.sendHeader("Location", "http://192.168.4.1/"); server.send(302, "text/plain", ""); return; }
  server.send(404, "text/plain", "Not found");
}

// =====================================================================
//  Core 0 task: captive DNS + web server
// =====================================================================
void webTask(void*) {
  for (;;) {
    dnsServer.processNextRequest();
    server.handleClient();
    vTaskDelay(1);
  }
}

// =====================================================================
//  Core 1: LED show + ESP-NOW queue drain
// =====================================================================
void updateLeds() {
  uint32_t now = millis();
  static uint32_t lastBlink = 0;
  static bool blinkOn = false;

  char st[12] = ""; bool em = false, moving = false;
  stateLock();
  em = gState.emergency; moving = gState.robotMoving;
  strncpy(st, gState.robotStation, sizeof(st) - 1);
  stateUnlock();

  pixels.clear();
  if (em) {
    if (now - lastBlink > 250) { lastBlink = now; blinkOn = !blinkOn; digitalWrite(BUZZER_PIN, blinkOn ? HIGH : LOW); }
    uint32_t c = blinkOn ? pixels.Color(255, 0, 0) : 0;
    for (uint8_t i = 0; i < STATION_COUNT; i++) pixels.setPixelColor(i, c);
  } else {
    digitalWrite(BUZZER_PIN, LOW);
    int idx = -1;
    for (uint8_t i = 0; i < STATION_COUNT; i++) if (strcmp(st, STATION_IDS[i]) == 0) { idx = i; break; }
    if (idx >= 0) pixels.setPixelColor(idx, moving ? pixels.Color(0, 0, 255) : pixels.Color(0, 200, 0));
  }
  pixels.show();
}

// =====================================================================
//  setup / loop
// =====================================================================
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\nMuseum Hub (Track ESP32) starting...");

  pixels.begin(); pixels.clear(); pixels.show();
  pinMode(BUZZER_PIN, OUTPUT); digitalWrite(BUZZER_PIN, LOW);
  randomSeed(esp_random());   // otherwise every boot mints the same visitor ids
  stateInit();

  if (!LittleFS.begin(true)) Serial.println("LittleFS mount FAILED (upload the web files!).");
  else Serial.println("LittleFS OK.");

  // Camera gallery: microSD when a card is fitted, LittleFS otherwise.
  gPhotos.begin();

  // Drop the single-image files this Hub used before the gallery existed. The
  // root is scanned instead of calling exists() per name, because a miss there
  // logs a VFS error on every boot for files that will never come back.
  removeLegacyImages();

  WiFi.mode(WIFI_AP);
  IPAddress ip(HUB_IP[0], HUB_IP[1], HUB_IP[2], HUB_IP[3]);
  WiFi.softAPConfig(ip, ip, IPAddress(255, 255, 255, 0));
  WiFi.softAP(HUB_SSID, HUB_PASS, HUB_CHANNEL);
  Serial.printf("SoftAP '%s' on ch %d  IP: ", HUB_SSID, HUB_CHANNEL);
  Serial.println(WiFi.softAPIP());

  dnsServer.start(53, "*", ip);  // captive portal: any DNS -> our IP

  const char* collectedHeaders[] = {"X-Museum-Station", "X-Museum-Bytes"};
  server.collectHeaders(collectedHeaders,
                        sizeof(collectedHeaders) / sizeof(collectedHeaders[0]));

  espnowQ = xQueueCreate(16, sizeof(museum::MuseumMessage));
  if (esp_now_init() == ESP_OK) {
    esp_now_register_recv_cb(onRecv);
    esp_now_peer_info_t peer{};
    memcpy(peer.peer_addr, museum::kBroadcastAddress, 6);
    peer.channel = HUB_CHANNEL; peer.ifidx = WIFI_IF_AP; peer.encrypt = false;
    esp_now_add_peer(&peer);
    Serial.println("ESP-NOW ready.");
  } else Serial.println("ESP-NOW init FAILED.");

  server.on("/api/state",          HTTP_GET,  handleState);
  server.on("/api/checkin",        HTTP_POST, handleCheckIn);
  server.on("/api/enter",          HTTP_GET,  handleEnter);
  server.on("/api/request",        HTTP_GET,  handleRequest);
  server.on("/api/approve",        HTTP_POST, handleApprove);
  server.on("/api/deny",           HTTP_POST, handleDeny);
  server.on("/api/emergency",      HTTP_POST, handleEmergency);
  server.on("/api/robot/advance",  HTTP_POST, handleAdvance);
  server.on("/api/image",          HTTP_GET,  handleImageGet);
  server.on("/api/gallery",        HTTP_GET,  handleGallery);
  server.on("/api/gallery/clear",  HTTP_POST, handleGalleryClear);
  server.addHandler(&imageHandler);
  server.onNotFound(handleFile);
  server.begin();

  // 16 KB, not the original 8 KB: this task now also walks the SD card, and
  // FATFS keeps its working buffers on the caller's stack.
  xTaskCreatePinnedToCore(webTask, "web", 16384, nullptr, 1, nullptr, 0);  // network on Core 0
  Serial.println("Image receiver: multipart/form-data field 'image'.");
  Serial.printf("Gallery: %s, keeping the newest %u photos.\n",
                gPhotos.backend(), static_cast<unsigned>(GALLERY_MAX_PHOTOS));
  Serial.println("Hub ready.  Visitor: http://192.168.4.1/   Admin: http://192.168.4.1/admin.html");
}

// "moving" is set by a robot LOCATION report (or the admin test button) and
// nothing ever clears it, so let it lapse back to "narrating" once the reports
// stop. Otherwise the dashboard dot and the station LED stay in the moving
// state for the rest of the demo.
void settleRobotMotion() {
  stateLock();
  if (gState.robotMoving && (millis() - gState.robotMs) > ROBOT_MOVE_SETTLE_MS)
    gState.robotMoving = false;
  stateUnlock();
}

void loop() {  // Core 1
  museum::MuseumMessage m;
  while (espnowQ && xQueueReceive(espnowQ, &m, 0) == pdTRUE) processMessage(m);
  settleRobotMotion();
  updateLeds();
  vTaskDelay(5);
}
