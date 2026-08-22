#pragma once

#include <RadioLib.h>
#include "app.h"

class SubGhzApp final : public App {
 public:
  SubGhzApp();
  const char* name() const override { return "Sub-GHz Scope"; }
  const char* shortName() const override { return "SUBG"; }
  void begin() override;
  void end() override;
  void tick(uint32_t nowMs) override;
  void render(TFT_eSPI& tft) override;
  void onEncoder(int delta) override;
  void onSelect() override;

 private:
  void configureBand();
  void sampleStep();
  Module module_;
  CC1101 radio_;
  bool ready_ = false;
  bool running_ = true;
  int band_ = 1;
  int cursor_ = 0;
  float levels_[64]{};
  uint32_t lastSampleUs_ = 0;
};
