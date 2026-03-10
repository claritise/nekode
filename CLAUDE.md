# nekode

Monorepo for the nekode project: an ESP32-S3 desktop companion paired with a terminal-based IDE that multiplexes 4 Claude Code instances.

## Repo structure

```
hardware/         — ESP32 firmware (PlatformIO/Arduino)
ide/              — Terminal IDE (Node/Ink)
docs/             — Design documents and plans
```

## Hardware (hardware/)

ESP32-S3 animated desktop companion — a tamagotchi-style neko cat on an OLED display with RGB LED strip and cherry switches. Controlled via serial commands.

- **MCU**: ESP32-S3-DevKitC-1 (16MB flash, PSRAM)
- **Display**: SSD1306 128x64 OLED over I2C (GPIO 16 SDA, 17 SCL)
- **LEDs**: 4x APA106 F8 8mm RGB on GPIO 11 via RMT
- **Mic**: INMP441 I2S microphone (GPIO 7 SCK, 15 WS, 5 SD)
- **Switches**: 8x Cherry MX (2×4 matrix), direct GPIO with pull-ups

Pin assignments and timing constants live in `hardware/src/config.h`.

### Build

```
cd hardware
./build.sh          # generate headers + build
./build.sh upload   # generate headers + build + flash
pio device monitor  # serial monitor (115200 baud)
```

### Hardware project structure

```
hardware/
  src/
    config.h        — pin definitions, timing constants
    main.cpp        — setup/loop, serial command dispatcher
    oled.h/.cpp     — OLED display: neko animation + spectrum analyser
    led_strip.h/.cpp — RGB LED strip: per-LED colour control via RMT
    mic.h/.cpp      — INMP441 microphone: I2S RMS volume
  include/          — auto-generated animation frame headers
  gifs/             — source animation GIFs
  gif_to_header.py  — converts GIF → C header with PROGMEM frame arrays
  set_status.sh     — sends serial commands to ESP32
  settings.json     — Claude Code hook configuration template
  build.sh          — build + flash script
  platformio.ini    — PlatformIO configuration
```

### Firmware architecture

Each subsystem (`oled`, `led`, `mic`) is a C++ namespace with `init()` and a tick/read function. `main.cpp` is a pure dispatcher — it polls serial, then calls `led::tick()` and `oled::drawFrame()` on timer intervals. All rendering logic is owned by the respective module.

Animation frames are 1-bit monochrome 64x64 bitmaps stored in PROGMEM, generated from GIFs by `gif_to_header.py`.

### Serial protocol

Send newline-terminated commands over USB CDC serial:
- `idle` — switch to idle animation
- `running` — switch to running animation
- `typing` — switch to typing animation
- `led:<0-3>:<state>` — set LED state for specific instance
- `focus:<0-3>` — set focused instance

## IDE (ide/)

Node/Ink terminal IDE that multiplexes 4 Claude Code instances. See [docs/nekode-tui-plan.md](docs/nekode-tui-plan.md).

### IDE scripts

```
cd ide
pnpm dev            # run in development (tsx, no build)
pnpm build          # build for distribution (tsup)
pnpm test           # run tests (vitest)
pnpm check          # lint + typecheck
```

## Conventions

- Namespaces for module separation, anonymous namespaces for file-private state
- `constexpr` for all compile-time constants in `config.h`
- `s` prefix for file-static variables, `g` prefix for globals in main
- `k` prefix for constants
- No dynamic allocation — all buffers are statically sized
