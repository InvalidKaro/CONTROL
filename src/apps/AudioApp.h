#pragma once

#include <driver/i2s.h>
#include "app.h"

class AudioApp final : public App {
 public:
  const char* name() const override { return "Audio Spectrum"; }
  const char* shortName() const override { return "AUDIO"; }
  void begin() override;
  void end() override;
  void tick(uint32_t nowMs) override;
  void render(TFT_eSPI& tft) override;
  void onSelect() override { running_ = !running_; }

 private:
  bool initMic();
  void sample();
  bool ready_ = false;
  bool running_ = true;
  float bands_[16]{};
  float levelDbfs_ = -90.0f;
  float peakDbfs_ = -90.0f;
  uint32_t lastSampleMs_ = 0;
};
