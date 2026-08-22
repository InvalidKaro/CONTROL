#include "DashboardApp.h"

#include <WiFi.h>
#include "Ui.h"
#include "theme.h"

void DashboardApp::render(TFT_eSPI& tft) {
  tft.fillScreen(Theme::Bg);
  Ui::header(tft, "CONTROL//OS", "LIVE DECK");
  const auto& b = power_.battery();

  Ui::panel(tft, 8, 31, 94, 44);
  Ui::text(tft, 14, 37, "BATTERY", Theme::Muted, 1);
  Ui::text(tft, 14, 52, b.present ? String(b.soc) + "%" : "N/A", Theme::PrimaryBright, 2);

  Ui::panel(tft, 113, 31, 94, 44);
  Ui::text(tft, 119, 37, "HEAP", Theme::Muted, 1);
  Ui::text(tft, 119, 52, String(ESP.getFreeHeap() / 1024) + "K", Theme::PrimaryBright, 2);

  Ui::panel(tft, 218, 31, 94, 44);
  Ui::text(tft, 224, 37, "WEB", Theme::Muted, 1);
  Ui::text(tft, 224, 52, String(web_.clientCount()), Theme::PrimaryBright, 2);

  Ui::panel(tft, 8, 84, 304, 45);
  Ui::text(tft, 15, 91, "POWER", Theme::Muted, 1);
  Ui::text(tft, 69, 91, power_.profileName(), Theme::Text, 1);
  Ui::text(tft, 15, 107, "WEBUI", Theme::Muted, 1);
  Ui::text(tft, 69, 107, web_.ip(), Theme::PrimaryBright, 1);
  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(Theme::Muted, Theme::Panel);
  tft.drawString(String(millis() / 60000) + " min", 303, 107, 1);
  tft.setTextDatum(TL_DATUM);

  Ui::footer(tft, "Live system overview", "USER: back");
}
