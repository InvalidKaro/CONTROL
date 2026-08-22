#include "QuickActionsApp.h"

#include "Ui.h"
#include <esp_sleep.h>
#include "board_pins.h"
#include "theme.h"

void QuickActionsApp::begin() { selected_ = 0; status_ = "Ready"; }
void QuickActionsApp::tick(uint32_t nowMs) { if (statusUntilMs_ && nowMs > statusUntilMs_) status_ = "Ready"; }

void QuickActionsApp::onEncoder(int delta) {
  if (!delta) return;
  int next = static_cast<int>(selected_) + delta;
  while (next < 0) next += ActionCount;
  selected_ = static_cast<uint8_t>(next % ActionCount);
}

void QuickActionsApp::onSelect() {
  switch (selected_) {
    case 0: leds_.setEnabled(true); leds_.setEffect(LedEffect::Matrix); status_ = "LED -> Matrix"; break;
    case 1: leds_.setEnabled(false); status_ = "LED -> Off"; break;
    case 2: power_.setProfile(PowerManager::Profile::Performance); status_ = "Power -> Performance"; break;
    case 3: power_.setProfile(PowerManager::Profile::Saver); status_ = "Power -> Saver"; break;
    case 4: themes_.setTheme(static_cast<uint8_t>(themes_.themeIndex() + 1)); status_ = "Theme -> " + String(themes_.themeName()); break;
    case 5: sounds_.setEnabled(!sounds_.enabled()); status_ = sounds_.enabled() ? "Sound -> On" : "Sound -> Off"; break;
    case 6: leds_.setEnabled(true); leds_.setEffect(LedEffect::CyberPulse); leds_.setColor(0, 255, 65); status_ = "Cyber mode"; break;
    case 7: leds_.setEnabled(true); leds_.setEffect(LedEffect::Heartbeat); leds_.setColor(255, 24, 24); status_ = "Alert mode"; break;
    case 8:
      status_ = "Deep sleep 30s";
      leds_.setEnabled(false);
      delay(60);
      esp_sleep_enable_timer_wakeup(30ULL * 1000000ULL);
      esp_sleep_enable_ext0_wakeup(static_cast<gpio_num_t>(BoardPins::UserKey), 0);
      esp_deep_sleep_start();
      break;
  }
  statusUntilMs_ = millis() + 1500;
  sounds_.success();
}

void QuickActionsApp::render(TFT_eSPI& tft) {
  static const char* labels[ActionCount] = {
      "LED: Matrix", "LED: Off", "Power: Performance", "Power: Saver",
      "Theme: Next", "Sound: Toggle", "Preset: Cyber", "Preset: Alert", "Deep Sleep: 30s"};
  tft.fillScreen(Theme::Bg);
  Ui::header(tft, "COMMAND PALETTE", status_);
  const uint8_t first = selected_ > 3 ? selected_ - 3 : 0;
  for (uint8_t row = 0; row < 5 && first + row < ActionCount; ++row) {
    const uint8_t idx = first + row;
    const int y = 32 + row * 22;
    Ui::panel(tft, 10, y, 300, 19, idx == selected_);
    Ui::text(tft, 17, y + 3, String(idx == selected_ ? "> " : "  ") + labels[idx],
             idx == selected_ ? Theme::PrimaryBright : Theme::Text, 1);
  }
  Ui::footer(tft, "TURN: command", "ENC: execute");
}
