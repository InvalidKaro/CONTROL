#pragma once

#include <Arduino.h>
#include <Preferences.h>

class ThemeManager {
 public:
  enum class ThemeId : uint8_t {
    ControlGreen = 0,
    AmberCrt,
    CyberBlue,
    PurpleNeon,
    Monochrome,
    RedAlert,
    Count,
  };

  void begin();
  void setTheme(ThemeId id);
  void setTheme(uint8_t id) { setTheme(static_cast<ThemeId>(id % themeCount())); }
  uint8_t themeIndex() const { return static_cast<uint8_t>(theme_); }
  const char* themeName() const;
  static constexpr uint8_t themeCount() { return static_cast<uint8_t>(ThemeId::Count); }
  static const char* themeName(ThemeId id);

 private:
  void apply();
  void save();
  ThemeId theme_ = ThemeId::ControlGreen;
  Preferences prefs_;
};
