#include "ControlOS.h"

#include <SPI.h>
#include <Wire.h>

#include "Ui.h"
#include "RuntimeDebug.h"
#include "board_pins.h"
#include "theme.h"

ControlOS::ControlOS()
    : dashboard_(power_, web_),
      diagnostics_(sounds_),
      ledApp_(leds_),
      quick_(leds_, power_, themes_, sounds_),
      system_(power_),
      settings_(themes_, power_, automations_),
      webApp_(web_),
      apps_{
          &dashboard_,
          &subGhz_,
          &wifi_,
          &ble_,
          &nfc_,
          &ir_,
          &nrf_,
          &audio_,
          &ledApp_,
          &qr_,
          &scripts_,
          &tools_,
          &quick_,
          &storage_,
          &system_,
          &diagnostics_,
          &settings_,
          &webApp_
      } {}

void ControlOS::begin() {
  RuntimeDebug::mark(
      RuntimeDebug::Stage::EnterControlOS,
      "ControlOS::begin entered"
  );

  pinMode(BoardPins::PowerEnable, OUTPUT);
  digitalWrite(BoardPins::PowerEnable, HIGH);

  pinMode(BoardPins::DisplayBacklight, OUTPUT);
  digitalWrite(BoardPins::DisplayBacklight, HIGH);

  RuntimeDebug::mark(
      RuntimeDebug::Stage::PowerPins,
      "power and backlight enabled"
  );

  /*
   * GPIO40 is shared with the known T-Embed CC1101 display reset /
   * speaker clock path. In the self-debug firmware the speaker stays
   * disabled during startup.
   */
  pinMode(BoardPins::VoiceLrclk, OUTPUT);
  digitalWrite(BoardPins::VoiceLrclk, HIGH);
  delay(5);
  digitalWrite(BoardPins::VoiceLrclk, LOW);
  delay(25);
  digitalWrite(BoardPins::VoiceLrclk, HIGH);
  delay(120);

  RuntimeDebug::mark(
      RuntimeDebug::Stage::DisplayReset,
      "display reset pulse completed"
  );

  pinMode(BoardPins::SdCs, OUTPUT);
  pinMode(BoardPins::Cc1101Cs, OUTPUT);
  pinMode(BoardPins::NrfCs, OUTPUT);
  pinMode(BoardPins::NrfCe, OUTPUT);

  digitalWrite(BoardPins::NrfCe, LOW);
  digitalWrite(BoardPins::SdCs, HIGH);
  digitalWrite(BoardPins::Cc1101Cs, HIGH);
  digitalWrite(BoardPins::NrfCs, HIGH);

  RuntimeDebug::mark(
      RuntimeDebug::Stage::DisplayInit,
      "calling TFT init"
  );

  tft_.init();
  tft_.setRotation(3);
  tft_.setTextWrap(false);

  tft_.writecommand(0x11);
  delay(120);

  tft_.writecommand(0x29);
  delay(20);

  digitalWrite(
      BoardPins::DisplayBacklight,
      HIGH
  );

  RuntimeDebug::attachDisplay(&tft_);

  RuntimeDebug::mark(
      RuntimeDebug::Stage::DisplayReady,
      "ST7789 initialized and visible"
  );

  SPI.begin(
      BoardPins::SpiSck,
      BoardPins::SpiMiso,
      BoardPins::SpiMosi
  );

  RuntimeDebug::mark(
      RuntimeDebug::Stage::SpiReady,
      "shared SPI bus initialized"
  );

  Wire.begin(
      BoardPins::I2cSda,
      BoardPins::I2cScl
  );

  RuntimeDebug::mark(
      RuntimeDebug::Stage::I2cReady,
      "I2C bus initialized"
  );

  /*
   * Persistent file logger:
   *
   * /logs/controlos-debug.log
   *
   * Every boot appends a session. The file rotates automatically at
   * roughly 64 KiB to /logs/controlos-debug.old.log.
   */
  if (RuntimeDebug::enableFileLogging()) {
    RuntimeDebug::mark(
        RuntimeDebug::Stage::FileLogReady,
        "/logs/controlos-debug.log active"
    );
  }

  themes_.begin();

  RuntimeDebug::mark(
      RuntimeDebug::Stage::ThemeReady,
      "ThemeManager ready"
  );

  power_.begin();

  RuntimeDebug::mark(
      RuntimeDebug::Stage::PowerReady,
      "PowerManager ready"
  );

  automations_.begin();

  RuntimeDebug::mark(
      RuntimeDebug::Stage::AutomationReady,
      "AutomationEngine ready"
  );

  input_.begin();

  RuntimeDebug::mark(
      RuntimeDebug::Stage::InputReady,
      "Input ready"
  );

  leds_.begin();

  RuntimeDebug::mark(
      RuntimeDebug::Stage::LedReady,
      "LED controller ready; speaker deferred"
  );

  /*
   * Speaker remains disabled in this diagnostic build so a possible
   * GPIO40 display/speaker conflict cannot hide the real boot stage.
   */

  RuntimeDebug::mark(
      RuntimeDebug::Stage::WebStarting,
      "starting WebUI / storage / Wi-Fi"
  );

  web_.begin(
      apps_,
      AppCount,
      &leds_,
      &power_,
      &themes_,
      &automations_
  );

  RuntimeDebug::mark(
      RuntimeDebug::Stage::WebReady,
      "WebUI begin returned"
  );

  BootSequence::RuntimeStatus bootStatus;

  bootStatus.webReady =
      web_.running();

  bootStatus.flashReady =
      web_.flashReady();

  bootStatus.sdReady =
      web_.sdReady();

  bootStatus.ip =
      web_.ip();

  RuntimeDebug::mark(
      RuntimeDebug::Stage::BootSequence,
      "starting ControlOS boot animation"
  );

  boot_.begin(
      tft_,
      bootStatus
  );

  syncWebState();
}

void ControlOS::openSelected() {
  if (active_ != nullptr) {
    active_->end();
  }

  active_ = apps_[selected_];
  active_->begin();

  sounds_.click();

  lastFrameMs_ = 0;

  syncWebState();

  Serial.printf(
      "[UI] Open app: %s\n",
      active_->name()
  );
}

void ControlOS::closeApp() {
  if (active_ == nullptr) {
    return;
  }

  Serial.printf(
      "[UI] Close app: %s\n",
      active_->name()
  );

  active_->end();
  sounds_.click();

  active_ = nullptr;

  syncWebState();
  renderLauncher();
}

void ControlOS::renderLauncher() {
  /*
   * Re-assert the display state on every launcher render.
   *
   * The boot sequence touches shared SPI devices (CC1101 / nRF24).
   * Reasserting the ST7789 orientation + backlight here makes the
   * transition back to the normal UI deterministic.
   */
  digitalWrite(
      BoardPins::DisplayBacklight,
      HIGH
  );

  tft_.setRotation(3);
  tft_.setTextWrap(false);

  tft_.fillScreen(Theme::Bg);

  Ui::header(
      tft_,
      "CONTROL//OS",
      "v1.0 CORE DECK"
  );

  const int centerX = 160;
  const int centerY = 86;

  tft_.drawCircle(
      centerX,
      centerY,
      43,
      Theme::PrimaryDim
  );

  tft_.drawCircle(
      centerX,
      centerY,
      44,
      Theme::PrimaryDim
  );

  for (int offset = -2; offset <= 2; ++offset) {
    const int index =
        (
            selected_ +
            offset +
            AppCount
        ) %
        AppCount;

    const int x =
        centerX +
        offset * 57;

    const int y =
        centerY -
        (
            offset == 0
                ? 31
                : 22
        );

    const int width =
        offset == 0
            ? 52
            : 45;

    const int height =
        offset == 0
            ? 62
            : 45;

    Ui::panel(
        tft_,
        x - width / 2,
        y,
        width,
        height,
        offset == 0
    );

    tft_.setTextDatum(MC_DATUM);

    tft_.setTextColor(
        offset == 0
            ? Theme::PrimaryBright
            : Theme::Muted,
        Theme::Panel
    );

    tft_.drawString(
        apps_[index]->shortName(),
        x,
        y + height / 2,
        offset == 0
            ? 2
            : 1
    );
  }

  tft_.setTextDatum(TC_DATUM);

  tft_.setTextColor(
      Theme::Text,
      Theme::Bg
  );

  tft_.drawString(
      apps_[selected_]->name(),
      centerX,
      132,
      2
  );

  tft_.setTextDatum(TL_DATUM);

  Ui::footer(
      tft_,
      "TURN: navigate",
      "ENC: open"
  );

  syncWebState();
}

void ControlOS::applyRemoteCommand(
    const RemoteCommand& command,
    InputEvents& events
) {
  switch (command.type) {
    case RemoteCommandType::Encoder:
      events.encoderDelta += command.value;
      break;

    case RemoteCommandType::Select:
      events.selectPressed = true;
      break;

    case RemoteCommandType::Back:
      events.backPressed = true;
      break;

    case RemoteCommandType::Home:
      if (active_ != nullptr) {
        closeApp();
      }
      break;

    case RemoteCommandType::OpenApp:
      if (
          command.value >= 0 &&
          command.value < AppCount
      ) {
        selected_ = command.value;
        openSelected();
      }
      break;

    case RemoteCommandType::Reboot:
      rebootAtMs_ =
          millis() + 450;
      break;

    case RemoteCommandType::ScreenOn:
      digitalWrite(
          BoardPins::DisplayBacklight,
          HIGH
      );
      break;

    case RemoteCommandType::ScreenOff:
      digitalWrite(
          BoardPins::DisplayBacklight,
          LOW
      );
      break;

    case RemoteCommandType::None:
    default:
      break;
  }
}

void ControlOS::syncWebState() {
  web_.setUiState(
      selected_,
      active_ == nullptr
          ? "Launcher"
          : active_->name()
  );
}

void ControlOS::loop() {
  static bool runtimeMarked = false;

  if (!runtimeMarked) {
    runtimeMarked = true;

    RuntimeDebug::mark(
        RuntimeDebug::Stage::LoopRunning,
        "ControlOS main loop entered"
    );
  }
  /*
   * These locals deliberately live for the lifetime of the firmware.
   * They allow the boot -> launcher handoff to be acknowledged exactly
   * once without changing ControlOS.h.
   */
  static bool launcherInitialized = false;
  static uint32_t launcherEnteredMs = 0;
  static uint32_t lastLauncherRecoveryFrameMs = 0;

  const uint32_t nowMs =
      millis();

  leds_.tick(nowMs);
  sounds_.tick(nowMs);
  power_.tick(nowMs);

  const auto& battery =
      power_.battery();

  automations_.tick(
      nowMs,
      battery.soc,
      battery.present,
      web_.clientCount(),
      leds_,
      power_
  );

  if (
      rebootAtMs_ != 0 &&
      static_cast<int32_t>(
          nowMs - rebootAtMs_
      ) >= 0
  ) {
    ESP.restart();
  }

  web_.loop();

  InputEvents events =
      input_.poll(nowMs);

  /*
   * ------------------------------------------------------------
   * BOOT MODE
   * ------------------------------------------------------------
   */
  if (boot_.active()) {
    RemoteCommand bootCommand;
    uint8_t bootProcessed = 0;

    while (
        bootProcessed < 4 &&
        web_.popCommand(bootCommand)
    ) {
      if (
          bootCommand.type ==
              RemoteCommandType::Reboot ||
          bootCommand.type ==
              RemoteCommandType::ScreenOn ||
          bootCommand.type ==
              RemoteCommandType::ScreenOff
      ) {
        applyRemoteCommand(
            bootCommand,
            events
        );
      } else if (
          bootCommand.type ==
              RemoteCommandType::Select ||
          bootCommand.type ==
              RemoteCommandType::Home
      ) {
        boot_.skip();
      }

      ++bootProcessed;
    }

    if (
        events.selectPressed ||
        events.backPressed
    ) {
      boot_.skip();
    }

    boot_.tick(nowMs);

    /*
     * IMPORTANT:
     *
     * BootSequence::finish() turns active() false during tick().
     * Handle the transition based on the post-tick state instead
     * of relying only on finished().
     */
    if (!boot_.active()) {
      digitalWrite(
          BoardPins::DisplayBacklight,
          HIGH
      );

      tft_.setRotation(3);

      renderLauncher();

      launcherInitialized = true;
      launcherEnteredMs = nowMs;
      lastLauncherRecoveryFrameMs = nowMs;

      RuntimeDebug::mark(
          RuntimeDebug::Stage::LauncherReady,
          "boot finished; launcher rendered"
      );

      Serial.println(
          "[UI] Boot finished -> launcher rendered"
      );
    }

    syncWebState();

    delay(1);
    return;
  }

  /*
   * Fallback:
   *
   * If the boot sequence became inactive between loop iterations for
   * any reason, guarantee that the launcher still receives its first
   * frame.
   */
  if (
      !launcherInitialized &&
      boot_.finished()
  ) {
    digitalWrite(
        BoardPins::DisplayBacklight,
        HIGH
    );

    tft_.setRotation(3);

    renderLauncher();

    launcherInitialized = true;
    launcherEnteredMs = nowMs;
    lastLauncherRecoveryFrameMs = nowMs;

    RuntimeDebug::mark(
        RuntimeDebug::Stage::LauncherReady,
        "launcher fallback rendered"
    );

    Serial.println(
        "[UI] Launcher fallback render"
    );
  }

  /*
   * ------------------------------------------------------------
   * NORMAL REMOTE COMMANDS
   * ------------------------------------------------------------
   */
  RemoteCommand command;
  uint8_t processed = 0;

  while (
      processed < 4 &&
      web_.popCommand(command)
  ) {
    applyRemoteCommand(
        command,
        events
    );

    ++processed;
  }

  /*
   * ------------------------------------------------------------
   * LAUNCHER
   * ------------------------------------------------------------
   */
  if (active_ == nullptr) {
    /*
     * Ignore physical select/back actions for a short moment after
     * the boot animation. This prevents a button held during boot
     * from immediately opening an app.
     */
    const bool inputUnlocked =
        nowMs - launcherEnteredMs >= 650;

    if (
        inputUnlocked &&
        events.encoderDelta != 0
    ) {
      selected_ =
          (
              selected_ +
              events.encoderDelta +
              AppCount * 8
          ) %
          AppCount;

      renderLauncher();

      lastLauncherRecoveryFrameMs =
          nowMs;
    }

    if (
        inputUnlocked &&
        events.backHeld
    ) {
      selected_ = 12;
      openSelected();
      return;
    }

    if (
        inputUnlocked &&
        events.selectPressed
    ) {
      openSelected();
      return;
    }

    /*
     * During the first few seconds after boot, redraw the launcher
     * periodically. This acts as a recovery guard if another shared
     * subsystem disturbed the display directly after the boot probes.
     *
     * It stops after five seconds, so the normal launcher does not
     * continuously repaint or flicker.
     */
    if (
        launcherInitialized &&
        nowMs - launcherEnteredMs < 5000 &&
        nowMs - lastLauncherRecoveryFrameMs >= 500
    ) {
      renderLauncher();

      lastLauncherRecoveryFrameMs =
          nowMs;
    }

    syncWebState();

    delay(1);
    return;
  }

  /*
   * ------------------------------------------------------------
   * ACTIVE APP
   * ------------------------------------------------------------
   */
  if (events.backPressed) {
    closeApp();
    return;
  }

  if (events.encoderDelta != 0) {
    active_->onEncoder(
        events.encoderDelta
    );
  }

  if (events.selectPressed) {
    active_->onSelect();
  }

  active_->tick(nowMs);

  if (
      nowMs - lastFrameMs_ >= 66
  ) {
    lastFrameMs_ = nowMs;

    active_->render(tft_);
  }

  syncWebState();

  delay(1);
}
