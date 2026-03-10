/** Number of Claude Code instance slots (matches hardware: 4 LEDs, 4+4 switches) */
export const INSTANCE_COUNT = 4;

/** Leader key for TUI commands (tmux-style prefix) */
export const LEADER_KEY = 'Ctrl+A';

/** Fallback terminal dimensions when stdout is unavailable (e.g. in tests) */
export const DEFAULT_COLUMNS = 80;
export const DEFAULT_ROWS = 24;

/** Height reserved for header bar (1 line) + status bar (1 line) */
export const CHROME_ROWS = 2;

/** xterm-headless scrollback buffer size (lines) */
export const TERMINAL_SCROLLBACK = 5000;

/** Timeout (ms) before SIGKILL after sending SIGTERM to a PTY process */
export const PTY_KILL_TIMEOUT_MS = 3000;

/** Render loop interval (ms) — ~60fps */
export const RENDER_INTERVAL_MS = 16;
