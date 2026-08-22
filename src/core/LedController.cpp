#include "LedController.h"

#include <math.h>

namespace {
constexpr char kPrefsNamespace[] = "control_led";
constexpr uint32_t kSaveDebounceMs = 900;
}

LedController::LedController()
    : pixels_(BoardPins::LedCount, BoardPins::LedData, NEO_GRB + NEO_KHZ800) {}

void LedController::begin() {
  pixels_.begin();
  pixels_.setBrightness(255);
  pixels_.clear();
  pixels_.show();
  load();
  renderFrame(millis());
}

void LedController::tick(uint32_t nowMs) {
  if (dirty_ && static_cast<uint32_t>(nowMs - dirtySinceMs_) >= kSaveDebounceMs) {
    save();
  }

  if (static_cast<uint32_t>(nowMs - lastFrameMs_) < frameIntervalMs()) return;
  lastFrameMs_ = nowMs;
  renderFrame(nowMs);
}

void LedController::setEnabled(bool enabled) {
  if (enabled_ == enabled) return;
  enabled_ = enabled;
  markDirty();
  renderFrame(millis());
}

void LedController::setEffect(LedEffect effect) {
  if (static_cast<uint8_t>(effect) >= effectCount()) effect = LedEffect::Solid;
  if (effect_ == effect) return;
  effect_ = effect;
  animationStep_ = 0;
  scannerIndex_ = 0;
  scannerDirection_ = 1;
  markDirty();
  renderFrame(millis());
}

void LedController::setEffect(uint8_t effectIndex) {
  setEffect(static_cast<LedEffect>(effectIndex % effectCount()));
}

void LedController::setColor(uint8_t red, uint8_t green, uint8_t blue) {
  if (red_ == red && green_ == green && blue_ == blue) return;
  red_ = red;
  green_ = green;
  blue_ = blue;
  markDirty();
  renderFrame(millis());
}

void LedController::setBrightness(uint8_t brightness) {
  if (brightness_ == brightness) return;
  brightness_ = brightness;
  markDirty();
  renderFrame(millis());
}

void LedController::setSpeed(uint8_t speed) {
  speed = constrain(speed, 1, 100);
  if (speed_ == speed) return;
  speed_ = speed;
  markDirty();
}

const char* LedController::effectName() const {
  return effectName(effect_);
}

const char* LedController::effectName(LedEffect effect) {
  switch (effect) {
    case LedEffect::Off:
      return "Off";
    case LedEffect::Solid:
      return "Solid";
    case LedEffect::Breathe:
      return "Breathe";
    case LedEffect::Rainbow:
      return "Rainbow";
    case LedEffect::Chase:
      return "Chase";
    case LedEffect::Scanner:
      return "Scanner";
    case LedEffect::Sparkle:
      return "Sparkle";
    case LedEffect::Comet:
      return "Comet";
    case LedEffect::ColorWipe:
      return "Color Wipe";
    case LedEffect::TheaterChase:
      return "Theater Chase";
    case LedEffect::Twinkle:
      return "Twinkle";
    case LedEffect::Meteor:
      return "Meteor";
    case LedEffect::Wave:
      return "Wave";
    case LedEffect::DualScanner:
      return "Dual Scanner";
    case LedEffect::Heartbeat:
      return "Heartbeat";
    case LedEffect::CyberPulse:
      return "Cyber Pulse";
    case LedEffect::Matrix:
      return "Matrix";
    case LedEffect::Fire:
      return "Fire";
    case LedEffect::Ocean:
      return "Ocean";
    case LedEffect::Confetti:
      return "Confetti";
    case LedEffect::Glitch:
      return "Glitch";
    case LedEffect::RandomColor:
      return "Random Color";
    case LedEffect::Aurora:
      return "Aurora";
    case LedEffect::Count:
    default:
      return "Unknown";
  }
}

String LedController::hexColor() const {
  char buffer[8];
  snprintf(buffer, sizeof(buffer), "#%02X%02X%02X", red_, green_, blue_);
  return String(buffer);
}

void LedController::markDirty() {
  dirty_ = true;
  dirtySinceMs_ = millis();
}

void LedController::load() {
  if (!preferences_.begin(kPrefsNamespace, true)) return;
  enabled_ = preferences_.getBool("enabled", enabled_);
  const uint8_t storedEffect = preferences_.getUChar("effect", effectIndex());
  effect_ = storedEffect < effectCount() ? static_cast<LedEffect>(storedEffect) : LedEffect::Solid;
  red_ = preferences_.getUChar("red", red_);
  green_ = preferences_.getUChar("green", green_);
  blue_ = preferences_.getUChar("blue", blue_);
  brightness_ = preferences_.getUChar("bright", brightness_);
  speed_ = constrain(preferences_.getUChar("speed", speed_), 1, 100);
  preferences_.end();
}

void LedController::save() {
  if (!preferences_.begin(kPrefsNamespace, false)) return;
  preferences_.putBool("enabled", enabled_);
  preferences_.putUChar("effect", effectIndex());
  preferences_.putUChar("red", red_);
  preferences_.putUChar("green", green_);
  preferences_.putUChar("blue", blue_);
  preferences_.putUChar("bright", brightness_);
  preferences_.putUChar("speed", speed_);
  preferences_.end();
  dirty_ = false;
}

uint32_t LedController::frameIntervalMs() const {
  const uint32_t slow = 220;
  const uint32_t fast = 18;
  return slow - ((slow - fast) * static_cast<uint32_t>(speed_ - 1) / 99U);
}

uint32_t LedController::scaledColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t level) const {
  const uint16_t scale = static_cast<uint16_t>(brightness_) * level / 255U;
  const uint8_t outR = static_cast<uint16_t>(red) * scale / 255U;
  const uint8_t outG = static_cast<uint16_t>(green) * scale / 255U;
  const uint8_t outB = static_cast<uint16_t>(blue) * scale / 255U;
  return pixels_.Color(outR, outG, outB);
}

void LedController::fill(uint32_t color) {
  for (uint8_t i = 0; i < BoardPins::LedCount; ++i) pixels_.setPixelColor(i, color);
}

void LedController::renderFrame(uint32_t nowMs) {
  pixels_.clear();

  if (!enabled_ || effect_ == LedEffect::Off || brightness_ == 0) {
    pixels_.show();
    return;
  }

  switch (effect_) {
    case LedEffect::Solid: {
      fill(scaledColor(red_, green_, blue_));
      break;
    }

    case LedEffect::Breathe: {
      const float speedFactor = 0.0022f + static_cast<float>(speed_) * 0.000075f;
      const float wave = (sinf(static_cast<float>(nowMs) * speedFactor) + 1.0f) * 0.5f;
      const uint8_t level = static_cast<uint8_t>(20.0f + wave * 235.0f);
      fill(scaledColor(red_, green_, blue_, level));
      break;
    }

    case LedEffect::Rainbow: {
      const uint16_t baseHue = static_cast<uint16_t>((animationStep_ * 700U) & 0xFFFFU);
      for (uint8_t i = 0; i < BoardPins::LedCount; ++i) {
        const uint16_t hue = static_cast<uint16_t>(baseHue + (65535UL * i / BoardPins::LedCount));
        const uint32_t rgb = Adafruit_NeoPixel::ColorHSV(hue, 255, brightness_);
        pixels_.setPixelColor(i, pixels_.gamma32(rgb));
      }
      ++animationStep_;
      break;
    }

    case LedEffect::Chase: {
      const uint8_t head = animationStep_ % BoardPins::LedCount;
      for (uint8_t i = 0; i < BoardPins::LedCount; ++i) {
        const uint8_t distance = static_cast<uint8_t>((i + BoardPins::LedCount - head) % BoardPins::LedCount);
        const uint8_t level = distance == 0 ? 255 : (distance == 1 ? 80 : 12);
        pixels_.setPixelColor(i, scaledColor(red_, green_, blue_, level));
      }
      ++animationStep_;
      break;
    }

    case LedEffect::Scanner: {
      for (uint8_t i = 0; i < BoardPins::LedCount; ++i) {
        const int distance = abs(static_cast<int>(i) - scannerIndex_);
        const uint8_t level = distance == 0 ? 255 : (distance == 1 ? 85 : 8);
        pixels_.setPixelColor(i, scaledColor(red_, green_, blue_, level));
      }
      scannerIndex_ += scannerDirection_;
      if (scannerIndex_ >= BoardPins::LedCount - 1) {
        scannerIndex_ = BoardPins::LedCount - 1;
        scannerDirection_ = -1;
      } else if (scannerIndex_ <= 0) {
        scannerIndex_ = 0;
        scannerDirection_ = 1;
      }
      break;
    }

    case LedEffect::Sparkle: {
      fill(scaledColor(red_, green_, blue_, 10));
      const uint8_t sparkleCount = speed_ > 70 ? 3 : (speed_ > 35 ? 2 : 1);
      for (uint8_t i = 0; i < sparkleCount; ++i) {
        pixels_.setPixelColor(random(BoardPins::LedCount), scaledColor(red_, green_, blue_, 255));
      }
      break;
    }

    case LedEffect::Comet: {
      const uint8_t head = animationStep_ % BoardPins::LedCount;
      for (uint8_t i = 0; i < BoardPins::LedCount; ++i) {
        const uint8_t distance = static_cast<uint8_t>((head + BoardPins::LedCount - i) % BoardPins::LedCount);
        uint8_t level = 0;
        if (distance == 0) level = 255;
        else if (distance == 1) level = 150;
        else if (distance == 2) level = 70;
        else if (distance == 3) level = 25;
        pixels_.setPixelColor(i, scaledColor(red_, green_, blue_, level));
      }
      ++animationStep_;
      break;
    }

    case LedEffect::ColorWipe: {
      const uint8_t head = animationStep_ % (BoardPins::LedCount + 1);
      for (uint8_t i = 0; i < BoardPins::LedCount; ++i) {
        pixels_.setPixelColor(i, i < head ? scaledColor(red_, green_, blue_) : 0);
      }
      animationStep_ = static_cast<uint16_t>((animationStep_ + 1) % ((BoardPins::LedCount + 1) * 2));
      if (animationStep_ > BoardPins::LedCount) {
        const uint8_t clearHead = animationStep_ - BoardPins::LedCount - 1;
        for (uint8_t i = 0; i < BoardPins::LedCount; ++i) {
          pixels_.setPixelColor(i, i <= clearHead ? 0 : scaledColor(red_, green_, blue_));
        }
      }
      break;
    }

    case LedEffect::TheaterChase: {
      const uint8_t offset = animationStep_ % 3;
      for (uint8_t i = 0; i < BoardPins::LedCount; ++i) {
        const uint8_t level = (i % 3 == offset) ? 255 : 10;
        pixels_.setPixelColor(i, scaledColor(red_, green_, blue_, level));
      }
      ++animationStep_;
      break;
    }

    case LedEffect::Twinkle: {
      for (uint8_t i = 0; i < BoardPins::LedCount; ++i) {
        const uint8_t seed = static_cast<uint8_t>((i * 37U + animationStep_ * 11U) & 0xFFU);
        const uint8_t level = seed > 225 ? 255 : (seed > 185 ? 75 : 7);
        pixels_.setPixelColor(i, scaledColor(red_, green_, blue_, level));
      }
      ++animationStep_;
      break;
    }

    case LedEffect::Meteor: {
      const uint8_t head = animationStep_ % (BoardPins::LedCount * 2);
      for (uint8_t i = 0; i < BoardPins::LedCount; ++i) {
        const int virtualHead = head < BoardPins::LedCount ? head : (BoardPins::LedCount * 2 - 1 - head);
        const int distance = abs(static_cast<int>(i) - virtualHead);
        const uint8_t level = distance == 0 ? 255 : (distance == 1 ? 145 : (distance == 2 ? 55 : 6));
        pixels_.setPixelColor(i, scaledColor(red_, green_, blue_, level));
      }
      ++animationStep_;
      break;
    }

    case LedEffect::Wave: {
      for (uint8_t i = 0; i < BoardPins::LedCount; ++i) {
        const float phase = static_cast<float>(animationStep_) * 0.32f + static_cast<float>(i) * 0.78f;
        const float wave = (sinf(phase) + 1.0f) * 0.5f;
        const uint8_t level = static_cast<uint8_t>(15.0f + wave * 240.0f);
        pixels_.setPixelColor(i, scaledColor(red_, green_, blue_, level));
      }
      ++animationStep_;
      break;
    }

    case LedEffect::DualScanner: {
      const uint8_t pos = animationStep_ % BoardPins::LedCount;
      const uint8_t mirror = static_cast<uint8_t>(BoardPins::LedCount - 1 - pos);
      for (uint8_t i = 0; i < BoardPins::LedCount; ++i) {
        const int d1 = abs(static_cast<int>(i) - static_cast<int>(pos));
        const int d2 = abs(static_cast<int>(i) - static_cast<int>(mirror));
        const int distance = d1 < d2 ? d1 : d2;
        const uint8_t level = distance == 0 ? 255 : (distance == 1 ? 72 : 5);
        pixels_.setPixelColor(i, scaledColor(red_, green_, blue_, level));
      }
      ++animationStep_;
      break;
    }

    case LedEffect::Heartbeat: {
      const uint16_t phase = animationStep_ % 28;
      uint8_t level = 10;
      if (phase == 1) level = 210;
      else if (phase == 2) level = 255;
      else if (phase == 3) level = 95;
      else if (phase == 6) level = 175;
      else if (phase == 7) level = 225;
      else if (phase == 8) level = 65;
      fill(scaledColor(red_, green_, blue_, level));
      ++animationStep_;
      break;
    }

    case LedEffect::CyberPulse: {
      const uint8_t head = animationStep_ % BoardPins::LedCount;
      for (uint8_t i = 0; i < BoardPins::LedCount; ++i) {
        const uint8_t distance = static_cast<uint8_t>((i + BoardPins::LedCount - head) % BoardPins::LedCount);
        uint8_t level = 5;
        if (distance == 0) level = 255;
        else if (distance == 1) level = 110;
        else if (distance == 2) level = 38;
        if (((animationStep_ / BoardPins::LedCount) % 6) == 5 && (i % 2 == 0)) level = 180;
        pixels_.setPixelColor(i, scaledColor(red_, green_, blue_, level));
      }
      ++animationStep_;
      break;
    }

    case LedEffect::Matrix: {
      for (uint8_t i = 0; i < BoardPins::LedCount; ++i) {
        const uint8_t n = static_cast<uint8_t>(random(100));
        const uint8_t level = n > 92 ? 255 : (n > 72 ? 70 : 6);
        pixels_.setPixelColor(i, scaledColor(0, 255, 65, level));
      }
      break;
    }

    case LedEffect::Fire: {
      for (uint8_t i = 0; i < BoardPins::LedCount; ++i) {
        const uint8_t heat = static_cast<uint8_t>(150 + random(106));
        const uint8_t green = static_cast<uint8_t>(random(25, 120));
        pixels_.setPixelColor(i, scaledColor(255, green, 0, heat));
      }
      break;
    }

    case LedEffect::Ocean: {
      const uint16_t baseHue = static_cast<uint16_t>(32000U + ((animationStep_ * 240U) & 0x1FFFU));
      for (uint8_t i = 0; i < BoardPins::LedCount; ++i) {
        const uint16_t hue = static_cast<uint16_t>(baseHue + i * 850U);
        const uint8_t value = static_cast<uint8_t>((static_cast<uint16_t>(brightness_) * (140U + ((i * 31U + animationStep_ * 7U) % 116U))) / 255U);
        pixels_.setPixelColor(i, pixels_.gamma32(Adafruit_NeoPixel::ColorHSV(hue, 220, value)));
      }
      ++animationStep_;
      break;
    }

    case LedEffect::Confetti: {
      fill(scaledColor(red_, green_, blue_, 4));
      const uint8_t count = speed_ > 65 ? 3 : 2;
      for (uint8_t i = 0; i < count; ++i) {
        const uint16_t hue = static_cast<uint16_t>(random(65536));
        pixels_.setPixelColor(random(BoardPins::LedCount), pixels_.gamma32(Adafruit_NeoPixel::ColorHSV(hue, 255, brightness_)));
      }
      break;
    }

    case LedEffect::Glitch: {
      const bool burst = (random(100) < 22);
      for (uint8_t i = 0; i < BoardPins::LedCount; ++i) {
        uint8_t level = burst ? static_cast<uint8_t>(random(40, 256)) : static_cast<uint8_t>(random(2, 28));
        if ((i + animationStep_) % 5 == 0) level = burst ? 255 : 65;
        pixels_.setPixelColor(i, scaledColor(red_, green_, blue_, level));
      }
      ++animationStep_;
      break;
    }

    case LedEffect::RandomColor: {
      const uint16_t hue = static_cast<uint16_t>((animationStep_ * 4096U) & 0xFFFFU);
      fill(pixels_.gamma32(Adafruit_NeoPixel::ColorHSV(hue, 255, brightness_)));
      ++animationStep_;
      break;
    }

    case LedEffect::Aurora: {
      const uint16_t drift = static_cast<uint16_t>(animationStep_ * 230U);
      for (uint8_t i = 0; i < BoardPins::LedCount; ++i) {
        const float wave = (sinf(static_cast<float>(animationStep_) * 0.13f + i * 0.9f) + 1.0f) * 0.5f;
        const uint16_t hue = static_cast<uint16_t>(21000U + drift + i * 1800U);
        const uint8_t value = static_cast<uint8_t>((0.35f + wave * 0.65f) * brightness_);
        pixels_.setPixelColor(i, pixels_.gamma32(Adafruit_NeoPixel::ColorHSV(hue, 205, value)));
      }
      ++animationStep_;
      break;
    }

    case LedEffect::Off:
    case LedEffect::Count:
    default:
      break;
  }

  pixels_.show();
}
