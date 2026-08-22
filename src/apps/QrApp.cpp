#include "QrApp.h"

#include <qrcode.h>
#include <WiFi.h>

#include "Ui.h"
#include "theme.h"

namespace {
void drawQr(TFT_eSPI& tft, const String& payload, int x, int y, int maxSize, bool invert) {
  QRCode qr;
  uint8_t data[qrcode_getBufferSize(4)];
  qrcode_initText(&qr, data, 4, ECC_LOW, payload.c_str());

  const int scale = max(1, maxSize / qr.size);
  const int side = qr.size * scale;
  const uint16_t bg = invert ? Theme::PrimaryBright : Theme::Bg;
  const uint16_t fg = invert ? Theme::Bg : Theme::PrimaryBright;
  tft.fillRect(x, y, side + 8, side + 8, bg);
  for (uint8_t row = 0; row < qr.size; ++row) {
    for (uint8_t col = 0; col < qr.size; ++col) {
      if (qrcode_getModule(&qr, col, row)) {
        tft.fillRect(x + 4 + col * scale, y + 4 + row * scale, scale, scale, fg);
      }
    }
  }
}
}  // namespace

void QrApp::begin() { mode_ = 0; }
void QrApp::tick(uint32_t nowMs) { (void)nowMs; }

void QrApp::onEncoder(int delta) {
  if (!delta) return;
  int next = static_cast<int>(mode_) + (delta > 0 ? 1 : -1);
  while (next < 0) next += 2;
  mode_ = static_cast<uint8_t>(next % 2);
}

void QrApp::onSelect() { invert_ = !invert_; }

void QrApp::render(TFT_eSPI& tft) {
  tft.fillScreen(Theme::Bg);
  const String apSsid = WiFi.softAPSSID().length() ? WiFi.softAPSSID() : "ControlOS";
  const String payload = mode_ == 0 ? "http://172.0.0.1" : "WIFI:T:WPA;S:" + apSsid + ";P:controlos;;";
  Ui::header(tft, "QR & SHARE", mode_ == 0 ? "WEBUI" : "WI-FI");
  drawQr(tft, payload, 101, 30, 112, invert_);
  Ui::centered(tft, 145, mode_ == 0 ? "172.0.0.1" : apSsid + " / controlos", Theme::Text, 1);
  Ui::footer(tft, "TURN: Web/Wi-Fi", "ENC: invert");
}
