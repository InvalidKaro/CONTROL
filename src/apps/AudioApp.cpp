#include "AudioApp.h"

#include <math.h>

#include "Ui.h"
#include "board_pins.h"
#include "theme.h"

namespace {

constexpr i2s_port_t kMicPort =
    I2S_NUM_0;

constexpr int kSamples =
    128;

constexpr int kRate =
    16000;

}  // namespace

bool AudioApp::initMic() {
  i2s_config_t config{};

  config.mode =
      static_cast<i2s_mode_t>(
          I2S_MODE_MASTER |
          I2S_MODE_RX |
          I2S_MODE_PDM
      );

  config.sample_rate =
      kRate;

  config.bits_per_sample =
      I2S_BITS_PER_SAMPLE_16BIT;

  config.channel_format =
      I2S_CHANNEL_FMT_ONLY_LEFT;

  config.communication_format =
      I2S_COMM_FORMAT_STAND_I2S;

  config.intr_alloc_flags =
      ESP_INTR_FLAG_LEVEL1;

  config.dma_buf_count =
      4;

  config.dma_buf_len =
      128;

  config.use_apll =
      false;

  config.tx_desc_auto_clear =
      false;

  config.fixed_mclk =
      0;

  if (
      i2s_driver_install(
          kMicPort,
          &config,
          0,
          nullptr
      ) != ESP_OK
  ) {
    return false;
  }

  i2s_pin_config_t pins{};

  pins.bck_io_num =
      BoardPins::MicClock;

  pins.ws_io_num =
      I2S_PIN_NO_CHANGE;

  pins.data_out_num =
      I2S_PIN_NO_CHANGE;

  pins.data_in_num =
      BoardPins::MicData;

  if (
      i2s_set_pin(
          kMicPort,
          &pins
      ) != ESP_OK
  ) {
    i2s_driver_uninstall(kMicPort);
    return false;
  }

  i2s_zero_dma_buffer(kMicPort);

  return true;
}

void AudioApp::begin() {
  ready_ = initMic();
  running_ = true;
  lastSampleMs_ = 0;
  levelDbfs_ = -90.0f;
  peakDbfs_ = -90.0f;

  for (float& band : bands_) {
    band = 0.0f;
  }
}

void AudioApp::end() {
  if (ready_) {
    i2s_driver_uninstall(kMicPort);
  }

  ready_ = false;
}

void AudioApp::tick(uint32_t nowMs) {
  if (
      !ready_ ||
      !running_ ||
      nowMs - lastSampleMs_ < 70
  ) {
    return;
  }

  lastSampleMs_ = nowMs;
  sample();
}

void AudioApp::sample() {
  int16_t samples[kSamples] = {};

  size_t got = 0;

  const esp_err_t readResult =
      i2s_read(
          kMicPort,
          samples,
          sizeof(samples),
          &got,
          pdMS_TO_TICKS(12)
      );

  if (
      readResult != ESP_OK ||
      got < sizeof(int16_t) * 32
  ) {
    return;
  }

  const int sampleCount =
      min(
          kSamples,
          static_cast<int>(
              got / sizeof(int16_t)
          )
      );

  if (sampleCount <= 0) {
    return;
  }

  double mean = 0.0;

  for (int i = 0; i < sampleCount; ++i) {
    mean += samples[i];
  }

  mean /=
      static_cast<double>(sampleCount);

  double sumSq = 0.0;

  for (int i = 0; i < sampleCount; ++i) {
    const double value =
        samples[i] - mean;

    sumSq +=
        value * value;
  }

  const double rms =
      sqrt(
          sumSq /
          static_cast<double>(sampleCount)
      );

  levelDbfs_ =
      rms > 0.5
          ? 20.0f *
                log10f(
                    static_cast<float>(
                        rms / 32768.0
                    )
                )
          : -90.0f;

  // Peak should decay towards quieter values, not towards 0 dBFS.
  if (levelDbfs_ >= peakDbfs_) {
    peakDbfs_ = levelDbfs_;
  } else {
    peakDbfs_ =
        max(
            levelDbfs_,
            peakDbfs_ - 0.6f
        );
  }

  for (int band = 0; band < 16; ++band) {
    const float frequency =
        125.0f +
        band * 430.0f;

    double real = 0.0;
    double imag = 0.0;

    for (
        int i = 0;
        i < sampleCount;
        ++i
    ) {
      const double angle =
          2.0 *
          PI *
          frequency *
          i /
          kRate;

      const double value =
          samples[i] - mean;

      real +=
          value * cos(angle);

      imag -=
          value * sin(angle);
    }

    const float magnitude =
        static_cast<float>(
            sqrt(
                real * real +
                imag * imag
            ) /
            sampleCount
        );

    const float normalized =
        constrain(
            log10f(
                1.0f + magnitude
            ) /
                4.2f,
            0.0f,
            1.0f
        );

    bands_[band] =
        bands_[band] * 0.65f +
        normalized * 0.35f;
  }
}

void AudioApp::render(TFT_eSPI& tft) {
  tft.fillScreen(Theme::Bg);

  Ui::header(
      tft,
      "AUDIO SCOPE",
      ready_
          ? (
                running_
                    ? "LIVE"
                    : "PAUSED"
            )
          : "MIC ERR"
  );

  tft.drawRect(
      8,
      32,
      304,
      91,
      Theme::PrimaryDim
  );

  for (int i = 0; i < 16; ++i) {
    const int height =
        static_cast<int>(
            bands_[i] * 78.0f
        );

    tft.fillRect(
        13 + i * 18,
        118 - height,
        12,
        height,
        Theme::Primary
    );

    if (height > 52) {
      tft.drawFastHLine(
          13 + i * 18,
          118 - height,
          12,
          Theme::PrimaryBright
      );
    }
  }

  Ui::text(
      tft,
      12,
      127,
      String(levelDbfs_, 1) +
          " dBFS",
      Theme::Text,
      1
  );

  tft.setTextDatum(TR_DATUM);

  tft.setTextColor(
      Theme::Muted,
      Theme::Bg
  );

  tft.drawString(
      String("PEAK ") +
          String(peakDbfs_, 1),
      308,
      127,
      1
  );

  tft.setTextDatum(TL_DATUM);

  Ui::footer(
      tft,
      "Relative mic spectrum",
      "ENC: pause/run"
  );
}
