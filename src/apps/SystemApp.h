#pragma once

#include "PowerManager.h"
#include "app.h"

class SystemApp final : public App {
 public:
  explicit SystemApp(PowerManager& power) : power_(power) {}
  const char* name() const override { return "System & Battery"; }
  const char* shortName() const override { return "SYS"; }
  void begin() override;
  void tick(uint32_t nowMs) override;
  void render(TFT_eSPI& tft) override;
  void onSelect() override;

 private:
  void sampleBattery(uint32_t nowMs);
  PowerManager& power_;
  int i2cCount_ = 0;
  uint32_t lastScanMs_ = 0;
  uint32_t lastBatterySampleMs_ = 0;
  uint8_t socHistory_[32]{};
  uint8_t historyCount_ = 0;
};
