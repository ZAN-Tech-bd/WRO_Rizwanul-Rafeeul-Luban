// =====================================================================
//  Emergency-button bench test — Hub PCB J20 port on YD-ESP32-S3 N16R8
//
//  Purpose: prove the emergency switch wiring works before flashing the
//  full hub firmware. Press the button -> "PRESSED" in the Serial
//  Monitor; release -> "RELEASED". That is all.
//
//  Pinout (PCB design/hub/WIRING-PLAN.md §"Emergency button"):
//    J20 (JST XH 3-pin)  1 = 3V3   -> button module VCC
//                        2 = GND   -> button module GND
//                        3 = SIG   -> R13 (1k) -> ESTOP_IO = GPIO21
//    The module outputs HIGH when pressed (own pull-down on the module;
//    this sketch adds INPUT_PULLDOWN exactly like firmware/hub does).
//
//  Baud: 115200  |  Chip verified by esptool: ESP32-S3, 16MB flash,
//  8MB PSRAM = N16R8, MAC 30:ed:a0:37:e9:00
// =====================================================================

#include <Arduino.h>

static const uint8_t  BUTTON_PIN      = 21;   // ESTOP_IO (hub net ESTOP -> R13 1k -> GPIO21)
static const uint32_t DEBOUNCE_MS     = 30;   // cheap mechanical-switch debounce
static const uint32_t HEARTBEAT_MS    = 5000; // "still alive" line so you see it booting

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLDOWN);        // idle LOW, pressed = HIGH (firmware/hub convention)

  delay(300);                                 // let the USB-CDC serial settle

  Serial.println();
  Serial.println("======================================");
  Serial.println(" EMERGENCY BUTTON TEST  (J20 -> GPIO21)");
  Serial.println("======================================");
  Serial.printf("Chip      : %s rev %d, %d cores\n",
                ESP.getChipModel(), ESP.getChipRevision(), ESP.getChipCores());
  Serial.printf("Flash     : %u bytes (%.0f MB)\n",
                ESP.getFlashChipSize(), ESP.getFlashChipSize() / 1048576.0);
  Serial.printf("SDK       : %s\n", ESP.getSdkVersion());
  Serial.printf("Button pin: GPIO%u  (INPUT_PULLDOWN, HIGH = pressed)\n", BUTTON_PIN);
  Serial.println("Press / release the emergency button now.");
  Serial.println("--------------------------------------");
}

void loop() {
  static bool     lastStable = LOW;    // debounced state we last reported
  static bool     lastRead   = LOW;    // raw reading, for debounce timing
  static uint32_t lastChange = 0;      // millis() of last raw change
  static uint32_t lastBeat   = 0;

  const uint32_t now  = millis();
  const bool     read = digitalRead(BUTTON_PIN);

  if (read != lastRead) {              // raw edge: restart the debounce window
    lastRead   = read;
    lastChange = now;
  }

  if ((now - lastChange) >= DEBOUNCE_MS && read != lastStable) {
    lastStable = read;
    Serial.printf("[%9lu ms]  %s  (GPIO21 = %s)\n",
                  now, read ? ">>> PRESSED <<<" : "--- released ---",
                  read ? "HIGH" : "LOW");
  }

  if (now - lastBeat >= HEARTBEAT_MS) {          // proof the sketch is alive
    lastBeat = now;
    Serial.printf("[%9lu ms]  idle, GPIO21 = %s\n",
                  now, lastStable ? "HIGH (still pressed)" : "LOW");
  }
}
