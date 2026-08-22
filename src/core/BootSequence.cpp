#include "BootSequence.h"

#include <SPI.h>
#include <Wire.h>
#include <math.h>

#include "board_pins.h"
#include "theme.h"

const BootSequence::Segment BootSequence::kSegments_[BootSequence::SegmentCount] = {
    {18, 84, 42, 84},   {42, 84, 42, 58},   {42, 58, 74, 58},   {74, 58, 88, 72},
    {88, 72, 118, 72},  {118, 72, 118, 42}, {118, 42, 151, 42}, {151, 42, 166, 57},
    {166, 57, 204, 57}, {204, 57, 204, 86}, {204, 86, 235, 86}, {235, 86, 251, 70},
    {251, 70, 293, 70}, {293, 70, 293, 110},{293, 110, 263, 110},{263, 110, 246, 127},
    {246, 127, 196, 127},{196, 127, 182, 113},
};

void BootSequence::begin(TFT_eSPI& tft, const RuntimeStatus& status) {
  tft_ = &tft;
  status_ = status;
  phase_ = Phase::Trace;
  active_ = true;
  finished_ = false;
  phaseStartedMs_ = millis();
  lastStepMs_ = 0;
  segmentIndex_ = 0;
  segmentProgress_ = 0.0f;
  hardwareLine_ = 0;
  turtleX_ = kSegments_[0].x1;
  turtleY_ = kSegments_[0].y1;
  lastTurtleX_ = turtleX_;
  lastTurtleY_ = turtleY_;

  tft_->fillScreen(Theme::Bg);
  tft_->setTextDatum(TL_DATUM);
  tft_->setTextColor(Theme::PrimaryDim, Theme::Bg);
  tft_->drawString("BOOT VECTOR // 00", 8, 6, 1);
  tft_->drawFastHLine(8, 18, 304, Theme::PrimaryDim);
  tft_->drawFastHLine(8, 147, 304, Theme::PrimaryDim);
  tft_->setTextColor(Theme::Muted, Theme::Bg);
  tft_->drawString("runtime vector renderer", 8, 153, 1);
  tft_->setTextDatum(TR_DATUM);
  tft_->drawString("ENC/BACK: skip", 312, 153, 1);
  tft_->setTextDatum(TL_DATUM);
  drawTurtle(turtleX_, turtleY_, 0.0f);
}

void BootSequence::tick(uint32_t nowMs) {
  if (!active_ || finished_ || tft_ == nullptr) return;

  switch (phase_) {
    case Phase::Trace:
      drawTraceStep(nowMs);
      break;
    case Phase::Logo:
      drawLogo(nowMs);
      break;
    case Phase::Hardware:
      drawHardware(nowMs);
      break;
    case Phase::Network:
      drawNetwork(nowMs);
      break;
    case Phase::Complete:
      finish();
      break;
  }
}

void BootSequence::skip() {
  if (!active_) return;
  finish();
}

void BootSequence::enterPhase(Phase next, uint32_t nowMs) {
  phase_ = next;
  phaseStartedMs_ = nowMs;
  lastStepMs_ = 0;
}

void BootSequence::drawTraceStep(uint32_t nowMs) {
  if (segmentIndex_ >= SegmentCount) {
    eraseTurtle(lastTurtleX_, lastTurtleY_);
    enterPhase(Phase::Logo, nowMs);
    return;
  }

  if (lastStepMs_ != 0 && nowMs - lastStepMs_ < 8) return;
  lastStepMs_ = nowMs;

  const Segment& s = kSegments_[segmentIndex_];
  const float dx = static_cast<float>(s.x2 - s.x1);
  const float dy = static_cast<float>(s.y2 - s.y1);
  const float length = sqrtf(dx * dx + dy * dy);
  const float step = length > 0.0f ? 3.4f / length : 1.0f;
  const float nextProgress = min(1.0f, segmentProgress_ + step);

  const int16_t x1 = static_cast<int16_t>(s.x1 + dx * segmentProgress_);
  const int16_t y1 = static_cast<int16_t>(s.y1 + dy * segmentProgress_);
  const int16_t x2 = static_cast<int16_t>(s.x1 + dx * nextProgress);
  const int16_t y2 = static_cast<int16_t>(s.y1 + dy * nextProgress);

  eraseTurtle(lastTurtleX_, lastTurtleY_);
  // Restore geometry temporarily covered by the moving turtle cursor.
  if (segmentIndex_ > 0) {
    const Segment& previous = kSegments_[segmentIndex_ - 1];
    tft_->drawLine(previous.x1, previous.y1, previous.x2, previous.y2, Theme::Primary);
    drawNode(previous.x2, previous.y2);
  }
  tft_->drawLine(s.x1, s.y1, x1, y1, Theme::Primary);
  tft_->drawLine(x1, y1, x2, y2, Theme::PrimaryBright);
  turtleX_ = x2;
  turtleY_ = y2;
  const float angle = atan2f(dy, dx);
  drawTurtle(turtleX_, turtleY_, angle);
  lastTurtleX_ = turtleX_;
  lastTurtleY_ = turtleY_;

  if (nextProgress >= 1.0f) {
    eraseTurtle(lastTurtleX_, lastTurtleY_);
    tft_->drawLine(s.x1, s.y1, s.x2, s.y2, Theme::Primary);
    drawNode(s.x2, s.y2);
    ++segmentIndex_;
    segmentProgress_ = 0.0f;
    if (segmentIndex_ < SegmentCount) {
      turtleX_ = kSegments_[segmentIndex_].x1;
      turtleY_ = kSegments_[segmentIndex_].y1;
      lastTurtleX_ = turtleX_;
      lastTurtleY_ = turtleY_;
    }
  } else {
    segmentProgress_ = nextProgress;
  }

  drawProgress(static_cast<uint8_t>(min(34, static_cast<int>((segmentIndex_ * 34) / SegmentCount))));
}

void BootSequence::drawLogo(uint32_t nowMs) {
  const uint32_t elapsed = nowMs - phaseStartedMs_;

  if (elapsed < 40) {
    tft_->fillRect(72, 57, 176, 58, Theme::Bg);
    tft_->drawRoundRect(72, 57, 176, 58, 6, Theme::PrimaryDim);
    tft_->drawRoundRect(74, 59, 172, 54, 5, Theme::Primary);
  }

  if (elapsed >= 90 && elapsed < 500) {
    tft_->setTextDatum(MC_DATUM);
    tft_->setTextColor(Theme::PrimaryBright, Theme::Bg);
    tft_->drawString("CONTROL//OS", 160, 76, 4);
    tft_->setTextColor(Theme::Muted, Theme::Bg);
    tft_->drawString("T-EMBED // BOOTSTRAP", 160, 100, 1);
    tft_->setTextDatum(TL_DATUM);
    drawProgress(44);
  }

  if (elapsed >= 520) {
    enterPhase(Phase::Hardware, nowMs);
    tft_->fillRect(8, 25, 304, 120, Theme::Bg);
    tft_->setTextColor(Theme::PrimaryBright, Theme::Bg);
    tft_->drawString("HARDWARE DISCOVERY", 10, 29, 2);
    tft_->drawFastHLine(10, 48, 300, Theme::PrimaryDim);
  }
}

void BootSequence::drawHardware(uint32_t nowMs) {
  if (lastStepMs_ != 0 && nowMs - lastStepMs_ < 145) return;
  lastStepMs_ = nowMs;

  switch (hardwareLine_) {
    case 0:
      drawStatusLine(55, "DISPLAY / ST7789", true, "320x170");
      break;
    case 1:
      cc1101Ready_ = probeCc1101();
      drawStatusLine(72, "SUBGHZ / CC1101", cc1101Ready_, cc1101Ready_ ? "SPI" : "NO RESP");
      break;
    case 2:
      nrf24Ready_ = probeNrf24();
      drawStatusLine(89, "2.4G / NRF24", nrf24Ready_, nrf24Ready_ ? "SPI" : "NO RESP");
      break;
    case 3:
      i2cCount_ = scanI2cBus();
      drawStatusLine(106, "I2C BUS", i2cCount_ > 0, String(i2cCount_) + " DEV");
      break;
    case 4:
      drawStatusLine(123, "FILESYSTEM", status_.flashReady,
                     status_.sdReady ? "FLASH+SD" : (status_.flashReady ? "FLASH" : "OFFLINE"));
      break;
    default:
      drawProgress(77);
      enterPhase(Phase::Network, nowMs);
      tft_->fillRect(8, 25, 304, 120, Theme::Bg);
      return;
  }

  drawProgress(static_cast<uint8_t>(50 + hardwareLine_ * 5));
  ++hardwareLine_;
}

void BootSequence::drawNetwork(uint32_t nowMs) {
  const uint32_t elapsed = nowMs - phaseStartedMs_;

  if (elapsed < 50) {
    tft_->setTextColor(Theme::PrimaryBright, Theme::Bg);
    tft_->drawString("CONTROL PLANE", 10, 29, 2);
    tft_->drawFastHLine(10, 48, 300, Theme::PrimaryDim);

    tft_->drawCircle(54, 88, 13, Theme::Primary);
    tft_->fillCircle(54, 88, 3, Theme::PrimaryBright);
    tft_->drawLine(67, 88, 113, 88, Theme::Primary);
    tft_->drawLine(113, 88, 128, 73, Theme::Primary);
    tft_->drawLine(113, 88, 128, 103, Theme::Primary);
    drawNode(128, 73);
    drawNode(128, 103);

    tft_->setTextColor(status_.webReady ? Theme::PrimaryBright : Theme::Warning, Theme::Bg);
    tft_->drawString(status_.webReady ? "WEBUI ONLINE" : "WEBUI DEGRADED", 146, 67, 2);
    tft_->setTextColor(Theme::Text, Theme::Bg);
    tft_->drawString(status_.ip, 146, 89, 2);
    tft_->setTextColor(Theme::Muted, Theme::Bg);
    tft_->drawString("AP+STA // HTTP CONTROL", 146, 110, 1);
    drawProgress(88);
  }

  if (elapsed >= 420 && elapsed < 760) {
    tft_->setTextDatum(MC_DATUM);
    tft_->setTextColor(Theme::PrimaryBright, Theme::Bg);
    tft_->drawString("READY", 160, 126, 2);
    tft_->setTextDatum(TL_DATUM);
    drawProgress(100);
  }

  if (elapsed >= 780) {
    enterPhase(Phase::Complete, nowMs);
  }
}

void BootSequence::finish() {
  active_ = false;
  finished_ = true;
  if (tft_ != nullptr) {
    tft_->setTextDatum(TL_DATUM);
  }
}

void BootSequence::drawTurtle(int16_t x, int16_t y, float angleRadians) {
  const float spread = 2.45f;
  const int16_t tipX = x + static_cast<int16_t>(cosf(angleRadians) * 7.0f);
  const int16_t tipY = y + static_cast<int16_t>(sinf(angleRadians) * 7.0f);
  const int16_t leftX = x + static_cast<int16_t>(cosf(angleRadians + spread) * 5.0f);
  const int16_t leftY = y + static_cast<int16_t>(sinf(angleRadians + spread) * 5.0f);
  const int16_t rightX = x + static_cast<int16_t>(cosf(angleRadians - spread) * 5.0f);
  const int16_t rightY = y + static_cast<int16_t>(sinf(angleRadians - spread) * 5.0f);

  tft_->drawTriangle(tipX, tipY, leftX, leftY, rightX, rightY, Theme::PrimaryBright);
  tft_->fillCircle(x, y, 1, Theme::PrimaryBright);
}

void BootSequence::eraseTurtle(int16_t x, int16_t y) {
  tft_->fillCircle(x, y, 8, Theme::Bg);
}

void BootSequence::drawNode(int16_t x, int16_t y) {
  tft_->fillCircle(x, y, 2, Theme::PrimaryBright);
  tft_->drawCircle(x, y, 4, Theme::PrimaryDim);
}

void BootSequence::drawProgress(uint8_t percent) {
  const uint8_t clamped = percent > 100 ? 100 : percent;
  tft_->drawRect(8, 135, 304, 8, Theme::PrimaryDim);
  tft_->fillRect(10, 137, 300, 4, Theme::Bg);
  const int width = static_cast<int>((300UL * clamped) / 100UL);
  if (width > 0) tft_->fillRect(10, 137, width, 4, Theme::Primary);
}

void BootSequence::drawStatusLine(int16_t y, const char* label, bool ok, const String& detail) {
  tft_->setTextDatum(TL_DATUM);
  tft_->setTextColor(Theme::Text, Theme::Bg);
  tft_->drawString(label, 12, y, 1);

  tft_->setTextDatum(TR_DATUM);
  if (!detail.isEmpty()) {
    tft_->setTextColor(Theme::Muted, Theme::Bg);
    tft_->drawString(detail, 270, y, 1);
  }
  tft_->setTextColor(ok ? Theme::PrimaryBright : Theme::Warning, Theme::Bg);
  tft_->drawString(ok ? "[OK]" : "[--]", 309, y, 1);
  tft_->setTextDatum(TL_DATUM);
}

bool BootSequence::probeCc1101() {
  pinMode(BoardPins::SpiMiso, INPUT);
  digitalWrite(BoardPins::SdCs, HIGH);
  digitalWrite(BoardPins::NrfCs, HIGH);
  digitalWrite(BoardPins::Cc1101Cs, HIGH);

  SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
  digitalWrite(BoardPins::Cc1101Cs, LOW);

  const uint32_t started = micros();
  while (digitalRead(BoardPins::SpiMiso) == HIGH && micros() - started < 700) {
    yield();
  }

  SPI.transfer(0xF1);  // VERSION status register, read + burst bit required for CC1101 status regs.
  const uint8_t version = SPI.transfer(0x00);
  digitalWrite(BoardPins::Cc1101Cs, HIGH);
  SPI.endTransaction();

  return version != 0x00 && version != 0xFF;
}

bool BootSequence::probeNrf24() {
  digitalWrite(BoardPins::SdCs, HIGH);
  digitalWrite(BoardPins::Cc1101Cs, HIGH);
  digitalWrite(BoardPins::NrfCs, HIGH);
  digitalWrite(BoardPins::NrfCe, LOW);

  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
  digitalWrite(BoardPins::NrfCs, LOW);
  const uint8_t status = SPI.transfer(0xFF);  // NOP returns STATUS.
  digitalWrite(BoardPins::NrfCs, HIGH);
  SPI.endTransaction();

  return status != 0xFF;
}

uint8_t BootSequence::scanI2cBus() {
  uint8_t count = 0;
  for (uint8_t address = 1; address < 127; ++address) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) ++count;
  }
  return count;
}
