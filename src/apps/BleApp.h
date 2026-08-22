#pragma once

#include <BLEDevice.h>
#include <BLEScan.h>
#include "app.h"

class BleApp final : public App {
 public:
  const char* name() const override { return "BLE Explorer"; }
  const char* shortName() const override { return "BLE"; }
  void begin() override;
  void end() override;
  void tick(uint32_t nowMs) override;
  void render(TFT_eSPI& tft) override;
  void onEncoder(int delta) override;
  void onSelect() override;

 private:
  struct DeviceRow {
    String name;
    String address;
    int rssi = -127;
  };
  static void scanTask(void* arg);
  void performScan();
  void requestScan();

  DeviceRow devices_[16];
  volatile uint8_t count_ = 0;
  volatile bool scanning_ = false;
  volatile bool resultsReady_ = false;
  uint8_t selected_ = 0;
  uint32_t lastScanMs_ = 0;
  portMUX_TYPE mux_ = portMUX_INITIALIZER_UNLOCKED;
};
