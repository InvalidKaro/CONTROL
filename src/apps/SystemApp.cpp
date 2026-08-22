#include "SystemApp.h"

#include <WiFi.h>
#include <cstring>
#include <Wire.h>

#include "Ui.h"
#include "theme.h"

void SystemApp::begin() {
  onSelect();
  sampleBattery(millis());
}

void SystemApp::onSelect() {
  i2cCount_ = 0;

  for (uint8_t address = 1; address < 127; ++address) {
    Wire.beginTransmission(address);

    if (Wire.endTransmission() == 0) {
      ++i2cCount_;
    }
  }

  lastScanMs_ = millis();
}

void SystemApp::sampleBattery(uint32_t nowMs) {
  const auto& battery = power_.battery();

  if (!battery.present) {
    return;
  }

  if (historyCount_ < 32) {
    socHistory_[historyCount_++] = battery.soc;
  } else {
    memmove(
        socHistory_,
        socHistory_ + 1,
        31 * sizeof(socHistory_[0])
    );

    socHistory_[31] = battery.soc;
  }

  lastBatterySampleMs_ = nowMs;
}

void SystemApp::tick(uint32_t nowMs) {
  if (nowMs - lastBatterySampleMs_ >= 10000) {
    sampleBattery(nowMs);
  }
}

void SystemApp::render(TFT_eSPI& tft) {
  tft.fillScreen(Theme::Bg);

  Ui::header(
      tft,
      "SYSTEM + BATTERY",
      power_.profileName()
  );

  const auto& battery = power_.battery();

  Ui::panel(tft, 8, 31, 96, 40);
  Ui::text(tft, 14, 37, "BATTERY", Theme::Muted, 1);
  Ui::text(
      tft,
      14,
      51,
      battery.present
          ? String(battery.soc) + "%"
          : String("N/A"),
      battery.present
          ? Theme::PrimaryBright
          : Theme::Error,
      2
  );

  Ui::panel(tft, 112, 31, 96, 40);
  Ui::text(tft, 118, 37, "VOLTAGE", Theme::Muted, 1);
  Ui::text(
      tft,
      118,
      51,
      battery.present
          ? String(battery.voltage, 2) + "V"
          : String("-"),
      Theme::Text,
      2
  );

  Ui::panel(tft, 216, 31, 96, 40);
  Ui::text(tft, 222, 37, "CURRENT", Theme::Muted, 1);
  Ui::text(
      tft,
      222,
      51,
      battery.present
          ? String(static_cast<int>(battery.currentMa)) + "mA"
          : String("-"),
      Theme::Text,
      1
  );

  Ui::panel(tft, 8, 77, 96, 38);
  Ui::text(tft, 14, 83, "HEAP", Theme::Muted, 1);
  Ui::text(
      tft,
      14,
      96,
      String(ESP.getFreeHeap() / 1024) + "K",
      Theme::PrimaryBright,
      2
  );

  Ui::panel(tft, 112, 77, 96, 38);
  Ui::text(tft, 118, 83, "PSRAM", Theme::Muted, 1);
  Ui::text(
      tft,
      118,
      96,
      String(ESP.getFreePsram() / 1024) + "K",
      Theme::PrimaryBright,
      2
  );

  Ui::panel(tft, 216, 77, 96, 38);
  Ui::text(tft, 222, 83, "I2C", Theme::Muted, 1);
  Ui::text(
      tft,
      222,
      96,
      String(i2cCount_),
      Theme::Text,
      2
  );

  Ui::panel(tft, 8, 121, 304, 24);

  if (historyCount_ > 1) {
    constexpr int left = 15;
    constexpr int top = 125;
    constexpr int width = 290;
    constexpr int height = 15;

    for (uint8_t i = 1; i < historyCount_; ++i) {
      const int previousSoc = constrain(
          static_cast<int>(socHistory_[i - 1]),
          0,
          100
      );

      const int currentSoc = constrain(
          static_cast<int>(socHistory_[i]),
          0,
          100
      );

      const int x0 =
          left + ((i - 1) * width) / 31;

      const int x1 =
          left + (i * width) / 31;

      const int y0 =
          top + height - (previousSoc * height) / 100;

      const int y1 =
          top + height - (currentSoc * height) / 100;

      tft.drawLine(
          x0,
          y0,
          x1,
          y1,
          Theme::PrimaryBright
      );
    }
  } else {
    Ui::text(
        tft,
        16,
        127,
        "Battery history collecting...",
        Theme::Muted,
        1
    );
  }

  const String footerRight =
      battery.present
          ? String(battery.temperatureC, 1) +
                "C / 10s graph"
          : String("BQ27220 not found");

  Ui::footer(
      tft,
      "ENC: rescan I2C",
      footerRight.c_str()
  );
}
