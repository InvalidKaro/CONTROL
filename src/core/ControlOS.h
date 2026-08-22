#pragma once

#include <TFT_eSPI.h>

#include "AudioApp.h"
#include "AutomationEngine.h"
#include "BleApp.h"
#include "BootSequence.h"
#include "DashboardApp.h"
#include "DiagnosticsApp.h"
#include "Input.h"
#include "IrApp.h"
#include "LedApp.h"
#include "LedController.h"
#include "NfcApp.h"
#include "NrfApp.h"
#include "PowerManager.h"
#include "QrApp.h"
#include "QuickActionsApp.h"
#include "ScriptApp.h"
#include "SettingsApp.h"
#include "SoundController.h"
#include "StorageApp.h"
#include "SubGhzApp.h"
#include "SystemApp.h"
#include "ThemeManager.h"
#include "ToolboxApp.h"
#include "WebApp.h"
#include "WebUi.h"
#include "WifiRadarApp.h"
#include "app.h"

class ControlOS {
 public:
  ControlOS();
  void begin();
  void loop();

 private:
  void renderLauncher();
  void openSelected();
  void closeApp();
  void applyRemoteCommand(const RemoteCommand& command, InputEvents& events);
  void syncWebState();

  TFT_eSPI tft_;
  Input input_;
  ThemeManager themes_;
  PowerManager power_;
  AutomationEngine automations_;
  LedController leds_;
  SoundController sounds_;
  WebUi web_;
  BootSequence boot_;

  DashboardApp dashboard_;
  DiagnosticsApp diagnostics_;
  SubGhzApp subGhz_;
  WifiRadarApp wifi_;
  BleApp ble_;
  NfcApp nfc_;
  IrApp ir_;
  NrfApp nrf_;
  QrApp qr_;
  AudioApp audio_;
  LedApp ledApp_;
  ScriptApp scripts_;
  ToolboxApp tools_;
  QuickActionsApp quick_;
  StorageApp storage_;
  SystemApp system_;
  SettingsApp settings_;
  WebApp webApp_;

  App* apps_[18];
  static constexpr int AppCount = 18;
  int selected_ = 0;
  App* active_ = nullptr;
  uint32_t lastFrameMs_ = 0;
  uint32_t rebootAtMs_ = 0;
};
