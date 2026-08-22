#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>

namespace Ui {
void header(TFT_eSPI& tft, const char* title, const String& status = String());
void footer(TFT_eSPI& tft, const char* left, const char* right);
void panel(TFT_eSPI& tft, int x, int y, int w, int h, bool selected = false);
void text(TFT_eSPI& tft, int x, int y, const String& value, uint16_t color, uint8_t font = 2);
void centered(TFT_eSPI& tft, int y, const String& value, uint16_t color, uint8_t font = 2);
}  // namespace Ui
