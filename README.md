# ControlOS v1.0 Core Deck

ControlOS is an encoder-first firmware for the **LILYGO T-Embed CC1101 Plus (ESP32-S3)**. It is designed as a coherent handheld control/diagnostics environment rather than a collection of unrelated menus.

## Core identity

- 320x170 ST7789 UI with radial encoder launcher
- Python-Turtle-inspired non-blocking boot sequence
- local WebUI at **http://172.0.0.1**
- WebUI login: `admin` / `control`
- Wi-Fi AP password: `controlos`
- persistent themes, LED settings, power profile, automation rules and learned IR code
- USER button remains the global Back action
- shared hardware abstraction for the board pin map
- GitHub Actions workflow that creates a Launcher-friendly application `.bin`

## Included apps

| App | Main features |
| --- | --- |
| Dashboard | battery, heap, WebUI clients, active power profile, uptime/network overview |
| Sub-GHz Spectrum | CC1101 passive sweep, three bands, peak-hold, mini-waterfall, strong-signal journal |
| Wi-Fi Analyzer | async AP scan, SSID/BSSID/RSSI/channel/security, channel map, signal-finder view |
| BLE Explorer | passive advertisement scan, device name/address/RSSI list |
| NFC Inspector | ISO14443A UID inspection, short history, NTAG/NDEF detection, normal NDEF URL write |
| IR Studio | learn supported decoded IR frames, persist learned code, replay with onboard IR TX |
| 2.4G Channel Lab | nRF24 passive carrier scan, live level, peak hold, busy-percentage view |
| Audio Spectrum | onboard microphone relative dBFS meter and 16-band spectrum |
| LED Studio | 22 WS2812 effects, RGB, brightness and speed with NVS persistence |
| QR & Share | QR for WebUI URL or ControlOS Wi-Fi credentials |
| Script Lab | load/run `.cos` Turtle scripts from LittleFS or microSD |
| Utility Toolbox | stopwatch, countdown timer, HEX/DEC/BIN converter |
| Command Palette | quick power/theme/LED/sound/deep-sleep actions |
| Storage | microSD browser without unmounting WebUI storage access |
| System + Battery | BQ27220 telemetry, I2C scan, heap/PSRAM and battery-history graph |
| Hardware Diagnostics | PN532/BQ27220/BQ25896/PSRAM/SD/speaker checks and speaker test |
| Settings & Profiles | theme, power profile and automation configuration |
| Web Control | AP address, credentials and WebUI state |

## WebUI

ControlOS starts an AP in `WIFI_AP_STA` mode so the local UI remains reachable while Wi-Fi analysis runs.

- SSID: `ControlOS-XXXXXX`
- Wi-Fi password: `controlos`
- Web address: `http://172.0.0.1`
- Web username: `admin`
- Web password: `control`
- telemetry WebSocket: `ws://172.0.0.1:81`

### WebUI features

- live dashboard / telemetry
- direct app launcher
- remote encoder/select/back/home
- display on/off and reboot
- full LED Studio
- theme selection
- power profiles
- automation settings
- LittleFS + microSD file manager
- upload/download/create/rename/delete
- text preview/editor
- drag & drop upload
- OTA application `.bin` update
- event log
- configuration export/import
- factory reset
- state mirror endpoint for current foreground app/launcher state

### API

Representative endpoints:

```text
GET  /api/status
GET  /api/apps
GET  /api/logs
POST /api/control?action=left|right|select|back|home|open|screen_on|screen_off|reboot
GET  /api/led
POST /api/led
GET  /api/theme
POST /api/theme
GET  /api/power
POST /api/power
GET  /api/automation
POST /api/automation
GET  /api/screen
GET  /api/files
GET  /api/read
POST /api/write
POST /api/mkdir
POST /api/delete
POST /api/rename
POST /api/upload
GET  /api/download
POST /api/ota
GET  /api/config/export
POST /api/config/import
POST /api/factory-reset
```

Mutating requests require HTTP Basic authentication and the header:

```text
X-ControlOS: 1
```

## Turtle / Script Lab

ControlOS can load `.cos` scripts from `/scripts` on LittleFS or microSD. The runtime is deliberately small and deterministic.

Example:

```text
CLEAR #000000
COLOR #00FF41
GOTO 30 85
FORWARD 70
RIGHT 90
FORWARD 25
LEFT 45
FORWARD 35
TEXT CONTROL//OS
WAIT 900
```

Supported commands include `CLEAR`, `COLOR`, `GOTO`, `FORWARD/FD`, `BACK/BK`, `LEFT/LT`, `RIGHT/RT`, `PENUP`, `PENDOWN`, `TEXT` and `WAIT`.

## Themes

- Control Green
- Amber CRT
- Cyber Blue
- Purple Neon
- Monochrome
- Red Alert

Themes persist in NVS and apply to the device UI immediately.

## LED Studio

22 modes are included:

```text
Off, Solid, Breathe, Rainbow, Chase, Scanner, Sparkle, Comet,
Color Wipe, Theater Chase, Twinkle, Meteor, Wave, Dual Scanner,
Heartbeat, Cyber Pulse, Matrix, Fire, Ocean, Confetti, Glitch,
Random Color, Aurora
```

## Power / battery

ControlOS reads the onboard BQ27220 when available and exposes:

- state of charge
- voltage
- current
- temperature
- remaining/full capacity
- battery history graph

Power profiles:

- Performance: 240 MHz
- Balanced: 160 MHz
- Battery Saver: 80 MHz
- Stealth: 80 MHz

## Automation engine

Current persistent automation rules include:

- low battery -> Battery Saver + red Heartbeat LED
- WebUI client connected -> optional Cyber Pulse LED

The engine is isolated in `src/core/AutomationEngine.*` so more rules can be added without coupling them to apps.

## Safety scope

ControlOS focuses on local administration, receive/inspection, diagnostics and normal device-control functions. It intentionally does not include RF jamming, Wi-Fi deauthentication, credential capture, rolling-code replay, destructive USB payloads or NFC credential cloning.

## Build

```bash
pip install platformio
pio run -e t_embed_cc1101_plus
```

Build output:

```text
.pio/build/t_embed_cc1101_plus/firmware.bin
```

The included GitHub Actions workflow also builds and publishes:

```text
controlos-t-embed-cc1101-plus.bin
controlos-t-embed-cc1101-plus.elf
```

## Important validation note

The source tree, local includes, app registry, embedded WebUI JavaScript and ZIP integrity were checked offline. This environment does not currently have PlatformIO installed and cannot download packages, so the final ESP32 compiler/linker validation must run through the included GitHub Actions workflow or a local PlatformIO installation.
