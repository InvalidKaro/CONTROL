#pragma once

#include <IRrecv.h>
#include <IRutils.h>
#include "app.h"

class IrApp final : public App {
 public:
  IrApp();
  const char* name() const override { return "IR Scope"; }
  const char* shortName() const override { return "IR"; }
  void begin() override;
  void end() override;
  void tick(uint32_t nowMs) override;
  void render(TFT_eSPI& tft) override;

 private:
  IRrecv receiver_;
  decode_results results_{};
  String protocol_ = "-";
  String value_ = "-";
  uint16_t bits_ = 0;
  uint32_t frames_ = 0;
};
