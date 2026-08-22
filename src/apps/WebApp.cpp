#include "WebApp.h"

#include "Ui.h"
#include "theme.h"

void WebApp::render(TFT_eSPI& tft) {
  tft.fillScreen(Theme::Bg);
  Ui::header(tft, "WEB CONTROL", web_.running() ? "ONLINE" : "OFFLINE");

  Ui::panel(tft, 8, 32, 148, 42);
  Ui::text(tft, 15, 39, "ADDRESS", Theme::Muted, 1);
  Ui::text(tft, 15, 54, web_.ip(), Theme::PrimaryBright, 2);

  Ui::panel(tft, 164, 32, 148, 42);
  Ui::text(tft, 171, 39, "CLIENTS", Theme::Muted, 1);
  Ui::text(tft, 171, 54, String(web_.clientCount()), Theme::PrimaryBright, 2);

  Ui::panel(tft, 8, 82, 304, 47);
  Ui::text(tft, 15, 89, "SSID", Theme::Muted, 1);
  Ui::text(tft, 66, 89, web_.ssid(), Theme::Text, 1);
  Ui::text(tft, 15, 104, "LOGIN", Theme::Muted, 1);
  Ui::text(tft, 66, 104, web_.webUser() + " / " + web_.webPassword(), Theme::Text, 1);
  Ui::text(tft, 15, 118, "AP PASS", Theme::Muted, 1);
  Ui::text(tft, 74, 118, "controlos", Theme::Text, 1);

  Ui::footer(tft, "Browser: 172.0.0.1", "USER: back");
}
