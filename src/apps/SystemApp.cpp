#include "SystemApp.h"

#include <WiFi.h>
#include <Wire.h>
#include "Ui.h"
#include "theme.h"

void SystemApp::begin() {
  onSelect();
}

void SystemApp::onSelect() {
  i2cCount_ = 0;
  for (uint8_t address = 1; address < 127; ++address) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) ++i2cCount_;
  }
  lastScanMs_ = millis();
}

void SystemApp::tick(uint32_t nowMs) {
  (void)nowMs;
}

void SystemApp::render(TFT_eSPI& tft) {
  tft.fillScreen(Theme::Bg);
  Ui::header(tft, "SYSTEM DECK", "ESP32-S3");

  const uint32_t heapKb = ESP.getFreeHeap() / 1024;
  const uint32_t psramKb = ESP.getFreePsram() / 1024;
  const uint32_t uptime = millis() / 1000;

  Ui::panel(tft, 9, 32, 145, 45);
  Ui::text(tft, 17, 39, "FREE HEAP", Theme::Muted, 1);
  Ui::text(tft, 17, 54, String(heapKb) + " KB", Theme::PrimaryBright, 2);

  Ui::panel(tft, 166, 32, 145, 45);
  Ui::text(tft, 174, 39, "FREE PSRAM", Theme::Muted, 1);
  Ui::text(tft, 174, 54, String(psramKb) + " KB", Theme::PrimaryBright, 2);

  Ui::panel(tft, 9, 84, 145, 45);
  Ui::text(tft, 17, 91, "I2C DEVICES", Theme::Muted, 1);
  Ui::text(tft, 17, 106, String(i2cCount_), Theme::Text, 2);

  Ui::panel(tft, 166, 84, 145, 45);
  Ui::text(tft, 174, 91, "UPTIME", Theme::Muted, 1);
  Ui::text(tft, 174, 106, String(uptime) + " s", Theme::Text, 2);

  Ui::footer(tft, "ENC: rescan I2C", "USER: back");
}
