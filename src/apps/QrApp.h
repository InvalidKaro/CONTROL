#pragma once

#include "app.h"

class QrApp final : public App {
 public:
  const char* name() const override { return "QR & Share"; }
  const char* shortName() const override { return "QR"; }
  void begin() override;
  void tick(uint32_t nowMs) override;
  void render(TFT_eSPI& tft) override;
  void onEncoder(int delta) override;
  void onSelect() override;

 private:
  uint8_t mode_ = 0;
  bool invert_ = false;
};
