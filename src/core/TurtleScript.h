#pragma once

#include <Arduino.h>
#include <FS.h>
#include <TFT_eSPI.h>

class TurtleScript {
 public:
  void attach(TFT_eSPI* tft) { tft_ = tft; }
  bool load(fs::FS& fs, const String& path);
  void start();
  void stop();
  void tick(uint32_t nowMs);
  bool running() const { return running_; }
  String status() const { return status_; }
  String scriptName() const { return scriptName_; }

 private:
  bool execute(const String& raw, uint32_t nowMs);
  void move(float distance, bool draw);
  uint16_t parseColor(String token) const;

  TFT_eSPI* tft_ = nullptr;
  String lines_[96];
  uint16_t lineCount_ = 0;
  uint16_t pc_ = 0;
  bool running_ = false;
  float x_ = 160.0f;
  float y_ = 85.0f;
  float angle_ = 0.0f;
  bool pen_ = true;
  uint16_t color_ = 0x07E0;
  uint32_t waitUntilMs_ = 0;
  String status_ = "idle";
  String scriptName_ = "-";
};
