// =====================================================================
//  robot_face.h — animated Jamdani Guide face for a 480x320 TFT.
//
//  Motion architecture for an 8-bit AVR + parallel TFT:
//    - eye housings are painted once and never erased during animation;
//    - gaze updates only the union of old/new pupil footprints;
//    - blink lids advance as paired scanline bands from both directions;
//    - motion is fixed-rate and eased, with no random per-frame movement.
// =====================================================================
#pragma once

#include <Adafruit_GFX.h>
#include "display_config.h"

class RobotFace {
 public:
  explicit RobotFace(Adafruit_GFX& gfx) : tft_(gfx) {}

  void drawFull(uint8_t emotion, uint8_t language) {
    emotion_ = emotion;
    (void)language;  // retained for robot-firmware API compatibility

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
    lastVisibleHalf_ = fullApertureHalfHeight();
    lastDrawnX_ = 0;
    lastDrawnY_ = 0;

    tft_.fillScreen(C_BG);
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

    // Freeze attention while blinking. This prevents the eye from emerging at
    // a suddenly different position and makes every gesture readable.
    if (blinkWasActive || blinkIsActive) {
      gazePhaseAt_ += frameElapsed;
    } else {
      updateGaze(now);
    }

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
  Adafruit_GFX& tft_;
  uint8_t emotion_ = EMOTION_HAPPY;

  // RGB565 palette inspired by indigo museum textiles and Jamdani accents.
  static const uint16_t C_BG      = 0x00C5;  // #071B2A
  static const uint16_t C_PANEL   = 0x1167;  // #102F3B
  static const uint16_t C_MINT    = 0xBF7C;  // #BDEFE7
  static const uint16_t C_CYAN    = 0x2E98;  // #2AD3C3
  static const uint16_t C_PUPIL   = 0x11C9;  // #12384A
  static const uint16_t C_IVORY   = 0xEFBE;  // #EDF6F4
  static const uint16_t C_GOLD    = 0xF5C8;  // #F4B942
  static const uint16_t C_CORAL   = 0xF36B;  // #F06F5D
  static const uint16_t C_ALERT   = 0xFD84;  // #FFB020

  static const int16_t HOUSING_W = 132;
  static const int16_t HOUSING_H = 94;
  static const int16_t APERTURE_HALF_W = 53;
  static const int16_t APERTURE_BEVEL = 12;

  // Small dirty region: the pupil moves 12 px over ~15 frames.
  static const int8_t GAZE_X = 12;
  static const int8_t GAZE_Y = -2;
  static const int8_t PUPIL_HALF_W = 17;
  static const int8_t PUPIL_HALF_H = 23;

  // 30 FPS is attainable because each gaze frame redraws only pupil pixels.
  static const uint16_t FRAME_MS = 33;

  // A calm robot blink: eased close, readable hold, softer reopening.
  static const uint16_t BLINK_CLOSE_MS = 200;
  static const uint16_t BLINK_HOLD_MS  = 65;
  static const uint16_t BLINK_OPEN_MS  = 280;
  static const uint16_t FIRST_BLINK_DELAY_MS = 3400;
  static const uint16_t BLINK_INTERVAL_MS = 5600;

  // Attention choreography: center, glance, observe, and return.
  static const uint16_t GAZE_CENTER_MS = 3100;
  static const uint16_t GAZE_MOVE_OUT_MS = 500;
  static const uint16_t GAZE_SIDE_MS = 1050;
  static const uint16_t GAZE_MOVE_HOME_MS = 560;

  enum BlinkPhase : uint8_t {
    BLINK_OPEN,
    BLINK_CLOSING,
    BLINK_HOLD,
    BLINK_OPENING,
  };

  enum GazePhase : uint8_t {
    GAZE_CENTER_HOLD,
    GAZE_MOVE_OUT,
    GAZE_SIDE_HOLD,
    GAZE_MOVE_HOME,
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

  int16_t fullApertureHalfHeight() const {
    switch (emotion_) {
      case EMOTION_HAPPY:  return 31;
      case EMOTION_ALERT:  return 39;
      case EMOTION_SLEEPY: return 17;
      default:             return 33;
    }
  }

  int16_t visibleApertureHalfHeight() const {
    return (static_cast<uint16_t>(fullApertureHalfHeight()) * eyeOpen_) / 255;
  }

  int16_t apertureHalfWidthAtY(int16_t y) const {
    const int16_t distance = abs(y - EYE_CY);
    const int16_t halfHeight = fullApertureHalfHeight();
    if (distance > halfHeight) return -1;

    const int16_t edgeDepth = halfHeight - distance;
    if (edgeDepth >= APERTURE_BEVEL) return APERTURE_HALF_W;
    return APERTURE_HALF_W - 2 * (APERTURE_BEVEL - edgeDepth);
  }

  void drawEyeHousing(int16_t cx) {
    const uint16_t frame = (emotion_ == EMOTION_ALERT) ? C_ALERT : C_CYAN;
    const int16_t x = cx - HOUSING_W / 2;
    const int16_t y = EYE_CY - HOUSING_H / 2;

    tft_.fillRoundRect(x, y, HOUSING_W, HOUSING_H, 31, C_PANEL);
    tft_.drawRoundRect(x, y, HOUSING_W, HOUSING_H, 31, frame);
    tft_.drawRoundRect(x + 2, y + 2, HOUSING_W - 4, HOUSING_H - 4, 29, frame);

    if (emotion_ == EMOTION_ALERT) {
      tft_.drawLine(cx - 42, y - 4, cx - 8, y - 10, C_GOLD);
      tft_.drawLine(cx + 8, y - 10, cx + 42, y - 4, C_GOLD);
    } else if (emotion_ == EMOTION_SLEEPY) {
      tft_.drawFastHLine(cx - 28, y - 5, 56, C_CYAN);
    }
  }

  void drawOpenEyeContents() {
    const int16_t halfHeight = fullApertureHalfHeight();
    for (int16_t y = EYE_CY - halfHeight; y <= EYE_CY + halfHeight; ++y) {
      drawApertureBaseLine(EYE_LEFT_CX, y);
      drawApertureBaseLine(EYE_RIGHT_CX, y);
    }
    drawPupils(gazeX_, gazeY_);
  }

  void drawApertureBaseLine(int16_t cx, int16_t y) {
    const int16_t halfWidth = apertureHalfWidthAtY(y);
    if (halfWidth >= 0) {
      tft_.drawFastHLine(cx - halfWidth, y, halfWidth * 2 + 1, C_MINT);
    }
  }

  void coverApertureLine(int16_t cx, int16_t y) {
    const int16_t halfWidth = apertureHalfWidthAtY(y);
    if (halfWidth >= 0) {
      tft_.drawFastHLine(cx - halfWidth, y, halfWidth * 2 + 1, C_PANEL);
    }
  }

  bool pupilBoundsAtY(int16_t y, int8_t gx, int8_t gy,
                      int16_t& left, int16_t& right) const {
    const int16_t relativeY = abs(y - (EYE_CY + gy));
    if (relativeY > PUPIL_HALF_H) return false;

    // Parabolic vertical capsule: smooth enough for the small pupil and much
    // cheaper than floating-point ellipse maths on the AVR.
    const int32_t hh = static_cast<int32_t>(PUPIL_HALF_H) * PUPIL_HALF_H;
    const int32_t remaining = hh - static_cast<int32_t>(relativeY) * relativeY;
    const int16_t halfWidth = 3 +
      (static_cast<int32_t>(PUPIL_HALF_W - 3) * remaining) / hh;
    left = gx - halfWidth;
    right = gx + halfWidth;
    return true;
  }

  void drawPupilLine(int16_t cx, int16_t y, int8_t gx, int8_t gy) {
    int16_t left, right;
    if (!pupilBoundsAtY(y, gx, gy, left, right)) return;

    tft_.drawFastHLine(cx + left, y, right - left + 1, C_PUPIL);

    // Small woven-diamond glint, clipped naturally one scanline at a time.
    const int16_t glintX = cx + gx - 6;
    const int16_t glintY = EYE_CY + gy - 8;
    const int16_t glintDistance = abs(y - glintY);
    if (glintDistance <= 3) {
      const int16_t glintHalf = 3 - glintDistance;
      tft_.drawFastHLine(glintX - glintHalf, y,
                         glintHalf * 2 + 1, C_IVORY);
    }
  }

  void drawPupils(int8_t gx, int8_t gy) {
    for (int16_t y = EYE_CY + gy - PUPIL_HALF_H;
         y <= EYE_CY + gy + PUPIL_HALF_H; ++y) {
      drawPupilLine(EYE_LEFT_CX, y, gx, gy);
      drawPupilLine(EYE_RIGHT_CX, y, gx, gy);
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
      // Close from the outer aperture edges inward. Each pair of scanlines is
      // applied to both eyes before the next pair, keeping them synchronized.
      for (int16_t distance = oldHalf; distance > newHalf; --distance) {
        coverApertureLine(EYE_LEFT_CX, EYE_CY - distance);
        coverApertureLine(EYE_RIGHT_CX, EYE_CY - distance);
        coverApertureLine(EYE_LEFT_CX, EYE_CY + distance);
        coverApertureLine(EYE_RIGHT_CX, EYE_CY + distance);
      }

      if (newHalf == 0) {
        coverApertureLine(EYE_LEFT_CX, EYE_CY);
        coverApertureLine(EYE_RIGHT_CX, EYE_CY);
        tft_.drawFastHLine(EYE_LEFT_CX - 30, EYE_CY, 61, C_CYAN);
        tft_.drawFastHLine(EYE_RIGHT_CX - 30, EYE_CY, 61, C_CYAN);
      }
      return;
    }

    // Open by restoring only newly exposed scanlines and their pupil slices.
    if (oldHalf == 0 && newHalf > 0) restoreOpenEyeLine(EYE_CY);
    for (int16_t distance = oldHalf + 1; distance <= newHalf; ++distance) {
      restoreOpenEyeLine(EYE_CY - distance);
      restoreOpenEyeLine(EYE_CY + distance);
    }
  }

  void renderGazeDelta(int8_t oldX, int8_t oldY,
                       int8_t newX, int8_t newY) {
    const int16_t firstY = min(EYE_CY + oldY - PUPIL_HALF_H,
                               EYE_CY + newY - PUPIL_HALF_H);
    const int16_t lastY = max(EYE_CY + oldY + PUPIL_HALF_H,
                              EYE_CY + newY + PUPIL_HALF_H);

    // Erase only the horizontal union of the old and new pupil footprint.
    for (int16_t y = firstY; y <= lastY; ++y) {
      int16_t oldLeft, oldRight, newLeft, newRight;
      const bool hasOld = pupilBoundsAtY(y, oldX, oldY, oldLeft, oldRight);
      const bool hasNew = pupilBoundsAtY(y, newX, newY, newLeft, newRight);
      if (!hasOld && !hasNew) continue;

      int16_t left = hasOld ? oldLeft : newLeft;
      int16_t right = hasOld ? oldRight : newRight;
      if (hasNew) {
        left = min(left, newLeft);
        right = max(right, newRight);
      }

      tft_.drawFastHLine(EYE_LEFT_CX + left, y, right - left + 1, C_MINT);
      tft_.drawFastHLine(EYE_RIGHT_CX + left, y, right - left + 1, C_MINT);
    }

    drawPupils(newX, newY);
  }

  void drawForeheadMark() {
    const int16_t cx = TFT_W / 2;
    const int16_t cy = 27;
    tft_.drawLine(cx, cy - 13, cx + 13, cy, C_GOLD);
    tft_.drawLine(cx + 13, cy, cx, cy + 13, C_GOLD);
    tft_.drawLine(cx, cy + 13, cx - 13, cy, C_GOLD);
    tft_.drawLine(cx - 13, cy, cx, cy - 13, C_GOLD);
    tft_.drawLine(cx, cy - 7, cx + 7, cy, C_CYAN);
    tft_.drawLine(cx + 7, cy, cx, cy + 7, C_CYAN);
    tft_.drawLine(cx, cy + 7, cx - 7, cy, C_CYAN);
    tft_.drawLine(cx - 7, cy, cx, cy - 7, C_CYAN);
    tft_.fillCircle(cx, cy, 2, C_IVORY);
  }

  void drawCheeks() {
    tft_.fillRoundRect(54, 187, 30, 10, 5, C_CORAL);
    tft_.fillRoundRect(TFT_W - 84, 187, 30, 10, 5, C_CORAL);
  }

  void drawMouth() {
    const int16_t cx = TFT_W / 2;

    if (emotion_ == EMOTION_ALERT) {
      tft_.fillRoundRect(cx - 13, MOUTH_Y - 13, 26, 27, 12, C_CORAL);
      tft_.fillRoundRect(cx - 7, MOUTH_Y - 7, 14, 15, 7, C_BG);
      return;
    }

    if (emotion_ == EMOTION_SLEEPY) {
      tft_.fillRoundRect(cx - 27, MOUTH_Y - 2, 54, 5, 2, C_CYAN);
      return;
    }

    if (emotion_ == EMOTION_NEUTRAL) {
      tft_.fillRoundRect(cx - 30, MOUTH_Y - 3, 60, 6, 3, C_CYAN);
      return;
    }

    int16_t previousX = cx - 38;
    int16_t previousY = MOUTH_Y - 8;
    for (int16_t x = -34; x <= 38; x += 4) {
      const int16_t y = MOUTH_Y + 10 -
        (static_cast<int32_t>(x) * x * 18L) / (38L * 38L);
      tft_.drawLine(previousX, previousY, cx + x, y, C_CORAL);
      tft_.drawLine(previousX, previousY + 1, cx + x, y + 1, C_CORAL);
      tft_.drawLine(previousX, previousY + 2, cx + x, y + 2, C_CORAL);
      previousX = cx + x;
      previousY = y;
    }
  }

};
