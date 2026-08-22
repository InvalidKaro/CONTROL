#pragma once

#include "app.h"

class SystemApp final : public App {
 public:
  const char* name() const override { return "System Deck"; }
  const char* shortName() const override { return "SYS"; }
  void begin() override;
  void tick(uint32_t nowMs) override;
  void render(TFT_eSPI& tft) override;
  void onSelect() override;

 private:
  int i2cCount_ = 0;
  uint32_t lastScanMs_ = 0;
};
