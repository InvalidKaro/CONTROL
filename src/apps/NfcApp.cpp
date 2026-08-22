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

void NfcApp::onEncoder(int delta) {
  if (!delta) return;
  view_ = (view_ + 1) % 3;
}

void NfcApp::onSelect() {
  if (view_ != 2) {
    uid_ = "-";
    uidLength_ = 0;
    return;
  }
  if (!ready_ || uidLength_ == 0) {
    writeStatus_ = "Tap NTAG first";
    return;
  }
  uint8_t cc[4] = {0};
  if (!nfc_.ntag2xx_ReadPage(3, cc) || cc[0] != 0xE1 || cc[1] != 0x10) {
    writeStatus_ = "Not NDEF NTAG";
    return;
  }
  const uint8_t dataLength = cc[2] * 8;
  char url[] = "172.0.0.1";
  writeStatus_ = nfc_.ntag2xx_WriteNDEFURI(NDEF_URIPREFIX_HTTP, url, dataLength) ? "WROTE http://172.0.0.1" : "WRITE FAILED";
}

void NfcApp::addHistory(const String& uid) {
  if (historyCount_ && history_[0] == uid) return;
  for (int i = 4; i > 0; --i) history_[i] = history_[i - 1];
  history_[0] = uid;
  if (historyCount_ < 5) ++historyCount_;
}

void NfcApp::tick(uint32_t nowMs) {
  if (!ready_ || nowMs - lastPollMs_ < 250) return;
  lastPollMs_ = nowMs;

  uint8_t uid[7] = {0};
  uint8_t uidLength = 0;
  if (nfc_.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 20)) {
    uidLength_ = uidLength;
    String next;
    for (uint8_t i = 0; i < uidLength; ++i) {
      if (uid[i] < 0x10) next += "0";
      next += String(uid[i], HEX);
      if (i + 1 < uidLength) next += ":";
    }
    next.toUpperCase();
    if (next != uid_) {
      addHistory(next);
      uint8_t cc[4] = {0};
      ndefReady_ = nfc_.ntag2xx_ReadPage(3, cc) && cc[0] == 0xE1 && cc[1] == 0x10;
      capacityBytes_ = ndefReady_ ? static_cast<uint16_t>(cc[2]) * 8U : 0;
    }
    uid_ = next;
  }
}

void NfcApp::render(TFT_eSPI& tft) {
  tft.fillScreen(Theme::Bg);
  Ui::header(tft, "NFC INSPECTOR", ready_ ? (view_ == 0 ? "LIVE" : (view_ == 1 ? "HISTORY" : "NDEF WRITE")) : "PN532 ERR");
  if (view_ == 0) {
    Ui::panel(tft, 12, 38, 296, 82);
    Ui::centered(tft, 48, "ISO14443A UID", Theme::Muted, 2);
    Ui::centered(tft, 76, uid_, ready_ ? Theme::PrimaryBright : Theme::Error, 2);
    Ui::centered(tft, 99, uidLength_ ? String(uidLength_) + " UID bytes" : "Tap a tag", Theme::Text, 1);
    if (uidLength_) Ui::centered(tft, 111, ndefReady_ ? "NDEF NTAG / " + String(capacityBytes_) + " B" : "NDEF not detected", ndefReady_ ? Theme::PrimaryBright : Theme::Muted, 1);
  } else if (view_ == 1) {
    for (uint8_t i = 0; i < historyCount_; ++i) {
      const int y = 34 + i * 21;
      Ui::panel(tft, 10, y, 300, 18);
      Ui::text(tft, 16, y + 2, String(i + 1) + "  " + history_[i], i == 0 ? Theme::PrimaryBright : Theme::Text, 1);
    }
    if (!historyCount_) Ui::centered(tft, 74, "No tags seen yet", Theme::Muted, 2);
  } else {
    Ui::panel(tft, 12, 40, 296, 78);
    Ui::centered(tft, 51, "Write normal NDEF URL", Theme::Muted, 2);
    Ui::centered(tft, 76, "http://172.0.0.1", Theme::PrimaryBright, 2);
    Ui::centered(tft, 102, writeStatus_, writeStatus_.indexOf("WROTE") >= 0 ? Theme::PrimaryBright : Theme::Text, 1);
  }
  Ui::footer(tft, "TURN: live/history/write", view_ == 2 ? "ENC: write NDEF URL" : "ENC: clear current");
}
