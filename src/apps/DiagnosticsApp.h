#pragma once

#include "SoundController.h"
#include "app.h"

class DiagnosticsApp final : public App {
 public:
  explicit DiagnosticsApp(SoundController& sound) : sound_(sound) {}
  const char* name() const override { return "Hardware Diagnostics"; }
  const char* shortName() const override { return "DIAG"; }
  void begin() override;
  void tick(uint32_t nowMs) override { (void)nowMs; }
  void render(TFT_eSPI& tft) override;
  void onEncoder(int delta) override;
  void onSelect() override;

 private:
  void runChecks();
  SoundController& sound_;
  uint8_t row_ = 0;
  bool pn532_ = false;
  bool gauge_ = false;
  bool charger_ = false;
  bool psram_ = false;
  bool sd_ = false;
};
