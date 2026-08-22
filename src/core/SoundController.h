#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include <driver/i2s.h>

class SoundController {
 public:
  void begin();
  void end();
  void tick(uint32_t nowMs);
  void beep(uint16_t frequency, uint16_t durationMs, uint8_t volume = 18);
  void click() { beep(920, 35, 10); }
  void success() { beep(1320, 80, 16); }
  void error() { beep(260, 140, 16); }
  void setEnabled(bool enabled);
  bool enabled() const { return enabled_; }
  bool ready() const { return ready_; }

 private:
  void save();
  bool ready_ = false;
  bool enabled_ = false;
  uint16_t frequency_ = 0;
  uint8_t volume_ = 0;
  uint32_t stopAtMs_ = 0;
  float phase_ = 0.0f;
  Preferences prefs_;
};
