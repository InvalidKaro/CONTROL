#pragma once

#include "WebUi.h"
#include "app.h"

class WebApp final : public App {
 public:
  explicit WebApp(WebUi& web) : web_(web) {}

  const char* name() const override { return "Web Control"; }
  const char* shortName() const override { return "WEB"; }
  void tick(uint32_t nowMs) override { (void)nowMs; }
  void render(TFT_eSPI& tft) override;

 private:
  WebUi& web_;
};
