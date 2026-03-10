import { render } from 'ink-testing-library';
import { describe, it, expect } from 'vitest';
import App from './app.js';
import { INSTANCE_COUNT, LEADER_KEY } from './lib/constants.js';

describe('App', () => {
  it('renders the nekode header', () => {
    const { lastFrame } = render(<App />);
    expect(lastFrame()).toContain('nekode');
  });

  it('renders all instance tabs', () => {
    const { lastFrame } = render(<App />);
    const frame = lastFrame()!;
    for (let i = 1; i <= INSTANCE_COUNT; i++) {
      expect(frame).toContain(`[${i}]`);
    }
  });

  it('renders keybinding hint with correct leader key and instance count', () => {
    const { lastFrame } = render(<App />);
    expect(lastFrame()).toContain(`${LEADER_KEY} → 1-${INSTANCE_COUNT} switch`);
  });

  it('stays alive waiting for input (does not immediately unmount)', async () => {
    const { lastFrame, unmount } = render(<App />);

    // App should still be rendering after creation
    await new Promise((resolve) => setTimeout(resolve, 50));
    expect(lastFrame()).toContain('nekode');

    unmount();
  });

  it('renders quit hint', () => {
    const { lastFrame } = render(<App />);
    expect(lastFrame()).toContain('Ctrl+D quit');
  });
});
