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

WebServer        server(80);
DNSServer        dnsServer;
Adafruit_NeoPixel pixels(STATION_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// ---- shared state (defined here; declared in MuseumState.h) ----
MuseumState       gState;
SemaphoreHandle_t gStateMutex = nullptr;

// ESP-NOW inbox (filled in the wifi callback on Core 0, drained in loop on Core 1)
QueueHandle_t espnowQ = nullptr;

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
  s += "]}";
  stateUnlock();
  return s;
}

// =====================================================================
//  HTTP handlers
// =====================================================================
void handleState()   { server.send(200, "application/json", stateJson()); }

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
  int idx = 0;
  for (int i = 0; i < STATION_COUNT; i++) if (strcmp(gState.robotStation, STATION_IDS[i]) == 0) { idx = i; break; }
  strncpy(gState.robotStation, STATION_IDS[(idx + 1) % STATION_COUNT], sizeof(gState.robotStation) - 1);
  gState.robotStation[sizeof(gState.robotStation) - 1] = 0;
  gState.robotMoving = true; gState.robotMs = millis();
  stateUnlock(); server.send(200, "application/json", "{\"ok\":true}");
}
void handleImageUpload() {  // ESP32-CAM posts a raw JPEG body here (CAM phase)
  if (server.hasArg("plain")) {
    LittleFS.remove("/last.jpg");
    File f = LittleFS.open("/last.jpg", "w");
    if (f) { f.write((const uint8_t*)server.arg("plain").c_str(), server.arg("plain").length()); f.close(); }
  }
  server.send(200, "application/json", "{\"ok\":true}");
}
void handleImageGet() {
  if (LittleFS.exists("/last.jpg")) { File f = LittleFS.open("/last.jpg", "r"); server.streamFile(f, "image/jpeg"); f.close(); }
  else server.send(404, "text/plain", "no image");
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
    File f = LittleFS.open(path, "r"); server.streamFile(f, mimeOf(path)); f.close(); return;
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
  stateInit();

  if (!LittleFS.begin(true)) Serial.println("LittleFS mount FAILED (upload the web files!).");
  else Serial.println("LittleFS OK.");

  WiFi.mode(WIFI_AP);
  IPAddress ip(HUB_IP[0], HUB_IP[1], HUB_IP[2], HUB_IP[3]);
  WiFi.softAPConfig(ip, ip, IPAddress(255, 255, 255, 0));
  WiFi.softAP(HUB_SSID, HUB_PASS, HUB_CHANNEL);
  Serial.printf("SoftAP '%s' on ch %d  IP: ", HUB_SSID, HUB_CHANNEL);
  Serial.println(WiFi.softAPIP());

  dnsServer.start(53, "*", ip);  // captive portal: any DNS -> our IP

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
  server.on("/api/image/upload",   HTTP_POST, handleImageUpload);
  server.onNotFound(handleFile);
  server.begin();

  xTaskCreatePinnedToCore(webTask, "web", 8192, nullptr, 1, nullptr, 0);  // network on Core 0
  Serial.println("Hub ready.  Visitor: http://192.168.4.1/   Admin: http://192.168.4.1/admin.html");
}

void loop() {  // Core 1
  museum::MuseumMessage m;
  while (espnowQ && xQueueReceive(espnowQ, &m, 0) == pdTRUE) processMessage(m);
  updateLeds();
  vTaskDelay(5);
}

