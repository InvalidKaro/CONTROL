#include "IrApp.h"

#include "Ui.h"
#include "board_pins.h"
#include "theme.h"

IrApp::IrApp() : receiver_(BoardPins::IrRx, 1024, 15, true) {}

void IrApp::begin() {
  receiver_.enableIRIn();
}

void IrApp::end() {
  receiver_.disableIRIn();
}

void IrApp::tick(uint32_t nowMs) {
  (void)nowMs;
  if (!receiver_.decode(&results_)) return;

  protocol_ = typeToString(results_.decode_type, results_.repeat);
  value_ = resultToHexidecimal(&results_);
  bits_ = results_.bits;
  ++frames_;
  receiver_.resume();
}

void IrApp::render(TFT_eSPI& tft) {
  tft.fillScreen(Theme::Bg);
  Ui::header(tft, "IR SCOPE", String(frames_) + " frames");
  Ui::panel(tft, 10, 34, 300, 100);
  Ui::text(tft, 20, 44, "Protocol", Theme::Muted, 1);
  Ui::text(tft, 120, 44, protocol_, Theme::PrimaryBright, 2);
  Ui::text(tft, 20, 74, "Value", Theme::Muted, 1);
  Ui::text(tft, 120, 74, value_, Theme::Text, 2);
  Ui::text(tft, 20, 105, "Bits", Theme::Muted, 1);
  Ui::text(tft, 120, 105, String(bits_), Theme::Text, 2);
  Ui::footer(tft, "Passive IR receive", "USER: back");
}
