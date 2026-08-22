#include "NrfApp.h"

#include "Ui.h"
#include "board_pins.h"
#include "theme.h"

NrfApp::NrfApp() : radio_(BoardPins::NrfCe, BoardPins::NrfCs) {}

void NrfApp::begin() {
  ready_ = radio_.begin();
  if (!ready_) return;
  radio_.setAutoAck(false);
  radio_.setPALevel(RF24_PA_MIN);
  radio_.setDataRate(RF24_1MBPS);
}

void NrfApp::end() { if (ready_) radio_.powerDown(); }
void NrfApp::onSelect() { running_ = !running_; }
void NrfApp::onEncoder(int delta) {
  if (!delta) return;
  int next = static_cast<int>(mode_) + delta;
  while (next < 0) next += 2;
  mode_ = next % 2;
}

void NrfApp::tick(uint32_t nowMs) {
  (void)nowMs;
  if (!ready_ || !running_) return;
  const uint32_t nowUs = micros();
  if (nowUs - lastStepUs_ < 600) return;
  lastStepUs_ = nowUs;

  radio_.setChannel(channel_);
  radio_.startListening();
  delayMicroseconds(150);
  const bool carrier = radio_.testCarrier();
  radio_.stopListening();

  uint8_t& level = levels_[channel_];
  level = static_cast<uint8_t>((level * 7 + (carrier ? 100 : 0)) / 8);
  peaks_[channel_] = max(peaks_[channel_], level);
  ++samples_[channel_];
  if (carrier) ++hits_[channel_];
  channel_ = static_cast<uint8_t>((channel_ + 1) % 126);
}

void NrfApp::render(TFT_eSPI& tft) {
  tft.fillScreen(Theme::Bg);
  Ui::header(tft, "2.4G CHANNEL LAB", ready_ ? (mode_ == 0 ? "LIVE" : "BUSY %") : "NRF ERR");
  tft.drawRect(8, 32, 304, 101, Theme::PrimaryDim);

  if (mode_ == 0) {
    for (int x = 0; x < 252; ++x) {
      const int ch = x / 2;
      const int h = map(levels_[ch], 0, 100, 1, 89);
      tft.drawFastVLine(10 + x, 129 - h, h, levels_[ch] > 45 ? Theme::PrimaryBright : Theme::PrimaryDim);
      const int peakY = 129 - map(peaks_[ch], 0, 100, 1, 89);
      tft.drawPixel(10 + x, peakY, Theme::Warning);
    }
  } else {
    for (int x = 0; x < 252; ++x) {
      const int ch = x / 2;
      const uint8_t busy = samples_[ch] ? static_cast<uint8_t>((hits_[ch] * 100UL) / samples_[ch]) : 0;
      const int h = map(busy, 0, 100, 1, 89);
      tft.drawFastVLine(10 + x, 129 - h, h, busy > 20 ? Theme::PrimaryBright : Theme::PrimaryDim);
    }
  }

  Ui::text(tft, 268, 42, "2400", Theme::Muted, 1);
  Ui::text(tft, 268, 69, "MHz", Theme::Muted, 1);
  Ui::text(tft, 268, 96, "2525", Theme::Muted, 1);
  Ui::footer(tft, "TURN: live/busy", running_ ? "ENC: pause" : "ENC: run");
}
