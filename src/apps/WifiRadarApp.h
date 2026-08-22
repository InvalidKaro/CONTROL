#pragma once

#include <WiFi.h>
#include "app.h"

class WifiRadarApp final : public App {
 public:
  const char* name() const override { return "Wi-Fi Radar"; }
  const char* shortName() const override { return "WIFI"; }
  void begin() override;
  void end() override;
  void tick(uint32_t nowMs) override;
  void render(TFT_eSPI& tft) override;
  void onSelect() override;

 private:
  void scan();
  bool requested_ = true;
  int count_ = 0;
  uint32_t lastScanMs_ = 0;
};
