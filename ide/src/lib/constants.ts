/** Number of Claude Code instance slots (matches hardware: 4 LEDs, 4+4 switches) */
export const INSTANCE_COUNT = 4;

/** Leader key for TUI commands (tmux-style prefix) */
export const LEADER_KEY = 'Ctrl+A';

/** Fallback terminal dimensions when stdout is unavailable (e.g. in tests) */
export const DEFAULT_COLUMNS = 80;
export const DEFAULT_ROWS = 24;
