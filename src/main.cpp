#include <Arduino.h>

#include "ControlOS.h"
#include "RuntimeDebug.h"

ControlOS os;

void setup() {
  RuntimeDebug::begin();

  RuntimeDebug::mark(
      RuntimeDebug::Stage::EnterSetup,
      "Arduino setup() entered"
  );

  os.begin();
}

void loop() {
  RuntimeDebug::heartbeat();
  os.loop();
}
