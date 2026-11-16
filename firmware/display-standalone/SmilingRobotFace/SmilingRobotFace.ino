/*
  SmilingRobotFace.ino

  A smooth animated smiling face for:
    - Arduino Mega 2560
    - 480x320 ILI9481 TFT shield
    - 16-bit MCUFRIEND-style parallel bus

  This sketch is intentionally self-contained. It needs no extra libraries.

  Display initialization and Mega shield port mapping are adapted from
  MCUFRIEND_kbv by David Prentice (FreeBSD License):
  https://github.com/prenticedavid/MCUFRIEND_kbv
  See THIRD_PARTY_LICENSES.md for the full notice.
*/

#include <Arduino.h>

#if !defined(__AVR_ATmega2560__)
#error "This sketch requires an Arduino Mega 2560."
#endif

// -----------------------------------------------------------------------------
// Minimal ILI9481 driver for the 16-bit Mega shield
// -----------------------------------------------------------------------------
class MegaIli9481 {
 public:
  static const int16_t WIDTH = 480;
  static const int16_t HEIGHT = 320;

  void begin() {
    // LCD D15..D8 = PORTA, D7..D0 = PORTC.
    DDRA = 0xFF;
    DDRC = 0xFF;

    // RD=PL6/D43, WR=PG2/D39, RS=PD7/D38, CS=PG1/D40, RST=PG0/D41.
    DDRL |= _BV(PL6);
    DDRG |= _BV(PG2) | _BV(PG1) | _BV(PG0);
    DDRD |= _BV(PD7);

    rdHigh();
    wrHigh();
    rsHigh();
    csHigh();

    resetHigh();
    delay(5);
    resetLow();
    delay(20);
    resetHigh();
    delay(150);

    command(0x01);  // Software reset
    delay(150);
    command(0x28);  // Display off while configuring

    static const uint8_t pixelFormat[] = {0x55};
    writeRegister(0x3A, pixelFormat, sizeof(pixelFormat));  // RGB565

    // Physically verified ILI9481 initialization from MCUFRIEND_kbv.
    static const uint8_t b0[] = {0x00};
    static const uint8_t b3[] = {0x02, 0x00, 0x00, 0x00};
    static const uint8_t b4[] = {0x00};
    static const uint8_t d0[] = {0x07, 0x42, 0x18};
    static const uint8_t d1[] = {0x00, 0x07, 0x10};
    static const uint8_t d2[] = {0x01, 0x02};
    static const uint8_t d3[] = {0x01, 0x02};
    static const uint8_t d4[] = {0x01, 0x02};
    static const uint8_t c0[] = {0x12, 0x3B, 0x00, 0x02, 0x11};
    static const uint8_t c1[] = {0x10, 0x10, 0x88};
    static const uint8_t c5[] = {0x03};
    static const uint8_t c6[] = {0x02};
    static const uint8_t c8[] = {
      0x00, 0x32, 0x36, 0x45, 0x06, 0x16,
      0x37, 0x75, 0x77, 0x54, 0x0C, 0x00
    };
    static const uint8_t cc[] = {0x00};

    writeRegister(0xB0, b0, sizeof(b0));
    writeRegister(0xB3, b3, sizeof(b3));
    writeRegister(0xB4, b4, sizeof(b4));
    writeRegister(0xD0, d0, sizeof(d0));
    writeRegister(0xD1, d1, sizeof(d1));
    writeRegister(0xD2, d2, sizeof(d2));
    writeRegister(0xD3, d3, sizeof(d3));
    writeRegister(0xD4, d4, sizeof(d4));
    writeRegister(0xC0, c0, sizeof(c0));
    writeRegister(0xC1, c1, sizeof(c1));
    writeRegister(0xC5, c5, sizeof(c5));
    writeRegister(0xC6, c6, sizeof(c6));
    writeRegister(0xC8, c8, sizeof(c8));
    writeRegister(0xCC, cc, sizeof(cc));

    static const uint8_t landscape[] = {0x28};
    writeRegister(0x36, landscape, sizeof(landscape));

    command(0x11);  // Sleep out
    delay(150);
    command(0x29);  // Display on
    delay(20);
  }

  void fillScreen(uint16_t color) {
    setAddressWindow(0, 0, WIDTH - 1, HEIGHT - 1);
    beginPixels();
    uint32_t pixels = static_cast<uint32_t>(WIDTH) * HEIGHT;
    while (pixels--) writePixel(color);
    endPixels();
  }

  void drawPixel(int16_t x, int16_t y, uint16_t color) {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return;
    setAddressWindow(x, y, x, y);
    beginPixels();
    writePixel(color);
    endPixels();
  }

  void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
    if (y < 0 || y >= HEIGHT || w <= 0 || x >= WIDTH || x + w <= 0) return;
    if (x < 0) {
      w += x;
      x = 0;
    }
    if (x + w > WIDTH) w = WIDTH - x;

    setAddressWindow(x, y, x + w - 1, y);
    beginPixels();
    while (w--) writePixel(color);
    endPixels();
  }

  void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
    if (x < 0 || x >= WIDTH || h <= 0 || y >= HEIGHT || y + h <= 0) return;
    if (y < 0) {
      h += y;
      y = 0;
    }
    if (y + h > HEIGHT) h = HEIGHT - y;

    setAddressWindow(x, y, x, y + h - 1);
    beginPixels();
    while (h--) writePixel(color);
    endPixels();
  }

  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (w <= 0 || h <= 0 || x >= WIDTH || y >= HEIGHT || x + w <= 0 || y + h <= 0) return;
    if (x < 0) {
      w += x;
      x = 0;
    }
    if (y < 0) {
      h += y;
      y = 0;
    }
    if (x + w > WIDTH) w = WIDTH - x;
    if (y + h > HEIGHT) h = HEIGHT - y;

    setAddressWindow(x, y, x + w - 1, y + h - 1);
    beginPixels();
    uint32_t pixels = static_cast<uint32_t>(w) * h;
    while (pixels--) writePixel(color);
    endPixels();
  }

  void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    const bool steep = abs(y1 - y0) > abs(x1 - x0);
    if (steep) {
      swapInt(x0, y0);
      swapInt(x1, y1);
    }
    if (x0 > x1) {
      swapInt(x0, x1);
      swapInt(y0, y1);
    }

    const int16_t dx = x1 - x0;
    const int16_t dy = abs(y1 - y0);
    int16_t error = dx / 2;
    const int16_t yStep = (y0 < y1) ? 1 : -1;

    for (; x0 <= x1; ++x0) {
      if (steep) drawPixel(y0, x0, color);
      else drawPixel(x0, y0, color);
      error -= dy;
      if (error < 0) {
        y0 += yStep;
        error += dx;
      }
    }
  }

  void fillCircle(int16_t x0, int16_t y0, int16_t radius, uint16_t color) {
    drawFastVLine(x0, y0 - radius, radius * 2 + 1, color);
    int16_t f = 1 - radius;
    int16_t ddFx = 1;
    int16_t ddFy = -2 * radius;
    int16_t x = 0;
    int16_t y = radius;

    while (x < y) {
      if (f >= 0) {
        --y;
        ddFy += 2;
        f += ddFy;
      }
      ++x;
      ddFx += 2;
      f += ddFx;
      drawFastVLine(x0 + x, y0 - y, y * 2 + 1, color);
      drawFastVLine(x0 - x, y0 - y, y * 2 + 1, color);
      drawFastVLine(x0 + y, y0 - x, x * 2 + 1, color);
      drawFastVLine(x0 - y, y0 - x, x * 2 + 1, color);
    }
  }

  void fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h,
                     int16_t radius, uint16_t color) {
    radius = min(radius, min(w, h) / 2);
    fillRect(x + radius, y, w - radius * 2, h, color);

    for (int16_t row = 0; row < radius; ++row) {
      const int32_t dy = radius - row - 1;
      const int32_t inside = static_cast<int32_t>(radius) * radius - dy * dy;
      const int16_t inset = radius - integerSqrt(inside);
      drawFastHLine(x + inset, y + row, w - inset * 2, color);
      drawFastHLine(x + inset, y + h - row - 1, w - inset * 2, color);
    }
  }

  void drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h,
                     int16_t radius, uint16_t color) {
    radius = min(radius, min(w, h) / 2);
    drawFastHLine(x + radius, y, w - radius * 2, color);
    drawFastHLine(x + radius, y + h - 1, w - radius * 2, color);
    drawFastVLine(x, y + radius, h - radius * 2, color);
    drawFastVLine(x + w - 1, y + radius, h - radius * 2, color);

    for (int16_t row = 0; row < radius; ++row) {
      const int32_t dy = radius - row - 1;
      const int32_t inside = static_cast<int32_t>(radius) * radius - dy * dy;
      const int16_t inset = radius - integerSqrt(inside);
      drawPixel(x + inset, y + row, color);
      drawPixel(x + w - inset - 1, y + row, color);
      drawPixel(x + inset, y + h - row - 1, color);
      drawPixel(x + w - inset - 1, y + h - row - 1, color);
    }
  }

 private:
  static void swapInt(int16_t& a, int16_t& b) {
    const int16_t temporary = a;
    a = b;
    b = temporary;
  }

  static int16_t integerSqrt(int32_t value) {
    int32_t root = 0;
    int32_t bit = 1L << 30;
    while (bit > value) bit >>= 2;
    while (bit != 0) {
      if (value >= root + bit) {
        value -= root + bit;
        root = (root >> 1) + bit;
      } else {
        root >>= 1;
      }
      bit >>= 2;
    }
    return static_cast<int16_t>(root);
  }

  static inline void rdHigh() { PORTL |= _BV(PL6); }
  static inline void wrHigh() { PORTG |= _BV(PG2); }
  static inline void wrLow() { PORTG &= ~_BV(PG2); }
  static inline void rsHigh() { PORTD |= _BV(PD7); }
  static inline void rsLow() { PORTD &= ~_BV(PD7); }
  static inline void csHigh() { PORTG |= _BV(PG1); }
  static inline void csLow() { PORTG &= ~_BV(PG1); }
  static inline void resetHigh() { PORTG |= _BV(PG0); }
  static inline void resetLow() { PORTG &= ~_BV(PG0); }

  static inline void writeBus(uint16_t value) {
    PORTA = static_cast<uint8_t>(value >> 8);
    PORTC = static_cast<uint8_t>(value);
    wrLow();
    wrHigh();
  }

  void command(uint8_t value) {
    csLow();
    rsLow();
    writeBus(value);
    csHigh();
  }

  void writeRegister(uint8_t address, const uint8_t* data, uint8_t count) {
    csLow();
    rsLow();
    writeBus(address);
    rsHigh();
    while (count--) writeBus(*data++);
    csHigh();
  }

  void setAddressWindow(int16_t x0, int16_t y0, int16_t x1, int16_t y1) {
    const uint8_t columns[] = {
      static_cast<uint8_t>(x0 >> 8), static_cast<uint8_t>(x0),
      static_cast<uint8_t>(x1 >> 8), static_cast<uint8_t>(x1)
    };
    const uint8_t rows[] = {
      static_cast<uint8_t>(y0 >> 8), static_cast<uint8_t>(y0),
      static_cast<uint8_t>(y1 >> 8), static_cast<uint8_t>(y1)
    };
    writeRegister(0x2A, columns, sizeof(columns));
    writeRegister(0x2B, rows, sizeof(rows));
  }

  void beginPixels() {
    csLow();
    rsLow();
    writeBus(0x2C);
    rsHigh();
  }

  static inline void writePixel(uint16_t color) { writeBus(color); }
  static inline void endPixels() { csHigh(); }
};

// -----------------------------------------------------------------------------
// Locked smiling-face renderer
// -----------------------------------------------------------------------------
class SmilingFace {
 public:
  explicit SmilingFace(MegaIli9481& display) : display_(display) {}

  void begin() {
    const uint32_t now = millis();
    blinkPhase_ = BLINK_OPEN;
    blinkPhaseAt_ = now;
    nextBlinkAt_ = now + FIRST_BLINK_DELAY_MS;
    eyeOpen_ = 255;

    gazePhase_ = GAZE_CENTER_HOLD;
    gazePhaseAt_ = now;
    gazeSide_ = 1;
    gazeX_ = 0;
    gazeY_ = 0;

    lastFrameAt_ = now;
    lastVisibleHalf_ = APERTURE_HALF_H;
    lastDrawnX_ = 0;
    lastDrawnY_ = 0;

    display_.fillScreen(C_BG);
    drawForeheadMark();
    drawCheeks();
    drawMouth();
    drawEyeHousing(EYE_LEFT_CX);
    drawEyeHousing(EYE_RIGHT_CX);
    drawOpenEyeContents();
  }

  void tick() {
    const uint32_t now = millis();
    const uint32_t frameElapsed = now - lastFrameAt_;
    if (frameElapsed < FRAME_MS) return;
    lastFrameAt_ = now;

    const bool blinkWasActive = (blinkPhase_ != BLINK_OPEN);
    updateBlink(now);
    const bool blinkIsActive = (blinkPhase_ != BLINK_OPEN);

    // Freeze gaze during a blink so the pupils do not jump while reopening.
    if (blinkWasActive || blinkIsActive) gazePhaseAt_ += frameElapsed;
    else updateGaze(now);

    const int16_t visibleHalf = visibleApertureHalfHeight();
    if (visibleHalf != lastVisibleHalf_) {
      renderBlinkDelta(lastVisibleHalf_, visibleHalf);
      lastVisibleHalf_ = visibleHalf;
    } else if (!blinkIsActive &&
               (gazeX_ != lastDrawnX_ || gazeY_ != lastDrawnY_)) {
      renderGazeDelta(lastDrawnX_, lastDrawnY_, gazeX_, gazeY_);
      lastDrawnX_ = gazeX_;
      lastDrawnY_ = gazeY_;
    }
  }

 private:
  MegaIli9481& display_;

  static const uint16_t C_BG    = 0x00C5;
  static const uint16_t C_PANEL = 0x1167;
  static const uint16_t C_MINT  = 0xBF7C;
  static const uint16_t C_CYAN  = 0x2E98;
  static const uint16_t C_PUPIL = 0x11C9;
  static const uint16_t C_IVORY = 0xEFBE;
  static const uint16_t C_GOLD  = 0xF5C8;
  static const uint16_t C_CORAL = 0xF36B;

  static const int16_t EYE_CY = 126;
  static const int16_t EYE_LEFT_CX = 142;
  static const int16_t EYE_RIGHT_CX = MegaIli9481::WIDTH - EYE_LEFT_CX;
  static const int16_t MOUTH_Y = 228;
  static const int16_t HOUSING_W = 132;
  static const int16_t HOUSING_H = 94;
  static const int16_t APERTURE_HALF_W = 53;
  static const int16_t APERTURE_HALF_H = 31;
  static const int16_t APERTURE_BEVEL = 12;

  static const int8_t GAZE_X = 12;
  static const int8_t GAZE_Y = -2;
  static const int8_t PUPIL_HALF_W = 17;
  static const int8_t PUPIL_HALF_H = 23;

  static const uint16_t FRAME_MS = 33;
  static const uint16_t BLINK_CLOSE_MS = 200;
  static const uint16_t BLINK_HOLD_MS = 65;
  static const uint16_t BLINK_OPEN_MS = 280;
  static const uint16_t FIRST_BLINK_DELAY_MS = 3400;
  static const uint16_t BLINK_INTERVAL_MS = 5600;

  static const uint16_t GAZE_CENTER_MS = 3100;
  static const uint16_t GAZE_MOVE_OUT_MS = 500;
  static const uint16_t GAZE_SIDE_MS = 1050;
  static const uint16_t GAZE_MOVE_HOME_MS = 560;

  enum BlinkPhase : uint8_t {
    BLINK_OPEN,
    BLINK_CLOSING,
    BLINK_HOLD,
    BLINK_OPENING
  };

  enum GazePhase : uint8_t {
    GAZE_CENTER_HOLD,
    GAZE_MOVE_OUT,
    GAZE_SIDE_HOLD,
    GAZE_MOVE_HOME
  };

  BlinkPhase blinkPhase_ = BLINK_OPEN;
  GazePhase gazePhase_ = GAZE_CENTER_HOLD;
  uint32_t blinkPhaseAt_ = 0;
  uint32_t nextBlinkAt_ = 0;
  uint32_t gazePhaseAt_ = 0;
  uint32_t lastFrameAt_ = 0;

  uint8_t eyeOpen_ = 255;
  int8_t gazeSide_ = 1;
  int8_t gazeX_ = 0;
  int8_t gazeY_ = 0;
  int8_t lastDrawnX_ = 0;
  int8_t lastDrawnY_ = 0;
  int16_t lastVisibleHalf_ = 0;

  static uint8_t progress8(uint32_t elapsed, uint16_t duration) {
    if (elapsed >= duration) return 255;
    return static_cast<uint8_t>((elapsed * 255UL) / duration);
  }

  // Integer smoothstep: 3t^2 - 2t^3.
  static uint8_t ease8(uint8_t t) {
    const uint32_t x = t;
    return static_cast<uint8_t>((x * x * (765UL - 2UL * x)) / 65025UL);
  }

  static int8_t lerp8(int8_t from, int8_t to, uint8_t amount) {
    return static_cast<int8_t>(from +
      (static_cast<int16_t>(to - from) * amount) / 255);
  }

  void updateBlink(uint32_t now) {
    switch (blinkPhase_) {
      case BLINK_OPEN:
        eyeOpen_ = 255;
        if (static_cast<int32_t>(now - nextBlinkAt_) >= 0) {
          blinkPhase_ = BLINK_CLOSING;
          blinkPhaseAt_ = now;
        }
        break;

      case BLINK_CLOSING: {
        const uint32_t elapsed = now - blinkPhaseAt_;
        if (elapsed >= BLINK_CLOSE_MS) {
          eyeOpen_ = 0;
          blinkPhase_ = BLINK_HOLD;
          blinkPhaseAt_ = now;
        } else {
          eyeOpen_ = 255 - ease8(progress8(elapsed, BLINK_CLOSE_MS));
        }
        break;
      }

      case BLINK_HOLD:
        eyeOpen_ = 0;
        if (now - blinkPhaseAt_ >= BLINK_HOLD_MS) {
          blinkPhase_ = BLINK_OPENING;
          blinkPhaseAt_ = now;
        }
        break;

      case BLINK_OPENING: {
        const uint32_t elapsed = now - blinkPhaseAt_;
        if (elapsed >= BLINK_OPEN_MS) {
          eyeOpen_ = 255;
          blinkPhase_ = BLINK_OPEN;
          blinkPhaseAt_ = now;
          nextBlinkAt_ = now + BLINK_INTERVAL_MS;
        } else {
          eyeOpen_ = ease8(progress8(elapsed, BLINK_OPEN_MS));
        }
        break;
      }
    }
  }

  void updateGaze(uint32_t now) {
    const uint32_t elapsed = now - gazePhaseAt_;

    switch (gazePhase_) {
      case GAZE_CENTER_HOLD:
        gazeX_ = 0;
        gazeY_ = 0;
        if (elapsed >= GAZE_CENTER_MS) {
          gazePhase_ = GAZE_MOVE_OUT;
          gazePhaseAt_ = now;
        }
        break;

      case GAZE_MOVE_OUT: {
        const uint8_t amount = ease8(progress8(elapsed, GAZE_MOVE_OUT_MS));
        gazeX_ = lerp8(0, gazeSide_ * GAZE_X, amount);
        gazeY_ = lerp8(0, GAZE_Y, amount);
        if (elapsed >= GAZE_MOVE_OUT_MS) {
          gazePhase_ = GAZE_SIDE_HOLD;
          gazePhaseAt_ = now;
        }
        break;
      }

      case GAZE_SIDE_HOLD:
        gazeX_ = gazeSide_ * GAZE_X;
        gazeY_ = GAZE_Y;
        if (elapsed >= GAZE_SIDE_MS) {
          gazePhase_ = GAZE_MOVE_HOME;
          gazePhaseAt_ = now;
        }
        break;

      case GAZE_MOVE_HOME: {
        const uint8_t amount = ease8(progress8(elapsed, GAZE_MOVE_HOME_MS));
        gazeX_ = lerp8(gazeSide_ * GAZE_X, 0, amount);
        gazeY_ = lerp8(GAZE_Y, 0, amount);
        if (elapsed >= GAZE_MOVE_HOME_MS) {
          gazeX_ = 0;
          gazeY_ = 0;
          gazeSide_ = -gazeSide_;
          gazePhase_ = GAZE_CENTER_HOLD;
          gazePhaseAt_ = now;
        }
        break;
      }
    }
  }

  int16_t visibleApertureHalfHeight() const {
    return (static_cast<uint16_t>(APERTURE_HALF_H) * eyeOpen_) / 255;
  }

  int16_t apertureHalfWidthAtY(int16_t y) const {
    const int16_t distance = abs(y - EYE_CY);
    if (distance > APERTURE_HALF_H) return -1;

    const int16_t edgeDepth = APERTURE_HALF_H - distance;
    if (edgeDepth >= APERTURE_BEVEL) return APERTURE_HALF_W;
    return APERTURE_HALF_W - 2 * (APERTURE_BEVEL - edgeDepth);
  }

  void drawEyeHousing(int16_t centerX) {
    const int16_t x = centerX - HOUSING_W / 2;
    const int16_t y = EYE_CY - HOUSING_H / 2;
    display_.fillRoundRect(x, y, HOUSING_W, HOUSING_H, 31, C_PANEL);
    display_.drawRoundRect(x, y, HOUSING_W, HOUSING_H, 31, C_CYAN);
    display_.drawRoundRect(x + 2, y + 2, HOUSING_W - 4, HOUSING_H - 4, 29, C_CYAN);
  }

  void drawOpenEyeContents() {
    for (int16_t y = EYE_CY - APERTURE_HALF_H;
         y <= EYE_CY + APERTURE_HALF_H; ++y) {
      drawApertureBaseLine(EYE_LEFT_CX, y);
      drawApertureBaseLine(EYE_RIGHT_CX, y);
    }
    drawPupils(gazeX_, gazeY_);
  }

  void drawApertureBaseLine(int16_t centerX, int16_t y) {
    const int16_t halfWidth = apertureHalfWidthAtY(y);
    if (halfWidth >= 0) {
      display_.drawFastHLine(centerX - halfWidth, y, halfWidth * 2 + 1, C_MINT);
    }
  }

  void coverApertureLine(int16_t centerX, int16_t y) {
    const int16_t halfWidth = apertureHalfWidthAtY(y);
    if (halfWidth >= 0) {
      display_.drawFastHLine(centerX - halfWidth, y, halfWidth * 2 + 1, C_PANEL);
    }
  }

  bool pupilBoundsAtY(int16_t y, int8_t gazeX, int8_t gazeY,
                      int16_t& left, int16_t& right) const {
    const int16_t relativeY = abs(y - (EYE_CY + gazeY));
    if (relativeY > PUPIL_HALF_H) return false;

    const int32_t hh = static_cast<int32_t>(PUPIL_HALF_H) * PUPIL_HALF_H;
    const int32_t remaining = hh - static_cast<int32_t>(relativeY) * relativeY;
    const int16_t halfWidth = 3 +
      (static_cast<int32_t>(PUPIL_HALF_W - 3) * remaining) / hh;
    left = gazeX - halfWidth;
    right = gazeX + halfWidth;
    return true;
  }

  void drawPupilLine(int16_t centerX, int16_t y, int8_t gazeX, int8_t gazeY) {
    int16_t left;
    int16_t right;
    if (!pupilBoundsAtY(y, gazeX, gazeY, left, right)) return;

    display_.drawFastHLine(centerX + left, y, right - left + 1, C_PUPIL);

    const int16_t glintX = centerX + gazeX - 6;
    const int16_t glintY = EYE_CY + gazeY - 8;
    const int16_t glintDistance = abs(y - glintY);
    if (glintDistance <= 3) {
      const int16_t glintHalf = 3 - glintDistance;
      display_.drawFastHLine(glintX - glintHalf, y, glintHalf * 2 + 1, C_IVORY);
    }
  }

  void drawPupils(int8_t gazeX, int8_t gazeY) {
    for (int16_t y = EYE_CY + gazeY - PUPIL_HALF_H;
         y <= EYE_CY + gazeY + PUPIL_HALF_H; ++y) {
      drawPupilLine(EYE_LEFT_CX, y, gazeX, gazeY);
      drawPupilLine(EYE_RIGHT_CX, y, gazeX, gazeY);
    }
  }

  void restoreOpenEyeLine(int16_t y) {
    drawApertureBaseLine(EYE_LEFT_CX, y);
    drawApertureBaseLine(EYE_RIGHT_CX, y);
    drawPupilLine(EYE_LEFT_CX, y, gazeX_, gazeY_);
    drawPupilLine(EYE_RIGHT_CX, y, gazeX_, gazeY_);
  }

  void renderBlinkDelta(int16_t oldHalf, int16_t newHalf) {
    if (newHalf < oldHalf) {
      for (int16_t distance = oldHalf; distance > newHalf; --distance) {
        coverApertureLine(EYE_LEFT_CX, EYE_CY - distance);
        coverApertureLine(EYE_RIGHT_CX, EYE_CY - distance);
        coverApertureLine(EYE_LEFT_CX, EYE_CY + distance);
        coverApertureLine(EYE_RIGHT_CX, EYE_CY + distance);
      }

      if (newHalf == 0) {
        coverApertureLine(EYE_LEFT_CX, EYE_CY);
        coverApertureLine(EYE_RIGHT_CX, EYE_CY);
        display_.drawFastHLine(EYE_LEFT_CX - 30, EYE_CY, 61, C_CYAN);
        display_.drawFastHLine(EYE_RIGHT_CX - 30, EYE_CY, 61, C_CYAN);
      }
      return;
    }

    if (oldHalf == 0 && newHalf > 0) restoreOpenEyeLine(EYE_CY);
    for (int16_t distance = oldHalf + 1; distance <= newHalf; ++distance) {
      restoreOpenEyeLine(EYE_CY - distance);
      restoreOpenEyeLine(EYE_CY + distance);
    }
  }

  void renderGazeDelta(int8_t oldX, int8_t oldY, int8_t newX, int8_t newY) {
    const int16_t firstY = min(EYE_CY + oldY - PUPIL_HALF_H,
                               EYE_CY + newY - PUPIL_HALF_H);
    const int16_t lastY = max(EYE_CY + oldY + PUPIL_HALF_H,
                              EYE_CY + newY + PUPIL_HALF_H);

    for (int16_t y = firstY; y <= lastY; ++y) {
      int16_t oldLeft;
      int16_t oldRight;
      int16_t newLeft;
      int16_t newRight;
      const bool hasOld = pupilBoundsAtY(y, oldX, oldY, oldLeft, oldRight);
      const bool hasNew = pupilBoundsAtY(y, newX, newY, newLeft, newRight);
      if (!hasOld && !hasNew) continue;

      int16_t left = hasOld ? oldLeft : newLeft;
      int16_t right = hasOld ? oldRight : newRight;
      if (hasNew) {
        left = min(left, newLeft);
        right = max(right, newRight);
      }

      display_.drawFastHLine(EYE_LEFT_CX + left, y, right - left + 1, C_MINT);
      display_.drawFastHLine(EYE_RIGHT_CX + left, y, right - left + 1, C_MINT);
    }

    drawPupils(newX, newY);
  }

  void drawForeheadMark() {
    const int16_t centerX = MegaIli9481::WIDTH / 2;
    const int16_t centerY = 27;
    display_.drawLine(centerX, centerY - 13, centerX + 13, centerY, C_GOLD);
    display_.drawLine(centerX + 13, centerY, centerX, centerY + 13, C_GOLD);
    display_.drawLine(centerX, centerY + 13, centerX - 13, centerY, C_GOLD);
    display_.drawLine(centerX - 13, centerY, centerX, centerY - 13, C_GOLD);
    display_.drawLine(centerX, centerY - 7, centerX + 7, centerY, C_CYAN);
    display_.drawLine(centerX + 7, centerY, centerX, centerY + 7, C_CYAN);
    display_.drawLine(centerX, centerY + 7, centerX - 7, centerY, C_CYAN);
    display_.drawLine(centerX - 7, centerY, centerX, centerY - 7, C_CYAN);
    display_.fillCircle(centerX, centerY, 2, C_IVORY);
  }

  void drawCheeks() {
    display_.fillRoundRect(54, 187, 30, 10, 5, C_CORAL);
    display_.fillRoundRect(MegaIli9481::WIDTH - 84, 187, 30, 10, 5, C_CORAL);
  }

  void drawMouth() {
    const int16_t centerX = MegaIli9481::WIDTH / 2;
    int16_t previousX = centerX - 38;
    int16_t previousY = MOUTH_Y - 8;

    for (int16_t x = -34; x <= 38; x += 4) {
      const int16_t y = MOUTH_Y + 10 -
        (static_cast<int32_t>(x) * x * 18L) / (38L * 38L);
      display_.drawLine(previousX, previousY, centerX + x, y, C_CORAL);
      display_.drawLine(previousX, previousY + 1, centerX + x, y + 1, C_CORAL);
      display_.drawLine(previousX, previousY + 2, centerX + x, y + 2, C_CORAL);
      previousX = centerX + x;
      previousY = y;
    }
  }
};

MegaIli9481 display;
SmilingFace face(display);

void setup() {
  display.begin();
  face.begin();
}

void loop() {
  face.tick();
}
