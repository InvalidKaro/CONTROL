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

void BleApp::end() {
  // BLE is intentionally kept initialized globally so reopening the app does
  // not repeatedly tear down the ESP32 BLE stack.
}

void BleApp::requestScan() {
  if (scanning_) {
    return;
  }

  scanning_ = true;

  const BaseType_t created = xTaskCreate(
      scanTask,
      "ble_scan",
      6144,
      this,
      1,
      nullptr
  );

  if (created != pdPASS) {
    scanning_ = false;
  }
}

void BleApp::scanTask(void* arg) {
  auto* app =
      static_cast<BleApp*>(arg);

  if (app != nullptr) {
    app->performScan();
  }

  vTaskDelete(nullptr);
}

void BleApp::performScan() {
  BLEScan* scan =
      BLEDevice::getScan();

  if (scan == nullptr) {
    scanning_ = false;
    return;
  }

  scan->setActiveScan(false);
  scan->setInterval(100);
  scan->setWindow(80);

  BLEScanResults results =
      scan->start(2, false);

  DeviceRow local[16];

  const int resultCount =
      results.getCount();

  const uint8_t localCount =
      static_cast<uint8_t>(
          constrain(
              resultCount,
              0,
              16
          )
      );

  for (uint8_t i = 0; i < localCount; ++i) {
    BLEAdvertisedDevice device =
        results.getDevice(i);

    local[i].name =
        device.haveName()
            ? String(device.getName().c_str())
            : String("<unnamed>");

    local[i].address =
        String(
            device.getAddress()
                .toString()
                .c_str()
        );

    local[i].rssi =
        device.getRSSI();
  }

  std::sort(
      local,
      local + localCount,
      [](
          const DeviceRow& left,
          const DeviceRow& right
      ) {
        return left.rssi > right.rssi;
      }
  );

  // Keep count at zero while String objects are copied. The renderer only
  // consumes devices_ after the complete snapshot has been committed.
  count_ = 0;

  for (uint8_t i = 0; i < localCount; ++i) {
    devices_[i] = local[i];
  }

  count_ = localCount;
  resultsReady_ = true;
  scanning_ = false;

  scan->clearResults();
}

void BleApp::tick(uint32_t nowMs) {
  if (!resultsReady_) {
    return;
  }

  resultsReady_ = false;
  lastScanMs_ = nowMs;

  if (selected_ >= count_) {
    selected_ = 0;
  }
}

void BleApp::onEncoder(int delta) {
  if (!count_ || delta == 0) {
    return;
  }

  int next =
      static_cast<int>(selected_) +
      delta;

  while (next < 0) {
    next += count_;
  }

  selected_ =
      static_cast<uint8_t>(
          next % count_
      );
}

void BleApp::onSelect() {
  requestScan();
}

void BleApp::render(TFT_eSPI& tft) {
  tft.fillScreen(Theme::Bg);

  Ui::header(
      tft,
      "BLE EXPLORER",
      scanning_
          ? String("SCANNING")
          : String(count_) + " devices"
  );

  if (!count_) {
    Ui::centered(
        tft,
        75,
        scanning_
            ? String("Listening for advertisements...")
            : String("No advertisements"),
        Theme::Muted,
        1
    );
  } else {
    const uint8_t start =
        selected_ > 2
            ? selected_ - 2
            : 0;

    for (
        uint8_t row = 0;
        row < 5 &&
        start + row < count_;
        ++row
    ) {
      const uint8_t index =
          start + row;

      const int y =
          31 + row * 22;

      if (index == selected_) {
        Ui::panel(
            tft,
            7,
            y,
            306,
            19,
            true
        );
      }

      String name =
          devices_[index].name;

      if (name.length() > 21) {
        name =
            name.substring(0, 21);
      }

      Ui::text(
          tft,
          13,
          y + 2,
          name,
          index == selected_
              ? Theme::PrimaryBright
              : Theme::Text,
          1
      );

      tft.setTextDatum(TR_DATUM);

      tft.setTextColor(
          devices_[index].rssi > -65
              ? Theme::PrimaryBright
              : Theme::Muted,
          index == selected_
              ? Theme::Panel
              : Theme::Bg
      );

      tft.drawString(
          String(devices_[index].rssi) +
              " dBm",
          306,
          y + 2,
          1
      );

      tft.setTextDatum(TL_DATUM);
    }

    Ui::text(
        tft,
        10,
        136,
        devices_[selected_].address,
        Theme::Muted,
        1
    );
  }

  Ui::footer(
      tft,
      "TURN: device",
      "ENC: rescan"
  );
}
