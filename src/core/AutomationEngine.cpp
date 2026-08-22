#include "AutomationEngine.h"

#include "LedController.h"
#include "PowerManager.h"

void AutomationEngine::begin() {
  if (!prefs_.begin("control_auto", true)) return;
  lowBatteryRule_ = prefs_.getBool("lowbat", true);
  webClientRule_ = prefs_.getBool("webcli", false);
  lowBatteryThreshold_ = constrain(prefs_.getUChar("lowpct", 15), 5, 50);
  prefs_.end();
}

void AutomationEngine::tick(uint32_t nowMs, uint8_t batterySoc, bool batteryPresent,
                            uint8_t webClients, LedController& leds, PowerManager& power) {
  if (nowMs - lastTickMs_ < 1000) return;
  lastTickMs_ = nowMs;

  if (lowBatteryRule_ && batteryPresent && batterySoc <= lowBatteryThreshold_) {
    if (!lowBatteryLatched_) {
      lowBatteryLatched_ = true;
      power.setProfile(PowerManager::Profile::Saver);
      leds.setEnabled(true);
      leds.setColor(255, 24, 24);
      leds.setEffect(LedEffect::Heartbeat);
      leds.setBrightness(70);
    }
  } else if (batterySoc > lowBatteryThreshold_ + 3) {
    lowBatteryLatched_ = false;
  }

  if (webClientRule_) {
    if (webClients > 0 && !clientLatched_) {
      clientLatched_ = true;
      leds.setEnabled(true);
      leds.setColor(0, 255, 90);
      leds.setEffect(LedEffect::CyberPulse);
    } else if (webClients == 0) {
      clientLatched_ = false;
    }
  }
}

void AutomationEngine::setLowBatteryRule(bool enabled) {
  lowBatteryRule_ = enabled;
  save();
}

void AutomationEngine::setWebClientRule(bool enabled) {
  webClientRule_ = enabled;
  save();
}

void AutomationEngine::setLowBatteryThreshold(uint8_t threshold) {
  lowBatteryThreshold_ = constrain(threshold, 5, 50);
  save();
}

void AutomationEngine::save() {
  if (!prefs_.begin("control_auto", false)) return;
  prefs_.putBool("lowbat", lowBatteryRule_);
  prefs_.putBool("webcli", webClientRule_);
  prefs_.putUChar("lowpct", lowBatteryThreshold_);
  prefs_.end();
}
