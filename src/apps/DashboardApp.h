#pragma once

#include "PowerManager.h"
#include "WebUi.h"
#include "app.h"

class DashboardApp final : public App {
 public:
  DashboardApp(PowerManager& power, WebUi& web) : power_(power), web_(web) {}
  const char* name() const override { return "Dashboard"; }
  const char* shortName() const override { return "HOME"; }
  void tick(uint32_t nowMs) override { (void)nowMs; }
  void render(TFT_eSPI& tft) override;

 private:
  PowerManager& power_;
  WebUi& web_;
};
