#include "StorageApp.h"

#include <FS.h>
#include <SPI.h>

#include "Ui.h"
#include "board_pins.h"
#include "theme.h"

void StorageApp::begin() {
  // The WebUI keeps SD mounted globally. Reuse the existing mount when
  // available and initialize it only when no card is currently visible.
  ready_ = SD.cardType() != CARD_NONE;

  if (!ready_) {
    ready_ = SD.begin(
        BoardPins::SdCs,
        SPI,
        10000000
    );
  }

  if (ready_) {
    refresh();
  } else {
    count_ = 0;
    offset_ = 0;
  }
}

void StorageApp::end() {
  // Do not call SD.end(). WebUi owns the persistent SD mount.
}

void StorageApp::tick(uint32_t nowMs) {
  (void)nowMs;
}

void StorageApp::onEncoder(int delta) {
  if (!ready_ || delta == 0) {
    return;
  }

  offset_ = constrain(
      offset_ + delta,
      0,
      max(0, count_ - 5)
  );
}

void StorageApp::refresh() {
  count_ = 0;

  fs::File root = SD.open("/");

  if (!root || !root.isDirectory()) {
    offset_ = 0;
    return;
  }

  fs::File file = root.openNextFile();

  while (file && count_ < 16) {
    names_[count_] = file.name();
    dirs_[count_] = file.isDirectory();

    ++count_;

    file.close();
    file = root.openNextFile();
  }

  root.close();

  offset_ = constrain(
      offset_,
      0,
      max(0, count_ - 5)
  );
}

void StorageApp::render(TFT_eSPI& tft) {
  tft.fillScreen(Theme::Bg);

  Ui::header(
      tft,
      "STORAGE",
      ready_
          ? String(count_) + " items"
          : String("NO SD")
  );

  if (!ready_) {
    Ui::centered(
        tft,
        72,
        "Insert microSD and reopen",
        Theme::Muted,
        2
    );
  } else {
    const int remaining =
        max(0, count_ - offset_);

    const int visible =
        min(5, remaining);

    for (int row = 0; row < visible; ++row) {
      const int index = row + offset_;
      const int y = 31 + row * 22;

      Ui::panel(
          tft,
          8,
          y,
          304,
          18
      );

      String label =
          dirs_[index]
              ? "[D] "
              : "[F] ";

      label += names_[index];

      if (label.length() > 34) {
        label =
            label.substring(0, 34);
      }

      Ui::text(
          tft,
          14,
          y + 2,
          label,
          dirs_[index]
              ? Theme::PrimaryBright
              : Theme::Text,
          1
      );
    }

    if (count_ == 0) {
      Ui::centered(
          tft,
          75,
          "SD is empty",
          Theme::Muted,
          2
      );
    }
  }

  Ui::footer(
      tft,
      "TURN: scroll",
      "USER: back"
  );
}
