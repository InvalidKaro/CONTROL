#include "BleApp.h"

#include <algorithm>
#include "Ui.h"
#include "theme.h"

void BleApp::begin() {
  static bool initialized = false;
  if (!initialized) {
    BLEDevice::init("ControlOS");
    initialized = true;
  }
  requestScan();
}

void BleApp::end() {}

void BleApp::requestScan() {
  if (scanning_) return;
  scanning_ = true;
  xTaskCreatePinnedToCore(scanTask, "ble_scan", 6144, this, 1, nullptr, 0);
}

void BleApp::scanTask(void* arg) {
  static_cast<BleApp*>(arg)->performScan();
  vTaskDelete(nullptr);
}

void BleApp::performScan() {
  BLEScan* scan = BLEDevice::getScan();
  scan->setActiveScan(false);
  scan->setInterval(100);
  scan->setWindow(80);
  BLEScanResults results = scan->start(2, false);

  DeviceRow local[16];
  uint8_t localCount = min(16, results.getCount());
  for (uint8_t i = 0; i < localCount; ++i) {
    BLEAdvertisedDevice d = results.getDevice(i);
    local[i].name = d.haveName() ? String(d.getName().c_str()) : String("<unnamed>");
    local[i].address = String(d.getAddress().toString().c_str());
    local[i].rssi = d.getRSSI();
  }
  std::sort(local, local + localCount, [](const DeviceRow& a, const DeviceRow& b) { return a.rssi > b.rssi; });

  portENTER_CRITICAL(&mux_);
  count_ = localCount;
  for (uint8_t i = 0; i < localCount; ++i) devices_[i] = local[i];
  resultsReady_ = true;
  scanning_ = false;
  portEXIT_CRITICAL(&mux_);
  scan->clearResults();
}

void BleApp::tick(uint32_t nowMs) {
  if (resultsReady_) {
    resultsReady_ = false;
    lastScanMs_ = nowMs;
    if (selected_ >= count_) selected_ = 0;
  }
}

void BleApp::onEncoder(int delta) {
  if (!count_) return;
  int next = static_cast<int>(selected_) + delta;
  while (next < 0) next += count_;
  selected_ = next % count_;
}

void BleApp::onSelect() { requestScan(); }

void BleApp::render(TFT_eSPI& tft) {
  tft.fillScreen(Theme::Bg);
  Ui::header(tft, "BLE EXPLORER", scanning_ ? "SCANNING" : String(count_) + " devices");
  if (!count_) {
    Ui::centered(tft, 75, scanning_ ? "Listening for advertisements..." : "No advertisements", Theme::Muted, 1);
  } else {
    const uint8_t start = selected_ > 2 ? selected_ - 2 : 0;
    for (uint8_t row = 0; row < 5 && start + row < count_; ++row) {
      const uint8_t i = start + row;
      const int y = 31 + row * 22;
      if (i == selected_) Ui::panel(tft, 7, y, 306, 19, true);
      String name = devices_[i].name;
      if (name.length() > 21) name = name.substring(0, 21);
      Ui::text(tft, 13, y + 2, name, i == selected_ ? Theme::PrimaryBright : Theme::Text, 1);
      tft.setTextDatum(TR_DATUM);
      tft.setTextColor(devices_[i].rssi > -65 ? Theme::PrimaryBright : Theme::Muted, i == selected_ ? Theme::Panel : Theme::Bg);
      tft.drawString(String(devices_[i].rssi) + " dBm", 306, y + 2, 1);
      tft.setTextDatum(TL_DATUM);
    }
    Ui::text(tft, 10, 136, devices_[selected_].address, Theme::Muted, 1);
  }
  Ui::footer(tft, "TURN: device", "ENC: rescan");
}
