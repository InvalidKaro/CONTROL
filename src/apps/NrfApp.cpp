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

void NrfApp::end() {
  if (ready_) radio_.powerDown();
}

void NrfApp::onSelect() {
  running_ = !running_;
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
  channel_ = static_cast<uint8_t>((channel_ + 1) % 126);
}

void NrfApp::render(TFT_eSPI& tft) {
  tft.fillScreen(Theme::Bg);
  Ui::header(tft, "2.4G SCOPE", ready_ ? "nRF24" : "NRF ERR");
  tft.drawRect(8, 32, 304, 105, Theme::PrimaryDim);

  for (int x = 0; x < 252; ++x) {
    const int ch = x / 2;
    const int h = map(levels_[ch], 0, 100, 1, 96);
    const uint16_t color = levels_[ch] > 45 ? Theme::PrimaryBright : Theme::PrimaryDim;
    tft.drawFastVLine(10 + x, 134 - h, h, color);
  }

  Ui::text(tft, 268, 40, "2.400", Theme::Muted, 1);
  Ui::text(tft, 268, 62, "to", Theme::Muted, 1);
  Ui::text(tft, 268, 82, "2.525", Theme::Muted, 1);
  Ui::footer(tft, "Passive carrier map", running_ ? "ENC: pause" : "ENC: run");
}
