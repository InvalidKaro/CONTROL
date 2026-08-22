#pragma once

#include "AutomationEngine.h"
#include "PowerManager.h"
#include "ThemeManager.h"
#include "app.h"

class SettingsApp final : public App {
 public:
  SettingsApp(ThemeManager& themes, PowerManager& power, AutomationEngine& automations)
      : themes_(themes), power_(power), automations_(automations) {}
  const char* name() const override { return "Settings & Profiles"; }
  const char* shortName() const override { return "SET"; }
  void tick(uint32_t nowMs) override { (void)nowMs; }
  void render(TFT_eSPI& tft) override;
  void onEncoder(int delta) override;
  void onSelect() override;

 private:
  ThemeManager& themes_;
  PowerManager& power_;
  AutomationEngine& automations_;
  uint8_t row_ = 0;
  bool editing_ = false;
  static constexpr uint8_t RowCount = 5;
};
