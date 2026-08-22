#include "WifiRadarApp.h"

#include <algorithm>
#include "Ui.h"
#include "theme.h"

void WifiRadarApp::begin() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.disconnect(false, false);
  requested_ = true;
  scanInProgress_ = false;
}

void WifiRadarApp::end() {
  if (scanInProgress_) WiFi.scanDelete();
  scanInProgress_ = false;
}

void WifiRadarApp::onEncoder(int delta) {
  if (!delta) return;
  int next = static_cast<int>(mode_) + delta;
  while (next < 0) next += 3;
  mode_ = next % 3;
}

void WifiRadarApp::onSelect() {
  if (!scanInProgress_) requested_ = true;
}

void WifiRadarApp::startScan() {
  WiFi.scanDelete();
  const int result = WiFi.scanNetworks(true, true, false, 120);
  if (result == WIFI_SCAN_RUNNING) {
    scanInProgress_ = true;
  } else if (result >= 0) {
    captureResults(result);
    scanInProgress_ = false;
    lastScanMs_ = millis();
  }
}

void WifiRadarApp::captureResults(int found) {
  count_ = min(found, 24);
  memset(channelUse_, 0, sizeof(channelUse_));
  for (int i = 0; i < count_; ++i) {
    aps_[i].ssid = WiFi.SSID(i);
    if (aps_[i].ssid.isEmpty()) aps_[i].ssid = "<hidden>";
    aps_[i].bssid = WiFi.BSSIDstr(i);
    aps_[i].rssi = WiFi.RSSI(i);
    aps_[i].channel = WiFi.channel(i);
    aps_[i].encryption = static_cast<uint8_t>(WiFi.encryptionType(i));
    if (aps_[i].channel >= 1 && aps_[i].channel <= 13) ++channelUse_[aps_[i].channel];
  }
  std::sort(aps_, aps_ + count_, [](const AccessPoint& a, const AccessPoint& b) { return a.rssi > b.rssi; });
  WiFi.scanDelete();
}

void WifiRadarApp::tick(uint32_t nowMs) {
  if (requested_ && !scanInProgress_ && nowMs - lastScanMs_ > 400) {
    requested_ = false;
    startScan();
  }
  if (!scanInProgress_) return;
  const int result = WiFi.scanComplete();
  if (result >= 0) {
    captureResults(result);
    scanInProgress_ = false;
    lastScanMs_ = nowMs;
  } else if (result == WIFI_SCAN_FAILED) {
    scanInProgress_ = false;
    lastScanMs_ = nowMs;
  }
}

String WifiRadarApp::authName(uint8_t auth) const {
  if (auth == WIFI_AUTH_OPEN) return "OPEN";
  if (auth == WIFI_AUTH_WEP) return "WEP";
  if (auth == WIFI_AUTH_WPA_PSK) return "WPA";
  if (auth == WIFI_AUTH_WPA2_PSK) return "WPA2";
#ifdef WIFI_AUTH_WPA3_PSK
  if (auth == WIFI_AUTH_WPA3_PSK) return "WPA3";
#endif
  return "SEC";
}

void WifiRadarApp::render(TFT_eSPI& tft) {
  tft.fillScreen(Theme::Bg);
  const char* modes[] = {"NETWORKS", "CHANNEL MAP", "SIGNAL FINDER"};
  Ui::header(tft, "WI-FI ANALYZER", scanInProgress_ ? "SCANNING" : modes[mode_]);

  if (mode_ == 0) {
    const int rows = min(count_, 5);
    for (int i = 0; i < rows; ++i) {
      const int y = 31 + i * 22;
      Ui::panel(tft, 7, y, 306, 18);
      String ssid = aps_[i].ssid;
      if (ssid.length() > 17) ssid = ssid.substring(0, 17);
      Ui::text(tft, 13, y + 2, ssid, Theme::Text, 1);
      Ui::text(tft, 145, y + 2, "CH" + String(aps_[i].channel), Theme::Muted, 1);
      Ui::text(tft, 187, y + 2, authName(aps_[i].encryption), Theme::Muted, 1);
      tft.setTextDatum(TR_DATUM);
      tft.setTextColor(aps_[i].rssi > -65 ? Theme::PrimaryBright : Theme::Muted, Theme::Panel);
      tft.drawString(String(aps_[i].rssi), 305, y + 2, 1);
      tft.setTextDatum(TL_DATUM);
    }
  } else if (mode_ == 1) {
    tft.drawRect(8, 32, 304, 96, Theme::PrimaryDim);
    uint8_t maxUse = 1;
    for (int ch = 1; ch <= 13; ++ch) maxUse = max(maxUse, channelUse_[ch]);
    for (int ch = 1; ch <= 13; ++ch) {
      const int h = map(channelUse_[ch], 0, maxUse, 2, 72);
      const int x = 14 + (ch - 1) * 22;
      tft.fillRect(x, 120 - h, 14, h, channelUse_[ch] == 0 ? Theme::PrimaryDim : Theme::Primary);
      Ui::text(tft, x + 2, 124, String(ch), Theme::Muted, 1);
    }
  } else {
    if (!count_) {
      Ui::centered(tft, 74, "No AP data", Theme::Muted, 2);
    } else {
      const auto& ap = aps_[0];
      Ui::centered(tft, 45, ap.ssid, Theme::Text, 2);
      Ui::centered(tft, 73, String(ap.rssi) + " dBm", ap.rssi > -60 ? Theme::PrimaryBright : Theme::Warning, 4);
      const int width = constrain(map(ap.rssi, -95, -30, 8, 286), 8, 286);
      tft.drawRect(16, 111, 288, 13, Theme::PrimaryDim);
      tft.fillRect(17, 112, width, 11, Theme::Primary);
      Ui::text(tft, 16, 129, ap.bssid + "  CH" + String(ap.channel), Theme::Muted, 1);
    }
  }
  Ui::footer(tft, "TURN: view", "ENC: rescan");
}
