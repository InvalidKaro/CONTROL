#include "ScriptApp.h"

#include "Ui.h"
#include "theme.h"

void ScriptApp::ensureDefaults() {
  if (!LittleFS.begin(true)) return;
  if (!LittleFS.exists("/scripts")) LittleFS.mkdir("/scripts");
  if (!LittleFS.exists("/scripts/circuit.cos")) {
    File f = LittleFS.open("/scripts/circuit.cos", FILE_WRITE);
    if (f) {
      f.println("CLEAR #000000");
      f.println("COLOR #00FF50");
      f.println("GOTO 24 85");
      f.println("FORWARD 55");
      f.println("RIGHT 90");
      f.println("FORWARD 28");
      f.println("LEFT 90");
      f.println("FORWARD 72");
      f.println("LEFT 45");
      f.println("FORWARD 35");
      f.println("TEXT CONTROL//OS");
      f.println("WAIT 900");
      f.close();
    }
  }
}

void ScriptApp::scanFs(fs::FS& fs, const char* rootPath, bool sd) {
  File root = fs.open(rootPath);
  if (!root || !root.isDirectory()) return;

  File f = root.openNextFile();
  while (f && count_ < 16) {
    if (!f.isDirectory()) {
      String path = f.name();

      if (path.endsWith(".cos")) {
        paths_[count_] = path;

        String name = path;
        const int slash = name.lastIndexOf('/');
        if (slash >= 0) {
          name = name.substring(slash + 1);
        }

        names_[count_] = String(sd ? "SD:" : "FS:") + name;
        onSd_[count_] = sd;
        ++count_;
      }
    }

    f.close();
    f = root.openNextFile();
  }

  root.close();
}

void ScriptApp::refresh() {
  count_ = 0;

  scanFs(LittleFS, "/scripts", false);

  if (SD.cardType() != CARD_NONE) {
    if (!SD.exists("/scripts")) {
      SD.mkdir("/scripts");
    }

    scanFs(SD, "/scripts", true);
  }

  if (selected_ >= count_) {
    selected_ = 0;
  }
}

void ScriptApp::begin() {
  ensureDefaults();
  refresh();
  runningView_ = false;
}

void ScriptApp::onEncoder(int delta) {
  if (runningView_ || count_ == 0) return;

  int next = static_cast<int>(selected_) + delta;

  while (next < 0) {
    next += count_;
  }

  selected_ = next % count_;
}

void ScriptApp::onSelect() {
  if (runningView_) {
    script_.stop();
    runningView_ = false;
    return;
  }

  if (!count_ || tft_ == nullptr) return;

  script_.attach(tft_);

  fs::FS& fs = onSd_[selected_]
                   ? static_cast<fs::FS&>(SD)
                   : static_cast<fs::FS&>(LittleFS);

  if (script_.load(fs, paths_[selected_])) {
    runningView_ = true;
    script_.start();
  }
}

void ScriptApp::tick(uint32_t nowMs) {
  if (runningView_) {
    script_.tick(nowMs);
  }
}

void ScriptApp::render(TFT_eSPI& tft) {
  tft_ = &tft;

  if (runningView_) {
    if (!script_.running()) {
      const String status = script_.status();
      Ui::footer(tft, status.c_str(), "ENC: return");
    }

    return;
  }

  tft.fillScreen(Theme::Bg);

  Ui::header(tft, "SCRIPT LAB", String(count_) + " scripts");

  if (!count_) {
    Ui::centered(
        tft,
        76,
        "No /scripts/*.cos files",
        Theme::Muted,
        2
    );
  } else {
    const uint8_t first = selected_ > 4 ? selected_ - 4 : 0;

    for (
        uint8_t row = 0;
        row < 5 && first + row < count_;
        ++row
    ) {
      const uint8_t i = first + row;
      const int y = 32 + row * 21;

      if (i == selected_) {
        Ui::panel(tft, 8, y, 304, 18, true);
      }

      Ui::text(
          tft,
          14,
          y + 2,
          names_[i],
          i == selected_
              ? Theme::PrimaryBright
              : Theme::Text,
          1
      );
    }
  }

  Ui::footer(tft, "TURN: script", "ENC: run");
}
