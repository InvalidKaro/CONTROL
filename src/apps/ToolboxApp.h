#pragma once

#include "app.h"

class ToolboxApp final : public App {
 public:
  const char* name() const override { return "Utility Toolbox"; }
  const char* shortName() const override { return "TOOLS"; }
  void begin() override;
  void tick(uint32_t nowMs) override;
  void render(TFT_eSPI& tft) override;
  void onEncoder(int delta) override;
  void onSelect() override;

 private:
  uint8_t mode_ = 0;
  bool running_ = false;
  uint32_t startedMs_ = 0;
  uint32_t elapsedMs_ = 0;
  uint32_t value_ = 0x2A;
  int timerSeconds_ = 300;
};
