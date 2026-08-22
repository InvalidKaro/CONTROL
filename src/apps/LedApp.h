#pragma once

#include "LedController.h"
#include "app.h"

class LedApp final : public App {
 public:
  explicit LedApp(LedController& leds) : leds_(leds) {}

  const char* name() const override { return "LED Studio"; }
  const char* shortName() const override { return "LED"; }
  void begin() override;
  void tick(uint32_t nowMs) override { (void)nowMs; }
  void render(TFT_eSPI& tft) override;
  void onEncoder(int delta) override;
  void onSelect() override;

 private:
  static constexpr uint8_t RowCount = 5;
  LedController& leds_;
  uint8_t row_ = 0;
  bool editing_ = false;
  uint16_t hue_ = 140;

  void syncHueFromRgb();
  void applyHue();
  static void hsvToRgb(uint16_t hue, uint8_t& red, uint8_t& green, uint8_t& blue);
  static uint16_t rgbToHue(uint8_t red, uint8_t green, uint8_t blue);
};
