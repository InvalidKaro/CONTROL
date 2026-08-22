#include "SystemApp.h"

#include <WiFi.h>
#include <cstring>
#include <Wire.h>
#include "Ui.h"
#include "theme.h"

void SystemApp::begin() { onSelect(); sampleBattery(millis()); }

void SystemApp::onSelect() {
  i2cCount_ = 0;
  for (uint8_t address = 1; address < 127; ++address) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) ++i2cCount_;
  }
  lastScanMs_ = millis();
}

void SystemApp::sampleBattery(uint32_t nowMs) {
  const auto& b = power_.battery();
  if (!b.present) return;
  if (historyCount_ < 32) {
    socHistory_[historyCount_++] = b.soc;
  } else {
    memmove(socHistory_, socHistory_ + 1, 31);
    socHistory_[31] = b.soc;
  }
  lastBatterySampleMs_ = nowMs;
}

void SystemApp::tick(uint32_t nowMs) {
  if (nowMs - lastBatterySampleMs_ >= 10000) sampleBattery(nowMs);
}

void SystemApp::render(TFT_eSPI& tft) {
  tft.fillScreen(Theme::Bg);
  Ui::header(tft, "SYSTEM + BATTERY", power_.profileName());
  const auto& b = power_.battery();

  Ui::panel(tft, 8, 31, 96, 40);
  Ui::text(tft, 14, 37, "BATTERY", Theme::Muted, 1);
  Ui::text(tft, 14, 51, b.present ? String(b.soc) + "%" : "N/A", b.present ? Theme::PrimaryBright : Theme::Error, 2);

  Ui::panel(tft, 112, 31, 96, 40);
  Ui::text(tft, 118, 37, "VOLTAGE", Theme::Muted, 1);
  Ui::text(tft, 118, 51, b.present ? String(b.voltage, 2) + "V" : "-", Theme::Text, 2);

  Ui::panel(tft, 216, 31, 96, 40);
  Ui::text(tft, 222, 37, "CURRENT", Theme::Muted, 1);
  Ui::text(tft, 222, 51, b.present ? String(static_cast<int>(b.currentMa)) + "mA" : "-", Theme::Text, 1);

  Ui::panel(tft, 8, 77, 96, 38);
  Ui::text(tft, 14, 83, "HEAP", Theme::Muted, 1);
  Ui::text(tft, 14, 96, String(ESP.getFreeHeap() / 1024) + "K", Theme::PrimaryBright, 2);

  Ui::panel(tft, 112, 77, 96, 38);
  Ui::text(tft, 118, 83, "PSRAM", Theme::Muted, 1);
  Ui::text(tft, 118, 96, String(ESP.getFreePsram() / 1024) + "K", Theme::PrimaryBright, 2);

  Ui::panel(tft, 216, 77, 96, 38);
  Ui::text(tft, 222, 83, "I2C", Theme::Muted, 1);
  Ui::text(tft, 222, 96, String(i2cCount_), Theme::Text, 2);

  Ui::panel(tft, 8, 121, 304, 24);
  if (historyCount_ > 1) {
    const int left = 15, top = 125, width = 290, height = 15;
    for (uint8_t i = 1; i < historyCount_; ++i) {
      const int x0 = left + ((i - 1) * width) / 31;
      const int x1 = left + (i * width) / 31;
      const int y0 = top + height - (socHistory_[i - 1] * height) / 100;
      const int y1 = top + height - (socHistory_[i] * height) / 100;
      tft.drawLine(x0, y0, x1, y1, Theme::PrimaryBright);
    }
  } else {
    Ui::text(tft, 16, 127, "Battery history collecting...", Theme::Muted, 1);
  }

  Ui::footer(tft, "ENC: rescan I2C", b.present ? String(b.temperatureC, 1) + "C / 10s graph" : "BQ27220 not found");
}
