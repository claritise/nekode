# nekode

ESP32-S3 animated desktop companion — a tamagotchi-style neko cat on an OLED display with audio-reactive spectrum analyser and RGB LED strip. Controlled via serial commands from Claude Code hooks.

## Hardware

- **MCU**: ESP32-S3-DevKitC-1 (16MB flash, PSRAM)
- **Display**: SSD1306 128x64 OLED over I2C (GPIO 16 SDA, 17 SCL)
- **LEDs**: 6x APA106 F8 8mm RGB on GPIO 11 via RMT
- **Mic**: INMP441 I2S microphone (GPIO 7 SCK, 15 WS, 5 SD)

Pin assignments and timing constants live in `src/config.h`.

## Build

PlatformIO project using the Arduino framework.

```
pio run              # build
pio run -t upload    # flash
pio device monitor   # serial monitor (115200 baud)
```

## Project structure

```
src/
  config.h        — pin definitions, timing constants
  main.cpp        — setup/loop, serial command dispatcher
  oled.h/.cpp     — OLED display: neko animation + spectrum analyser
  led_strip.h/.cpp — RGB LED strip: rainbow animation via RMT
  mic.h/.cpp      — INMP441 microphone: I2S RMS volume
include/
  idle.h          — idle animation frames (generated)
  running.h       — running animation frames (generated)
  typing.h        — typing animation frames (generated)
gif_to_header.py  — converts GIF → C header with PROGMEM frame arrays
set_status.sh     — sends serial commands to ESP32 (used by Claude Code hooks)
```

## Architecture

Each subsystem (`oled`, `led`, `mic`) is a C++ namespace with `init()` and a tick/read function. `main.cpp` is a pure dispatcher — it polls serial, then calls `led::tick()` and `oled::drawFrame()` on timer intervals. All rendering logic is owned by the respective module.

Animation frames are 1-bit monochrome 64x64 bitmaps stored in PROGMEM, generated from GIFs by `gif_to_header.py`.

## Serial protocol

Send newline-terminated commands over USB CDC serial:
- `idle` — switch to idle animation
- `running` — switch to running animation
- `typing` — switch to typing animation

## Conventions

- Namespaces for module separation, anonymous namespaces for file-private state
- `constexpr` for all compile-time constants in `config.h`
- `s` prefix for file-static variables, `g` prefix for globals in main
- `k` prefix for constants
- No dynamic allocation — all buffers are statically sized
