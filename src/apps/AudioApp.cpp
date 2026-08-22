#include "AudioApp.h"

#include <math.h>
#include "Ui.h"
#include "board_pins.h"
#include "theme.h"

namespace {
constexpr i2s_port_t kMicPort = I2S_NUM_0;
constexpr int kSamples = 128;
constexpr int kRate = 16000;
}

bool AudioApp::initMic() {
  i2s_config_t cfg{};
  cfg.mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM);
  cfg.sample_rate = kRate;
  cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  cfg.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
  cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  cfg.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  cfg.dma_buf_count = 4;
  cfg.dma_buf_len = 128;
  cfg.use_apll = false;
  cfg.tx_desc_auto_clear = false;
  cfg.fixed_mclk = 0;
  if (i2s_driver_install(kMicPort, &cfg, 0, nullptr) != ESP_OK) return false;
  i2s_pin_config_t pins{};
  pins.bck_io_num = BoardPins::MicClock;
  pins.ws_io_num = I2S_PIN_NO_CHANGE;
  pins.data_out_num = I2S_PIN_NO_CHANGE;
  pins.data_in_num = BoardPins::MicData;
  if (i2s_set_pin(kMicPort, &pins) != ESP_OK) {
    i2s_driver_uninstall(kMicPort);
    return false;
  }
  i2s_zero_dma_buffer(kMicPort);
  return true;
}

void AudioApp::begin() {
  ready_ = initMic();
  running_ = true;
}

void AudioApp::end() {
  if (ready_) i2s_driver_uninstall(kMicPort);
  ready_ = false;
}

void AudioApp::tick(uint32_t nowMs) {
  if (!ready_ || !running_ || nowMs - lastSampleMs_ < 70) return;
  lastSampleMs_ = nowMs;
  sample();
}

void AudioApp::sample() {
  int16_t samples[kSamples]{};
  size_t got = 0;
  if (i2s_read(kMicPort, samples, sizeof(samples), &got, pdMS_TO_TICKS(12)) != ESP_OK || got < 64) return;
  const int n = min(kSamples, static_cast<int>(got / sizeof(int16_t)));
  double sumSq = 0.0;
  double mean = 0.0;
  for (int i = 0; i < n; ++i) mean += samples[i];
  mean /= max(1, n);
  for (int i = 0; i < n; ++i) {
    const double v = samples[i] - mean;
    sumSq += v * v;
  }
  const double rms = sqrt(sumSq / max(1, n));
  levelDbfs_ = rms > 0.5 ? 20.0f * log10f(static_cast<float>(rms / 32768.0)) : -90.0f;
  peakDbfs_ = max(levelDbfs_, peakDbfs_ * 0.98f);

  for (int b = 0; b < 16; ++b) {
    const float freq = 125.0f + b * 430.0f;
    double real = 0.0, imag = 0.0;
    for (int i = 0; i < n; ++i) {
      const double angle = 2.0 * PI * freq * i / kRate;
      const double v = samples[i] - mean;
      real += v * cos(angle);
      imag -= v * sin(angle);
    }
    const float mag = static_cast<float>(sqrt(real * real + imag * imag) / max(1, n));
    const float norm = constrain(log10f(1.0f + mag) / 4.2f, 0.0f, 1.0f);
    bands_[b] = bands_[b] * 0.65f + norm * 0.35f;
  }
}

void AudioApp::render(TFT_eSPI& tft) {
  tft.fillScreen(Theme::Bg);
  Ui::header(tft, "AUDIO SCOPE", ready_ ? (running_ ? "LIVE" : "PAUSED") : "MIC ERR");
  tft.drawRect(8, 32, 304, 91, Theme::PrimaryDim);
  for (int i = 0; i < 16; ++i) {
    const int h = static_cast<int>(bands_[i] * 78.0f);
    tft.fillRect(13 + i * 18, 118 - h, 12, h, Theme::Primary);
    if (h > 52) tft.drawFastHLine(13 + i * 18, 118 - h, 12, Theme::PrimaryBright);
  }
  Ui::text(tft, 12, 127, String(levelDbfs_, 1) + " dBFS", Theme::Text, 1);
  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(Theme::Muted, Theme::Bg);
  tft.drawString("PEAK " + String(peakDbfs_, 1), 308, 127, 1);
  tft.setTextDatum(TL_DATUM);
  Ui::footer(tft, "Relative mic spectrum", "ENC: pause/run");
}
