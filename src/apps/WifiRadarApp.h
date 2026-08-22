#pragma once

#include <WiFi.h>
#include "app.h"

class WifiRadarApp final : public App {
 public:
  const char* name() const override { return "Wi-Fi Analyzer"; }
  const char* shortName() const override { return "WIFI"; }
  void begin() override;
  void end() override;
  void tick(uint32_t nowMs) override;
  void render(TFT_eSPI& tft) override;
  void onSelect() override;
  void onEncoder(int delta) override;

 private:
  struct AccessPoint {
    String ssid;
    String bssid;
    int32_t rssi = -127;
    int32_t channel = 0;
    uint8_t encryption = 0;
  };

  void startScan();
  void captureResults(int count);
  String authName(uint8_t auth) const;
  bool requested_ = true;
  bool scanInProgress_ = false;
  uint8_t mode_ = 0; // list, channels, finder
  AccessPoint aps_[24];
  int count_ = 0;
  uint8_t channelUse_[14]{};
  uint32_t lastScanMs_ = 0;
};
