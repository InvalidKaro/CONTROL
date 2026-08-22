#pragma once

#include "LedController.h"
#include "PowerManager.h"
#include "SoundController.h"
#include "ThemeManager.h"
#include "app.h"

class QuickActionsApp final : public App {
 public:
  QuickActionsApp(LedController& leds, PowerManager& power, ThemeManager& themes, SoundController& sounds)
      : leds_(leds), power_(power), themes_(themes), sounds_(sounds) {}
  const char* name() const override { return "Command Palette"; }
  const char* shortName() const override { return "CMD"; }
  void begin() override;
  void tick(uint32_t nowMs) override;
  void render(TFT_eSPI& tft) override;
  void onEncoder(int delta) override;
  void onSelect() override;

 private:
  static constexpr uint8_t ActionCount = 9;
  LedController& leds_;
  PowerManager& power_;
  ThemeManager& themes_;
  SoundController& sounds_;
  uint8_t selected_ = 0;
  String status_;
  uint32_t statusUntilMs_ = 0;
};
