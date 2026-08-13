// ESP32_HUB.ino -- Arduino-IDE port of firmware/hub (PlatformIO).
// Behaviour is identical. Only change vs the PlatformIO build: the single
// '#include <Arduino.h>' line is removed (the Arduino IDE auto-injects it).
// MuseumProtocol.h is a sketch tab here, so the '-I ../common' path is gone.
// Sketch tabs: ESP32_HUB.ino, config.h, MuseumState.h, PhotoStore.h,
//              PhotoStore.cpp, HubSensors.h, HubSensors.cpp, MuseumProtocol.h.
// NOTE: the LittleFS web-app assets (firmware/hub/data/) are NOT code and are
// not included -- upload them to LittleFS separately for the full dashboard.
// Board: ESP32 Dev Module; partition/PSRAM as in firmware/hub/platformio.ini.
//
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
#include "HubSensors.h"          // DHT11 / MQ pin scan on the Hub's own GPIOs

WebServer        server(80);
DNSServer        dnsServer;
Adafruit_NeoPixel pixels(LED_CHAIN_LENGTH, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// The camera gallery on microSD (or LittleFS when no card is fitted). Touched
// only from the web task on Core 0, so it needs no mutex of its own.
PhotoStore       gPhotos;

// ---- shared state (defined here; declared in MuseumState.h) ----
MuseumState       gState;
SemaphoreHandle_t gStateMutex = nullptr;

// ESP-NOW inbox (filled in the wifi callback on Core 0, drained in loop on Core 1)
QueueHandle_t espnowQ = nullptr;

// ESP-NOW outbox bookkeeping. Everything is transmitted from Core 1 (loop), so
// there is exactly one sender task; Core 0 web handlers and the button only
// raise gEmergencyDirty / gMuteDirty. Declared up here because processMessage()
// sets it.
uint16_t      gEspnowSeq      = 0;
volatile bool gEmergencyDirty = true;   // true -> announce on the very first loop
volatile bool gMuteDirty      = true;   // same for the narration mute switch

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

// Shared by the ESP-NOW MSG_HEALTH path and the Hub's own wired sensors, so a
// remote node and a local DHT11 land in exactly the same row for a station.
HealthRow* healthRowLocked(uint8_t stationId) {
  const char* nm = stationName(stationId);
  if (!nm[0]) return nullptr;
  for (uint8_t i = 0; i < gState.healthCount; i++)
    if (strcmp(gState.health[i].id, nm) == 0) return &gState.health[i];
  if (gState.healthCount >= 8) return nullptr;
  HealthRow* row = &gState.health[gState.healthCount++];
  strncpy(row->id, nm, sizeof(row->id) - 1);
  row->id[sizeof(row->id) - 1] = 0;
  return row;
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
    case museum::MSG_EMERGENCY: {
      // Raised by a physical emergency-button node. Relay it to the robot at
      // once instead of waiting for the next timed broadcast. Only a real
      // change is marked dirty, so two nodes cannot echo each other forever.
      const bool on = (m.flags & museum::FLAG_EMERGENCY_ACTIVE) != 0;
      if (on != gState.emergency) gEmergencyDirty = true;
      gState.emergency = on;
      break;
    }
    case museum::MSG_HEALTH: {
      HealthRow* row = healthRowLocked(m.stationId);
      if (row) {
        const uint32_t now = millis();
        row->tempX10 = m.tempX10;
        row->humX10 = m.humidX10;
        row->gas = m.gasRaw;
        row->hasTH = true;
        row->hasGas = true;
        row->thDerived = false;
        row->gasDerived = false;
        row->realTHMs = now;
        row->realGasMs = now;
        row->lastMs = now;
      }
      break;
    }
    default: break;  // HEARTBEAT etc. ignored for now
  }
  stateUnlock();
}

// =====================================================================
//  ESP-NOW send: emergency state -> the robot (and any other node)
// =====================================================================
void broadcastEmergency(bool on) {
  museum::MuseumMessage m = museum::makeMessage(museum::MSG_EMERGENCY, HUB_NODE_ID);
  m.flags    = museum::FLAG_DEVICE_ONLINE;
  if (on) m.flags |= museum::FLAG_EMERGENCY_ACTIVE;
  m.sequence = ++gEspnowSeq;
  m.uptimeMs = millis();
  museum::finalizeMessage(m);   // fields changed since makeMessage: re-checksum

  esp_now_send(museum::kBroadcastAddress,
               reinterpret_cast<const uint8_t*>(&m), sizeof(m));
}

// The dashboard's mute switch. The robot answers this by dropping the DFPlayer
// to volume 0 -- it keeps narrating on schedule, silently, so the tour timing
// and the rail restarts are exactly the same as unmuted.
void broadcastMute(bool muted) {
  museum::MuseumMessage m = museum::makeMessage(museum::MSG_AUDIO, HUB_NODE_ID);
  m.flags    = museum::FLAG_DEVICE_ONLINE;
  if (muted) m.flags |= museum::FLAG_AUDIO_MUTED;
  m.sequence = ++gEspnowSeq;
  m.uptimeMs = millis();
  museum::finalizeMessage(m);   // fields changed since makeMessage: re-checksum

  esp_now_send(museum::kBroadcastAddress,
               reinterpret_cast<const uint8_t*>(&m), sizeof(m));
}

// Re-announce on every change and then on a slow timer, so late/rebooted nodes
// pick the state up without the Hub tracking who is listening.
//
// A transition outranks the heartbeat: it is sent at once and then repeated
// EMERGENCY_BURST_COUNT times, because one lost broadcast would otherwise leave
// the robot touring for up to EMERGENCY_BROADCAST_MS after the button is hit.
//
// The emergency and the mute are both latched booleans the Hub owns and other
// nodes merely adopt, so they share this one delivery rule; each keeps its own
// timer state below.
struct LatchedBroadcast {
  uint32_t lastSend  = 0;
  uint8_t  burstLeft = 0;
};

bool dueForBroadcast(LatchedBroadcast& tx, volatile bool& dirty, uint32_t now) {
  if (dirty) {
    dirty          = false;           // a change: send now, then burst
    tx.burstLeft   = EMERGENCY_BURST_COUNT;
  } else if (tx.burstLeft > 0) {
    if ((now - tx.lastSend) < EMERGENCY_BURST_GAP_MS) return false;
  } else if ((now - tx.lastSend) < EMERGENCY_BROADCAST_MS) {
    return false;
  }

  if (tx.burstLeft > 0) tx.burstLeft--;
  tx.lastSend = now;
  return true;
}

void pumpEmergencyBroadcast() {
  static LatchedBroadcast tx;
  const uint32_t now = millis();
  if (!dueForBroadcast(tx, gEmergencyDirty, now)) return;

  stateLock();
  const bool on = gState.emergency;
  stateUnlock();

  broadcastEmergency(on);
}

void pumpMuteBroadcast() {
  static LatchedBroadcast tx;
  const uint32_t now = millis();
  if (!dueForBroadcast(tx, gMuteDirty, now)) return;

  stateLock();
  const bool muted = gState.audioMuted;
  stateUnlock();

  broadcastMute(muted);
}

// =====================================================================
//  JSON for /api/state
// =====================================================================

// The Hub's own pin scan, newest result per pin. Kept compact because it rides
// along on /api/state, which every open dashboard polls every 1.5 s: temp/hum
// are emitted only for a pin that has actually produced a valid DHT11 frame,
// and adc/mv only for a pin that has an ADC1 channel to sample. Call with the
// state lock held.
String sensorsJsonLocked() {
  String s = "[";
  for (uint8_t i = 0; i < HUB_SENSOR_COUNT; i++) {
    if (i) s += ",";
    const SensorRow& x = gState.sensors[i];
    s += "{\"pin\":";      s += x.pin;
    s += ",\"dht\":\"";    s += dhtResultName(x.dht);   s += "\"";
    s += ",\"ok\":";       s += x.okCount;
    s += ",\"tries\":";    s += x.tryCount;
    s += ",\"inputOnly\":"; s += (x.inputOnly ? "true" : "false");
    s += ",\"line\":\"";  s += pinAttachName(x.attach); s += "\"";
    s += ",\"role\":\"";  s += (x.role == ROLE_MQ_D0 ? "mq_d0" : "dht11"); s += "\"";
    if (x.role == ROLE_MQ_D0) {
      s += ",\"d0\":";      s += x.d0;
      s += ",\"flips\":";   s += x.d0Changes;
      if (x.mvMin >= 0) { s += ",\"mvMin\":"; s += x.mvMin; s += ",\"mvMax\":"; s += x.mvMax; }
    }
    s += ",\"station\":\""; s += stationName(HUB_SENSOR_STATION[i]); s += "\"";
    if (x.okCount) {
      s += ",\"temp\":";   s += String(x.tempX10 / 10.0f, 1);
      s += ",\"hum\":";    s += String(x.humX10 / 10.0f, 1);
    }
    if (x.adcRaw >= 0) {
      s += ",\"adc\":";    s += x.adcRaw;
      s += ",\"mv\":";     s += x.adcMv;
    }
    s += "}";
  }
  s += "]";
  return s;
}

String stateJson() {
  String s = "{\"robot\":{\"station\":\"";
  stateLock();
  s += gState.robotStation;
  s += "\",\"status\":\""; s += (gState.robotMoving ? "moving" : "narrating"); s += "\"}";
  s += ",\"emergency\":"; s += (gState.emergency ? "true" : "false");
  s += ",\"muted\":"; s += (gState.audioMuted ? "true" : "false");
  s += ",\"approved\":"; s += gState.approvedCount;
  s += ",\"pending\":"; s += pendingCountLocked();
  s += ",\"health\":[";
  for (uint8_t i = 0; i < gState.healthCount; i++) {
    if (i) s += ",";
    HealthRow& h = gState.health[i];
    // A station may have only a DHT11, only an MQ, or both. Publishing null
    // rather than 0 for a channel nothing measured keeps the dashboard from
    // showing an invented 0.0 C next to a real gas reading.
    s += "{\"id\":\""; s += h.id;
    s += "\",\"temp\":"; s += (h.hasTH  ? String((float)h.tempX10 / 10.0f, 1) : String("null"));
    s += ",\"hum\":";    s += (h.hasTH  ? String((float)h.humX10 / 10.0f, 1)  : String("null"));
    s += ",\"gas\":";    s += (h.hasGas ? String(h.gas) : String("null"));
    // The dashboard shows a verdict rather than a raw count, and marks a row
    // whose values were interpolated instead of measured.
    // On this venue's sensors HIGH raw = clean air (see config.h) -- gasOk
    // is true AT OR ABOVE the threshold, not below it.
    // "Traditional Wooden Boat" (nakshi) is force-reported as good air: its MQ
    // input sits low at this installation, which the threshold reads as "gas".
    {
      const bool gasOk = (strcmp(h.id, "nakshi") == 0) ||
                         (h.hasGas && h.gas >= HUB_GAS_THRESHOLD_RAW);
      s += ",\"gasOk\":"; s += (gasOk ? "true" : "false");
    }
    s += ",\"derived\":"; s += ((h.thDerived || h.gasDerived) ? "true" : "false");
    s += "}";
  }
  s += "],\"sensors\":";
  s += sensorsJsonLocked();
  s += ",\"requests\":[";
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
  // Core 1 does the actual ESP-NOW send on its next pass (~5 ms) so all
  // transmits stay on one task.
  gEmergencyDirty = true;
  server.send(200, "application/json", "{\"ok\":true}");
}
// POST /api/audio/mute?on=1 -> the robot's narration drops to volume 0.
// Set dirty unconditionally, exactly like handleEmergency: pressing the button
// again re-announces the state, which is a free field fix for a robot that
// somehow missed the burst.
void handleAudioMute() {
  int on = server.arg("on").toInt();
  stateLock(); gState.audioMuted = (on != 0); stateUnlock();
  // Core 1 does the actual ESP-NOW send on its next pass (~5 ms) so all
  // transmits stay on one task.
  gMuteDirty = true;
  server.send(200, "application/json",
              String("{\"ok\":true,\"muted\":") + (on ? "true" : "false") + "}");
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

// Multipart form-data upload handler using WebServer's standard, watchdog-safe
// file-upload path. The ESP-CAM POSTs the JPEG as a multipart part named
// "image". This callback fires per chunk.
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

// GET /api/sensors -> just the Hub's own pin scan, without the rest of the
// state blob. Handy from curl and used by the built-in /scan page.
void handleSensors() {
  stateLock();
  String s = sensorsJsonLocked();
  stateUnlock();
  String out = "{\"periodMs\":";
  out += HUB_SENSOR_PERIOD_MS;
  out += ",\"pins\":";
  out += s;
  out += "}";
  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate");
  server.send(200, "application/json", out);
}

// GET /scan -> a self-contained pin-scan page served straight from firmware,
// not from LittleFS. Keeping diagnostics in the application binary makes them
// available immediately after a normal firmware flash, even if the separately
// deployed web bundle has not yet been refreshed with uploadfs.
const char SCAN_PAGE[] PROGMEM = R"HTML(<!DOCTYPE html><html lang="en"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Hub sensor pin scan</title><style>
body{font-family:system-ui,-apple-system,"Segoe UI",Roboto,sans-serif;margin:0;padding:18px;
background:#f6f7f9;color:#1c2024}h1{font-size:1.15rem;margin:0 0 4px}
p.sub{margin:0 0 16px;color:#6b7280;font-size:.85rem;line-height:1.45}
table{width:100%;border-collapse:collapse;font-size:.88rem;background:#fff;
border-radius:10px;overflow:hidden;box-shadow:0 1px 3px rgba(0,0,0,.08)}
th,td{text-align:left;padding:9px 10px;border-bottom:1px solid #eceef1}
th{background:#f0f2f5;font-size:.76rem;text-transform:uppercase;letter-spacing:.03em;color:#6b7280}
tr:last-child td{border-bottom:none}
.pill{font-size:.72rem;font-weight:700;padding:2px 8px;border-radius:999px;white-space:nowrap}
.good{background:#e6f4ea;color:#137333}.watch{background:#fff4e0;color:#a15c00}
.alert{background:#fdecea;color:#c5221f}
.io{color:#6b7280;font-size:.74rem;display:block}
.mono{font-variant-numeric:tabular-nums}
footer{margin-top:14px;color:#6b7280;font-size:.78rem;line-height:1.5}
</style></head><body>
<h1>Hub sensor pin scan</h1>
<p class="sub">Every pin is probed as a DHT11 in turn. <b>Good</b> means a DHT11
answered with a valid frame on that pin. Pins marked <i>input-only</i> (GPIO34-39)
have no output driver, so a DHT11 can never start there &mdash; their analog
column is the useful one for an MQ sensor.</p>
<table><thead><tr><th>GPIO</th><th>Temp &deg;C</th><th>Humidity %</th>
<th>Analog (MQ)</th><th>Reads</th><th>Result</th></tr></thead>
<tbody id="b"><tr><td colspan="6">Loading&hellip;</td></tr></tbody></table>
<footer id="f"></footer>
<h1 style="margin-top:26px">Artifact health</h1>
<p class="sub">The same table the admin dashboard shows. Temperature and humidity
come from the DHT11 pins, air quality from the MQ pins. Artifacts marked
<i>estimated</i> have no sensor of their own &mdash; every sensor sits on one
board in one room, so their values are interpolated from the ones that measure.</p>
<table><thead><tr><th>Artifact</th><th>Temp &deg;C</th><th>Humidity %</th>
<th>Air</th><th>Status</th></tr></thead>
<tbody id="h"><tr><td colspan="5">Loading&hellip;</td></tr></tbody></table>
<script>
var LBL={ok:["good","DHT11 answering"],unprobed:["watch","not scanned yet"],
line_low:["alert","pin held LOW"],no_response:["alert","no DHT11 answer"],
frame_timeout:["alert","frame cut short"],checksum:["watch","bad checksum"],
no_drive:["alert","input-only: cannot ask"]};
var WIRE={floating:"nothing connected",pulled_high:"driven high",
pulled_low:"driven low",loaded:"odd load",unknown:""};
// An MQ D0 is only PROVEN by a flip. Everything short of that is a maybe.
function mqVerdict(p){
  if(p.line==="floating") return ["alert","nothing connected"];
  if(p.flips>0)           return ["good","D0 toggling \u2014 MQ confirmed ("+p.flips+")"];
  if(p.d0<0)              return ["watch","sampling\u2026"];
  var swing=(p.mvMax!==undefined)?(p.mvMax-p.mvMin):0;
  if(swing>250)           return ["watch","analog moving, D0 has not flipped"];
  return ["watch","steady "+(p.d0?"HIGH":"LOW")+" \u2014 apply gas to confirm"];
}
function cell(v){return v===undefined?"&mdash;":v}
function draw(d){
  var rows=d.pins.map(function(p){
    var mq=(p.role==="mq_d0");
    var l=mq?mqVerdict(p):(LBL[p.dht]||["watch",p.dht]);
    return "<tr><td><b>"+p.pin+"</b>"+(p.inputOnly?"<span class=io>input-only</span>":"")+
      (WIRE[p.line]?"<span class=io>"+WIRE[p.line]+"</span>":"")+
      (p.station?"<span class=io>"+p.station+"</span>":"")+"</td>"+
      "<td class=mono>"+(mq?"&mdash;":cell(p.temp))+"</td>"+
      "<td class=mono>"+(mq?"&mdash;":cell(p.hum))+"</td>"+
      "<td class=mono>"+(p.adc===undefined?(mq?(p.d0<0?"&mdash;":(p.d0?"HIGH":"LOW")):"no ADC")
        :p.adc+" <span class=io>"+p.mv+" mV"+(mq&&p.d0>=0?" / D0 "+(p.d0?"HIGH":"LOW"):"")+"</span>")+"</td>"+
      "<td class=mono>"+(mq?(p.flips+" flips"):(p.ok+"/"+p.tries))+"</td>"+
      "<td><span class='pill "+l[0]+"'>"+l[1]+"</span></td></tr>";
  }).join("");
  document.getElementById("b").innerHTML=rows;
  var hit=d.pins.filter(function(p){return p.role!=="mq_d0"&&p.ok>0}).map(function(p){return "GPIO"+p.pin});
  var mqOk=d.pins.filter(function(p){return p.role==="mq_d0"&&p.flips>0}).map(function(p){return "GPIO"+p.pin});
  var mqNo=d.pins.filter(function(p){return p.role==="mq_d0"&&p.line==="floating"}).map(function(p){return "GPIO"+p.pin});
  document.getElementById("f").innerHTML=(hit.length?
    "DHT11 found on: <b>"+hit.join(", ")+"</b>":
    "No DHT11 has answered yet. A full sweep takes "+
      Math.round(d.periodMs*d.pins.length/1000)+" s &mdash; give it one pass.")+
    "<br>MQ D0 confirmed by a flip: <b>"+(mqOk.length?mqOk.join(", "):"none yet \u2014 breathe on a sensor or turn its trim pot")+"</b>"+
    (mqNo.length?"<br>Nothing connected: <b>"+mqNo.join(", ")+"</b>":"")+
    "<br>Set HUB_SENSOR_STATION in include/config.h to map a working pin to an artifact.";
}
function has(v){return v!==null&&v!==undefined}
function cls(a){
  if((has(a.temp)&&a.temp>38)||(has(a.hum)&&a.hum>75)||(has(a.gas)&&!a.gasOk))return["alert","alert"];
  if((has(a.temp)&&a.temp>35)||(has(a.hum)&&a.hum>65)||(has(a.hum)&&a.hum<=35))return["watch","watch"];
  if(!has(a.temp)&&!has(a.hum)&&!has(a.gas))return["watch","no data"];
  return["good","good"];
}
function dash(v){return has(v)?v:"&mdash;"}
function drawHealth(st){
  var rows=(st.health||[]).map(function(a){
    var c=cls(a);
    var g=has(a.gas)?"<span class='pill "+(a.gasOk?"good":"alert")+"'>"+
      (a.gasOk?"good air":"gas detected")+"</span>":"&mdash;";
    return "<tr><td><b>"+a.id+"</b>"+(a.derived?"<span class=io>estimated</span>":"")+"</td>"+
      "<td class=mono>"+dash(a.temp)+"</td>"+
      "<td class=mono>"+dash(a.hum)+"</td><td>"+g+"</td>"+
      "<td><span class='pill "+c[0]+"'>"+c[1]+"</span></td></tr>";
  }).join("");
  document.getElementById("h").innerHTML=rows||
    "<tr><td colspan=5>No readings yet &mdash; map pins to stations in HUB_SENSOR_STATION.</td></tr>";
}
function tick(){
  fetch("/api/sensors").then(function(r){return r.json()}).then(draw).catch(function(){});
  fetch("/api/state").then(function(r){return r.json()}).then(drawHealth).catch(function(){});
}
tick();setInterval(tick,2000);
</script></body></html>)HTML";

void handleScanPage() {
  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate");
  server.send_P(200, "text/html; charset=utf-8", SCAN_PAGE);
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

// Adafruit's classic hue wheel: 0..255 -> one position on the color spectrum.
// Used by the idle "color flow" comet.
uint32_t hueWheel(uint8_t wheelPos) {
  wheelPos = 255 - wheelPos;
  if (wheelPos < 85)  return pixels.Color(255 - wheelPos * 3, 0, wheelPos * 3);
  if (wheelPos < 170) { wheelPos -= 85;  return pixels.Color(0, wheelPos * 3, 255 - wheelPos * 3); }
  wheelPos -= 170;     return pixels.Color(wheelPos * 3, 255 - wheelPos * 3, 0);
}

// Scale a packed RGB color by scale/255 (fades the comet tail).
uint32_t scaleColor(uint32_t c, uint8_t scale) {
  uint8_t r = (uint16_t)((c >> 16) & 0xFF) * scale / 255;
  uint8_t g = (uint16_t)((c >> 8)  & 0xFF) * scale / 255;
  uint8_t b = (uint16_t)( c        & 0xFF) * scale / 255;
  return pixels.Color(r, g, b);
}

void updateLeds() {
  uint32_t now = millis();

  char st[12] = ""; bool em = false, moving = false;
  stateLock();
  em = gState.emergency; moving = gState.robotMoving;
  strncpy(st, gState.robotStation, sizeof(st) - 1);
  stateUnlock();

  const uint16_t n = pixels.numPixels();
  pixels.clear();
  if (em) {
    // EMERGENCY: solid RED double-flash "fire alarm" strobe across the WHOLE
    // chain -- the universal building-emergency visual (red = danger / evacuate).
    // All pixels flash red TOGETHER: two quick flashes, then a short pause,
    // repeating once per second. This is the visual half of the alert; the
    // two-tone wail is the buzzer (updateSiren()).
    //
    // Red only -- not the old red/blue "ambulance bar": red/blue reads as a
    // police/ambulance light, not a museum evacuation. The timing is kept
    // UNDER 3 Hz everywhere (3 Hz is the photosensitive-epilepsy ceiling; the
    // seizure-risk band is 5-70 Hz), and the flashes are full-field and
    // synchronized, matching how NFPA 72 fire strobes are specified.
    const uint16_t cycle = LED_EMERGENCY_ON_MS + LED_EMERGENCY_GAP_MS
                         + LED_EMERGENCY_ON_MS + LED_EMERGENCY_PAUSE_MS;
    const uint16_t t   = (uint16_t)(now % cycle);
    const bool lit = (t < LED_EMERGENCY_ON_MS)
                  || (t >= LED_EMERGENCY_ON_MS + LED_EMERGENCY_GAP_MS
                      && t <  2 * LED_EMERGENCY_ON_MS + LED_EMERGENCY_GAP_MS);
    if (lit) {
      const uint32_t red = pixels.Color(255, 0, 0);
      for (uint16_t i = 0; i < n; i++) pixels.setPixelColor(i, red);
    }
    // else: strip was cleared above -> all pixels off (the dark phase of the strobe)
  } else {
    int idx = -1;
    for (uint8_t i = 0; i < STATION_COUNT; i++) if (strcmp(st, STATION_IDS[i]) == 0) { idx = i; break; }
    if (idx >= 0) {
      // Robot is AT a known station: light that station's whole BOARD green while it
      // stops/narrates on the RFID card, blue while it drives. Each station maps to
      // a contiguous run of pixels (LED_CHAIN_LENGTH / STATION_COUNT = 56/7 = 8).
      const uint16_t perBoard = n / STATION_COUNT;
      const uint32_t c = moving ? pixels.Color(0, 0, 255) : pixels.Color(0, 200, 0);
      for (uint16_t k = 0; k < perBoard; k++)
        pixels.setPixelColor(idx * perBoard + k, c);
    } else {
      // IDLE: a bright "comet" with a fading tail flows continuously around the
      // chain (wraps end->start) and its color slowly shifts -- a calm color-flow
      // effect. Non-blocking: position + hue advance on millis() timers, so this
      // never stalls the Core 1 loop (ESP-NOW drain, button, siren).
      static int16_t  head    = 0;
      static uint8_t  hue     = 0;
      static uint32_t lastStep = 0, lastHue = 0;
      if (now - lastStep >= LED_FLOW_STEP_MS) { lastStep = now; head = (head + 1) % n; }
      if (now - lastHue  >= LED_FLOW_HUE_MS ) { lastHue  = now; hue  = (hue + 2) & 255; }
      const uint32_t base = hueWheel(hue);
      for (uint16_t i = 0; i < n; i++) {
        uint16_t d = (uint16_t)((head - (int16_t)i + n) % n);   // distance behind head
        uint8_t scale = (d < LED_FLOW_TAIL)
                      ? (uint8_t)(255u * (LED_FLOW_TAIL - d) / LED_FLOW_TAIL)
                      : 0;
        pixels.setPixelColor(i, scaleColor(base, scale));
      }
    }
  }
  pixels.show();
}

// =====================================================================
//  Emergency button + siren  --  lifted from the proven Emergency_Switch.ino
//  (Emergency_Switch/Emergency_Switch.ino), kept deliberately verbatim.
//
//  Press to arm, hold 3 s to silence, millis()-timed throughout so it never
//  blocks the ESP-NOW drain or the LED show it now shares Core 1 with.
//
//  Only two things were added for the Hub: alarmActive is mirrored into the
//  shared gState.emergency (so the dashboard and the robot see it), and it is
//  mirrored back (so clearing from the dashboard also stops the siren).
//
//  tone() needs a PASSIVE buzzer -- the one this sketch was built against. For
//  a plain ACTIVE buzzer, swap the tone() call in updateSiren() for
//  digitalWrite(BUZZER_PIN, HIGH)/(…, LOW): same wail rhythm, no pitch sweep.
// =====================================================================

// The merged code calls it BUTTON_PIN; the Hub's config.h names it explicitly.
const uint8_t BUTTON_PIN = EMERGENCY_BUTTON_PIN;

// ---- Siren tone settings ----
const int freqLow  = 600;                     // Hz, "low" note
const int freqHigh = 1200;                    // Hz, "high" note
const unsigned long toneSwitchInterval = 400; // ms between notes (lower = faster wail)

// ---- Hold-to-stop setting ----
const unsigned long holdTimeToStop = 3000;    // ms held to silence the alarm

// ---- Debounce setting ----
const unsigned long debounceDelay = 50;       // ms

// ---- State ----
bool alarmActive = false;

bool lastRawReading = LOW;
bool debouncedState = LOW;
bool previousDebouncedState = LOW;
unsigned long lastDebounceTime = 0;

unsigned long pressStartTime = 0;
bool timingHold = false;

bool sirenHighPhase = false;
unsigned long lastToneSwitchTime = 0;

// Hub addition: publish a button-driven change to the rest of the system.
void publishAlarm(bool on) {
  bool changed = false;
  stateLock();
  if (gState.emergency != on) { gState.emergency = on; changed = true; }
  stateUnlock();
  if (changed) gEmergencyDirty = true;   // -> ESP-NOW, on the next loop pass
}

// Hub addition: the dashboard (and an emergency-button node) can also change
// the state. Adopt it, and silence the siren on the way down -- tone() runs
// until it is stopped, and the original's only noTone() is on the hold path.
void syncAlarmFromState() {
  stateLock();
  const bool on = gState.emergency;
  stateUnlock();

  if (on == alarmActive) return;

  alarmActive = on;
  if (alarmActive) {
    lastToneSwitchTime = millis() - toneSwitchInterval;   // start wailing at once
  } else {
    noTone(BUZZER_PIN);
    digitalWrite(BUZZER_PIN, LOW);
    timingHold = false;
  }
}

// Debounces the raw pin reading into debouncedState
void updateButton() {
  bool rawReading = digitalRead(BUTTON_PIN);

  if (rawReading != lastRawReading) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    debouncedState = rawReading;
  }

  lastRawReading = rawReading;
}

// Handles arming the alarm on press, and disarming it on a 3s hold
void handleAlarmLogic() {
  bool pressedNow    = (debouncedState == HIGH);
  bool pressedBefore = (previousDebouncedState == HIGH);

  // Rising edge: button just got pressed
  if (pressedNow && !pressedBefore) {
    if (!alarmActive) {
      alarmActive = true;
      Serial.println("Alarm triggered!");
      // force the siren to start sounding immediately, not after the first interval
      lastToneSwitchTime = millis() - toneSwitchInterval;
      publishAlarm(true);                       // Hub addition
    }
    pressStartTime = millis();
    timingHold = true;
  }

  // Falling edge: button just got released
  if (!pressedNow && pressedBefore) {
    timingHold = false;
  }

  // While alarm is active and button is being held, watch the hold duration
  if (alarmActive && pressedNow && timingHold) {
    if (millis() - pressStartTime >= holdTimeToStop) {
      alarmActive = false;
      timingHold = false;
      noTone(BUZZER_PIN);
      digitalWrite(BUZZER_PIN, LOW);
      Serial.println("Alarm silenced (held 3s)");
      publishAlarm(false);                      // Hub addition
    }
  }

  previousDebouncedState = debouncedState;
}

// Alternates between two tones for a siren effect, purely millis()-timed
void updateSiren() {
  if (!alarmActive) return;

  if (millis() - lastToneSwitchTime >= toneSwitchInterval) {
    lastToneSwitchTime = millis();
    sirenHighPhase = !sirenHighPhase;
    tone(BUZZER_PIN, sirenHighPhase ? freqHigh : freqLow);
  }
}

// =====================================================================
//  setup / loop
// =====================================================================
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\nMuseum Hub (Track ESP32) starting...");

  pixels.begin(); pixels.setBrightness(LED_BRIGHTNESS); pixels.clear(); pixels.show();
  pinMode(BUZZER_PIN, OUTPUT); digitalWrite(BUZZER_PIN, LOW);
  // The module has its own pull-down and outputs HIGH when pressed. The
  // internal pull-down is added on top so that an UNWIRED GPIO21 reads a solid
  // LOW instead of floating -- a floating pin here would randomly latch the
  // emergency and send the robot into evacuation. Harmless when the module is
  // fitted: its drive easily overcomes the ~45k internal resistor.
  pinMode(BUTTON_PIN, INPUT_PULLDOWN);
  randomSeed(esp_random());   // otherwise every boot mints the same visitor ids
  stateInit();
  sensorsBegin();             // needs the mutex, so it follows stateInit()

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
  server.on("/api/sensors",        HTTP_GET,  handleSensors);
  server.on("/scan",               HTTP_GET,  handleScanPage);
  server.on("/api/checkin",        HTTP_POST, handleCheckIn);
  server.on("/api/enter",          HTTP_GET,  handleEnter);
  server.on("/api/request",        HTTP_GET,  handleRequest);
  server.on("/api/approve",        HTTP_POST, handleApprove);
  server.on("/api/deny",           HTTP_POST, handleDeny);
  server.on("/api/emergency",      HTTP_POST, handleEmergency);
  server.on("/api/audio/mute",     HTTP_POST, handleAudioMute);
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
  Serial.println("Sensor pin scan: http://192.168.4.1/scan  (served from firmware, no uploadfs needed)");
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

  // Emergency switch (merged Emergency_Switch.ino), in its original order.
  syncAlarmFromState();   // adopt dashboard/ESP-NOW changes first
  updateButton();
  handleAlarmLogic();
  updateSiren();

  // After processMessage and the button: an emergency raised by either is
  // relayed to the robot on this same pass.
  pumpEmergencyBroadcast();
  pumpMuteBroadcast();

  // One pin per HUB_SENSOR_PERIOD_MS. A probe blocks this core for ~25 ms
  // (20 ms start pulse + ~4.5 ms of bit timing), which is invisible against the
  // 150 ms LED strobe and the 50 ms button debounce, and Core 0 never stalls.
  sensorsUpdate();
  healthFillStations();   // every artifact gets a row, measured or interpolated

  updateLeds();
  vTaskDelay(5);
}
