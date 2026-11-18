// Hub PCB bench test v4 for SIX stations S1..S6 (hub ports J10..J15), YD-ESP32-S3 N16R8.
// LED: one buffered chain on GPIO45: U2.3 -> R6 330R -> J10.7 (S1 DIN);
//      S(k) DOUT -> 220R -> S(k+1) DIN. Total 48 pixels = 6 x 8.
//      Stations must be populated CONSECUTIVELY (S1,S2,S3...) - an empty middle
//      port interrupts the chain to later ports (no SB bypasses on this hub).
// DHT11: S1=15 S2=16 S3=17 S4=18 S5=38 S6=39 (hub R31..R36 1k series; station J3 XH-3).
// MQ-5:  S1=4  S2=5  S3=6  S4=7  S5=1  S6=2  (ADC1; station 68k/100k divider; hub C3..C8).
// PROX:  Z1(S1+S2)=41, Z2(S3+S4)=42, Z3(S5+S6)=3  LOW = detected (Schottky pulls LOW).
// Web:   SoftAP S1Bench / museum2026 -> http://192.168.4.1/  JSON /api/data  LED /led?s=&c=
// Serial commands: 1..6 = green on that station; r/g/b/w = ALL stations; a = RGB cycle;
//   t = pixel walk over the whole chain; 0/x = all off; h/? = help;
//   + / - = GPIO45 steady HIGH/LOW for 30 s (buffer/level meter test, NOT LED data).
// Station names follow NOV_HUB: terracotta woodenboat rickshaw pottery flute basket.
// Power OFF before swapping stations. Regulated 5V, common GND.
// This bench sketch does NOT implement emergency-stop / robot-control functions.

#include <Adafruit_NeoPixel.h>
#include <DHT.h>
#include <WiFi.h>
#include <WebServer.h>
#include <driver/gpio.h>
#include "web_page.h"

constexpr uint8_t LED_PIN = 45;
constexpr uint8_t STATION_COUNT = 6;
constexpr uint8_t PIXELS_PER_STATION = 8;
constexpr uint8_t TOTAL_PIXELS = STATION_COUNT * PIXELS_PER_STATION;
constexpr uint8_t BRIGHTNESS = 16;
constexpr uint32_t COLOUR_STEP_MS = 2000;
constexpr uint32_t PIXEL_STEP_MS = 400;
constexpr uint32_t REFRESH_MS = 250;   // Re-send if LED power came on after MCU.
constexpr uint32_t METER_HOLD_MS = 30000;

constexpr uint8_t DHT_PINS[STATION_COUNT] = {15, 16, 17, 18, 38, 39};
constexpr uint8_t MQ_PINS[STATION_COUNT] = {4, 5, 6, 7, 1, 2};
// PROX is shared per zone pair: S1+S2, S3+S4, S5+S6.
constexpr uint8_t PROX_PINS[STATION_COUNT] = {41, 41, 42, 42, 3, 3};
constexpr float MQ_DIVIDER_RATIO = 1.68f;  // Station 68k/100k: AO = V_pin * 1.68.

constexpr uint32_t DHT_STEP_MS = 500;   // One station per step -> each read every 3 s.
constexpr uint32_t MQ_PERIOD_MS = 1000;
constexpr uint32_t DATA_PRINT_MS = 1000;
constexpr uint8_t MQ_SAMPLES = 4;

const char* AP_SSID = "S1Bench";
const char* AP_PASS = "museum2026";
const char* STATION_NAMES[STATION_COUNT] = {
  "terracotta", "woodenboat", "rickshaw", "pottery", "flute", "basket"
};

Adafruit_NeoPixel strip(TOTAL_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);
DHT dhtSensor[STATION_COUNT] = {
  DHT(15, DHT11), DHT(16, DHT11), DHT(17, DHT11),
  DHT(18, DHT11), DHT(38, DHT11), DHT(39, DHT11)
};
WebServer server(80);

enum class Mode : uint8_t { Off, Solid, Cycle, Walk, MeterHigh, MeterLow };
Mode mode = Mode::Cycle;
uint32_t stationColour[STATION_COUNT] = {};
uint32_t cycleColourValue = 0;
uint8_t colourIndex = 0;
uint8_t walkPixel = 0;
uint32_t lastStep = 0;
uint32_t lastRefresh = 0;
uint32_t meterStarted = 0;

float temperatureC[STATION_COUNT] = {};
float humidityPct[STATION_COUNT] = {};
bool dhtOk[STATION_COUNT] = {};
bool dhtSeen[STATION_COUNT] = {};
uint16_t mqRaw[STATION_COUNT] = {};
bool mqSeen[STATION_COUNT] = {};
uint8_t dhtRoundRobin = 0;
uint32_t lastDhtMs = 0;
uint32_t lastMqMs = 0;
uint32_t lastDataMs = 0;

bool meterMode() {
  return mode == Mode::MeterHigh || mode == Mode::MeterLow;
}

uint32_t cycleColour(uint8_t index) {
  switch (index % 3) {
    case 0: return strip.Color(255, 0, 0);
    case 1: return strip.Color(0, 255, 0);
    default: return strip.Color(0, 0, 255);
  }
}

const char* modeName() {
  switch (mode) {
    case Mode::Off: return "OFF";
    case Mode::Solid: return "SOLID";
    case Mode::Cycle: return "CYCLE";
    case Mode::Walk: return "WALK";
    case Mode::MeterHigh: return "METER HIGH";
    default: return "METER LOW";
  }
}

bool proxDetected(uint8_t station) {
  return digitalRead(PROX_PINS[station]) == LOW;  // LOW = detected.
}

void setAllStations(uint32_t colour) {
  for (uint8_t s = 0; s < STATION_COUNT; ++s) stationColour[s] = colour;
}

void help() {
  Serial.println("\n=== 6-STATION BENCH v4: LED chain + DHT + MQ + PROX + WEB ===");
  Serial.println("LED: 48 pixels (6x8) on the S1..S6 chain (J10..J15). Populate stations");
  Serial.println("consecutively - an empty middle port breaks the chain to later ports.");
  Serial.println("Wi-Fi AP: S1Bench / museum2026 -> open http://192.168.4.1/");
  Serial.println("Serial data row every 1s: LED mode + all stations DHT/MQ/PROX.");
  Serial.println("1..6 : green on that station only; r/g/b/w : ALL stations");
  Serial.println("t : pixel walk over the 48-pixel chain; a : RGB cycle; 0/x : all OFF");
  Serial.println("+ : GPIO45 steady HIGH for 30s; - : steady LOW for 30s (meter test)");
  Serial.println("METER MODES ARE NOT LED DATA; strip may stay dark. Then returns OFF.");
  Serial.println("Never inject 5V into GPIO45. Serial shows commands, NOT proof LEDs lit.");
}

void status() {
  Serial.printf("[%lu ms] LED=%s colours:", static_cast<unsigned long>(millis()),
                modeName());
  for (uint8_t s = 0; s < STATION_COUNT; ++s) {
    Serial.printf(" S%u=#%06lX", s + 1, static_cast<unsigned long>(stationColour[s]));
  }
  Serial.println();
}

void render() {
  if (meterMode()) return;  // Never overwrite the steady level with RMT data.
  strip.clear();
  if (mode == Mode::Cycle) {
    strip.fill(cycleColourValue);
  } else if (mode == Mode::Walk) {
    strip.setPixelColor(walkPixel, strip.Color(0, 255, 0));
  } else if (mode == Mode::Solid) {
    for (uint8_t s = 0; s < STATION_COUNT; ++s) {
      if (stationColour[s] == 0) continue;
      for (uint8_t p = 0; p < PIXELS_PER_STATION; ++p) {
        strip.setPixelColor(s * PIXELS_PER_STATION + p, stationColour[s]);
      }
    }
  }
  strip.show();
  lastRefresh = millis();
}

void startMeter(bool high) {
  // First command the LEDs off; then disconnect peripheral routing from GPIO45.
  mode = Mode::Off;
  render();
  gpio_reset_pin(static_cast<gpio_num_t>(LED_PIN));
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, high ? HIGH : LOW);
  mode = high ? Mode::MeterHigh : Mode::MeterLow;
  meterStarted = millis();
  status();
}

void command(char c) {
  if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
  if (c == '\r' || c == '\n' || c == ' ' || c == '\t') return;
  if (c == 'h' || c == '?') {
    help();
    status();
    return;
  }
  if (c == '+' || c == '-') {
    startMeter(c == '+');
    return;
  }

  if (c >= '1' && c <= '0' + STATION_COUNT) {
    uint8_t s = c - '1';
    stationColour[s] = strip.Color(0, 255, 0);
    mode = Mode::Solid;
    Serial.printf("Station %u (%s) green.\n", s + 1, STATION_NAMES[s]);
  } else if (c == 'r' || c == 'g' || c == 'b' || c == 'w') {
    uint32_t colour = c == 'r' ? strip.Color(255, 0, 0)
                    : c == 'g' ? strip.Color(0, 255, 0)
                    : c == 'b' ? strip.Color(0, 0, 255)
                               : strip.Color(255, 255, 255);
    setAllStations(colour);
    mode = Mode::Solid;
  } else if (c == 't') {
    mode = Mode::Walk;
    walkPixel = 0;
  } else if (c == 'a') {
    mode = Mode::Cycle;
    colourIndex = 0;
    cycleColourValue = cycleColour(colourIndex);
  } else if (c == '0' || c == 'x') {
    setAllStations(0);
    mode = Mode::Off;
  } else {
    Serial.println("Unknown command; send h for help.");
    return;
  }
  // If leaving a meter mode, show() reconnects the RMT peripheral to GPIO45.
  lastStep = millis();
  render();
  status();
}

// ---- Per-station web/serial LED control ----

void applyLedCommand(uint8_t station, char c) {
  // station 0 = all stations.
  if (c == '0' || c == 'x') {
    if (station == 0) { setAllStations(0); mode = Mode::Off; }
    else stationColour[station - 1] = 0;
    if (mode == Mode::Off) setAllStations(0);
  } else if (c == 'r' || c == 'g' || c == 'b' || c == 'w') {
    uint32_t colour = c == 'r' ? strip.Color(255, 0, 0)
                    : c == 'g' ? strip.Color(0, 255, 0)
                    : c == 'b' ? strip.Color(0, 0, 255)
                               : strip.Color(255, 255, 255);
    if (station == 0) setAllStations(colour);
    else stationColour[station - 1] = colour;
    mode = Mode::Solid;
  }
  lastStep = millis();
  render();
  status();
}

void readDhtStation(uint8_t s) {
  float t = dhtSensor[s].readTemperature();
  float h = dhtSensor[s].readHumidity();
  if (isnan(t) || isnan(h)) {
    dhtOk[s] = false;
  } else {
    dhtOk[s] = true;
    dhtSeen[s] = true;
    temperatureC[s] = t;
    humidityPct[s] = h;
  }
}

void readAllMq() {
  for (uint8_t s = 0; s < STATION_COUNT; ++s) {
    uint32_t sum = 0;
    for (uint8_t i = 0; i < MQ_SAMPLES; ++i) {
      sum += analogRead(MQ_PINS[s]);
      delay(2);
    }
    mqRaw[s] = sum / MQ_SAMPLES;
    mqSeen[s] = true;
  }
}

void printDataRow() {
  Serial.printf("[%lu ms] LED=%s |", static_cast<unsigned long>(millis()), modeName());
  for (uint8_t s = 0; s < STATION_COUNT; ++s) {
    const float vPin = mqRaw[s] * 3.3f / 4095.0f;
    Serial.printf(" S%u:", s + 1);
    if (dhtSeen[s] && dhtOk[s]) {
      Serial.printf("DHT%.1fC/%.0f%%", temperatureC[s], humidityPct[s]);
    } else {
      Serial.print("DHT--");
    }
    Serial.printf(" MQ%u(%.2f/%.2fV) P:%s", mqRaw[s], vPin, vPin * MQ_DIVIDER_RATIO,
                  proxDetected(s) ? "DET" : "CLR");
  }
  Serial.printf(" | clients=%u\n", static_cast<unsigned>(WiFi.softAPgetStationNum()));
}

// ---------- Web server ----------

void handleRoot() {
  server.send_P(200, "text/html", INDEX_HTML);
}

void handleData() {
  String json = "{\"mode\":\"";
  json += modeName();
  json += "\",\"stations\":[";
  for (uint8_t s = 0; s < STATION_COUNT; ++s) {
    const float vPin = mqRaw[s] * 3.3f / 4095.0f;
    if (s) json += ',';
    json += "{\"i\":";
    json += s + 1;
    json += ",\"name\":\"";
    json += STATION_NAMES[s];
    json += "\",\"led\":\"";
    char hex[7];
    snprintf(hex, sizeof(hex), "%06lX", static_cast<unsigned long>(stationColour[s]));
    json += hex;
    json += "\",\"dht\":{\"ok\":";
    json += dhtSeen[s] && dhtOk[s] ? "true" : "false";
    json += ",\"t\":";
    json += dhtSeen[s] && dhtOk[s] ? String(temperatureC[s], 1) : "null";
    json += ",\"h\":";
    json += dhtSeen[s] && dhtOk[s] ? String(humidityPct[s], 0) : "null";
    json += "},\"mq\":{\"raw\":";
    json += mqRaw[s];
    json += ",\"v\":";
    json += String(vPin, 2);
    json += ",\"ao\":";
    json += String(vPin * MQ_DIVIDER_RATIO, 2);
    json += "},\"prox\":\"";
    json += proxDetected(s) ? "DETECTED" : "CLEAR";
    json += "\"}";
  }
  json += "],\"up_ms\":";
  json += static_cast<unsigned long>(millis());
  json += ",\"clients\":";
  json += static_cast<unsigned>(WiFi.softAPgetStationNum());
  json += "}";
  server.send(200, "application/json", json);
}

void handleLed() {
  long station = 0;
  if (server.hasArg("s")) station = server.arg("s").toInt();
  if (station < 0 || station > STATION_COUNT) {
    server.send(400, "text/plain", "bad s");
    return;
  }
  if (!server.hasArg("c")) {
    server.send(400, "text/plain", "missing c");
    return;
  }
  const String c = server.arg("c");
  if (c == "plus") {
    startMeter(true);
    server.send(200, "text/plain", "ok");
    return;
  }
  if (c == "minus") {
    startMeter(false);
    server.send(200, "text/plain", "ok");
    return;
  }
  if (c == "a") {
    mode = Mode::Cycle;
    colourIndex = 0;
    cycleColourValue = cycleColour(colourIndex);
    lastStep = millis();
    render();
    status();
    server.send(200, "text/plain", "ok");
    return;
  }
  if (c == "t") {
    mode = Mode::Walk;
    walkPixel = 0;
    lastStep = millis();
    render();
    status();
    server.send(200, "text/plain", "ok");
    return;
  }
  if (c.length() == 1) {
    applyLedCommand(static_cast<uint8_t>(station), c[0]);
    server.send(200, "text/plain", "ok");
    return;
  }
  server.send(400, "text/plain", "bad c");
}

void startWifi() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.printf("SoftAP '%s' ready. Open http://%s/ from a phone/laptop on this AP.\n",
                AP_SSID, WiFi.softAPIP().toString().c_str());
}

void setup() {
  Serial.begin(115200);
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.clear();
  strip.show();
  cycleColourValue = cycleColour(0);

  for (uint8_t s = 0; s < STATION_COUNT; ++s) {
    pinMode(PROX_PINS[s], INPUT_PULLUP);  // Zone reads HIGH when clear.
    dhtSensor[s].begin();
  }
  analogReadResolution(12);

  startWifi();
  server.on("/", handleRoot);
  server.on("/api/data", handleData);
  server.on("/led", handleLed);
  server.begin();

  delay(300);
  help();
  Serial.printf("Chip: %s; flash: %u MB; PSRAM heap: %u bytes\n",
                ESP.getChipModel(), ESP.getFlashChipSize() / 1048576U,
                ESP.getPsramSize());
  lastStep = millis();
  lastDhtMs = lastMqMs = lastDataMs = millis();
  render();
  status();
}

void loop() {
  while (Serial.available()) command(static_cast<char>(Serial.read()));

  const uint32_t now = millis();
  if (meterMode()) {
    if (now - meterStarted >= METER_HOLD_MS) {
      Serial.println("Meter timeout: returning GPIO45 LOW / LEDs OFF.");
      command('0');
    }
  } else {
    if (mode == Mode::Cycle && now - lastStep >= COLOUR_STEP_MS) {
      colourIndex = (colourIndex + 1) % 3;
      cycleColourValue = cycleColour(colourIndex);
      lastStep = now;
      render();
      status();
    } else if (mode == Mode::Walk && now - lastStep >= PIXEL_STEP_MS) {
      walkPixel = (walkPixel + 1) % TOTAL_PIXELS;
      lastStep = now;
      render();
      status();
    }
    if (now - lastRefresh >= REFRESH_MS) render();
  }

  if (now - lastDhtMs >= DHT_STEP_MS) {
    lastDhtMs = now;
    readDhtStation(dhtRoundRobin);
    dhtRoundRobin = (dhtRoundRobin + 1) % STATION_COUNT;
  }
  if (now - lastMqMs >= MQ_PERIOD_MS) {
    lastMqMs = now;
    readAllMq();
  }
  if (now - lastDataMs >= DATA_PRINT_MS) {
    lastDataMs = now;
    printDataRow();
  }

  server.handleClient();
  delay(1);
}
