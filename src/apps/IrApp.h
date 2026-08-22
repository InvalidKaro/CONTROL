#pragma once

#include <IRrecv.h>
#include <IRsend.h>
#include <IRutils.h>
#include <Preferences.h>
#include "app.h"

class IrApp final : public App {
 public:
  IrApp();
  const char* name() const override { return "IR Studio"; }
  const char* shortName() const override { return "IR"; }
  void begin() override;
  void end() override;
  void tick(uint32_t nowMs) override;
  void render(TFT_eSPI& tft) override;
  void onSelect() override;
  void onEncoder(int delta) override;

 private:
  void replay();
  void loadLearned();
  void saveLearned();
  IRrecv receiver_;
  IRsend sender_;
  Preferences prefs_;
  decode_results results_{};
  String protocol_ = "-";
  String value_ = "-";
  uint16_t bits_ = 0;
  uint32_t frames_ = 0;
  decode_type_t learnedType_ = decode_type_t::UNKNOWN;
  uint64_t learnedValue_ = 0;
  uint16_t learnedBits_ = 0;
  uint32_t lastReplayMs_ = 0;
  uint8_t mode_ = 0;
  bool lastSendOk_ = false;
};
