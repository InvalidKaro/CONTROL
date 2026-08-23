#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>

namespace RuntimeDebug {

enum class Stage : uint32_t {
  None = 0,
  EnterSetup = 10,
  EnterControlOS = 20,
  PowerPins = 30,
  DisplayReset = 40,
  DisplayInit = 50,
  DisplayReady = 60,
  SpiReady = 70,
  I2cReady = 80,
  FileLogReady = 85,
  ThemeReady = 90,
  PowerReady = 100,
  AutomationReady = 110,
  InputReady = 120,
  LedReady = 130,
  WebStarting = 140,
  WebReady = 150,
  BootSequence = 160,
  LauncherReady = 170,
  LoopRunning = 180
};

void begin();
void attachDisplay(TFT_eSPI* display);

// Mounts LittleFS and enables persistent logging.
// Returns true when /logs/controlos-debug.log is writable.
bool enableFileLogging();

void mark(Stage stage, const char* message);
void heartbeat();

uint32_t previousStage();
uint32_t currentStage();

const char* stageName(uint32_t stage);
const char* resetReasonName();

String logPath();

}  // namespace RuntimeDebug
