#include "StorageApp.h"

#include <SPI.h>

#include "Ui.h"
#include "board_pins.h"
#include "theme.h"

void StorageApp::begin() {
  // The WebUI keeps SD mounted globally so remote file access remains available.
  // Reuse the existing mount and only initialize it when no card is visible yet.
  ready_ = SD.cardType() != CARD_NONE;
  if (!ready_) ready_ = SD.begin(BoardPins::SdCs, SPI, 10000000);
  if (ready_) refresh();
}

void StorageApp::end() {
  // Intentionally do not call SD.end(): the WebUI owns the persistent mount.
}

void StorageApp::tick(uint32_t nowMs) {
  (void)nowMs;
}

void StorageApp::onEncoder(int delta) {
  offset_ = constrain(offset_ + delta, 0, max(0, count_ - 5));
}

void StorageApp::refresh() {
  count_ = 0;
  File root = SD.open("/");
  if (!root) return;
  File file = root.openNextFile();
  while (file && count_ < 16) {
    names_[count_] = file.name();
    dirs_[count_] = file.isDirectory();
    ++count_;
    file.close();
    file = root.openNextFile();
  }
  root.close();
}

void StorageApp::render(TFT_eSPI& tft) {
  tft.fillScreen(Theme::Bg);
  Ui::header(tft, "STORAGE", ready_ ? String(count_) + " items" : "NO SD");
  if (!ready_) {
    Ui::centered(tft, 72, "Insert microSD and reopen", Theme::Muted, 2);
  } else {
    const int visible = min(5, count_ - offset_);
    for (int row = 0; row < visible; ++row) {
      const int i = row + offset_;
      const int y = 31 + row * 22;
      Ui::panel(tft, 8, y, 304, 18);
      String label = dirs_[i] ? "[D] " : "[F] ";
      label += names_[i];
      if (label.length() > 34) label = label.substring(0, 34);
      Ui::text(tft, 14, y + 2, label, dirs_[i] ? Theme::PrimaryBright : Theme::Text, 1);
    }
  }
  Ui::footer(tft, "TURN: scroll", "USER: back");
}
