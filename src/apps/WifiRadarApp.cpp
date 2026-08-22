#include "WifiRadarApp.h"

#include "Ui.h"
#include "theme.h"

void WifiRadarApp::begin() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(false, false);
  requested_ = true;
}

void WifiRadarApp::end() {
  WiFi.scanDelete();
}

void WifiRadarApp::onSelect() {
  requested_ = true;
}

void WifiRadarApp::scan() {
  WiFi.scanDelete();
  count_ = WiFi.scanNetworks(false, true, false, 250);
  lastScanMs_ = millis();
}

void WifiRadarApp::tick(uint32_t nowMs) {
  if (requested_ && nowMs - lastScanMs_ > 500) {
    requested_ = false;
    scan();
  }
}

void WifiRadarApp::render(TFT_eSPI& tft) {
  tft.fillScreen(Theme::Bg);
  Ui::header(tft, "WI-FI RADAR", String(count_) + " APs");

  const int rows = min(count_, 5);
  for (int i = 0; i < rows; ++i) {
    const int y = 31 + i * 22;
    Ui::panel(tft, 7, y, 306, 18);
    String ssid = WiFi.SSID(i);
    if (ssid.isEmpty()) ssid = "<hidden>";
    if (ssid.length() > 22) ssid = ssid.substring(0, 22);
    Ui::text(tft, 13, y + 2, ssid, Theme::Text, 1);
    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(WiFi.RSSI(i) > -65 ? Theme::PrimaryBright : Theme::Muted, Theme::Panel);
    tft.drawString(String(WiFi.RSSI(i)) + " dBm", 305, y + 2, 1);
    tft.setTextDatum(TL_DATUM);
  }

  if (count_ <= 0) Ui::centered(tft, 76, "Press encoder to scan", Theme::Muted, 2);
  Ui::footer(tft, "USER: back", "ENC: rescan");
}
