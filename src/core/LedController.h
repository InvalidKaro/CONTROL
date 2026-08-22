#pragma once

#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <Preferences.h>

#include "board_pins.h"

enum class LedEffect : uint8_t {
  Off = 0,
  Solid,
  Breathe,
  Rainbow,
  Chase,
  Scanner,
  Sparkle,
  Comet,
  ColorWipe,
  TheaterChase,
  Twinkle,
  Meteor,
  Wave,
  DualScanner,
  Heartbeat,
  CyberPulse,
  Matrix,
  Fire,
  Ocean,
  Confetti,
  Glitch,
  RandomColor,
  Aurora,
  Count,
};

class LedController {
 public:
  LedController();

  void begin();
  void tick(uint32_t nowMs);

  void setEnabled(bool enabled);
  void setEffect(LedEffect effect);
  void setEffect(uint8_t effectIndex);
  void setColor(uint8_t red, uint8_t green, uint8_t blue);
  void setBrightness(uint8_t brightness);
  void setSpeed(uint8_t speed);

  bool enabled() const { return enabled_; }
  LedEffect effect() const { return effect_; }
  uint8_t effectIndex() const { return static_cast<uint8_t>(effect_); }
  const char* effectName() const;
  static const char* effectName(LedEffect effect);
  static constexpr uint8_t effectCount() { return static_cast<uint8_t>(LedEffect::Count); }

  uint8_t red() const { return red_; }
  uint8_t green() const { return green_; }
  uint8_t blue() const { return blue_; }
  uint8_t brightness() const { return brightness_; }
  uint8_t speed() const { return speed_; }
  String hexColor() const;

 private:
  Adafruit_NeoPixel pixels_;
  Preferences preferences_;

  bool enabled_ = true;
  LedEffect effect_ = LedEffect::Solid;
  uint8_t red_ = 0;
  uint8_t green_ = 255;
  uint8_t blue_ = 80;
  uint8_t brightness_ = 72;
  uint8_t speed_ = 45;

  bool dirty_ = false;
  uint32_t dirtySinceMs_ = 0;
  uint32_t lastFrameMs_ = 0;
  uint16_t animationStep_ = 0;
  int8_t scannerDirection_ = 1;
  int16_t scannerIndex_ = 0;

  void markDirty();
  void load();
  void save();
  uint32_t frameIntervalMs() const;
  uint32_t scaledColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t level = 255) const;
  void fill(uint32_t color);
  void renderFrame(uint32_t nowMs);
};
