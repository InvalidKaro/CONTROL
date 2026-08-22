#pragma once

#include <Adafruit_PN532.h>
#include "app.h"

class NfcApp final : public App {
 public:
  NfcApp();
  const char* name() const override { return "NFC Inspector"; }
  const char* shortName() const override { return "NFC"; }
  void begin() override;
  void tick(uint32_t nowMs) override;
  void render(TFT_eSPI& tft) override;

 private:
  Adafruit_PN532 nfc_;
  bool ready_ = false;
  String uid_ = "-";
  uint8_t uidLength_ = 0;
  uint32_t lastPollMs_ = 0;
};
