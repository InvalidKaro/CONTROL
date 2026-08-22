#include "ToolboxApp.h"

#include "Ui.h"
#include "theme.h"

void ToolboxApp::begin() { mode_ = 0; running_ = false; }

void ToolboxApp::tick(uint32_t nowMs) {
  if (mode_ == 0 && running_) elapsedMs_ = nowMs - startedMs_;
  if (mode_ == 1 && running_ && static_cast<int32_t>((startedMs_ + timerSeconds_ * 1000UL) - nowMs) <= 0) running_ = false;
}

void ToolboxApp::onEncoder(int delta) {
  if (running_) return;
  if (mode_ == 1) {
    timerSeconds_ = constrain(timerSeconds_ + delta * 30, 30, 7200);
  } else if (mode_ == 2) {
    int64_t next = static_cast<int64_t>(value_) + delta;
    value_ = static_cast<uint32_t>(constrain(next, 0LL, 0xFFFFFFLL));
  } else {
    int next = static_cast<int>(mode_) + delta;
    while (next < 0) next += 3;
    mode_ = next % 3;
  }
}

void ToolboxApp::onSelect() {
  if (mode_ == 0) {
    if (!running_) { startedMs_ = millis() - elapsedMs_; running_ = true; }
    else running_ = false;
  } else if (mode_ == 1) {
    running_ = !running_;
    if (running_) startedMs_ = millis();
  } else {
    value_ = 0;
  }
}

void ToolboxApp::render(TFT_eSPI& tft) {
  tft.fillScreen(Theme::Bg);
  const char* names[] = {"STOPWATCH", "TIMER", "HEX/DEC/BIN"};
  Ui::header(tft, "UTILITY TOOLBOX", names[mode_]);
  if (mode_ == 0) {
    const uint32_t ms = running_ ? millis() - startedMs_ : elapsedMs_;
    char buf[24];
    snprintf(buf, sizeof(buf), "%02lu:%02lu.%03lu", (ms / 60000) % 100, (ms / 1000) % 60, ms % 1000);
    Ui::centered(tft, 70, buf, Theme::PrimaryBright, 4);
    Ui::centered(tft, 112, running_ ? "RUNNING" : "PAUSED", Theme::Muted, 1);
  } else if (mode_ == 1) {
    uint32_t remaining = timerSeconds_;
    if (running_) {
      const uint32_t passed = (millis() - startedMs_) / 1000;
      remaining = passed >= static_cast<uint32_t>(timerSeconds_) ? 0 : timerSeconds_ - passed;
    }
    char buf[16]; snprintf(buf, sizeof(buf), "%02lu:%02lu", remaining / 60, remaining % 60);
    Ui::centered(tft, 70, buf, remaining == 0 ? Theme::Warning : Theme::PrimaryBright, 4);
    Ui::centered(tft, 112, running_ ? "COUNTDOWN" : "TURN: +/-30 sec", Theme::Muted, 1);
  } else {
    Ui::panel(tft, 14, 37, 292, 88);
    Ui::text(tft, 24, 47, "DEC", Theme::Muted, 1); Ui::text(tft, 80, 45, String(value_), Theme::PrimaryBright, 2);
    Ui::text(tft, 24, 75, "HEX", Theme::Muted, 1); Ui::text(tft, 80, 73, "0x" + String(value_, HEX), Theme::Text, 2);
    Ui::text(tft, 24, 103, "BIN", Theme::Muted, 1); Ui::text(tft, 80, 101, String(value_, BIN), Theme::Text, 1);
  }
  Ui::footer(tft, running_ ? "ENC: pause" : "TURN: mode/value", "ENC: action");
}
