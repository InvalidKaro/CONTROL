#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>

class App {
 public:
  virtual ~App() = default;
  virtual const char* name() const = 0;
  virtual const char* shortName() const = 0;
  virtual void begin() {}
  virtual void end() {}
  virtual void tick(uint32_t nowMs) = 0;
  virtual void render(TFT_eSPI& tft) = 0;
  virtual void onEncoder(int delta) { (void)delta; }
  virtual void onSelect() {}
};
