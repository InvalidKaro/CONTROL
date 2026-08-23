#pragma once

#include <Adafruit_PN532.h>

#include "app.h"

class NfcApp final : public App {
 public:
  NfcApp();

  const char* name() const override {
    return "NFC Inspector";
  }

  const char* shortName() const override {
    return "NFC";
  }

  void begin() override;
  void tick(uint32_t nowMs) override;
  void render(TFT_eSPI& tft) override;
  void onEncoder(int delta) override;
  void onSelect() override;

 private:
  void addHistory(const String& uid);

  Adafruit_PN532 nfc_;

  bool ready_ = false;

  String uid_ = "-";
  uint8_t uidLength_ = 0;
  uint32_t lastPollMs_ = 0;

  // 0 = live, 1 = history, 2 = NDEF writer
  uint8_t view_ = 0;

  bool ndefReady_ = false;
  uint16_t capacityBytes_ = 0;

  String history_[5];
  uint8_t historyCount_ = 0;

  String writeStatus_;
};
