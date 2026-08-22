#include "NfcApp.h"

#include <Wire.h>
#include "Ui.h"
#include "board_pins.h"
#include "theme.h"

NfcApp::NfcApp() : nfc_(BoardPins::Pn532Irq, BoardPins::Pn532Reset, &Wire) {}

void NfcApp::begin() {
  nfc_.begin();
  const uint32_t version = nfc_.getFirmwareVersion();
  ready_ = version != 0;
  if (ready_) {
    nfc_.SAMConfig();
    nfc_.setPassiveActivationRetries(0x01);
  }
}

void NfcApp::tick(uint32_t nowMs) {
  if (!ready_ || nowMs - lastPollMs_ < 250) return;
  lastPollMs_ = nowMs;

  uint8_t uid[7] = {0};
  uint8_t uidLength = 0;
  if (nfc_.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 20)) {
    uidLength_ = uidLength;
    uid_ = "";
    for (uint8_t i = 0; i < uidLength; ++i) {
      if (uid[i] < 0x10) uid_ += "0";
      uid_ += String(uid[i], HEX);
      if (i + 1 < uidLength) uid_ += ":";
    }
    uid_.toUpperCase();
  }
}

void NfcApp::render(TFT_eSPI& tft) {
  tft.fillScreen(Theme::Bg);
  Ui::header(tft, "NFC INSPECTOR", ready_ ? "PN532 READY" : "PN532 ERR");
  Ui::panel(tft, 12, 38, 296, 82);
  Ui::centered(tft, 48, "ISO14443A UID", Theme::Muted, 2);
  Ui::centered(tft, 76, uid_, ready_ ? Theme::PrimaryBright : Theme::Error, 2);
  Ui::centered(tft, 103, uidLength_ ? String(uidLength_) + " bytes" : "Tap a tag", Theme::Text, 1);
  Ui::footer(tft, "Read-only inspector", "USER: back");
}
