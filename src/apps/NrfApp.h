#pragma once

#include <RF24.h>
#include "app.h"

class NrfApp final : public App {
 public:
  NrfApp();
  const char* name() const override { return "2.4G Channel Lab"; }
  const char* shortName() const override { return "2.4G"; }
  void begin() override;
  void end() override;
  void tick(uint32_t nowMs) override;
  void render(TFT_eSPI& tft) override;
  void onSelect() override;
  void onEncoder(int delta) override;

 private:
  RF24 radio_;
  bool ready_ = false;
  bool running_ = true;
  uint8_t mode_ = 0;
  uint8_t channel_ = 0;
  uint8_t levels_[126]{};
  uint8_t peaks_[126]{};
  uint16_t hits_[126]{};
  uint16_t samples_[126]{};
  uint32_t lastStepUs_ = 0;
};
