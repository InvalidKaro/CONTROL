#include "SubGhzApp.h"

#include <FS.h>
#include <LittleFS.h>

#include "Ui.h"
#include "board_pins.h"
#include "theme.h"

SubGhzApp::SubGhzApp()
    : module_(
          BoardPins::Cc1101Cs,
          BoardPins::Cc1101Gdo0,
          RADIOLIB_NC,
          BoardPins::Cc1101Gdo2
      ),
      radio_(&module_) {
  for (int i = 0; i < 64; ++i) {
    peaks_[i] = -120.0f;
  }
}

void SubGhzApp::configureBand() {
  digitalWrite(
      BoardPins::Cc1101Switch1,
      band_ == 2 ? LOW : HIGH
  );

  digitalWrite(
      BoardPins::Cc1101Switch0,
      band_ == 0 ? LOW : HIGH
  );
}

float SubGhzApp::frequencyAt(uint8_t index) const {
  const float start =
      band_ == 0
          ? 300.0f
          : (band_ == 1 ? 400.0f : 840.0f);

  const float span =
      band_ == 0
          ? 48.0f
          : (band_ == 1 ? 64.0f : 88.0f);

  return start + (span * index / 63.0f);
}

void SubGhzApp::begin() {
  pinMode(BoardPins::Cc1101Switch1, OUTPUT);
  pinMode(BoardPins::Cc1101Switch0, OUTPUT);

  configureBand();

  const float initial =
      band_ == 0
          ? 315.0f
          : (band_ == 1 ? 433.92f : 868.3f);

  ready_ =
      radio_.begin(initial) == RADIOLIB_ERR_NONE;

  if (ready_) {
    radio_.startReceive();
  }
}

void SubGhzApp::end() {
  if (ready_) {
    radio_.standby();
  }
}

void SubGhzApp::onEncoder(int delta) {
  if (!delta) {
    return;
  }

  band_ = (band_ + delta + 30) % 3;
  strongestRssi_ = -120.0f;
  strongestFreq_ = 0.0f;

  for (int i = 0; i < 64; ++i) {
    peaks_[i] = -120.0f;
  }

  if (ready_) {
    radio_.standby();
  }

  configureBand();

  const float center =
      band_ == 0
          ? 315.0f
          : (band_ == 1 ? 433.92f : 868.3f);

  ready_ =
      radio_.setFrequency(center) == RADIOLIB_ERR_NONE;

  if (ready_) {
    radio_.startReceive();
  }
}

void SubGhzApp::onSelect() {
  running_ = !running_;
}

void SubGhzApp::sampleStep() {
  if (!ready_ || !running_) {
    return;
  }

  const float freq = frequencyAt(cursor_);

  radio_.standby();

  if (radio_.setFrequency(freq) == RADIOLIB_ERR_NONE) {
    radio_.startReceive();

    delayMicroseconds(250);

    const float rssi = radio_.getRSSI();

    levels_[cursor_] = rssi;
    peaks_[cursor_] = max(peaks_[cursor_], rssi);

    if (rssi > strongestRssi_) {
      strongestRssi_ = rssi;
      strongestFreq_ = freq;
    }
  }

  ++cursor_;

  if (cursor_ >= 64) {
    cursor_ = 0;
    completeSweep();
  }
}

void SubGhzApp::completeSweep() {
  for (int i = 0; i < 64; ++i) {
    waterfall_[waterfallRow_][i] =
        static_cast<uint8_t>(
            constrain(
                map(
                    static_cast<int>(levels_[i]),
                    -110,
                    -35,
                    0,
                    15
                ),
                0,
                15
            )
        );
  }

  waterfallRow_ =
      (waterfallRow_ + 1) % 4;

  if (
      strongestRssi_ > -58.0f &&
      millis() - lastJournalMs_ > 5000 &&
      LittleFS.begin(false)
  ) {
    if (!LittleFS.exists("/logs")) {
      LittleFS.mkdir("/logs");
    }

    fs::File f =
        LittleFS.open(
            "/logs/subghz.csv",
            FILE_APPEND
        );

    if (f) {
      f.printf(
          "%lu,%.3f,%.1f\n",
          millis(),
          strongestFreq_,
          strongestRssi_
      );

      f.close();
      lastJournalMs_ = millis();
    }
  }
}

void SubGhzApp::tick(uint32_t nowMs) {
  (void)nowMs;

  const uint32_t nowUs = micros();

  if (nowUs - lastSampleUs_ >= 1500) {
    lastSampleUs_ = nowUs;
    sampleStep();
  }
}

void SubGhzApp::render(TFT_eSPI& tft) {
  tft.fillScreen(Theme::Bg);

  const char* bandName =
      band_ == 0
          ? "300-348"
          : (band_ == 1 ? "400-464" : "840-928");

  Ui::header(
      tft,
      "SUB-GHZ SPECTRUM",
      ready_ ? bandName : "CC1101 ERR"
  );

  tft.drawRect(
      8,
      32,
      304,
      82,
      Theme::PrimaryDim
  );

  for (int i = 0; i < 64; ++i) {
    const float rssi =
        constrain(
            levels_[i],
            -110.0f,
            -30.0f
        );

    const int h =
        map(
            static_cast<int>(rssi),
            -110,
            -30,
            1,
            72
        );

    const int x =
        11 + i * 4;

    tft.drawFastVLine(
        x,
        110 - h,
        h,
        rssi > -65
            ? Theme::PrimaryBright
            : Theme::PrimaryDim
    );

    const int py =
        110 -
        map(
            static_cast<int>(
                constrain(
                    peaks_[i],
                    -110.0f,
                    -30.0f
                )
            ),
            -110,
            -30,
            1,
            72
        );

    tft.drawPixel(
        x,
        py,
        Theme::Warning
    );
  }

  for (int row = 0; row < 4; ++row) {
    const int source =
        (waterfallRow_ + row) % 4;

    for (int i = 0; i < 64; ++i) {
      if (waterfall_[source][i] > 4) {
        tft.drawFastHLine(
            11 + i * 4,
            117 + row * 3,
            3,
            waterfall_[source][i] > 10
                ? Theme::PrimaryBright
                : Theme::PrimaryDim
        );
      }
    }
  }

  const String peakText =
      strongestFreq_ > 0
          ? String(strongestFreq_, 3) +
                " MHz  " +
                String(strongestRssi_, 0) +
                " dBm"
          : String("Waiting for sweep");

  Ui::text(
      tft,
      10,
      133,
      peakText,
      Theme::Muted,
      1
  );

  Ui::footer(
      tft,
      "TURN: band | USER: back",
      running_
          ? "ENC: pause"
          : "ENC: run"
  );
}
