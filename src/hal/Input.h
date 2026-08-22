#pragma once

#include <Arduino.h>

struct InputEvents {
  int encoderDelta = 0;
  bool selectPressed = false;
  bool backPressed = false;
  bool selectHeld = false;
  bool backHeld = false;
};

class Input {
 public:
  void begin();
  InputEvents poll(uint32_t nowMs);

 private:
  uint8_t encoderState_ = 0;
  bool selectState_ = true;
  bool backState_ = true;
  uint32_t selectChangedAt_ = 0;
  uint32_t backChangedAt_ = 0;
  uint32_t selectDownAt_ = 0;
  uint32_t backDownAt_ = 0;
  bool selectHoldFired_ = false;
  bool backHoldFired_ = false;
  int8_t accumulator_ = 0;
};
