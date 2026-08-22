#include "SoundController.h"

#include <math.h>
#include "board_pins.h"

namespace {
constexpr i2s_port_t kPort = I2S_NUM_1;
constexpr uint32_t kSampleRate = 22050;
}

void SoundController::begin() {
  if (prefs_.begin("control_sound", true)) {
    enabled_ = prefs_.getBool("enabled", false);
    prefs_.end();
  }
  i2s_config_t cfg{};
  cfg.mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_TX);
  cfg.sample_rate = kSampleRate;
  cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  cfg.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
  cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  cfg.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  cfg.dma_buf_count = 4;
  cfg.dma_buf_len = 128;
  cfg.use_apll = false;
  cfg.tx_desc_auto_clear = true;
  cfg.fixed_mclk = 0;
  if (i2s_driver_install(kPort, &cfg, 0, nullptr) != ESP_OK) return;
  i2s_pin_config_t pins{};
  pins.bck_io_num = BoardPins::VoiceBclk;
  pins.ws_io_num = BoardPins::VoiceLrclk;
  pins.data_out_num = BoardPins::VoiceDin;
  pins.data_in_num = I2S_PIN_NO_CHANGE;
  if (i2s_set_pin(kPort, &pins) != ESP_OK) {
    i2s_driver_uninstall(kPort);
    return;
  }
  i2s_zero_dma_buffer(kPort);
  ready_ = true;
}

void SoundController::end() {
  if (ready_) i2s_driver_uninstall(kPort);
  ready_ = false;
}

void SoundController::setEnabled(bool enabled) {
  if (enabled_ == enabled) return;
  enabled_ = enabled;
  save();
}

void SoundController::beep(uint16_t frequency, uint16_t durationMs, uint8_t volume) {
  if (!ready_ || !enabled_) return;
  frequency_ = constrain(frequency, 80, 6000);
  volume_ = constrain(volume, 1, 40);
  phase_ = 0.0f;
  stopAtMs_ = millis() + durationMs;
}

void SoundController::tick(uint32_t nowMs) {
  if (!ready_ || !enabled_ || stopAtMs_ == 0) return;
  if (static_cast<int32_t>(nowMs - stopAtMs_) >= 0) {
    stopAtMs_ = 0;
    i2s_zero_dma_buffer(kPort);
    return;
  }
  int16_t frames[64 * 2];
  const float step = 2.0f * PI * frequency_ / kSampleRate;
  const float amp = 32767.0f * (volume_ / 100.0f);
  for (int i = 0; i < 64; ++i) {
    const int16_t sample = static_cast<int16_t>(sinf(phase_) * amp);
    phase_ += step;
    if (phase_ > 2.0f * PI) phase_ -= 2.0f * PI;
    frames[i * 2] = sample;
    frames[i * 2 + 1] = sample;
  }
  size_t written = 0;
  i2s_write(kPort, frames, sizeof(frames), &written, 0);
}

void SoundController::save() {
  if (!prefs_.begin("control_sound", false)) return;
  prefs_.putBool("enabled", enabled_);
  prefs_.end();
}
