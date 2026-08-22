#pragma once

#include <Arduino.h>
#include <Preferences.h>

class PowerManager {
 public:
  enum class Profile : uint8_t { Performance = 0, Balanced, Saver, Stealth, Count };

  struct BatteryState {
    bool present = false;
    float voltage = 0.0f;
    float currentMa = 0.0f;
    float temperatureC = 0.0f;
    uint8_t soc = 0;
    uint16_t remainingMah = 0;
    uint16_t fullMah = 0;
  };

  void begin();
  void tick(uint32_t nowMs);
  void setProfile(Profile profile);
  void setProfile(uint8_t profile) { setProfile(static_cast<Profile>(profile % profileCount())); }
  Profile profile() const { return profile_; }
  const char* profileName() const;
  static const char* profileName(Profile p);
  static constexpr uint8_t profileCount() { return static_cast<uint8_t>(Profile::Count); }
  const BatteryState& battery() const { return battery_; }

 private:
  bool readWord(uint8_t reg, uint16_t& value);
  void readBattery();
  void applyProfile();
  void save();

  BatteryState battery_{};
  Profile profile_ = Profile::Balanced;
  uint32_t lastBatteryMs_ = 0;
  Preferences prefs_;
};
