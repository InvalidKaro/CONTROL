#include "ScriptApp.h"

#include "Ui.h"
#include "theme.h"

void ScriptApp::ensureDefaults() {
  if (!LittleFS.begin(true)) {
    return;
  }

  if (!LittleFS.exists("/scripts")) {
    LittleFS.mkdir("/scripts");
  }

  if (!LittleFS.exists("/scripts/circuit.cos")) {
    fs::File file =
        LittleFS.open(
            "/scripts/circuit.cos",
            FILE_WRITE
        );

    if (file) {
      file.println("CLEAR #000000");
      file.println("COLOR #00FF50");
      file.println("GOTO 24 85");
      file.println("FORWARD 55");
      file.println("RIGHT 90");
      file.println("FORWARD 28");
      file.println("LEFT 90");
      file.println("FORWARD 72");
      file.println("LEFT 45");
      file.println("FORWARD 35");
      file.println("TEXT CONTROL//OS");
      file.println("WAIT 900");
      file.close();
    }
  }
}

void ScriptApp::scanFs(
    fs::FS& fs,
    const char* rootPath,
    bool sd
) {
  fs::File root =
      fs.open(rootPath);

  if (!root || !root.isDirectory()) {
    return;
  }

  fs::File file =
      root.openNextFile();

  while (file && count_ < 16) {
    if (!file.isDirectory()) {
      String path =
          file.name();

      if (path.endsWith(".cos")) {
        paths_[count_] =
            path;

        String name =
            path;

        const int slash =
            name.lastIndexOf('/');

        if (slash >= 0) {
          name =
              name.substring(slash + 1);
        }

        names_[count_] =
            String(
                sd
                    ? "SD:"
                    : "FS:"
            ) +
            name;

        onSd_[count_] = sd;

        ++count_;
      }
    }

    file.close();
    file = root.openNextFile();
  }

  root.close();
}

void ScriptApp::refresh() {
  count_ = 0;

  scanFs(
      LittleFS,
      "/scripts",
      false
  );

  if (SD.cardType() != CARD_NONE) {
    if (!SD.exists("/scripts")) {
      SD.mkdir("/scripts");
    }

    scanFs(
        SD,
        "/scripts",
        true
    );
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
  if (
      runningView_ ||
      count_ == 0 ||
      delta == 0
  ) {
    return;
  }

  int next =
      static_cast<int>(selected_) +
      delta;

  while (next < 0) {
    next += count_;
  }

  selected_ =
      static_cast<uint8_t>(
          next % count_
      );
}

void ScriptApp::onSelect() {
  if (runningView_) {
    script_.stop();
    runningView_ = false;
    return;
  }

  if (
      count_ == 0 ||
      tft_ == nullptr
  ) {
    return;
  }

  script_.attach(tft_);

  fs::FS& fs =
      onSd_[selected_]
          ? static_cast<fs::FS&>(SD)
          : static_cast<fs::FS&>(LittleFS);

  if (
      script_.load(
          fs,
          paths_[selected_]
      )
  ) {
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
      const String status =
          script_.status();

      Ui::footer(
          tft,
          status.c_str(),
          "ENC: return"
      );
    }

    return;
  }

  tft.fillScreen(Theme::Bg);

  Ui::header(
      tft,
      "SCRIPT LAB",
      String(count_) +
          " scripts"
  );

  if (!count_) {
    Ui::centered(
        tft,
        76,
        "No /scripts/*.cos files",
        Theme::Muted,
        2
    );
  } else {
    const uint8_t first =
        selected_ > 4
            ? selected_ - 4
            : 0;

    for (
        uint8_t row = 0;
        row < 5 &&
        first + row < count_;
        ++row
    ) {
      const uint8_t index =
          first + row;

      const int y =
          32 + row * 21;

      if (index == selected_) {
        Ui::panel(
            tft,
            8,
            y,
            304,
            18,
            true
        );
      }

      Ui::text(
          tft,
          14,
          y + 2,
          names_[index],
          index == selected_
              ? Theme::PrimaryBright
              : Theme::Text,
          1
      );
    }
  }

  Ui::footer(
      tft,
      "TURN: script",
      "ENC: run"
  );
}
