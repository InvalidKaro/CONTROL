#include "RuntimeDebug.h"

#include <FS.h>
#include <LittleFS.h>
#include <esp_attr.h>
#include <esp_system.h>

namespace {

constexpr uint32_t kMagic = 0x434F5344;  // "COSD"
constexpr size_t kMaxLogBytes = 64 * 1024;

RTC_NOINIT_ATTR uint32_t rtcMagic;
RTC_NOINIT_ATTR uint32_t rtcStage;
RTC_NOINIT_ATTR uint32_t rtcBootCount;

TFT_eSPI* tft = nullptr;

uint32_t previous = 0;
uint32_t current = 0;
uint32_t bootCount = 0;
uint32_t lastHeartbeatMs = 0;

bool fsMounted = false;
bool fileLogging = false;

uint8_t screenLine = 0;

constexpr const char* kLogDir = "/logs";
constexpr const char* kLogFile = "/logs/controlos-debug.log";
constexpr const char* kOldLogFile = "/logs/controlos-debug.old.log";

void drawLine(const String& text, uint16_t color) {
  if (tft == nullptr) {
    return;
  }

  constexpr int kTop = 7;
  constexpr int kLineHeight = 12;
  constexpr int kMaxLines = 13;

  if (screenLine >= kMaxLines) {
    tft->fillRect(0, kTop, 320, 156, TFT_BLACK);
    screenLine = 0;
  }

  tft->setTextDatum(TL_DATUM);
  tft->setTextColor(color, TFT_BLACK);
  tft->drawString(
      text,
      4,
      kTop + screenLine * kLineHeight,
      1
  );

  ++screenLine;
}

void rotateLogIfNeeded() {
  if (!fsMounted || !LittleFS.exists(kLogFile)) {
    return;
  }

  fs::File file = LittleFS.open(kLogFile, FILE_READ);

  if (!file) {
    return;
  }

  const size_t size = file.size();
  file.close();

  if (size < kMaxLogBytes) {
    return;
  }

  if (LittleFS.exists(kOldLogFile)) {
    LittleFS.remove(kOldLogFile);
  }

  LittleFS.rename(kLogFile, kOldLogFile);
}

void appendLogLine(const String& line) {
  if (!fileLogging || !fsMounted) {
    return;
  }

  rotateLogIfNeeded();

  fs::File file =
      LittleFS.open(kLogFile, FILE_APPEND);

  if (!file) {
    fileLogging = false;
    Serial.println("[DBG] ERROR: cannot append debug log");
    return;
  }

  file.println(line);
  file.flush();
  file.close();
}

String stageLine(
    uint32_t stage,
    const char* message
) {
  String line;
  line.reserve(120);

  line += "[";
  line += String(millis());
  line += " ms]";
  line += " stage=";
  line += String(stage);
  line += " ";
  line += RuntimeDebug::stageName(stage);

  if (message != nullptr && message[0] != '\0') {
    line += " | ";
    line += message;
  }

  line += " | heap=";
  line += String(ESP.getFreeHeap());

  line += " | psram=";
  line += String(ESP.getFreePsram());

  return line;
}

}  // namespace

namespace RuntimeDebug {

const char* stageName(uint32_t stage) {
  switch (static_cast<Stage>(stage)) {
    case Stage::None:
      return "NONE";

    case Stage::EnterSetup:
      return "ENTER_SETUP";

    case Stage::EnterControlOS:
      return "ENTER_CONTROL_OS";

    case Stage::PowerPins:
      return "POWER_PINS";

    case Stage::DisplayReset:
      return "DISPLAY_RESET";

    case Stage::DisplayInit:
      return "DISPLAY_INIT";

    case Stage::DisplayReady:
      return "DISPLAY_READY";

    case Stage::SpiReady:
      return "SPI_READY";

    case Stage::I2cReady:
      return "I2C_READY";

    case Stage::FileLogReady:
      return "FILE_LOG_READY";

    case Stage::ThemeReady:
      return "THEME_READY";

    case Stage::PowerReady:
      return "POWER_READY";

    case Stage::AutomationReady:
      return "AUTOMATION_READY";

    case Stage::InputReady:
      return "INPUT_READY";

    case Stage::LedReady:
      return "LED_READY";

    case Stage::WebStarting:
      return "WEB_STARTING";

    case Stage::WebReady:
      return "WEB_READY";

    case Stage::BootSequence:
      return "BOOT_SEQUENCE";

    case Stage::LauncherReady:
      return "LAUNCHER_READY";

    case Stage::LoopRunning:
      return "LOOP_RUNNING";

    default:
      return "UNKNOWN";
  }
}

const char* resetReasonName() {
  switch (esp_reset_reason()) {
    case ESP_RST_UNKNOWN:
      return "UNKNOWN";

    case ESP_RST_POWERON:
      return "POWERON";

    case ESP_RST_EXT:
      return "EXTERNAL";

    case ESP_RST_SW:
      return "SOFTWARE";

    case ESP_RST_PANIC:
      return "PANIC";

    case ESP_RST_INT_WDT:
      return "INT_WDT";

    case ESP_RST_TASK_WDT:
      return "TASK_WDT";

    case ESP_RST_WDT:
      return "WDT";

    case ESP_RST_DEEPSLEEP:
      return "DEEPSLEEP";

    case ESP_RST_BROWNOUT:
      return "BROWNOUT";

    case ESP_RST_SDIO:
      return "SDIO";

    default:
      return "OTHER";
  }
}

void begin() {
  if (rtcMagic != kMagic) {
    rtcMagic = kMagic;
    rtcStage = 0;
    rtcBootCount = 0;
  }

  previous = rtcStage;
  current = 0;

  ++rtcBootCount;
  bootCount = rtcBootCount;

  Serial.begin(115200);
  delay(35);

  Serial.println();
  Serial.println("========================================");
  Serial.println(" CONTROL//OS SELF-DEBUG FIRMWARE");
  Serial.println("========================================");

  Serial.printf(
      "[DBG] boot count     : %lu\n",
      bootCount
  );

  Serial.printf(
      "[DBG] reset reason   : %s\n",
      resetReasonName()
  );

  Serial.printf(
      "[DBG] previous stage : %lu (%s)\n",
      previous,
      stageName(previous)
  );
}

void attachDisplay(TFT_eSPI* display) {
  tft = display;
  screenLine = 0;

  if (tft == nullptr) {
    return;
  }

  tft->fillScreen(TFT_BLACK);
  tft->setTextDatum(TL_DATUM);

  drawLine(
      "CONTROL//OS SELF DEBUG",
      TFT_GREEN
  );

  drawLine(
      String("RESET: ") + resetReasonName(),
      TFT_WHITE
  );

  drawLine(
      String("PREV: ") + stageName(previous),
      previous >= static_cast<uint32_t>(Stage::LauncherReady)
          ? TFT_GREEN
          : TFT_YELLOW
  );
}

bool enableFileLogging() {
  if (!fsMounted) {
    fsMounted = LittleFS.begin(true);
  }

  if (!fsMounted) {
    Serial.println(
        "[DBG] ERROR: LittleFS mount failed"
    );

    if (tft != nullptr) {
      drawLine(
          "FILE LOG: MOUNT FAIL",
          TFT_RED
      );
    }

    return false;
  }

  if (!LittleFS.exists(kLogDir)) {
    LittleFS.mkdir(kLogDir);
  }

  rotateLogIfNeeded();

  fs::File file =
      LittleFS.open(kLogFile, FILE_APPEND);

  if (!file) {
    Serial.println(
        "[DBG] ERROR: debug logfile open failed"
    );

    if (tft != nullptr) {
      drawLine(
          "FILE LOG: OPEN FAIL",
          TFT_RED
      );
    }

    return false;
  }

  fileLogging = true;

  file.println();
  file.println("========================================");
  file.println("CONTROL//OS SELF-DEBUG BOOT SESSION");
  file.println("========================================");

  file.printf(
      "boot_count=%lu\n",
      bootCount
  );

  file.printf(
      "reset_reason=%s\n",
      resetReasonName()
  );

  file.printf(
      "previous_stage=%lu (%s)\n",
      previous,
      stageName(previous)
  );

  file.printf(
      "free_heap=%u\n",
      ESP.getFreeHeap()
  );

  file.printf(
      "free_psram=%u\n",
      ESP.getFreePsram()
  );

  file.println("----------------------------------------");
  file.flush();
  file.close();

  Serial.printf(
      "[DBG] file logging enabled: %s\n",
      kLogFile
  );

  if (tft != nullptr) {
    drawLine(
        "FILE LOG: READY",
        TFT_GREEN
    );
  }

  return true;
}

void mark(
    Stage stage,
    const char* message
) {
  current =
      static_cast<uint32_t>(stage);

  rtcStage = current;

  const String line =
      stageLine(
          current,
          message
      );

  Serial.print("[DBG] ");
  Serial.println(line);

  appendLogLine(line);

  if (tft != nullptr) {
    drawLine(
        String("[") +
            String(current) +
            "] " +
            stageName(current),
        TFT_GREEN
    );
  }
}

void heartbeat() {
  const uint32_t now = millis();

  if (
      now - lastHeartbeatMs <
      5000
  ) {
    return;
  }

  lastHeartbeatMs = now;

  String line;
  line.reserve(120);

  line += "[";
  line += String(now);
  line += " ms] HEARTBEAT";
  line += " stage=";
  line += stageName(current);
  line += " heap=";
  line += String(ESP.getFreeHeap());
  line += " psram=";
  line += String(ESP.getFreePsram());

  Serial.print("[DBG] ");
  Serial.println(line);

  appendLogLine(line);
}

uint32_t previousStage() {
  return previous;
}

uint32_t currentStage() {
  return current;
}

String logPath() {
  return String(kLogFile);
}

}  // namespace RuntimeDebug
