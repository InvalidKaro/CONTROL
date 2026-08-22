#pragma once

#include <SD.h>
#include "app.h"

class StorageApp final : public App {
 public:
  const char* name() const override { return "Storage"; }
  const char* shortName() const override { return "SD"; }
  void begin() override;
  void end() override;
  void tick(uint32_t nowMs) override;
  void render(TFT_eSPI& tft) override;
  void onEncoder(int delta) override;

 private:
  void refresh();
  bool ready_ = false;
  String names_[16];
  bool dirs_[16]{};
  int count_ = 0;
  int offset_ = 0;
};
