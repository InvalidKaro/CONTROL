#include "ControlOS.h"

#include <SPI.h>
#include <Wire.h>

#include "Ui.h"
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
      apps_{&dashboard_, &subGhz_, &wifi_, &ble_, &nfc_, &ir_, &nrf_, &audio_, &ledApp_,
            &qr_, &scripts_, &tools_, &quick_, &storage_, &system_, &diagnostics_, &settings_, &webApp_} {}

void ControlOS::begin() {
  Serial.begin(115200);
  pinMode(BoardPins::PowerEnable, OUTPUT);
  digitalWrite(BoardPins::PowerEnable, HIGH);
  pinMode(BoardPins::DisplayBacklight, OUTPUT);
  digitalWrite(BoardPins::DisplayBacklight, HIGH);

  pinMode(BoardPins::SdCs, OUTPUT);
  pinMode(BoardPins::Cc1101Cs, OUTPUT);
  pinMode(BoardPins::NrfCs, OUTPUT);
  pinMode(BoardPins::NrfCe, OUTPUT);
  digitalWrite(BoardPins::NrfCe, LOW);
  digitalWrite(BoardPins::SdCs, HIGH);
  digitalWrite(BoardPins::Cc1101Cs, HIGH);
  digitalWrite(BoardPins::NrfCs, HIGH);

  SPI.begin(BoardPins::SpiSck, BoardPins::SpiMiso, BoardPins::SpiMosi);
  Wire.begin(BoardPins::I2cSda, BoardPins::I2cScl);

  themes_.begin();
  power_.begin();
  automations_.begin();

  tft_.init();
  tft_.setRotation(3);
  tft_.fillScreen(Theme::Bg);
  input_.begin();
  leds_.begin();
  sounds_.begin();

  web_.begin(apps_, AppCount, &leds_, &power_, &themes_, &automations_);

  BootSequence::RuntimeStatus bootStatus;
  bootStatus.webReady = web_.running();
  bootStatus.flashReady = web_.flashReady();
  bootStatus.sdReady = web_.sdReady();
  bootStatus.ip = web_.ip();
  boot_.begin(tft_, bootStatus);
  syncWebState();
}

void ControlOS::openSelected() {
  if (active_ != nullptr) active_->end();
  active_ = apps_[selected_];
  active_->begin();
  sounds_.click();
  lastFrameMs_ = 0;
  syncWebState();
}

void ControlOS::closeApp() {
  if (active_ == nullptr) return;
  active_->end();
  sounds_.click();
  active_ = nullptr;
  syncWebState();
  renderLauncher();
}

void ControlOS::renderLauncher() {
  tft_.fillScreen(Theme::Bg);
  Ui::header(tft_, "CONTROL//OS", "v1.0 CORE DECK");

  const int centerX = 160;
  const int centerY = 86;
  tft_.drawCircle(centerX, centerY, 43, Theme::PrimaryDim);
  tft_.drawCircle(centerX, centerY, 44, Theme::PrimaryDim);

  for (int offset = -2; offset <= 2; ++offset) {
    const int index = (selected_ + offset + AppCount) % AppCount;
    const int x = centerX + offset * 57;
    const int y = centerY - (offset == 0 ? 31 : 22);
    const int w = offset == 0 ? 52 : 45;
    const int h = offset == 0 ? 62 : 45;
    Ui::panel(tft_, x - w / 2, y, w, h, offset == 0);
    tft_.setTextDatum(MC_DATUM);
    tft_.setTextColor(offset == 0 ? Theme::PrimaryBright : Theme::Muted, Theme::Panel);
    tft_.drawString(apps_[index]->shortName(), x, y + h / 2, offset == 0 ? 2 : 1);
  }

  tft_.setTextDatum(TC_DATUM);
  tft_.setTextColor(Theme::Text, Theme::Bg);
  tft_.drawString(apps_[selected_]->name(), centerX, 132, 2);
  tft_.setTextDatum(TL_DATUM);
  Ui::footer(tft_, "TURN: navigate", "ENC: open");
  syncWebState();
}

void ControlOS::applyRemoteCommand(const RemoteCommand& command, InputEvents& events) {
  switch (command.type) {
    case RemoteCommandType::Encoder: events.encoderDelta += command.value; break;
    case RemoteCommandType::Select: events.selectPressed = true; break;
    case RemoteCommandType::Back: events.backPressed = true; break;
    case RemoteCommandType::Home: if (active_ != nullptr) closeApp(); break;
    case RemoteCommandType::OpenApp:
      if (command.value >= 0 && command.value < AppCount) { selected_ = command.value; openSelected(); }
      break;
    case RemoteCommandType::Reboot: rebootAtMs_ = millis() + 450; break;
    case RemoteCommandType::ScreenOn: digitalWrite(BoardPins::DisplayBacklight, HIGH); break;
    case RemoteCommandType::ScreenOff: digitalWrite(BoardPins::DisplayBacklight, LOW); break;
    case RemoteCommandType::None: default: break;
  }
}

void ControlOS::syncWebState() { web_.setUiState(selected_, active_ == nullptr ? "Launcher" : active_->name()); }

void ControlOS::loop() {
  const uint32_t nowMs = millis();
  leds_.tick(nowMs);
  sounds_.tick(nowMs);
  power_.tick(nowMs);
  const auto& battery = power_.battery();
  automations_.tick(nowMs, battery.soc, battery.present, web_.clientCount(), leds_, power_);

  if (rebootAtMs_ != 0 && static_cast<int32_t>(nowMs - rebootAtMs_) >= 0) ESP.restart();
  web_.loop();
  InputEvents events = input_.poll(nowMs);

  if (boot_.active()) {
    RemoteCommand bootCommand;
    uint8_t bootProcessed = 0;
    while (bootProcessed < 4 && web_.popCommand(bootCommand)) {
      if (bootCommand.type == RemoteCommandType::Reboot || bootCommand.type == RemoteCommandType::ScreenOn || bootCommand.type == RemoteCommandType::ScreenOff) {
        applyRemoteCommand(bootCommand, events);
      } else if (bootCommand.type == RemoteCommandType::Select || bootCommand.type == RemoteCommandType::Home) {
        boot_.skip();
      }
      ++bootProcessed;
    }
    if (events.selectPressed || events.backPressed) boot_.skip();
    boot_.tick(nowMs);
    if (boot_.finished()) renderLauncher();
    syncWebState();
    delay(1);
    return;
  }

  RemoteCommand command;
  uint8_t processed = 0;
  while (processed < 4 && web_.popCommand(command)) { applyRemoteCommand(command, events); ++processed; }

  if (active_ == nullptr) {
    if (events.encoderDelta != 0) {
      selected_ = (selected_ + events.encoderDelta + AppCount * 8) % AppCount;
      renderLauncher();
    }
    if (events.backHeld) { selected_ = 12; openSelected(); return; }
    if (events.selectPressed) openSelected();
    syncWebState();
    delay(1);
    return;
  }

  if (events.backPressed) { closeApp(); return; }
  if (events.encoderDelta != 0) active_->onEncoder(events.encoderDelta);
  if (events.selectPressed) active_->onSelect();

  active_->tick(nowMs);
  if (nowMs - lastFrameMs_ >= 66) {
    lastFrameMs_ = nowMs;
    active_->render(tft_);
  }
  syncWebState();
  delay(1);
}
