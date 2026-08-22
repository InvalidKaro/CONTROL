#include "IrApp.h"

#include "Ui.h"
#include "board_pins.h"
#include "theme.h"

IrApp::IrApp() : receiver_(BoardPins::IrRx, 1024, 15, true), sender_(BoardPins::IrTx) {}

void IrApp::loadLearned() {
  prefs_.begin("control_ir", true);
  const uint16_t rawType = prefs_.getUShort("type", static_cast<uint16_t>(decode_type_t::UNKNOWN));
  learnedType_ = static_cast<decode_type_t>(rawType);
  learnedValue_ = prefs_.getULong64("value", 0);
  learnedBits_ = prefs_.getUShort("bits", 0);
  prefs_.end();
  if (learnedBits_) {
    protocol_ = typeToString(learnedType_, false);
    value_ = "0x" + String(static_cast<uint32_t>(learnedValue_ >> 32), HEX) + String(static_cast<uint32_t>(learnedValue_), HEX);
    value_.toUpperCase();
    bits_ = learnedBits_;
  }
}

void IrApp::saveLearned() {
  prefs_.begin("control_ir", false);
  prefs_.putUShort("type", static_cast<uint16_t>(learnedType_));
  prefs_.putULong64("value", learnedValue_);
  prefs_.putUShort("bits", learnedBits_);
  prefs_.end();
}

void IrApp::begin() {
  loadLearned();
  sender_.begin();
  receiver_.enableIRIn();
}

void IrApp::end() { receiver_.disableIRIn(); }

void IrApp::onEncoder(int delta) {
  if (delta == 0) return;
  mode_ ^= 1U;
}

void IrApp::onSelect() {
  if (mode_ == 1) replay();
}

void IrApp::replay() {
  if (learnedType_ == decode_type_t::UNKNOWN || learnedBits_ == 0) return;
  receiver_.disableIRIn();
  delay(3);
  lastSendOk_ = sender_.send(learnedType_, learnedValue_, learnedBits_);
  lastReplayMs_ = millis();
  delay(3);
  receiver_.enableIRIn();
}

void IrApp::tick(uint32_t nowMs) {
  (void)nowMs;
  if (!receiver_.decode(&results_)) return;

  protocol_ = typeToString(results_.decode_type, results_.repeat);
  value_ = resultToHexidecimal(&results_);
  bits_ = results_.bits;
  ++frames_;

  if (results_.decode_type != decode_type_t::UNKNOWN && results_.bits > 0 && results_.bits <= 64 && !results_.repeat) {
    const bool changed = learnedType_ != results_.decode_type || learnedValue_ != results_.value || learnedBits_ != results_.bits;
    learnedType_ = results_.decode_type;
    learnedValue_ = results_.value;
    learnedBits_ = results_.bits;
    if (changed) saveLearned();
  }
  receiver_.resume();
}

void IrApp::render(TFT_eSPI& tft) {
  tft.fillScreen(Theme::Bg);
  Ui::header(tft, "IR STUDIO", mode_ == 0 ? "LEARN" : "REPLAY");
  Ui::panel(tft, 10, 34, 300, 92);
  Ui::text(tft, 20, 42, "Protocol", Theme::Muted, 1);
  Ui::text(tft, 120, 42, protocol_, Theme::PrimaryBright, 2);
  Ui::text(tft, 20, 70, "Value", Theme::Muted, 1);
  Ui::text(tft, 120, 70, value_, Theme::Text, 2);
  Ui::text(tft, 20, 98, "Bits", Theme::Muted, 1);
  Ui::text(tft, 120, 98, String(bits_), Theme::Text, 2);
  Ui::text(tft, 220, 98, learnedBits_ ? "NVS SAVED" : "WAITING", learnedBits_ ? Theme::PrimaryBright : Theme::Muted, 1);
  if (lastReplayMs_ && millis() - lastReplayMs_ < 1500) Ui::text(tft, 220, 42, lastSendOk_ ? "TX OK" : "TX ERR", lastSendOk_ ? Theme::PrimaryBright : Theme::Error, 1);
  Ui::footer(tft, "TURN: Learn/Replay", mode_ == 1 ? "ENC: send learned" : "Point remote at IR RX");
}
