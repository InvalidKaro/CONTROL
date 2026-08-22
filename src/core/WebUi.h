#pragma once

#include <Arduino.h>
#include <FS.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

#include "LedController.h"
#include "app.h"

class PowerManager;
class ThemeManager;
class AutomationEngine;

enum class RemoteCommandType : uint8_t {
  None = 0,
  Encoder,
  Select,
  Back,
  Home,
  OpenApp,
  Reboot,
  ScreenOn,
  ScreenOff,
};

struct RemoteCommand {
  RemoteCommandType type = RemoteCommandType::None;
  int value = 0;
};

class WebUi {
 public:
  WebUi();

  void begin(
      App** apps,
      int appCount,
      LedController* leds = nullptr,
      PowerManager* power = nullptr,
      ThemeManager* themes = nullptr,
      AutomationEngine* automations = nullptr
  );

  void loop();
  void setUiState(int selected, const char* activeApp);
  bool popCommand(RemoteCommand& command);

  bool running() const {
    return running_;
  }

  String ssid() const {
    return ssid_;
  }

  String ip() const;

  String webUser() const {
    return "admin";
  }

  String webPassword() const {
    return webPassword_;
  }

  uint8_t clientCount() const;

  bool sdReady() const {
    return sdReady_;
  }

  bool flashReady() const {
    return flashReady_;
  }

 private:
  // ESP32 Arduino FS keeps File inside namespace fs.
  // Making it a class alias also fixes unqualified File usages inside
  // WebUi member-function definitions in WebUi.cpp.
  using File = fs::File;

  static constexpr uint8_t QueueSize = 12;
  static constexpr uint8_t LogSize = 20;

  WebServer server_;
  WebSocketsServer ws_;

  App** apps_ = nullptr;
  int appCount_ = 0;
  int selected_ = 0;

  LedController* leds_ = nullptr;
  PowerManager* power_ = nullptr;
  ThemeManager* themes_ = nullptr;
  AutomationEngine* automations_ = nullptr;

  String activeApp_ = "Launcher";

  bool running_ = false;
  uint32_t lastWsBroadcastMs_ = 0;

  String ssid_;
  String webPassword_;

  bool flashReady_ = false;
  bool sdReady_ = false;

  RemoteCommand queue_[QueueSize];
  uint8_t queueHead_ = 0;
  uint8_t queueTail_ = 0;

  String logs_[LogSize];
  uint8_t logHead_ = 0;
  uint8_t logCount_ = 0;

  File uploadFile_;
  fs::FS* uploadFs_ = nullptr;
  bool uploadOk_ = false;
  String uploadError_;

  bool otaOk_ = false;
  String otaError_;

  void configureRoutes();
  bool authorize();
  bool mutationAllowed();
  void enqueue(RemoteCommandType type, int value = 0);
  void log(const String& message);

  void sendIndex();
  void sendStatus();
  String buildStatusJson() const;
  void sendApps();
  void sendLogs();
  void handleControl();

  void sendLed();
  void handleLed();

  void sendTheme();
  void handleTheme();

  void sendPower();
  void handlePower();

  void sendAutomation();
  void handleAutomation();

  void sendScreenState();

  void exportConfig();
  void importConfig();
  void factoryReset();

  void handleFiles();
  void handleMkdir();
  void handleDelete();
  void handleRename();
  void handleRead();
  void handleWrite();
  void handleDownload();

  void handleUploadRequest();
  void handleUploadData();

  void handleOtaRequest();
  void handleOtaData();

  fs::FS* selectFs(const String& name);
  bool ensureSd();

  String normalizePath(const String& raw) const;

  bool removeRecursive(
      fs::FS& fs,
      const String& path,
      uint8_t depth = 0
  );

  static String jsonEscape(const String& input);
  static String formatBytes(uint64_t bytes);
  static String contentTypeFor(const String& path);
};
