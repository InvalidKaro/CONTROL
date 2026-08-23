#pragma once

#include <LittleFS.h>
#include <SD.h>

#include "TurtleScript.h"
#include "app.h"

class ScriptApp final : public App {
 public:
  const char* name() const override {
    return "Script Lab";
  }

  const char* shortName() const override {
    return "SCRIPT";
  }

  void begin() override;
  void tick(uint32_t nowMs) override;
  void render(TFT_eSPI& tft) override;
  void onEncoder(int delta) override;
  void onSelect() override;

 private:
  void ensureDefaults();
  void refresh();
  void scanFs(fs::FS& fs, const char* rootPath, bool sd);

  String names_[16];
  String paths_[16];
  bool onSd_[16]{};

  uint8_t count_ = 0;
  uint8_t selected_ = 0;

  bool runningView_ = false;

  TurtleScript script_;
  TFT_eSPI* tft_ = nullptr;
};
