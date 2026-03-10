import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest';
import xterm from '@xterm/headless';
const { Terminal } = xterm;
import { renderTerminalToScreen } from './terminal-renderer.js';

describe('terminal-renderer', () => {
  let terminal: InstanceType<typeof Terminal>;
  let writtenOutput: string;
  const originalWrite = process.stdout.write;

  beforeEach(() => {
    terminal = new Terminal({ cols: 20, rows: 5, allowProposedApi: true });
    writtenOutput = '';
    // Capture stdout.write calls
    process.stdout.write = vi.fn((data: string | Uint8Array) => {
      writtenOutput += typeof data === 'string' ? data : new TextDecoder().decode(data);
      return true;
    }) as typeof process.stdout.write;
  });

  afterEach(() => {
    process.stdout.write = originalWrite;
    terminal.dispose();
  });

  it('renders empty terminal without crashing', () => {
    renderTerminalToScreen(terminal, 1, 1);
    expect(writtenOutput).toContain('\x1b[?25l'); // Hide cursor
    expect(writtenOutput).toContain('\x1b[?25h'); // Show cursor
  });

  it('renders text written to terminal', async () => {
    await writeAndWait(terminal, 'hello');
    renderTerminalToScreen(terminal, 1, 1);
    expect(writtenOutput).toContain('hello');
  });

  it('positions cursor at correct offset', () => {
    renderTerminalToScreen(terminal, 5, 3);
    // First row should be at row 5, col 3
    expect(writtenOutput).toContain('\x1b[5;3H');
  });

  it('handles bold attribute', async () => {
    await writeAndWait(terminal, '\x1b[1mbold\x1b[0m');
    renderTerminalToScreen(terminal, 1, 1);
    expect(writtenOutput).toContain('\x1b[1m');
    expect(writtenOutput).toContain('bold');
  });

  it('handles 16-color ANSI foreground', async () => {
    // Red foreground (SGR 31 = color index 1)
    await writeAndWait(terminal, '\x1b[31mred\x1b[0m');
    renderTerminalToScreen(terminal, 1, 1);
    expect(writtenOutput).toContain('\x1b[31m');
    expect(writtenOutput).toContain('red');
  });

  it('handles bright ANSI foreground', async () => {
    // Bright red (SGR 91 = color index 9)
    await writeAndWait(terminal, '\x1b[91mbright\x1b[0m');
    renderTerminalToScreen(terminal, 1, 1);
    expect(writtenOutput).toContain('\x1b[91m');
    expect(writtenOutput).toContain('bright');
  });

  it('handles 256-color foreground', async () => {
    // 256-color index 208 (orange)
    await writeAndWait(terminal, '\x1b[38;5;208morange\x1b[0m');
    renderTerminalToScreen(terminal, 1, 1);
    expect(writtenOutput).toContain('\x1b[38;5;208m');
    expect(writtenOutput).toContain('orange');
  });

  it('handles truecolor RGB foreground', async () => {
    await writeAndWait(terminal, '\x1b[38;2;255;128;0mrgb\x1b[0m');
    renderTerminalToScreen(terminal, 1, 1);
    expect(writtenOutput).toContain('\x1b[38;2;255;128;0m');
    expect(writtenOutput).toContain('rgb');
  });

  it('handles background colors', async () => {
    // Blue background (SGR 44 = bg color index 4)
    await writeAndWait(terminal, '\x1b[44mbg\x1b[0m');
    renderTerminalToScreen(terminal, 1, 1);
    expect(writtenOutput).toContain('\x1b[44m');
    expect(writtenOutput).toContain('bg');
  });

  it('handles dim attribute', async () => {
    await writeAndWait(terminal, '\x1b[2mdim\x1b[0m');
    renderTerminalToScreen(terminal, 1, 1);
    expect(writtenOutput).toContain('\x1b[2m');
    expect(writtenOutput).toContain('dim');
  });

  it('handles inverse attribute', async () => {
    await writeAndWait(terminal, '\x1b[7minverse\x1b[0m');
    renderTerminalToScreen(terminal, 1, 1);
    expect(writtenOutput).toContain('\x1b[7m');
    expect(writtenOutput).toContain('inverse');
  });

  it('clears each line with ECH escape', () => {
    renderTerminalToScreen(terminal, 1, 1);
    // Should contain erase character sequence for 20 cols
    expect(writtenOutput).toContain('\x1b[20X');
  });

  it('resets attributes at end of render', () => {
    renderTerminalToScreen(terminal, 1, 1);
    // Should end with reset before cursor positioning
    expect(writtenOutput).toContain('\x1b[0m');
  });
});

/** Helper to write to terminal and wait for parsing to complete */
function writeAndWait(terminal: InstanceType<typeof Terminal>, data: string): Promise<void> {
  return new Promise((resolve) => {
    terminal.write(data, resolve);
  });
}
