#include "SettingsApp.h"

#include "Ui.h"
#include "theme.h"

void SettingsApp::onSelect() { editing_ = !editing_; }

void SettingsApp::onEncoder(int delta) {
  if (!delta) return;
  if (!editing_) {
    int next = static_cast<int>(row_) + delta;
    while (next < 0) next += RowCount;
    row_ = next % RowCount;
    return;
  }
  switch (row_) {
    case 0: {
      int next = static_cast<int>(themes_.themeIndex()) + delta;
      while (next < 0) next += ThemeManager::themeCount();
      themes_.setTheme(static_cast<uint8_t>(next % ThemeManager::themeCount()));
      break;
    }
    case 1: {
      int next = static_cast<int>(power_.profile()) + delta;
      while (next < 0) next += PowerManager::profileCount();
      power_.setProfile(static_cast<uint8_t>(next % PowerManager::profileCount()));
      break;
    }
    case 2:
      automations_.setLowBatteryRule(!automations_.lowBatteryRule());
      break;
    case 3:
      automations_.setLowBatteryThreshold(static_cast<uint8_t>(constrain(static_cast<int>(automations_.lowBatteryThreshold()) + delta, 5, 50)));
      break;
    case 4:
      automations_.setWebClientRule(!automations_.webClientRule());
      break;
  }
}

void SettingsApp::render(TFT_eSPI& tft) {
  tft.fillScreen(Theme::Bg);
  Ui::header(tft, "SETTINGS", editing_ ? "EDIT" : "SELECT");
  const char* labels[RowCount] = {"THEME", "POWER", "LOW BAT RULE", "LOW BAT %", "WEB LED RULE"};
  String values[RowCount] = {
      themes_.themeName(), power_.profileName(), automations_.lowBatteryRule() ? "ON" : "OFF",
      String(automations_.lowBatteryThreshold()) + "%", automations_.webClientRule() ? "ON" : "OFF"};
  for (uint8_t i = 0; i < RowCount; ++i) {
    const int y = 31 + i * 22;
    if (i == row_) Ui::panel(tft, 7, y - 2, 306, 20, editing_);
    Ui::text(tft, 13, y + 3, labels[i], i == row_ ? Theme::PrimaryBright : Theme::Muted, 1);
    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(i == row_ ? Theme::Text : Theme::Muted, i == row_ ? Theme::Panel : Theme::Bg);
    tft.drawString(values[i], 306, y + 3, 1);
    tft.setTextDatum(TL_DATUM);
  }
  Ui::footer(tft, editing_ ? "TURN: change" : "TURN: select", "ENC: toggle/edit");
}
