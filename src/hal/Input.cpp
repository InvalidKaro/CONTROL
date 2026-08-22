#include "Input.h"

#include "board_pins.h"

void Input::begin() {
  pinMode(BoardPins::EncoderA, INPUT_PULLUP);
  pinMode(BoardPins::EncoderB, INPUT_PULLUP);
  pinMode(BoardPins::EncoderKey, INPUT_PULLUP);
  pinMode(BoardPins::UserKey, INPUT_PULLUP);
  encoderState_ = (digitalRead(BoardPins::EncoderA) << 1) | digitalRead(BoardPins::EncoderB);
  selectState_ = digitalRead(BoardPins::EncoderKey);
  backState_ = digitalRead(BoardPins::UserKey);
}

InputEvents Input::poll(uint32_t nowMs) {
  InputEvents events;

  static constexpr int8_t transition[16] = {
      0, -1, 1, 0,
      1, 0, 0, -1,
      -1, 0, 0, 1,
      0, 1, -1, 0,
  };

  const uint8_t nextState = (digitalRead(BoardPins::EncoderA) << 1) | digitalRead(BoardPins::EncoderB);
  const uint8_t index = static_cast<uint8_t>((encoderState_ << 2) | nextState);
  accumulator_ += transition[index];
  encoderState_ = nextState;

  if (accumulator_ >= 4) {
    events.encoderDelta = 1;
    accumulator_ = 0;
  } else if (accumulator_ <= -4) {
    events.encoderDelta = -1;
    accumulator_ = 0;
  }

  const bool selectNow = digitalRead(BoardPins::EncoderKey);
  if (selectNow != selectState_ && nowMs - selectChangedAt_ > 30) {
    selectChangedAt_ = nowMs;
    selectState_ = selectNow;
    if (!selectNow) {
      selectDownAt_ = nowMs;
      selectHoldFired_ = false;
      events.selectPressed = true;
    }
  }
  if (!selectState_ && !selectHoldFired_ && nowMs - selectDownAt_ >= 700) {
    selectHoldFired_ = true;
    events.selectHeld = true;
  }

  const bool backNow = digitalRead(BoardPins::UserKey);
  if (backNow != backState_ && nowMs - backChangedAt_ > 30) {
    backChangedAt_ = nowMs;
    backState_ = backNow;
    if (!backNow) {
      backDownAt_ = nowMs;
      backHoldFired_ = false;
      events.backPressed = true;
    }
  }
  if (!backState_ && !backHoldFired_ && nowMs - backDownAt_ >= 700) {
    backHoldFired_ = true;
    events.backHeld = true;
  }

  return events;
}
