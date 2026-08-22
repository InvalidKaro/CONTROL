#include "DiagnosticsApp.h"

#include <SD.h>
#include <Wire.h>
#include "Ui.h"
#include "theme.h"

namespace {
bool i2cPresent(uint8_t addr) {
  Wire.beginTransmission(addr);
  return Wire.endTransmission() == 0;
}
}

void DiagnosticsApp::begin() { runChecks(); }

void DiagnosticsApp::runChecks() {
  pn532_ = i2cPresent(0x24);
  gauge_ = i2cPresent(0x55);
  charger_ = i2cPresent(0x6B);
  psram_ = ESP.getPsramSize() > 0 && ESP.getFreePsram() > 0;
  sd_ = SD.cardType() != CARD_NONE;
}

void DiagnosticsApp::onEncoder(int delta) {
  if (!delta) return;
  int next = static_cast<int>(row_) + delta;
  while (next < 0) next += 3;
  row_ = next % 3;
}

void DiagnosticsApp::onSelect() {
  if (row_ == 0) runChecks();
  else if (row_ == 1) {
    sound_.setEnabled(!sound_.enabled());
    if (sound_.enabled()) sound_.success();
  } else if (row_ == 2) {
    sound_.beep(880, 250, 20);
  }
}

void DiagnosticsApp::render(TFT_eSPI& tft) {
  tft.fillScreen(Theme::Bg);
  Ui::header(tft, "HARDWARE DIAGNOSTICS", "SELF TEST");
  const char* names[] = {"PN532", "BQ27220", "BQ25896", "PSRAM", "microSD", "SPEAKER"};
  bool ok[] = {pn532_, gauge_, charger_, psram_, sd_, sound_.ready()};
  for (int i = 0; i < 6; ++i) {
    const int col = i % 3;
    const int row = i / 3;
    const int x = 8 + col * 103;
    const int y = 34 + row * 43;
    Ui::panel(tft, x, y, 96, 36);
    Ui::text(tft, x + 7, y + 6, names[i], Theme::Muted, 1);
    Ui::text(tft, x + 7, y + 20, ok[i] ? "OK" : "MISS", ok[i] ? Theme::PrimaryBright : Theme::Error, 1);
  }
  const char* actions[] = {"RESCAN", sound_.enabled() ? "SOUND ON" : "SOUND OFF", "BEEP TEST"};
  for (int i = 0; i < 3; ++i) {
    if (i == row_) Ui::panel(tft, 8 + i * 103, 122, 96, 18, true);
    Ui::text(tft, 14 + i * 103, 125, actions[i], i == row_ ? Theme::PrimaryBright : Theme::Muted, 1);
  }
  Ui::footer(tft, "TURN: action", "ENC: execute");
}
