#include "TurtleScript.h"

#include <math.h>

bool TurtleScript::load(
    fs::FS& fs,
    const String& path
) {
  fs::File file =
      fs.open(
          path,
          FILE_READ
      );

  if (!file || file.isDirectory()) {
    status_ = "open failed";
    return false;
  }

  lineCount_ = 0;

  while (
      file.available() &&
      lineCount_ < 96
  ) {
    String line =
        file.readStringUntil('\n');

    line.trim();

    if (
        line.length() &&
        !line.startsWith("#")
    ) {
      lines_[lineCount_++] =
          line;
    }
  }

  file.close();

  scriptName_ = path;
  status_ =
      String(lineCount_) +
      " lines";

  return lineCount_ > 0;
}

void TurtleScript::start() {
  pc_ = 0;
  x_ = 160.0f;
  y_ = 85.0f;
  angle_ = 0.0f;
  pen_ = true;
  waitUntilMs_ = 0;

  running_ =
      lineCount_ > 0;

  status_ =
      running_
          ? "running"
          : "empty";
}

void TurtleScript::stop() {
  running_ = false;
  status_ = "stopped";
}

void TurtleScript::tick(uint32_t nowMs) {
  if (
      !running_ ||
      tft_ == nullptr
  ) {
    return;
  }

  if (
      waitUntilMs_ &&
      static_cast<int32_t>(
          nowMs - waitUntilMs_
      ) < 0
  ) {
    return;
  }

  waitUntilMs_ = 0;

  uint8_t budget = 0;

  while (
      running_ &&
      pc_ < lineCount_ &&
      budget++ < 4
  ) {
    const String line =
        lines_[pc_++];

    if (!execute(line, nowMs)) {
      break;
    }
  }

  if (pc_ >= lineCount_) {
    running_ = false;
    status_ = "done";
  }
}

bool TurtleScript::execute(
    const String& raw,
    uint32_t nowMs
) {
  String line = raw;

  const int split =
      line.indexOf(' ');

  String cmd =
      split < 0
          ? line
          : line.substring(0, split);

  String arg =
      split < 0
          ? ""
          : line.substring(split + 1);

  cmd.toUpperCase();
  arg.trim();

  if (cmd == "CLEAR") {
    tft_->fillScreen(
        parseColor(
            arg.length()
                ? arg
                : "#000000"
        )
    );
  } else if (cmd == "COLOR") {
    color_ =
        parseColor(arg);
  } else if (cmd == "GOTO") {
    const int separator =
        arg.indexOf(' ');

    if (separator > 0) {
      const float nextX =
          arg.substring(
              0,
              separator
          ).toFloat();

      const float nextY =
          arg.substring(
              separator + 1
          ).toFloat();

      if (pen_) {
        tft_->drawLine(
            static_cast<int>(x_),
            static_cast<int>(y_),
            static_cast<int>(nextX),
            static_cast<int>(nextY),
            color_
        );
      }

      x_ = nextX;
      y_ = nextY;
    }
  } else if (
      cmd == "FORWARD" ||
      cmd == "FD"
  ) {
    move(
        arg.toFloat(),
        pen_
    );
  } else if (
      cmd == "BACK" ||
      cmd == "BK"
  ) {
    move(
        -arg.toFloat(),
        pen_
    );
  } else if (
      cmd == "RIGHT" ||
      cmd == "RT"
  ) {
    angle_ +=
        arg.toFloat();
  } else if (
      cmd == "LEFT" ||
      cmd == "LT"
  ) {
    angle_ -=
        arg.toFloat();
  } else if (cmd == "PENUP") {
    pen_ = false;
  } else if (cmd == "PENDOWN") {
    pen_ = true;
  } else if (cmd == "WAIT") {
    const long waitMs =
        arg.toInt();

    waitUntilMs_ =
        nowMs +
        static_cast<uint32_t>(
            waitMs > 0L
                ? waitMs
                : 0L
        );

    return false;
  } else if (cmd == "TEXT") {
    tft_->setTextColor(color_);
    tft_->setTextDatum(TL_DATUM);

    tft_->drawString(
        arg,
        static_cast<int>(x_),
        static_cast<int>(y_),
        2
    );
  } else if (cmd == "CIRCLE") {
    const long parsedRadius =
        arg.toInt();

    const int radius =
        parsedRadius > 1L
            ? static_cast<int>(
                  min(parsedRadius, 1024L)
              )
            : 1;

    tft_->drawCircle(
        static_cast<int>(x_),
        static_cast<int>(y_),
        radius,
        color_
    );
  } else if (cmd == "RECT") {
    const int separator =
        arg.indexOf(' ');

    if (separator > 0) {
      const long parsedWidth =
          arg.substring(
              0,
              separator
          ).toInt();

      const long parsedHeight =
          arg.substring(
              separator + 1
          ).toInt();

      const int width =
          static_cast<int>(
              constrain(
                  parsedWidth,
                  1L,
                  2048L
              )
          );

      const int height =
          static_cast<int>(
              constrain(
                  parsedHeight,
                  1L,
                  2048L
              )
          );

      tft_->drawRect(
          static_cast<int>(x_),
          static_cast<int>(y_),
          width,
          height,
          color_
      );
    }
  } else if (cmd == "ANGLE") {
    angle_ =
        arg.toFloat();
  }

  return true;
}

void TurtleScript::move(
    float distance,
    bool draw
) {
  const float radians =
      angle_ * DEG_TO_RAD;

  const float nextX =
      x_ +
      cosf(radians) *
          distance;

  const float nextY =
      y_ +
      sinf(radians) *
          distance;

  if (draw) {
    tft_->drawLine(
        static_cast<int>(x_),
        static_cast<int>(y_),
        static_cast<int>(nextX),
        static_cast<int>(nextY),
        color_
    );
  }

  x_ = nextX;
  y_ = nextY;
}

uint16_t TurtleScript::parseColor(
    String token
) const {
  token.trim();

  if (token.startsWith("#")) {
    token.remove(0, 1);
  }

  if (token.length() != 6) {
    return color_;
  }

  const uint32_t rgb =
      strtoul(
          token.c_str(),
          nullptr,
          16
      );

  const uint8_t red =
      (rgb >> 16) & 0xFF;

  const uint8_t green =
      (rgb >> 8) & 0xFF;

  const uint8_t blue =
      rgb & 0xFF;

  return static_cast<uint16_t>(
      ((red & 0xF8) << 8) |
      ((green & 0xFC) << 3) |
      (blue >> 3)
  );
}
