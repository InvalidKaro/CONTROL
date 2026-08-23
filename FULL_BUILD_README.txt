CONTROL//OS - NORMAL FULL BUILD / CONSOLIDATED FIXES

This is NOT the minimal test build.

Copy the contents of this archive over the repository root and replace
all matching files.

The complete ControlOS source tree remains enabled. There is no
build_src_filter.

Included fixes:
- Normal full ControlOS build restored.
- 16 MB flash + OPI PSRAM build configuration.
- T-Embed ST7789 TFT reset uses GPIO40 instead of -1.
- USE_HSPI_PORT enabled.
- TFT SPI set to 80 MHz, read SPI to 20 MHz.
- Previous src/minimal_boot.cpp is neutralized so it cannot create
  duplicate setup()/loop() symbols.
- Runtime boot diagnostics remain active.
- Persistent runtime log:
      /logs/controlos-debug.log
  with rotation to:
      /logs/controlos-debug.old.log
- WebUi fs::File fix.
- WebUi RemoteCommand queue assignment fix.
- TurtleScript int/long max() compile fix.
- TurtleScript fs::File fixes.
- SystemApp PowerManager constructor mismatch fixed.
- SystemApp battery-history memmove fix.
- ScriptApp String/footer and fs::File fixes.
- SubGHz fs::File/LittleFS fixes.
- Storage fs::File and bounds fixes.
- BLE scan robustness improvements.
- NFC encoder / NDEF length fixes.
- Audio sample/peak fixes.

IMPORTANT HARDWARE NOTE:
GPIO40 is used as the ST7789 reset pin and is also listed as the speaker
LRCLK pin in the current project pin map. The consolidated ControlOS startup
therefore keeps speaker initialization out of the critical display startup
path. Display stability takes priority.

Build:
    pio run -e t_embed_cc1101_plus

Launcher BIN:
    .pio/build/t_embed_cc1101_plus/firmware.bin

Your GitHub workflow copies that application image to:
    dist/controlos-t-embed-cc1101-plus.bin
