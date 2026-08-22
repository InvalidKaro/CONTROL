#include <Arduino.h>
#include "ControlOS.h"

ControlOS os;

void setup() {
  os.begin();
}

void loop() {
  os.loop();
}
