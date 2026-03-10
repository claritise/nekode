import type { Terminal } from '@xterm/headless';
import type { IBufferCell } from '@xterm/headless';

/**
 * Build the SGR escape sequence for a cell's foreground color.
 */
function fgColorSgr(cell: IBufferCell): string {
  if (cell.isFgDefault()) return '';
  if (cell.isFgRGB()) {
    const c = cell.getFgColor();
    return `\x1b[38;2;${(c >> 16) & 0xff};${(c >> 8) & 0xff};${c & 0xff}m`;
  }
  if (cell.isFgPalette()) {
    const c = cell.getFgColor();
    // 0-7: standard SGR 30-37, 8-15: bright SGR 90-97, 16+: 256-color
    if (c < 8) return `\x1b[${30 + c}m`;
    if (c < 16) return `\x1b[${90 + c - 8}m`;
    return `\x1b[38;5;${c}m`;
  }
  return '';
}

/**
 * Build the SGR escape sequence for a cell's background color.
 */
function bgColorSgr(cell: IBufferCell): string {
  if (cell.isBgDefault()) return '';
  if (cell.isBgRGB()) {
    const c = cell.getBgColor();
    return `\x1b[48;2;${(c >> 16) & 0xff};${(c >> 8) & 0xff};${c & 0xff}m`;
  }
  if (cell.isBgPalette()) {
    const c = cell.getBgColor();
    if (c < 8) return `\x1b[${40 + c}m`;
    if (c < 16) return `\x1b[${100 + c - 8}m`;
    return `\x1b[48;5;${c}m`;
  }
  return '';
}

/**
 * Render the visible xterm buffer to a rectangular screen region
 * using direct ANSI escape sequences via process.stdout.write().
 *
 * @param terminal - The xterm-headless Terminal instance
 * @param offsetRow - The row offset on screen where the pane starts (1-based)
 * @param offsetCol - The column offset on screen where the pane starts (1-based)
 */
export function renderTerminalToScreen(
  terminal: Terminal,
  offsetRow: number,
  offsetCol: number,
): void {
  const buffer = terminal.buffer.active;
  const cols = terminal.cols;
  const rows = terminal.rows;
  const cell: IBufferCell = buffer.getNullCell();

  let output = '';
  // Hide cursor during rendering to prevent flicker
  output += '\x1b[?25l';

  for (let y = 0; y < rows; y++) {
    const line = buffer.getLine(y);
    if (!line) continue;

    // Move cursor to the start of this row and clear it
    output += `\x1b[${offsetRow + y};${offsetCol}H`;
    output += `\x1b[${cols}X`; // Erase N characters (clears stale content)

    let prevFg = -1;
    let prevBg = -1;
    let prevFgMode = -1;
    let prevBgMode = -1;
    let prevBold = false;
    let prevDim = false;
    let prevItalic = false;
    let prevUnderline = false;
    let prevInverse = false;
    let prevStrikethrough = false;

    for (let x = 0; x < cols; x++) {
      line.getCell(x, cell);

      // Skip continuation cells of wide characters
      if (cell.getWidth() === 0) continue;

      const char = cell.getChars() || ' ';

      // Extract attributes
      const fg = cell.getFgColor();
      const bg = cell.getBgColor();
      const fgMode = cell.getFgColorMode();
      const bgMode = cell.getBgColorMode();
      const bold = cell.isBold() !== 0;
      const dim = cell.isDim() !== 0;
      const italic = cell.isItalic() !== 0;
      const underline = cell.isUnderline() !== 0;
      const inverse = cell.isInverse() !== 0;
      const strikethrough = cell.isStrikethrough() !== 0;

      // Only emit SGR sequences when attributes change
      const fgChanged = fg !== prevFg || fgMode !== prevFgMode;
      const bgChanged = bg !== prevBg || bgMode !== prevBgMode;
      const attrsChanged =
        bold !== prevBold ||
        dim !== prevDim ||
        italic !== prevItalic ||
        underline !== prevUnderline ||
        inverse !== prevInverse ||
        strikethrough !== prevStrikethrough;

      if (fgChanged || bgChanged || attrsChanged || x === 0) {
        output += '\x1b[0m'; // Reset all attributes
        if (bold) output += '\x1b[1m';
        if (dim) output += '\x1b[2m';
        if (italic) output += '\x1b[3m';
        if (underline) output += '\x1b[4m';
        if (inverse) output += '\x1b[7m';
        if (strikethrough) output += '\x1b[9m';
        output += fgColorSgr(cell);
        output += bgColorSgr(cell);
      }

      prevFg = fg;
      prevBg = bg;
      prevFgMode = fgMode;
      prevBgMode = bgMode;
      prevBold = bold;
      prevDim = dim;
      prevItalic = italic;
      prevUnderline = underline;
      prevInverse = inverse;
      prevStrikethrough = strikethrough;

      output += char;
    }
  }

  // Reset attributes
  output += '\x1b[0m';

  // Position cursor where xterm's cursor is
  const cursorY = buffer.cursorY;
  const cursorX = buffer.cursorX;
  output += `\x1b[${offsetRow + cursorY};${offsetCol + cursorX}H`;
  output += '\x1b[?25h'; // Show cursor

  process.stdout.write(output);
}
