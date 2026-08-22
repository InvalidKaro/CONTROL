#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>

class BootSequence {
 public:
  struct RuntimeStatus {
    bool webReady = false;
    bool flashReady = false;
    bool sdReady = false;
    String ip = "172.0.0.1";
  };

  void begin(TFT_eSPI& tft, const RuntimeStatus& status);
  void tick(uint32_t nowMs);
  void skip();

  bool active() const { return active_; }
  bool finished() const { return finished_; }

 private:
  enum class Phase : uint8_t {
    Trace,
    Logo,
    Hardware,
    Network,
    Complete,
  };

  struct Segment {
    int16_t x1;
    int16_t y1;
    int16_t x2;
    int16_t y2;
  };

  static constexpr uint8_t SegmentCount = 18;

  TFT_eSPI* tft_ = nullptr;
  RuntimeStatus status_;
  Phase phase_ = Phase::Trace;
  bool active_ = false;
  bool finished_ = false;

  uint32_t phaseStartedMs_ = 0;
  uint32_t lastStepMs_ = 0;
  uint8_t segmentIndex_ = 0;
  float segmentProgress_ = 0.0f;
  int16_t turtleX_ = 18;
  int16_t turtleY_ = 84;
  int16_t lastTurtleX_ = 18;
  int16_t lastTurtleY_ = 84;
  uint8_t hardwareLine_ = 0;

  bool cc1101Ready_ = false;
  bool nrf24Ready_ = false;
  uint8_t i2cCount_ = 0;

  static const Segment kSegments_[SegmentCount];

  void enterPhase(Phase next, uint32_t nowMs);
  void drawTraceStep(uint32_t nowMs);
  void drawLogo(uint32_t nowMs);
  void drawHardware(uint32_t nowMs);
  void drawNetwork(uint32_t nowMs);
  void finish();

  void drawTurtle(int16_t x, int16_t y, float angleRadians);
  void eraseTurtle(int16_t x, int16_t y);
  void drawNode(int16_t x, int16_t y);
  void drawProgress(uint8_t percent);
  void drawStatusLine(int16_t y, const char* label, bool ok, const String& detail = String());

  bool probeCc1101();
  bool probeNrf24();
  uint8_t scanI2cBus();
};
