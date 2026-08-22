#pragma once

#include <Arduino.h>
#include <Preferences.h>

class LedController;
class PowerManager;

class AutomationEngine {
 public:
  void begin();
  void tick(uint32_t nowMs, uint8_t batterySoc, bool batteryPresent, uint8_t webClients,
            LedController& leds, PowerManager& power);

  bool lowBatteryRule() const { return lowBatteryRule_; }
  bool webClientRule() const { return webClientRule_; }
  void setLowBatteryRule(bool enabled);
  void setWebClientRule(bool enabled);
  uint8_t lowBatteryThreshold() const { return lowBatteryThreshold_; }
  void setLowBatteryThreshold(uint8_t threshold);

 private:
  void save();
  Preferences prefs_;
  bool lowBatteryRule_ = true;
  bool webClientRule_ = false;
  bool lowBatteryLatched_ = false;
  bool clientLatched_ = false;
  uint8_t lowBatteryThreshold_ = 15;
  uint32_t lastTickMs_ = 0;
};
