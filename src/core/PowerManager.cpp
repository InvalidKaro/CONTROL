#include "PowerManager.h"

#include <Wire.h>
#include "board_pins.h"

namespace {
constexpr uint8_t kGauge = 0x55;
}

void PowerManager::begin() {
  if (prefs_.begin("control_power", true)) {
    const uint8_t stored = prefs_.getUChar("profile", static_cast<uint8_t>(Profile::Balanced));
    prefs_.end();
    if (stored < profileCount()) profile_ = static_cast<Profile>(stored);
  }
  applyProfile();
  readBattery();
}

void PowerManager::tick(uint32_t nowMs) {
  if (nowMs - lastBatteryMs_ >= 2000) {
    lastBatteryMs_ = nowMs;
    readBattery();
  }
}

void PowerManager::setProfile(Profile profile) {
  if (static_cast<uint8_t>(profile) >= profileCount()) profile = Profile::Balanced;
  if (profile_ == profile) return;
  profile_ = profile;
  applyProfile();
  save();
}

const char* PowerManager::profileName() const { return profileName(profile_); }
const char* PowerManager::profileName(Profile p) {
  switch (p) {
    case Profile::Performance: return "Performance";
    case Profile::Balanced: return "Balanced";
    case Profile::Saver: return "Battery Saver";
    case Profile::Stealth: return "Stealth";
    case Profile::Count: default: return "Unknown";
  }
}

bool PowerManager::readWord(uint8_t reg, uint16_t& value) {
  Wire.beginTransmission(kGauge);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(static_cast<int>(kGauge), 2) != 2) return false;
  const uint8_t lo = Wire.read();
  const uint8_t hi = Wire.read();
  value = static_cast<uint16_t>(lo | (hi << 8));
  return true;
}

void PowerManager::readBattery() {
  uint16_t temp = 0, voltage = 0, current = 0, soc = 0, rem = 0, full = 0;
  const bool ok = readWord(0x08, voltage) && readWord(0x2C, soc);
  battery_.present = ok;
  if (!ok) return;
  readWord(0x06, temp);
  readWord(0x0C, current);
  readWord(0x10, rem);
  readWord(0x12, full);
  battery_.voltage = voltage / 1000.0f;
  battery_.currentMa = static_cast<int16_t>(current);
  battery_.temperatureC = temp > 0 ? (temp / 10.0f - 273.15f) : 0.0f;
  battery_.soc = static_cast<uint8_t>(constrain(static_cast<int>(soc), 0, 100));
  battery_.remainingMah = rem;
  battery_.fullMah = full;
}

void PowerManager::applyProfile() {
  switch (profile_) {
    case Profile::Performance:
      setCpuFrequencyMhz(240);
      break;
    case Profile::Balanced:
      setCpuFrequencyMhz(160);
      break;
    case Profile::Saver:
      setCpuFrequencyMhz(80);
      break;
    case Profile::Stealth:
      setCpuFrequencyMhz(80);
      break;
    case Profile::Count:
      break;
  }
}

void PowerManager::save() {
  if (!prefs_.begin("control_power", false)) return;
  prefs_.putUChar("profile", static_cast<uint8_t>(profile_));
  prefs_.end();
}
