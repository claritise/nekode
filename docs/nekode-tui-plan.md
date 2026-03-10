# nekode TUI — Implementation Plan

A terminal-based IDE that multiplexes 4 Claude Code instances across git worktrees, controlled by an ESP32 with cherry switches and per-instance LED feedback.

## Overview

```
┌─────────────────────────────────────────────────┐
│  nekode TUI (Node/Ink)                          │
│                                                 │
│  ┌───────────┐ ┌───────────┐ ┌───────────┐ ┌───────────┐
│  │ Instance 1│ │ Instance 2│ │ Instance 3│ │ Instance 4│
│  │ worktree/1│ │ worktree/2│ │ worktree/3│ │ worktree/4│
│  │ claude-code│ │ claude-code│ │ claude-code│ │ claude-code│
│  └─────┬─────┘ └─────┬─────┘ └─────┬─────┘ └─────┬─────┘
│        │              │              │              │
│        └──────────────┴──────┬───────┴──────────────┘
│                              │                      │
│                        Hook shims                   │
│                              │                      │
│                     ┌────────▼────────┐             │
│                     │  Device Bridge  │             │
│                     │  (serial owner) │             │
│                     └────────┬────────┘             │
└──────────────────────────────┼──────────────────────┘
                               │ USB serial
                     ┌─────────▼─────────┐
                     │     ESP32-S3      │
                     │  4 LEDs + 8 keys  │
                     │  OLED + spectrum   │
                     └───────────────────┘
```

## Hardware Changes

### Switches

8x Cherry MX switches in a 2×4 matrix:

```
 Top row (select instance):  [1] [2] [3] [4]
 Bottom row (send keystroke): [A] [B] [C] [D]
```

**Wiring**: Direct GPIO with internal pull-ups (no matrix scanning needed for 8 keys). Each switch connects its GPIO to GND when pressed.

**Proposed pin assignments** (avoiding existing pins 5, 7, 11, 15, 16, 17):

| Switch | GPIO | Function |
|--------|------|----------|
| TOP_1  | 1    | Select instance 1 |
| TOP_2  | 2    | Select instance 2 |
| TOP_3  | 3    | Select instance 3 |
| TOP_4  | 4    | Select instance 4 |
| BOT_1  | 6    | Select option 1 |
| BOT_2  | 8    | Select option 2 |
| BOT_3  | 9    | Select option 3 |
| BOT_4  | 10   | Select option 4 |

### LEDs

Reduce from 6 to 4 APA106 LEDs. Each LED maps 1:1 to an instance. LED colour encodes instance state:

| State     | Colour        |
|-----------|---------------|
| idle      | dim white     |
| typing    | blue pulse    |
| running   | green pulse   |
| thinking  | yellow pulse  |
| error     | red flash     |
| done      | green solid   |
| waiting   | dim white     |
| sleeping  | off           |

The active (focused) instance's LED is brighter than the others.

### OLED

Shows the animation for the currently focused instance (same as today). Instance number shown in corner.

---

## Firmware Changes

### New module: `src/switches.h/.cpp`

```
namespace switches {
  void init();          // configure GPIOs, internal pull-ups, attach ISRs
  uint8_t poll();       // returns bitmask of newly pressed keys (debounced)
}
```

- 20ms debounce in software (timestamp per key)
- `poll()` returns edge-detected presses (not held state)

### Serial protocol additions

New commands **from TUI → device**:

| Command | Description |
|---------|-------------|
| `focus:<0-3>` | Set which instance is focused (brightens that LED) |
| `led:<0-3>:<state>` | Set LED state for a specific instance |
| `oled:<state>` | Set OLED animation (same as existing commands) |

New messages **from device → TUI**:

| Message | Description |
|---------|-------------|
| `key:top:<0-3>` | Top row switch pressed |
| `key:bot:<0-3>` | Bottom row switch pressed |

### Changes to existing modules

- **led_strip**: Refactor to support per-LED colour/mode instead of uniform strip mode. Add `led::set(index, r, g, b)` and `led::setBrightness(index, brightness)`.
- **main.cpp**: Add switch polling to loop. Send key events over serial. Parse new `focus:` and `led:` commands.
- **config.h**: Add switch GPIO definitions. Change `kLedCount` from 6 to 4.

---

## TUI Application

### Tech stack

| Layer | Choice | Reason |
|-------|--------|--------|
| Runtime | Node.js | Same as Claude Code |
| UI framework | Ink (React for CLI) | Composable terminal UI |
| Terminal multiplexer | node-pty | Spawn and control PTY subprocesses |
| Serial | serialport | USB serial communication |
| Git | simple-git | Worktree management |

### Project structure

```
tui/
  package.json
  tsconfig.json
  src/
    index.tsx              — entry point, Ink render root
    app.tsx                — top-level App component, state orchestration
    components/
      terminal-pane.tsx    — single Claude Code PTY viewport
      status-bar.tsx       — bottom bar: instance tabs, device status
      header-bar.tsx       — top bar: project name, worktree info
    lib/
      device-bridge.ts     — serial connection to ESP32, command send/receive
      instance-manager.ts  — spawn/manage 4 Claude Code processes + PTYs
      worktree-manager.ts  — git worktree create/list/destroy
      hook-shim.ts         — generate per-instance hook configs
      keymap.ts            — map device buttons → PTY keystrokes
```

### Component hierarchy

```
<App>
  <HeaderBar instance={focused} worktree={path} />
  <TerminalPane pty={instances[focused].pty} />
  <StatusBar instances={instances} focused={focused} />
</App>
```

Only the focused instance's terminal is rendered. All 4 PTYs stay alive in background.

### Instance lifecycle

```
1. TUI starts
2. For each instance 0-3:
   a. Create git worktree:  git worktree add .worktrees/i-{n} -b nekode-{n} HEAD
   b. Write hook shim config to .worktrees/i-{n}/.claude/settings.json
   c. Spawn: claude --dangerously-skip-permissions (in worktree cwd)
   d. Attach PTY to terminal pane
3. On shutdown:
   a. Send SIGTERM to all Claude Code processes
   b. Wait for exit
   c. Remove worktrees: git worktree remove .worktrees/i-{n}
```

### Hook shimming

Each worktree gets a `.claude/settings.json` that routes status updates through a per-instance shim script instead of `set_status.sh`. The shim writes to a named pipe or unix socket that the TUI listens on, tagged with the instance index:

```bash
#!/bin/bash
# .worktrees/i-0/hooks/set_status.sh
echo "status:0:$1" | nc -U /tmp/nekode-tui.sock
```

The TUI's device bridge receives these, updates internal state, and sends the appropriate `led:` and `oled:` commands to the ESP32.

Alternative (simpler v1): shim writes to a temp file `/tmp/nekode-{n}.status` and the TUI polls it. Less elegant but zero dependencies.

### Keyboard handling

**From physical device:**

| Button | Action |
|--------|--------|
| Top 1-4 | Switch focused instance |
| Bot 1 | Select option 1 (first choice in Claude Code prompt) |
| Bot 2 | Select option 2 (second choice) |
| Bot 3 | Select option 3 (third choice) |
| Bot 4 | Select option 4 (fourth choice) |

**From keyboard (TUI has focus):**

| Key | Action |
|-----|--------|
| Ctrl+1/2/3/4 | Switch focused instance |
| All other input | Forwarded to focused PTY |

---

## Implementation Phases

### Phase 1: TUI MVP (no device)

Deliverables:
- [ ] Node/Ink project scaffold with TypeScript
- [ ] Worktree manager: create 4 worktrees on start, destroy on exit
- [ ] Instance manager: spawn Claude Code in each worktree via node-pty
- [ ] Terminal pane: render focused instance's PTY output
- [ ] Keyboard routing: Ctrl+1-4 to switch, all else forwarded to PTY
- [ ] Status bar showing 4 instance tabs with labels

Test: run `nekode-tui` in the nekode repo, get 4 functional Claude Code sessions switchable via keyboard.

### Phase 2: Hook integration

Deliverables:
- [ ] Hook shim generator: writes per-instance settings.json + shim script
- [ ] Unix socket listener in TUI for status updates
- [ ] Internal state tracking: per-instance status (idle, typing, running, etc.)
- [ ] Status bar updates with per-instance state colours

Test: Claude Code activity in any instance updates the status bar in real time.

### Phase 3: Firmware — switches

Deliverables:
- [ ] switches module: GPIO init, debounce, edge detection
- [ ] Serial output: `key:top:N` / `key:bot:N` messages
- [ ] Update main.cpp loop to poll switches
- [ ] Update config.h with new pin assignments

Test: press switches, see key events on serial monitor.

### Phase 4: Firmware — per-instance LEDs

Deliverables:
- [ ] Refactor led_strip to support per-LED colour control
- [ ] Parse `led:<n>:<state>` and `focus:<n>` serial commands
- [ ] LED state machine with pulse/flash animations per LED
- [ ] Brightness boost for focused instance

Test: send `led:0:running` over serial, see green pulse on LED 0.

### Phase 5: Device bridge

Deliverables:
- [ ] Serial connection manager in TUI (auto-detect `/dev/cu.usbmodem*`)
- [ ] Parse incoming `key:` messages, route to instance manager
- [ ] Send `led:` and `focus:` commands based on hook status updates
- [ ] Send `oled:` commands for focused instance animation
- [ ] Physical buttons control the TUI

Test: full loop — press top-row button → TUI switches instance → LED updates → OLED shows new animation.

### Phase 6: Polish

- [ ] Graceful error handling (device disconnect/reconnect, Claude Code crash)
- [ ] Instance restart (if Claude Code exits, offer to respawn)
- [ ] OLED instance indicator (show number in corner)
---

## Design Decisions

1. **Bottom row mapping**: Buttons map positionally to Claude Code prompt options 1-4. The TUI inspects the current prompt and sends the keystroke corresponding to that position.
2. **Worktree branches**: Always branch from HEAD of main.
3. **Max instances**: Hard-coded at 4 slots (matches hardware).
4. **OLED**: Always shows the currently focused instance's animation full-screen.
