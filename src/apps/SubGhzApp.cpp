#include "SubGhzApp.h"

#include "Ui.h"
#include "board_pins.h"
#include "theme.h"

SubGhzApp::SubGhzApp()
    : module_(BoardPins::Cc1101Cs, BoardPins::Cc1101Gdo0, RADIOLIB_NC, BoardPins::Cc1101Gdo2),
      radio_(&module_) {}

void SubGhzApp::configureBand() {
  digitalWrite(BoardPins::Cc1101Switch1, band_ == 2 ? LOW : HIGH);
  digitalWrite(BoardPins::Cc1101Switch0, band_ == 0 ? LOW : HIGH);
}

void SubGhzApp::begin() {
  pinMode(BoardPins::Cc1101Switch1, OUTPUT);
  pinMode(BoardPins::Cc1101Switch0, OUTPUT);
  configureBand();

  const float initial = band_ == 0 ? 315.0f : (band_ == 1 ? 433.92f : 868.3f);
  ready_ = radio_.begin(initial) == RADIOLIB_ERR_NONE;
  if (ready_) radio_.startReceive();
}

void SubGhzApp::end() {
  if (ready_) radio_.standby();
}

void SubGhzApp::onEncoder(int delta) {
  if (delta == 0) return;
  band_ = (band_ + delta + 3) % 3;
  if (ready_) radio_.standby();
  configureBand();
  const float center = band_ == 0 ? 315.0f : (band_ == 1 ? 433.92f : 868.3f);
  ready_ = radio_.setFrequency(center) == RADIOLIB_ERR_NONE;
  if (ready_) radio_.startReceive();
}

void SubGhzApp::onSelect() {
  running_ = !running_;
}

void SubGhzApp::sampleStep() {
  if (!ready_ || !running_) return;

  const float start = band_ == 0 ? 300.0f : (band_ == 1 ? 400.0f : 840.0f);
  const float span = band_ == 0 ? 48.0f : (band_ == 1 ? 64.0f : 88.0f);
  const float freq = start + (span * cursor_ / 63.0f);

  radio_.standby();
  if (radio_.setFrequency(freq) == RADIOLIB_ERR_NONE) {
    radio_.startReceive();
    delayMicroseconds(250);
    levels_[cursor_] = radio_.getRSSI();
  }
  cursor_ = (cursor_ + 1) % 64;
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
  const char* bandName = band_ == 0 ? "315" : (band_ == 1 ? "433" : "868");
  Ui::header(tft, "SUB-GHZ SCOPE", ready_ ? bandName : "CC1101 ERR");

  tft.drawRect(8, 32, 304, 104, Theme::PrimaryDim);
  for (int i = 0; i < 64; ++i) {
    const float rssi = constrain(levels_[i], -110.0f, -30.0f);
    const int h = map(static_cast<int>(rssi), -110, -30, 1, 96);
    const int x = 11 + i * 4;
    tft.drawFastVLine(x, 133 - h, h, rssi > -65 ? Theme::PrimaryBright : Theme::PrimaryDim);
    tft.drawFastVLine(x + 1, 133 - h, h, Theme::Primary);
  }

  Ui::footer(tft, "TURN: band | USER: back", running_ ? "ENC: pause" : "ENC: run");
}
