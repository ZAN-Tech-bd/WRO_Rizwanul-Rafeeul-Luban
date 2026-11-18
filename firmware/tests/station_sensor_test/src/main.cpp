// =====================================================================
//  Station DHT11 bench test — Hub PCB v0.4 + YD-ESP32-S3 N16R8
//
//  PROVEN CODE PATH (user-verified 2026-11-11): Adafruit DHT library,
//  same structure as the working bench diagnostic.
//
//  Reads the two DHT11 sensors AND the two MQ-5 gas sensors:
//    S1  J10.4 -> GPIO15 (U1A.8)   DHT11, 3V3-powered module
//    S2  J11.4 -> GPIO16 (U1A.9)   DHT11, 3V3-powered module
//    S1  J10.5 -> GPIO4  (U1A.4)   MQ-5 AO, direct, 100 nF (C3) at pin
//    S2  J11.5 -> GPIO5  (U1A.5)   MQ-5 AO, direct, 100 nF (C4) at pin
//
//  MQ lines carry NO series resistor on the hub (by design, §5) — the
//  station's own 68k/100k divider already limits the cable to <=3.0 V,
//  so the R31-R37 removal cannot affect them. Source V = Vadc * 1.68.
//
//  HARDWARE CHANGE (user decision, 2026-11-11): hub DHT series resistors
//  R31–R37 (1 kΩ by design) were REMOVED — the DHT11 only answered when
//  the resistor was bypassed. Data lines are now direct Jk.4 -> GPIO.
//  Consequence: the GPIO's mis-crimp / hot-plug / ESD protection on the
//  DHT lines is gone; a 5 V crimp error on pin 4 can now reach the S3.
//  Root cause of the original failure was never conclusively proven
//  (suspected wrong value stuffed in R31/R32; see README.md).
//
//  Auto-reads BOTH sensors every 3 s with SUCCESS/FAIL counters,
//  and prints each data line's idle state at boot (HIGH expected).
// =====================================================================

#include <Arduino.h>
#include <DHT.h>

constexpr uint8_t DHT_S1_PIN = 15;   // station S1, hub J10 pin 4
constexpr uint8_t DHT_S2_PIN = 16;   // station S2, hub J11 pin 4
constexpr uint8_t MQ_S1_PIN  = 4;    // station S1, hub J10 pin 5 (ADC1)
constexpr uint8_t MQ_S2_PIN  = 5;    // station S2, hub J11 pin 5 (ADC1)
constexpr uint8_t DHT_TYPE   = DHT11;

DHT dht1(DHT_S1_PIN, DHT_TYPE);
DHT dht2(DHT_S2_PIN, DHT_TYPE);

int ok1 = 0, fail1 = 0;
int ok2 = 0, fail2 = 0;

void report(const char *name, uint8_t pin, DHT &d, int &ok, int &fail) {
  const float humidity    = d.readHumidity();
  const float temperature = d.readTemperature();
  if (isnan(temperature) || isnan(humidity)) {
    fail++;
    Serial.printf("%s GPIO%-2u FAIL    #%d: no valid DHT11 data\n",
                  name, pin, fail);
  } else {
    ok++;
    Serial.printf("%s GPIO%-2u SUCCESS #%d: %.1f C | %.1f %% RH\n",
                  name, pin, ok, temperature, humidity);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println("\n--- STATION DHT11 DIAGNOSTIC (S1 + S2) ---");

  // Idle-state check straight from the proven diagnostic: the data line
  // should idle HIGH through the module's pull-up. LOW here means wiring
  // / power / damaged-module trouble before any read is attempted.
  pinMode(DHT_S1_PIN, INPUT);
  pinMode(DHT_S2_PIN, INPUT);
  delay(200);
  Serial.printf("S1 GPIO%u idle: %s\n", DHT_S1_PIN,
                digitalRead(DHT_S1_PIN) ? "HIGH (good)" : "LOW  (check wiring/power)");
  Serial.printf("S2 GPIO%u idle: %s\n", DHT_S2_PIN,
                digitalRead(DHT_S2_PIN) ? "HIGH (good)" : "LOW  (check wiring/power)");

  dht1.begin();
  dht2.begin();
  delay(2500);   // DHT11 needs ~1 s stable + library settle after begin()

  analogReadResolution(12);   // 0..4095 counts

  Serial.println("Reading every 3 s: DHT (GPIO15/16) + MQ (GPIO4/5).");
  Serial.println("-------------------------------------------");
}

uint32_t mqAverage(uint8_t pin) {          // 16 samples to smooth the ADC
  uint32_t sum = 0;
  for (int i = 0; i < 16; i++) { sum += analogRead(pin); delay(2); }
  return sum / 16;
}

void reportMQ(const char *name, uint8_t pin) {
  const uint32_t raw  = mqAverage(pin);
  const float    adcV = raw * 3.3f / 4095.0f;
  const float    srcV = adcV * 1.68f;    // undo station 68k/100k divider
  Serial.printf("%s GPIO%-2u MQ  raw=%4lu  adc=%.2fV  sensor≈%.2fV\n",
                name, pin, (unsigned long)raw, adcV, srcV);
}

void loop() {
  report("S1", DHT_S1_PIN, dht1, ok1, fail1);
  report("S2", DHT_S2_PIN, dht2, ok2, fail2);
  reportMQ("S1", MQ_S1_PIN);
  reportMQ("S2", MQ_S2_PIN);
  Serial.printf("totals: S1 %d ok / %d fail | S2 %d ok / %d fail\n",
                ok1, fail1, ok2, fail2);
  Serial.println();
  delay(3000);
}
