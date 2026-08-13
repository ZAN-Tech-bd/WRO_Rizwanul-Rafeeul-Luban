// =====================================================================
//  LED Test Bench  —  WS2812B demo for the Hub's 74HCT125 level-shifted chain
//
//  PURPOSE
//    A standalone "does my wiring work?" program. It does NOT replicate
//    the Hub's logic — it just drives the LED strip through the same
//    74HCT125 channel the Hub uses, so you can confirm every pixel lights
//    in sequence with a variety of colors (a PC-RGB look).
//
//  DATA PATH (identical to the Hub)
//    ESP32 GPIO15  ->  74HCT125 pin 2 (1A)
//    74HCT125 pin 1 (/1OE) -> GND   (enables channel 1)
//    74HCT125 pin 3 (1Y)   -> 330-470 ohm -> first board DIN
//    board N DOUT -> board N+1 DIN  ... last board DOUT unconnected
//
//  EDIT THE CONFIG BLOCK BELOW to match YOUR chain (pin + pixel count).
// =====================================================================

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#if defined(ARDUINO_ARCH_ESP32) && !defined(LED_BUILTIN)
#define LED_BUILTIN 2   // most DevKit boards have a blue LED on GPIO2
#endif

// ============================== CONFIG ==============================
// Change these two lines to match what you actually wired.
#define LED_PIN        15      // ESP32 GPIO that feeds 74HCT125 pin 2
#define PIXEL_COUNT    56      // 7 boards x 8 pixels. Set to YOUR total.

#define BRIGHTNESS     110     // 0..255 global cap. Lower = dimmer + less current.
                               // 56 pixels full-white can pull ~3.4 A, so this
                               // also acts as a soft current limit.
// ====================================================================

Adafruit_NeoPixel strip(PIXEL_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// Named colors are assigned in setup() (strip must exist first).
uint32_t C_RED, C_GREEN, C_BLUE, C_CYAN, C_MAGENTA, C_YELLOW,
         C_PURPLE, C_ORANGE, C_WHITE;

// ---------------------------------------------------------------------
//  Helpers
// ---------------------------------------------------------------------

// Adafruit's classic hue generator: 0..255 -> a color wheel position.
uint32_t Wheel(byte wheelPos) {
  wheelPos = 255 - wheelPos;
  if (wheelPos < 85) {
    return strip.Color(255 - wheelPos * 3, 0, wheelPos * 3);
  }
  if (wheelPos < 170) {
    wheelPos -= 85;
    return strip.Color(0, wheelPos * 3, 255 - wheelPos * 3);
  }
  wheelPos -= 170;
  return strip.Color(wheelPos * 3, 255 - wheelPos * 3, 0);
}

// Dim a packed color by scale/255 (used to fade comet tails and to breathe).
uint32_t dimColor(uint32_t color, uint8_t scale) {
  uint8_t r = (uint16_t)((color >> 16) & 0xFF) * scale / 255;
  uint8_t g = (uint16_t)((color >> 8)  & 0xFF) * scale / 255;
  uint8_t b = (uint16_t)((color)       & 0xFF) * scale / 255;
  return strip.Color(r, g, b);
}

// ---------------------------------------------------------------------
//  Effect 1 — startup sweep
//  Lights every pixel one-by-one so you can COUNT the chain and confirm
//  order + color. This is the "light them all, but in sequence" test.
// ---------------------------------------------------------------------
void startupSweep(uint16_t wait) {
  Serial.println(F("[sweep] white one-by-one"));
  strip.clear();
  for (int i = 0; i < PIXEL_COUNT; i++) {
    strip.setPixelColor(i, C_WHITE);
    strip.show();
    delay(wait);
  }
  delay(300);
}

// ---------------------------------------------------------------------
//  Effect 2 — color wipe
//  Fills the strip with one color from end to end, then blacks it out.
// ---------------------------------------------------------------------
void colorWipe(uint32_t color, uint16_t wait) {
  for (int i = 0; i < PIXEL_COUNT; i++) {
    strip.setPixelColor(i, color);
    strip.show();
    delay(wait);
  }
}

// ---------------------------------------------------------------------
//  Effect 3 — comet / Larson scanner  (the signature PC-RGB look)
//  A bright head races up and down the strip leaving a fading tail.
// ---------------------------------------------------------------------
void comet(uint32_t headColor, int passes, uint16_t wait) {
  const int TAIL = 10;                 // tail length in pixels
  for (int p = 0; p < passes; p++) {
    int dir   = (p % 2 == 0) ? 1 : -1; // alternate direction
    int start = (dir == 1) ? 0 : PIXEL_COUNT - 1;

    for (int step = 0; step < PIXEL_COUNT; step++) {
      int head = start + dir * step;
      strip.clear();
      for (int t = 0; t < TAIL; t++) {
        int pos = head - dir * t;
        if (pos >= 0 && pos < PIXEL_COUNT) {
          uint8_t scale = (uint8_t)(255.0 * (1.0 - (float)t / TAIL));
          strip.setPixelColor(pos, dimColor(headColor, scale));
        }
      }
      strip.show();
      delay(wait);
    }
  }
}

// ---------------------------------------------------------------------
//  Effect 4 — rainbow
//  A full spectrum sits across the strip and slowly slides along it.
// ---------------------------------------------------------------------
void rainbow(uint16_t wait) {
  static uint16_t j = 0;
  for (int i = 0; i < PIXEL_COUNT; i++) {
    strip.setPixelColor(i, Wheel((i + j) & 255));
  }
  strip.show();
  delay(wait);
  j = (j + 1) & 255;
}

// ---------------------------------------------------------------------
//  Effect 5 — theater-chase rainbow
//  Sparkly three-pixel chase with a shifting hue (very "gamer RGB").
// ---------------------------------------------------------------------
void theaterChaseRainbow(int frames, uint16_t wait) {
  for (int j = 0; j < 256; j += 16) {        // cycle hues
    for (int q = 0; q < 3; q++) {            // chase phase
      for (int i = 0; i < PIXEL_COUNT; i += 3) {
        strip.setPixelColor(i + q, Wheel((i + j) & 255));
      }
      strip.show();
      delay(wait);
      for (int i = 0; i < PIXEL_COUNT; i += 3) {
        strip.setPixelColor(i + q, 0);       // erase this phase
      }
    }
  }
}

// ---------------------------------------------------------------------
//  Effect 6 — breathing
//  Whole strip smoothly fades in and out of a single color.
// ---------------------------------------------------------------------
void breathing(uint32_t baseColor, int cycles, uint16_t wait) {
  for (int c = 0; c < cycles; c++) {
    for (int b = 10; b <= 255; b += 5) {     // fade in
      strip.fill(dimColor(baseColor, (uint8_t)b), 0, PIXEL_COUNT);
      strip.show();
      delay(wait);
    }
    for (int b = 255; b >= 10; b -= 5) {     // fade out
      strip.fill(dimColor(baseColor, (uint8_t)b), 0, PIXEL_COUNT);
      strip.show();
      delay(wait);
    }
  }
}

// ---------------------------------------------------------------------
//  Setup
// ---------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println(F("=== LED Test Bench ==="));
  Serial.print  (F("Data pin   : GPIO")); Serial.println(LED_PIN);
  Serial.print  (F("Pixel count: "));      Serial.println(PIXEL_COUNT);
  Serial.println(F("Path: GPIO15 -> 74HCT125 ch1 -> board 1 DIN"));

  pinMode(LED_BUILTIN, OUTPUT);

  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.clear();
  strip.show();                 // start dark

  // Build the palette now that strip exists.
  C_RED     = strip.Color(255,   0,   0);
  C_GREEN   = strip.Color(  0, 255,   0);
  C_BLUE    = strip.Color(  0,   0, 255);
  C_CYAN    = strip.Color(  0, 255, 255);
  C_MAGENTA = strip.Color(255,   0, 255);
  C_YELLOW  = strip.Color(255, 255,   0);
  C_PURPLE  = strip.Color(160,   0, 255);
  C_ORANGE  = strip.Color(255, 110,   0);
  C_WHITE   = strip.Color(255, 255, 255);

  Serial.println(F("Running effects... (open Tools > Serial Monitor)"));
}

// ---------------------------------------------------------------------
//  Main loop — cycle through the demo forever
// ---------------------------------------------------------------------
void loop() {
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));   // heartbeat

  // 1) Confirm the whole chain, in order, one pixel at a time.
  startupSweep(40);

  // 2) Color wipes (sequential fill).
  Serial.println(F("[wipe] red"));
  colorWipe(C_RED, 20);
  delay(200); colorWipe(0, 20);
  Serial.println(F("[wipe] green"));
  colorWipe(C_GREEN, 20);
  delay(200); colorWipe(0, 20);
  Serial.println(F("[wipe] blue"));
  colorWipe(C_BLUE, 20);
  delay(200); colorWipe(0, 20);

  // 3) Comet scanner in a few colors (the PC-RGB hero effect).
  Serial.println(F("[comet] cyan"));
  comet(C_CYAN, 4, 18);
  Serial.println(F("[comet] magenta"));
  comet(C_MAGENTA, 4, 18);
  Serial.println(F("[comet] orange"));
  comet(C_ORANGE, 4, 18);

  // 4) Rainbow slide.
  Serial.println(F("[rainbow]"));
  for (int k = 0; k < 256; k++) rainbow(12);

  // 5) Sparkly rainbow chase.
  Serial.println(F("[theater rainbow]"));
  theaterChaseRainbow(1, 60);

  // 6) Breathing pulse.
  Serial.println(F("[breathe] purple"));
  breathing(C_PURPLE, 3, 8);
  Serial.println(F("[breathe] white"));
  breathing(C_WHITE, 3, 8);

  Serial.println(F("--- loop ---"));
}
