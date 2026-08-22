#include "LedApp.h"

#include <math.h>

#include "Ui.h"
#include "theme.h"

void LedApp::begin() {
  row_ = 0;
  editing_ = false;
  syncHueFromRgb();
}

void LedApp::onSelect() {
  editing_ = !editing_;
}

void LedApp::onEncoder(int delta) {
  if (delta == 0) return;

  if (!editing_) {
    int next = static_cast<int>(row_) + delta;
    while (next < 0) next += RowCount;
    row_ = static_cast<uint8_t>(next % RowCount);
    return;
  }

  switch (row_) {
    case 0: {
      int next = static_cast<int>(leds_.effectIndex()) + delta;
      while (next < 0) next += LedController::effectCount();
      leds_.setEffect(static_cast<uint8_t>(next % LedController::effectCount()));
      break;
    }
    case 1: {
      int next = static_cast<int>(hue_) + delta * 4;
      while (next < 0) next += 360;
      hue_ = static_cast<uint16_t>(next % 360);
      applyHue();
      break;
    }
    case 2: {
      const int next = constrain(static_cast<int>(leds_.brightness()) + delta * 5, 0, 255);
      leds_.setBrightness(static_cast<uint8_t>(next));
      break;
    }
    case 3: {
      const int next = constrain(static_cast<int>(leds_.speed()) + delta * 3, 1, 100);
      leds_.setSpeed(static_cast<uint8_t>(next));
      break;
    }
    case 4:
      leds_.setEnabled(!leds_.enabled());
      break;
    default:
      break;
  }
}

void LedApp::render(TFT_eSPI& tft) {
  tft.fillScreen(Theme::Bg);
  Ui::header(tft, "LED STUDIO", leds_.enabled() ? "ACTIVE" : "DISABLED");

  const char* labels[RowCount] = {"EFFECT", "COLOR", "BRIGHTNESS", "SPEED", "POWER"};
  String values[RowCount] = {
      leds_.effectName(),
      leds_.hexColor(),
      String(leds_.brightness()) + " / 255",
      String(leds_.speed()) + "%",
      leds_.enabled() ? "ON" : "OFF",
  };

  for (uint8_t i = 0; i < RowCount; ++i) {
    const int y = 31 + i * 22;
    const bool selected = i == row_;
    if (selected) Ui::panel(tft, 7, y - 2, 306, 20, editing_);
    Ui::text(tft, 13, y + 3, labels[i], selected ? Theme::PrimaryBright : Theme::Muted, 1);
    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(selected ? Theme::Text : Theme::Muted, selected ? Theme::Panel : Theme::Bg);
    tft.drawString(values[i], 306, y + 3, 1);
    tft.setTextDatum(TL_DATUM);
  }

  const uint16_t previewColor = tft.color565(leds_.red(), leds_.green(), leds_.blue());
  tft.fillRect(201, 55, 12, 10, previewColor);
  tft.drawRect(200, 54, 14, 12, Theme::PrimaryDim);

  Ui::footer(tft, editing_ ? "TURN: change" : "TURN: select", editing_ ? "ENC: done" : "ENC: edit");
}

void LedApp::syncHueFromRgb() {
  hue_ = rgbToHue(leds_.red(), leds_.green(), leds_.blue());
}

void LedApp::applyHue() {
  uint8_t red = 0;
  uint8_t green = 0;
  uint8_t blue = 0;
  hsvToRgb(hue_, red, green, blue);
  leds_.setColor(red, green, blue);
}

void LedApp::hsvToRgb(uint16_t hue, uint8_t& red, uint8_t& green, uint8_t& blue) {
  const float h = static_cast<float>(hue % 360) / 60.0f;
  const float x = 1.0f - fabsf(fmodf(h, 2.0f) - 1.0f);
  float r = 0.0f;
  float g = 0.0f;
  float b = 0.0f;

  if (h < 1.0f) {
    r = 1.0f; g = x;
  } else if (h < 2.0f) {
    r = x; g = 1.0f;
  } else if (h < 3.0f) {
    g = 1.0f; b = x;
  } else if (h < 4.0f) {
    g = x; b = 1.0f;
  } else if (h < 5.0f) {
    r = x; b = 1.0f;
  } else {
    r = 1.0f; b = x;
  }

  red = static_cast<uint8_t>(r * 255.0f);
  green = static_cast<uint8_t>(g * 255.0f);
  blue = static_cast<uint8_t>(b * 255.0f);
}

uint16_t LedApp::rgbToHue(uint8_t red, uint8_t green, uint8_t blue) {
  const float r = red / 255.0f;
  const float g = green / 255.0f;
  const float b = blue / 255.0f;
  float maxV = r;
  if (g > maxV) maxV = g;
  if (b > maxV) maxV = b;
  float minV = r;
  if (g < minV) minV = g;
  if (b < minV) minV = b;
  const float delta = maxV - minV;
  if (delta < 0.001f) return 0;

  float hue = 0.0f;
  if (maxV == r) hue = 60.0f * fmodf((g - b) / delta, 6.0f);
  else if (maxV == g) hue = 60.0f * (((b - r) / delta) + 2.0f);
  else hue = 60.0f * (((r - g) / delta) + 4.0f);
  if (hue < 0.0f) hue += 360.0f;
  return static_cast<uint16_t>(hue);
}
