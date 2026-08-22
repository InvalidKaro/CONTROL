#include "ThemeManager.h"

#include "theme.h"

namespace {
uint16_t c565(uint8_t r, uint8_t g, uint8_t b) {
  return static_cast<uint16_t>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}
}

void ThemeManager::begin() {
  if (prefs_.begin("control_theme", true)) {
    const uint8_t stored = prefs_.getUChar("theme", 0);
    prefs_.end();
    if (stored < themeCount()) theme_ = static_cast<ThemeId>(stored);
  }
  apply();
}

void ThemeManager::setTheme(ThemeId id) {
  if (static_cast<uint8_t>(id) >= themeCount()) id = ThemeId::ControlGreen;
  if (theme_ == id) return;
  theme_ = id;
  apply();
  save();
}

const char* ThemeManager::themeName() const { return themeName(theme_); }

const char* ThemeManager::themeName(ThemeId id) {
  switch (id) {
    case ThemeId::ControlGreen: return "Control Green";
    case ThemeId::AmberCrt: return "Amber CRT";
    case ThemeId::CyberBlue: return "Cyber Blue";
    case ThemeId::PurpleNeon: return "Purple Neon";
    case ThemeId::Monochrome: return "Monochrome";
    case ThemeId::RedAlert: return "Red Alert";
    case ThemeId::Count: default: return "Unknown";
  }
}

void ThemeManager::apply() {
  Theme::Bg = c565(2, 5, 3);
  Theme::Panel = c565(8, 17, 12);
  Theme::Text = c565(230, 255, 238);
  Theme::Muted = c565(118, 145, 126);
  Theme::Warning = c565(255, 210, 70);
  Theme::Error = c565(255, 80, 96);

  switch (theme_) {
    case ThemeId::ControlGreen:
      Theme::Primary = c565(0, 210, 85);
      Theme::PrimaryBright = c565(0, 255, 115);
      Theme::PrimaryDim = c565(16, 92, 48);
      break;
    case ThemeId::AmberCrt:
      Theme::Primary = c565(220, 140, 20);
      Theme::PrimaryBright = c565(255, 188, 58);
      Theme::PrimaryDim = c565(104, 66, 16);
      break;
    case ThemeId::CyberBlue:
      Theme::Primary = c565(0, 160, 255);
      Theme::PrimaryBright = c565(50, 210, 255);
      Theme::PrimaryDim = c565(16, 72, 110);
      break;
    case ThemeId::PurpleNeon:
      Theme::Primary = c565(190, 50, 255);
      Theme::PrimaryBright = c565(235, 105, 255);
      Theme::PrimaryDim = c565(78, 30, 108);
      break;
    case ThemeId::Monochrome:
      Theme::Primary = c565(180, 190, 184);
      Theme::PrimaryBright = c565(250, 255, 252);
      Theme::PrimaryDim = c565(80, 88, 83);
      break;
    case ThemeId::RedAlert:
      Theme::Primary = c565(215, 35, 50);
      Theme::PrimaryBright = c565(255, 72, 86);
      Theme::PrimaryDim = c565(110, 22, 30);
      break;
    case ThemeId::Count:
      break;
  }
}

void ThemeManager::save() {
  if (!prefs_.begin("control_theme", false)) return;
  prefs_.putUChar("theme", themeIndex());
  prefs_.end();
}
