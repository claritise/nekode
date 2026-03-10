# nekode TUI — Implementation Plan

A terminal-based IDE that multiplexes 4 Claude Code instances across git worktrees, with keyboard controls emulating the hardware switches (device integration comes later).

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
│                     │  State Manager  │             │
│                     └────────┬────────┘             │
│                              │                      │
│                     ┌────────▼────────┐             │
│                     │  Device Bridge  │  (future)   │
│                     │  (serial owner) │             │
│                     └────────┬────────┘             │
└──────────────────────────────┼──────────────────────┘
                               │ USB serial (future)
                     ┌─────────▼─────────┐
                     │     ESP32-S3      │  (future)
                     │  4 LEDs + 8 keys  │
                     │  OLED + spectrum   │
                     └───────────────────┘
```

---

## Claude Code Integration

### Claude Agent SDK

Claude Code has an official SDK (`@anthropic-ai/claude-agent-sdk`) that provides programmatic control. However, for v1 we use **raw PTY spawning** because:
- We want the full interactive Claude Code UI (React + Ink) rendered in each pane
- The SDK's `query()` is headless — no visual terminal output
- Users need to see and interact with Claude Code's native UI directly

The SDK becomes relevant later if we want structured event streams or programmatic hook callbacks instead of shell shims.

### Built-in worktree support

Claude Code v2.1.50+ has native worktree support via `-w <name>`, which creates `.claude/worktrees/<name>/`. However, we manage worktrees ourselves for full control over placement and cleanup.

### Key CLI flags for spawning

| Flag | Purpose |
|------|---------|
| `--dangerously-skip-permissions` | Skip all permission prompts (if desired) |
| `--permission-mode <mode>` | `default`, `acceptEdits`, `plan`, `bypassPermissions` |
| `--session-id <uuid>` | Use specific session UUID for tracking |

### How Claude Code handles input

Claude Code uses **React + Ink** (same as our TUI), which puts `process.stdin` into **raw mode**. Key bindings:
- `y` — accept/allow at permission prompts
- `n` — reject/deny at permission prompts
- `a` — accept all (don't ask again for this tool)
- `Ctrl+C` — cancel current generation
- `Ctrl+D` — exit session
- Left/Right arrows — cycle through dialog tabs

### Hook system

Hooks fire at lifecycle events and receive JSON on stdin. Configuration in `settings.json`:

```json
{
  "hooks": {
    "PreToolUse": [
      {
        "matcher": "Bash",
        "hooks": [{ "type": "command", "command": "./hook.sh" }]
      }
    ]
  }
}
```

Hook events: `SessionStart`, `SessionEnd`, `UserPromptSubmit`, `PreToolUse`, `PostToolUse`, `PostToolUseFailure`, `PermissionRequest`, `Notification`, `SubagentStart`, `SubagentStop`, `Stop`, `PreCompact`.

Settings precedence: `~/.claude/settings.json` < `.claude/settings.json` < `.claude/settings.local.json` < CLI args.

---

## Keyboard Controls (v1)

v1 is keyboard-only, emulating the two rows of 4 hardware switches.

### Option key caveats

**The Option key is unreliable on macOS by default.** Only Kitty sends ESC-prefix sequences out of the box. iTerm2, Terminal.app, Alacritty, Ghostty, and WezTerm all produce Unicode characters instead (e.g., Option+H → "Ó"). Users must enable "Use Option as Meta" in their terminal settings.

Additionally, **Option+, and Option+. are broken even with correct settings** — Ink's `useInput` parser only recognises `ESC + [a-zA-Z0-9]` as meta keys. Punctuation after ESC is not detected as a meta combination.

### Keybinding scheme: leader key (Ctrl+A prefix)

To avoid Option key issues, we use a **tmux-style leader key**: press `Ctrl+A`, then a plain key. This works in every terminal on every platform with zero configuration.

```
 Leader: Ctrl+A
 Top row (select instance):     Ctrl+A → 1  Ctrl+A → 2  Ctrl+A → 3  Ctrl+A → 4
 Bottom row (select option):    Ctrl+A → Q  Ctrl+A → W  Ctrl+A → E  Ctrl+A → R
```

| Sequence | Action |
|----------|--------|
| `Ctrl+A` → `1` | Switch to instance 1 |
| `Ctrl+A` → `2` | Switch to instance 2 |
| `Ctrl+A` → `3` | Switch to instance 3 |
| `Ctrl+A` → `4` | Switch to instance 4 |
| `Ctrl+A` → `Q` | Select option 1 (send to focused PTY) |
| `Ctrl+A` → `W` | Select option 2 (send to focused PTY) |
| `Ctrl+A` → `E` | Select option 3 (send to focused PTY) |
| `Ctrl+A` → `R` | Select option 4 (send to focused PTY) |
| `Ctrl+A` → `Ctrl+A` | Send literal Ctrl+A to focused PTY |
| All other input | Forwarded directly to focused PTY |

The leader key has a 500ms timeout — if no follow-up key arrives, the `Ctrl+A` is forwarded to the PTY. The bottom row sends the keystroke corresponding to the Nth option in the current Claude Code prompt (positional mapping).

---

## Hardware (future)

### Switches

8x Cherry MX switches in a 2×4 matrix:

```
 Top row (select instance):  [1] [2] [3] [4]
 Bottom row (select option): [A] [B] [C] [D]
```

Direct GPIO with internal pull-ups. Each switch connects GPIO to GND when pressed.

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

4x APA106 LEDs (reduced from 6). Each LED maps 1:1 to an instance. LED colour encodes instance state:

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

### Serial protocol additions

**TUI → device:**

| Command | Description |
|---------|-------------|
| `focus:<0-3>` | Set focused instance (brightens that LED) |
| `led:<0-3>:<state>` | Set LED state for a specific instance |
| `oled:<state>` | Set OLED animation (same as existing commands) |

**Device → TUI:**

| Message | Description |
|---------|-------------|
| `key:top:<0-3>` | Top row switch pressed |
| `key:bot:<0-3>` | Bottom row switch pressed |

### Firmware changes needed (future)

- **New module `switches.h/.cpp`**: GPIO init, 20ms debounce, edge detection, serial output
- **Refactor `led_strip`**: Per-LED colour/mode instead of uniform strip. Add `led::set(index, r, g, b)`
- **Update `main.cpp`**: Poll switches, send key events, parse new `focus:` and `led:` commands
- **Update `config.h`**: Switch GPIO definitions, `kLedCount` 6 → 4

---

## TUI Application

### Tech stack

| Layer | Choice | Reason |
|-------|--------|--------|
| Runtime | Node.js | Same as Claude Code (also React + Ink) |
| UI framework | Ink (React for CLI) | Composable terminal UI, same as Claude Code |
| Terminal emulation | @xterm/headless | Parse PTY ANSI output into a cell buffer (see "PTY rendering" below) |
| PTY spawning | node-pty | Spawn and control PTY subprocesses |
| Git | child_process.execFile | Thin wrapper around `git` CLI for worktree ops (see "Worktree management" below) |
| Serial | serialport v13 | USB CDC communication with ESP32 (Phase 5) |
| Testing | vitest | Unit + integration tests with v8 coverage |
| Linting | eslint (flat config) + prettier | TypeScript-eslint recommended + consistent formatting |
| Build | tsup | Bundle TypeScript for distribution |

### PTY rendering architecture

**Ink cannot render raw PTY output.** Ink uses React reconciliation with Yoga layout — it has no terminal emulation capability. Claude Code's PTY output contains ANSI escape sequences (cursor movement, alternate screen buffer, colours) that `<Text>` cannot interpret.

**Solution: Ink for chrome + @xterm/headless for the terminal pane.**

```
node-pty spawn → raw bytes with ANSI escapes
    ↓
@xterm/headless (Terminal instance) → parses ANSI, maintains cell buffer
    ↓
render loop: read buffer rows/cols → write to reserved screen region via process.stdout.write()
```

- Ink renders the header bar and status bar as normal React components
- A rectangular region is reserved for the terminal pane
- `@xterm/headless` (xterm.js without DOM deps, v6.0.0+) parses node-pty output into a character grid with attributes
- On each xterm refresh, the buffer is rendered directly to the reserved region using ANSI cursor positioning via `process.stdout.write()` (bypassing Ink's reconciler)

This is the same pattern used by `blessed-xterm` (stmux) and Zellij's screen thread. node-pty handles stdin isolation correctly — each PTY has its own master/slave fd pair, so the inner Claude Code's raw mode doesn't conflict with the outer TUI's raw mode.

### Worktree management

**Shell out to `git` directly** instead of using `simple-git`. `simple-git` has no native worktree API (you'd call `.raw()` anyway), and `nodegit` has native compilation burden. A thin async wrapper around `child_process.execFile('git', ['worktree', ...])` is ~50 lines and gives exact control.

Key lifecycle details:
- **Startup**: always run `git worktree prune` first to clear stale entries from previous crashes
- **Branch conflicts**: force-delete leftover `nekode-{0..3}` branches before creating new ones
- **Shutdown**: `SIGTERM` → timeout → `SIGKILL` to process groups, then `git worktree remove --force`
- **Signal handlers**: register for `SIGINT`, `SIGTERM`, `SIGHUP`, `beforeExit`, `uncaughtException`
- **node_modules**: symlink from main repo for v1 (all worktrees share same deps at same commit)
- **Placement**: `.worktrees/` in repo root, added to `.gitignore`
- **Concurrency**: safe since each worktree is on its own branch; each has its own index file. Avoid parallel `git fetch`

### Project structure

```
ide/
  package.json
  tsconfig.json
  vitest.config.ts
  eslint.config.js
  src/
    index.tsx              — entry point, Ink render root
    app.tsx                — top-level App component, state orchestration
    components/
      terminal-pane.tsx    — single Claude Code PTY viewport (xterm-headless → stdout)
      status-bar.tsx       — bottom bar: instance tabs + state colours
      header-bar.tsx       — top bar: project name, worktree info
    lib/
      instance-manager.ts  — spawn/manage 4 Claude Code processes + PTYs
      worktree-manager.ts  — git worktree create/list/destroy (execFile wrapper)
      hook-shim.ts         — generate per-instance hook configs
      device-bridge.ts     — serial connection singleton (Phase 5)
      keymap.ts            — map keyboard shortcuts → actions
      types.ts             — shared types (instance state, events)
    test/
      setup.ts             — vitest global setup
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
   a. Run `git worktree prune` to clear stale entries from previous crashes
   b. Force-delete any leftover nekode-{0..3} branches
   c. Create Unix socket at /tmp/nekode-tui.sock
2. For each instance 0-3:
   a. Create git worktree:  git worktree add .worktrees/i-{n} -b nekode-{n} HEAD
   b. Symlink node_modules from main repo
   c. Copy .env and other non-git files if they exist
   d. mkdir -p .worktrees/i-{n}/.claude && write settings.local.json with hook shims
   e. Spawn: claude (in worktree cwd) via node-pty, with NEKODE_SOCK + NEKODE_INSTANCE env vars
   f. Attach PTY output to xterm-headless Terminal instance
3. On shutdown:
   a. Send SIGTERM to all Claude Code process groups
   b. Wait for exit (timeout → SIGKILL)
   c. Remove worktrees: git worktree remove --force .worktrees/i-{n}
   d. Clean up branches: git branch -D nekode-{0..3}
   e. Remove Unix socket
```

### Hook shimming

Each worktree gets a `.claude/settings.local.json` (gitignored by Claude Code) that routes status updates through a per-instance shim. Hook arrays **merge across scopes** (they concatenate, not replace), so per-instance hooks in `settings.local.json` run alongside any user-level hooks in `~/.claude/settings.json`.

**IPC mechanism: Unix domain socket + environment variables.**

1. TUI creates a Unix socket at `/tmp/nekode-tui.sock` before spawning instances
2. TUI sets `NEKODE_SOCK` and `NEKODE_INSTANCE=N` in each Claude Code process's environment
3. Hook shim commands read those env vars and send JSON to the socket
4. All status hooks use `"async": true` to avoid blocking Claude Code

**Generated `settings.local.json` per worktree:**

```json
{
  "hooks": {
    "PreToolUse": [{
      "matcher": "",
      "hooks": [{ "type": "command", "command": "jq -c '{instance: env.NEKODE_INSTANCE, state: \"running\", tool: .tool_name}' | nc -U \"$NEKODE_SOCK\"", "async": true }]
    }],
    "Stop": [{
      "hooks": [{ "type": "command", "command": "echo '{\"instance\": '\"$NEKODE_INSTANCE\"', \"state\": \"done\"}' | nc -U \"$NEKODE_SOCK\"", "async": true }]
    }],
    "Notification": [{
      "hooks": [{ "type": "command", "command": "jq -c '{instance: env.NEKODE_INSTANCE, state: \"waiting\", type: .notification_type}' | nc -U \"$NEKODE_SOCK\"", "async": true }]
    }]
  }
}
```

**State inference from hooks** covers all needed states — no PTY output parsing required:

| Desired state | Hook event |
|---------------|------------|
| idle/done | `Stop` |
| running (tool) | `PreToolUse` (Bash) |
| reading/searching | `PreToolUse` (Read/Grep/Glob) |
| writing/editing | `PreToolUse` (Edit/Write) |
| waiting for permission | `Notification` (permission_prompt) or `PermissionRequest` |
| waiting for input | `Notification` (idle_prompt) |
| error | `PostToolUseFailure` |

The TUI receives these, updates its internal state map, and reflects the state in the status bar (and later, sends `led:` commands to the device).

---

## Implementation Phases

### Phase 1a: Project scaffold

Deliverables:
- [ ] Node/Ink project with TypeScript (strict, ESM, `target: ES2022`, `moduleResolution: bundler`)
- [ ] ESLint flat config with `typescript-eslint` recommended rules + `consistent-type-imports`
- [ ] Prettier config
- [ ] vitest config with `globals: true`, v8 coverage provider, 80% threshold
- [ ] tsup build config
- [ ] Entry point (`index.tsx`) that renders a placeholder `<App>`
- [ ] `pnpm dev` launches TUI via tsx, `pnpm build` via tsup, `pnpm test` via vitest
- [ ] Basic `<App>` component with a `<Box>` that fills the terminal

Test: `pnpm dev` in `ide/` renders a blank full-screen Ink app. `pnpm test` and `pnpm lint` pass.

### Phase 1b: Single PTY pane

Deliverables:
- [ ] `@xterm/headless` Terminal instance to parse PTY ANSI output into cell buffer
- [ ] `<TerminalPane>` component that renders the xterm buffer to a reserved screen region via `process.stdout.write()`
- [ ] Instance manager: spawn a single Claude Code process via node-pty in the repo root
- [ ] Forward all keyboard input to the PTY (Ink captures stdin in raw mode, writes to `pty.write()`)
- [ ] Handle PTY resize on terminal resize (resize both node-pty and xterm Terminal)

Test: `pnpm dev` launches one Claude Code session, fully interactive — identical to running `claude` directly.

### Phase 1c: Worktree lifecycle

Deliverables:
- [ ] Worktree manager: thin async wrapper around `child_process.execFile('git', [...])`
- [ ] `create(n)`: `git worktree add .worktrees/i-{n} -b nekode-{n} HEAD`
- [ ] `destroyAll()`: `git worktree remove --force` + `git branch -D` for each
- [ ] Startup: `git worktree prune`, force-delete stale `nekode-*` branches, then create 4 worktrees
- [ ] Shutdown: signal handlers for `SIGINT`, `SIGTERM`, `SIGHUP`, `beforeExit` — process group kill + worktree cleanup
- [ ] Symlink `node_modules` from main repo into each worktree
- [ ] Copy `.env` and other non-git files if they exist
- [ ] Add `.worktrees/` to `.gitignore`

Test: `pnpm dev` creates 4 worktrees, `Ctrl+C` removes them. Verify with `git worktree list`.

### Phase 1d: 4 instances + switching

Deliverables:
- [ ] Instance manager: spawn 4 Claude Code PTYs, one per worktree, each with its own xterm-headless Terminal
- [ ] Leader key state machine: `Ctrl+A` → `1/2/3/4` switches focused instance
- [ ] All other input forwarded to the focused PTY only
- [ ] Background PTYs stay alive when not focused (xterm buffers keep updating)
- [ ] 500ms leader timeout — if no follow-up key, forward `Ctrl+A` to PTY
- [ ] `Ctrl+A` → `Ctrl+A` sends literal `Ctrl+A` to focused PTY

Test: switch between 4 Claude Code sessions with `Ctrl+A → 1/2/3/4`. Each session retains its state.

### Phase 1e: Status bar + option keys

Deliverables:
- [ ] `<StatusBar>` at bottom showing 4 instance tabs with labels (`[1] [2] [3] [4]`)
- [ ] Highlight the focused instance tab
- [ ] `<HeaderBar>` at top showing project name + current worktree path
- [ ] `Ctrl+A` → `Q/W/E/R` sends the positional option keystroke to the focused PTY

Test: full Phase 1 — run `nekode-tui`, get 4 switchable Claude Code sessions with status bar and option key forwarding.

### Phase 2: Hook integration + status tracking

Deliverables:
- [ ] Hook shim generator: writes per-instance `.claude/settings.local.json` with inline jq commands
- [ ] Set `NEKODE_SOCK` and `NEKODE_INSTANCE` env vars per Claude Code process
- [ ] Unix socket listener (`net.createServer` with path `/tmp/nekode-tui.sock`) in TUI
- [ ] All hooks marked `async: true` to avoid blocking Claude Code
- [ ] Internal state tracking: per-instance status map (idle, running, reading, writing, waiting, error, done)
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
- [ ] Singleton `DeviceBridge` class with `serialport` v13
- [ ] Auto-detect ESP32 by VID `0x303A` / PID `0x1001`, fallback to `/dev/cu.usbmodem*` glob
- [ ] `@serialport/parser-readline` with `delimiter: '\r\n'` for message framing
- [ ] Write queue to prevent command interleaving from multiple components
- [ ] Auto-reconnect: poll `SerialPort.list()` every 2s on disconnect
- [ ] Explicitly disable RTS/DTR to prevent accidental ESP32 resets
- [ ] Parse incoming `key:` messages, route to instance manager
- [ ] Send `led:` and `focus:` commands based on hook status updates
- [ ] Send `oled:` commands for focused instance animation
- [ ] Physical buttons control the TUI (same actions as keyboard shortcuts)
- [ ] Graceful degradation: TUI works fully without device connected

Test: full loop — press top-row button → TUI switches instance → LED updates → OLED shows new animation.

### Phase 6: Polish

- [ ] Graceful error handling (device disconnect/reconnect, Claude Code crash)
- [ ] Instance restart (if Claude Code exits, offer to respawn)
- [ ] OLED instance indicator (show number in corner)

---

## Learnings from t3code (pingdotgg/t3code)

t3code is a web/desktop IDE that orchestrates multiple Codex CLI agents. Key patterns and takeaways:

### Patterns to adopt

1. **Per-instance locking** — t3code serializes all terminal operations (open, write, resize, close) per thread via a mutex. We need the same to prevent race conditions across 4 concurrent PTYs. Each instance gets its own lock.

2. **Terminal history buffer** — 5,000 line cap per session, debounced to disk every 40ms. Keeps memory bounded when all 4 instances are active. We should do the same with a ring buffer per PTY.

3. **Subprocess cleanup** — they explicitly kill child process trees on shutdown (process group kill on Unix). Critical for us since orphaned Claude Code processes would hold worktree locks.

4. **Event projection pipeline** — provider runtime events are ingested, converted to orchestration events, then projected into a read model for the UI. We can use a simpler version: hook shim events → instance state map → render cycle + device bridge.

5. **Shared contracts/types** — single source of truth for all message schemas between components. For us this means shared types between the hook shims, device bridge, and TUI renderer.

### Patterns we don't need (yet)

- **JSON-RPC over stdio** — t3code talks to Codex via a structured protocol. We're using raw PTY + keystroke forwarding, which is simpler.
- **Event sourcing + CQRS** — full command/event/projector pipeline. Overkill for 4 fixed slots.
- **WebSocket protocol** — they're a web app. We're a TUI.
- **Dual PTY adapters (Bun/Node)** — we just use node-pty.

### Key differences from t3code

| Aspect | t3code | nekode TUI |
|--------|--------|------------|
| UI | Web + Electron | Terminal (Ink) |
| Agent protocol | JSON-RPC over stdio | Raw PTY + keystroke forwarding |
| Instance count | Dynamic | Fixed at 4 (hardware-bound) |
| Worktrees | Shared git operations | 1 worktree per instance |
| State feedback | WebSocket to browser | Status bar (later: serial to ESP32) |
| Hook integration | None (uses Codex protocol) | Claude Code hooks shimmed per instance |

---

## Learnings from Claude Code internals

### Architecture

Claude Code itself is built with **React + Ink** (same stack we're using for the TUI). The entire app is a single bundled `cli.js` (~10.5 MB) built with Bun. It uses Yoga for layout. About 90% of it was written by Claude Code itself.

### Terminal behaviour

- Puts `process.stdin` into **raw mode** — intercepts all keystrokes including Ctrl+C
- Ink patches `console` methods to avoid overlapping with the React render
- This means our TUI (also Ink) needs to carefully manage raw mode handoff to the PTY

### Permission prompt keystrokes

| Key | Action |
|-----|--------|
| `y` | Accept/allow |
| `n` | Reject/deny |
| `a` | Accept all (don't ask again) |
| Left/Right | Cycle dialog tabs |

Our bottom-row buttons (Option+NM,.) map these positionally — the TUI sends the appropriate key to the focused PTY based on which option is in that position.

### Multiple instances

Running multiple Claude Code instances in the same repo works but they'll overwrite each other's files. The solution is **git worktrees** — each instance gets its own working directory and branch. Known caveats:
- Shared databases/ports cause interference
- `node_modules` may need per-worktree install
- Non-git files (`.env`) need manual copying
- All worktrees share the same `.git` directory

### Claude Agent SDK

`@anthropic-ai/claude-agent-sdk` provides a programmatic `query()` function that spawns Claude Code processes. Key features:
- `sessionId` for tracking
- `abortController` for cancellation
- `cwd` for working directory
- Programmatic hook callbacks (not just shell scripts)
- `streamInput()` for multi-turn conversations

We don't use this for v1 (we need the full interactive UI), but it could replace PTY spawning if we ever want a headless mode or structured event streams.

---

## Design Decisions

1. **Bottom row mapping**: Buttons map positionally to Claude Code prompt options 1-4. The TUI sends the appropriate keystroke to the focused PTY.
2. **Worktree branches**: Always branch from HEAD of main.
3. **Max instances**: Hard-coded at 4 slots (matches hardware).
4. **OLED**: Always shows the currently focused instance's animation full-screen.
5. **v1 is keyboard-only**: Leader key (`Ctrl+A`) + `1-4` for instance switching, `Ctrl+A` + `Q/W/E/R` for option selection. Hardware integration in Phase 3-5.
6. **PTY over SDK**: Use raw PTY spawning for the full Claude Code interactive UI. SDK is a future option for headless/structured mode.
7. **settings.local.json for hooks**: Use `.claude/settings.local.json` (gitignored) for per-instance hook shims so they don't pollute the repo.
8. **Leader key over Option/Alt**: Option+key requires terminal configuration on 5 of 6 major macOS terminals, and Ink's parser doesn't handle `ESC + punctuation`. Leader key works everywhere with zero config.
9. **xterm-headless over blessed**: Ink for chrome + `@xterm/headless` for PTY rendering. Avoids blessed (unmaintained since 2017) while keeping React component model.
10. **Shell out for git**: `child_process.execFile('git', [...])` over simple-git/nodegit. No library overhead for 5 commands, and worktree cleanup on crash is more reliable with direct control.
11. **Unix socket for hook IPC**: Lower latency than file polling, supports concurrent writers, one-liner hook commands with `nc -U`.
12. **Async hooks**: All status-reporting hooks use `async: true` to avoid adding latency to Claude Code tool calls.

---

## UI / Styling Direction

Mirror Claude Code's own terminal aesthetic:
- **Dark theme only** — black/near-black background
- **Monospace typography** — the terminal handles this, but UI chrome should use Unicode box-drawing for structure
- **Minimal colour palette** — white text, dim grey for secondary info, accent colours only for state indicators
- **Status bar** inspired by tmux/vim: compact, bottom of screen, instance tabs with state-coloured indicators
- **Header bar**: project name left-aligned, focused worktree path right-aligned
- **No borders on the terminal pane** — it should feel like a native terminal, not a windowed app

---

## Project Hygiene (inspired by polymer)

### TypeScript

```json
{
  "compilerOptions": {
    "target": "ES2022",
    "module": "ESNext",
    "moduleResolution": "bundler",
    "strict": true,
    "declaration": true,
    "declarationMap": true,
    "sourceMap": true,
    "outDir": "dist",
    "rootDir": "src",
    "esModuleInterop": true,
    "skipLibCheck": true
  }
}
```

- `"type": "module"` in package.json (ESM only)
- Strict mode, no exceptions

### ESLint (flat config)

```javascript
// eslint.config.js
import tseslint from 'typescript-eslint';

export default tseslint.config(
  { ignores: ['dist/', 'node_modules/'] },
  ...tseslint.configs.recommended,
  {
    rules: {
      '@typescript-eslint/no-unused-vars': ['warn', { argsIgnorePattern: '^_', varsIgnorePattern: '^_' }],
      '@typescript-eslint/consistent-type-imports': ['warn', { prefer: 'type-imports', fixStyle: 'inline-type-imports' }],
    },
  },
);
```

### Prettier

Minimal config, consistent formatting:

```json
{
  "singleQuote": true,
  "trailingComma": "all",
  "printWidth": 100
}
```

### Vitest

```typescript
// vitest.config.ts
import { defineConfig } from 'vitest/config';

export default defineConfig({
  test: {
    globals: true,
    environment: 'node',
    include: ['src/**/*.test.{ts,tsx}'],
    setupFiles: ['./src/test/setup.ts'],
    testTimeout: 10_000,
    coverage: {
      provider: 'v8',
      reporter: ['text', 'lcov'],
      thresholds: {
        lines: 80,
        branches: 80,
        functions: 80,
        statements: 80,
      },
    },
  },
});
```

### Scripts

```json
{
  "scripts": {
    "dev": "tsx src/index.tsx",
    "build": "tsup src/index.tsx --format esm --dts",
    "test": "vitest run",
    "test:watch": "vitest",
    "test:coverage": "vitest run --coverage",
    "lint": "eslint src/",
    "lint:fix": "eslint src/ --fix",
    "format:check": "prettier --check 'src/**/*.{ts,tsx}'",
    "format:write": "prettier --write 'src/**/*.{ts,tsx}'",
    "typecheck": "tsc --noEmit",
    "check": "pnpm lint && pnpm typecheck"
  }
}
```

### Package manager

pnpm (matches polymer monorepo convention). Use `pnpm-workspace.yaml` at repo root if/when `ide/` and `hardware/` need shared tooling.
