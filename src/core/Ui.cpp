#include "Ui.h"

#include "theme.h"

namespace Ui {
void header(TFT_eSPI& tft, const char* title, const String& status) {
  tft.fillRect(0, 0, 320, 24, Theme::Bg);
  tft.drawFastHLine(0, 23, 320, Theme::PrimaryDim);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(Theme::PrimaryBright, Theme::Bg);
  tft.drawString(title, 8, 4, 2);
  if (!status.isEmpty()) {
    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(Theme::Muted, Theme::Bg);
    tft.drawString(status, 312, 4, 2);
  }
  tft.setTextDatum(TL_DATUM);
}

void footer(TFT_eSPI& tft, const char* left, const char* right) {
  tft.fillRect(0, 150, 320, 20, Theme::Bg);
  tft.drawFastHLine(0, 149, 320, Theme::PrimaryDim);
  tft.setTextColor(Theme::Muted, Theme::Bg);
  tft.setTextDatum(TL_DATUM);
  tft.drawString(left, 7, 153, 1);
  tft.setTextDatum(TR_DATUM);
  tft.drawString(right, 313, 153, 1);
  tft.setTextDatum(TL_DATUM);
}

void panel(TFT_eSPI& tft, int x, int y, int w, int h, bool selected) {
  tft.fillRoundRect(x, y, w, h, 7, Theme::Panel);
  tft.drawRoundRect(x, y, w, h, 7, selected ? Theme::PrimaryBright : Theme::PrimaryDim);
}

void text(TFT_eSPI& tft, int x, int y, const String& value, uint16_t color, uint8_t font) {
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(color, Theme::Bg);
  tft.drawString(value, x, y, font);
}

void centered(TFT_eSPI& tft, int y, const String& value, uint16_t color, uint8_t font) {
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(color, Theme::Bg);
  tft.drawString(value, 160, y, font);
  tft.setTextDatum(TL_DATUM);
}
}  // namespace Ui
