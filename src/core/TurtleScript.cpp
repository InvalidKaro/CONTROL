#include "TurtleScript.h"

#include <math.h>

bool TurtleScript::load(fs::FS& fs, const String& path) {
  File file = fs.open(path, FILE_READ);
  if (!file || file.isDirectory()) {
    status_ = "open failed";
    return false;
  }
  lineCount_ = 0;
  while (file.available() && lineCount_ < 96) {
    String line = file.readStringUntil('\n');
    line.trim();
    if (line.length() && !line.startsWith("#")) lines_[lineCount_++] = line;
  }
  file.close();
  scriptName_ = path;
  status_ = String(lineCount_) + " lines";
  return lineCount_ > 0;
}

void TurtleScript::start() {
  pc_ = 0;
  x_ = 160.0f;
  y_ = 85.0f;
  angle_ = 0.0f;
  pen_ = true;
  waitUntilMs_ = 0;
  running_ = lineCount_ > 0;
  status_ = running_ ? "running" : "empty";
}

void TurtleScript::stop() {
  running_ = false;
  status_ = "stopped";
}

void TurtleScript::tick(uint32_t nowMs) {
  if (!running_ || tft_ == nullptr) return;
  if (waitUntilMs_ && static_cast<int32_t>(nowMs - waitUntilMs_) < 0) return;
  waitUntilMs_ = 0;

  uint8_t budget = 0;
  while (running_ && pc_ < lineCount_ && budget++ < 4) {
    const String line = lines_[pc_++];
    if (!execute(line, nowMs)) break;
  }
  if (pc_ >= lineCount_) {
    running_ = false;
    status_ = "done";
  }
}

bool TurtleScript::execute(const String& raw, uint32_t nowMs) {
  String line = raw;
  const int split = line.indexOf(' ');
  String cmd = split < 0 ? line : line.substring(0, split);
  String arg = split < 0 ? "" : line.substring(split + 1);
  cmd.toUpperCase();
  arg.trim();

  if (cmd == "CLEAR") {
    tft_->fillScreen(parseColor(arg.length() ? arg : "#000000"));
  } else if (cmd == "COLOR") {
    color_ = parseColor(arg);
  } else if (cmd == "GOTO") {
    const int sp = arg.indexOf(' ');
    if (sp > 0) {
      const float nx = arg.substring(0, sp).toFloat();
      const float ny = arg.substring(sp + 1).toFloat();
      if (pen_) tft_->drawLine(static_cast<int>(x_), static_cast<int>(y_), static_cast<int>(nx), static_cast<int>(ny), color_);
      x_ = nx; y_ = ny;
    }
  } else if (cmd == "FORWARD" || cmd == "FD") {
    move(arg.toFloat(), pen_);
  } else if (cmd == "BACK" || cmd == "BK") {
    move(-arg.toFloat(), pen_);
  } else if (cmd == "RIGHT" || cmd == "RT") {
    angle_ += arg.toFloat();
  } else if (cmd == "LEFT" || cmd == "LT") {
    angle_ -= arg.toFloat();
  } else if (cmd == "PENUP") {
    pen_ = false;
  } else if (cmd == "PENDOWN") {
    pen_ = true;
  } else if (cmd == "WAIT") {
    waitUntilMs_ = nowMs + static_cast<uint32_t>(max(0L, arg.toInt()));
    return false;
  } else if (cmd == "TEXT") {
    tft_->setTextColor(color_);
    tft_->setTextDatum(TL_DATUM);
    tft_->drawString(arg, static_cast<int>(x_), static_cast<int>(y_), 2);
  } else if (cmd == "CIRCLE") {
    tft_->drawCircle(static_cast<int>(x_), static_cast<int>(y_), max(1, arg.toInt()), color_);
  } else if (cmd == "RECT") {
    const int sp = arg.indexOf(' ');
    if (sp > 0) tft_->drawRect(static_cast<int>(x_), static_cast<int>(y_), arg.substring(0, sp).toInt(), arg.substring(sp + 1).toInt(), color_);
  } else if (cmd == "ANGLE") {
    angle_ = arg.toFloat();
  }
  return true;
}

void TurtleScript::move(float distance, bool draw) {
  const float radians = angle_ * DEG_TO_RAD;
  const float nx = x_ + cosf(radians) * distance;
  const float ny = y_ + sinf(radians) * distance;
  if (draw) tft_->drawLine(static_cast<int>(x_), static_cast<int>(y_), static_cast<int>(nx), static_cast<int>(ny), color_);
  x_ = nx;
  y_ = ny;
}

uint16_t TurtleScript::parseColor(String token) const {
  token.trim();
  if (token.startsWith("#")) token.remove(0, 1);
  if (token.length() != 6) return color_;
  const uint32_t rgb = strtoul(token.c_str(), nullptr, 16);
  const uint8_t r = (rgb >> 16) & 0xFF;
  const uint8_t g = (rgb >> 8) & 0xFF;
  const uint8_t b = rgb & 0xFF;
  return static_cast<uint16_t>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}
