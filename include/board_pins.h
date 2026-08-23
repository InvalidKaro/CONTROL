#pragma once

#include <Arduino.h>

namespace BoardPins {

constexpr uint8_t PowerEnable = 15;
constexpr uint8_t UserKey = 6;

constexpr uint8_t EncoderA = 4;
constexpr uint8_t EncoderB = 5;
constexpr uint8_t EncoderKey = 0;

constexpr uint8_t SpiSck = 11;
constexpr uint8_t SpiMosi = 9;
constexpr uint8_t SpiMiso = 10;

constexpr uint8_t DisplayCs = 41;
constexpr uint8_t DisplayDc = 16;

/*
 * T-Embed CC1101 Plus:
 * ST7789 hardware reset is GPIO40.
 *
 * NOTE:
 * GPIO40 is also used by the speaker LRCLK path on the known
 * working board pinout. Do not initialize I2S audio while the
 * display is being reset/initialized.
 */
constexpr int8_t DisplayReset = 40;

constexpr uint8_t DisplayBacklight = 21;

constexpr uint8_t SdCs = 13;

constexpr uint8_t Cc1101Cs = 12;
constexpr uint8_t Cc1101Gdo0 = 3;
constexpr uint8_t Cc1101Gdo2 = 38;
constexpr uint8_t Cc1101Switch1 = 47;
constexpr uint8_t Cc1101Switch0 = 48;

constexpr uint8_t NrfCe = 43;
constexpr uint8_t NrfCs = 44;

constexpr uint8_t I2cSda = 8;
constexpr uint8_t I2cScl = 18;

constexpr uint8_t Pn532Irq = 17;
constexpr uint8_t Pn532Reset = 45;

constexpr uint8_t IrTx = 2;
constexpr uint8_t IrRx = 1;

constexpr uint8_t MicData = 42;
constexpr uint8_t MicClock = 39;

constexpr uint8_t VoiceBclk = 46;

/*
 * Shared with DisplayReset.
 * Audio must be initialized only after the display is fully running,
 * and ideally with explicit pin-multiplex handling.
 */
constexpr uint8_t VoiceLrclk = 40;

constexpr uint8_t VoiceDin = 7;

constexpr uint8_t LedData = 14;
constexpr uint8_t LedCount = 8;

}  // namespace BoardPins
