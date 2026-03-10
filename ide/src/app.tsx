import { useEffect, useRef, useState } from 'react';
import { Box, Text, useApp, useInput, useStdout } from 'ink';
import {
  CHROME_ROWS,
  DEFAULT_COLUMNS,
  DEFAULT_ROWS,
  INSTANCE_COUNT,
  LEADER_KEY,
} from './lib/constants.js';
import {
  createInstance,
  destroyInstance,
  resizeInstance,
  writeToInstance,
} from './lib/instance-manager.js';
import { registerCleanupHandlers, setCleanupInstances } from './lib/cleanup.js';
import TerminalPane from './components/terminal-pane.js';
import type { Instance } from './lib/types.js';

interface AppProps {
  /** Skip PTY spawning (for tests) */
  skipPty?: boolean;
}

export default function App({ skipPty = false }: AppProps) {
  const { exit } = useApp();
  const { stdout } = useStdout();
  const columns = stdout?.columns ?? DEFAULT_COLUMNS;
  const rows = stdout?.rows ?? DEFAULT_ROWS;
  const paneRows = rows - CHROME_ROWS;

  const tabs = Array.from({ length: INSTANCE_COUNT }, (_, i) => i + 1);
  const [instance, setInstance] = useState<Instance | null>(null);
  const instanceRef = useRef<Instance | null>(null);

  // Register cleanup handlers once on mount
  useEffect(() => {
    if (skipPty) return;
    registerCleanupHandlers();
  }, [skipPty]);

  // Spawn Claude Code on mount, clean up on unmount
  useEffect(() => {
    if (skipPty) return;

    const inst = createInstance(0, columns, paneRows, process.cwd());
    instanceRef.current = inst;
    setInstance(inst);
    setCleanupInstances([inst]);

    return () => {
      instanceRef.current = null;
      setCleanupInstances([]);
      void destroyInstance(inst);
    };
  }, [skipPty]);

  // Handle terminal resize
  useEffect(() => {
    if (!instanceRef.current) return;
    resizeInstance(instanceRef.current, columns, paneRows);
  }, [columns, paneRows]);

  // Forward all input to the PTY
  useInput(
    (input, key) => {
      // Ctrl+D exits the nekode IDE
      if (key.ctrl && input === 'd') {
        exit();
        return;
      }

      if (!instanceRef.current) return;

      // Translate Ink key events back to raw terminal sequences
      if (key.return) {
        writeToInstance(instanceRef.current, '\r');
      } else if (key.backspace || key.delete) {
        writeToInstance(instanceRef.current, '\x7f');
      } else if (key.escape) {
        writeToInstance(instanceRef.current, '\x1b');
      } else if (key.upArrow) {
        writeToInstance(instanceRef.current, '\x1b[A');
      } else if (key.downArrow) {
        writeToInstance(instanceRef.current, '\x1b[B');
      } else if (key.rightArrow) {
        writeToInstance(instanceRef.current, '\x1b[C');
      } else if (key.leftArrow) {
        writeToInstance(instanceRef.current, '\x1b[D');
      } else if (key.tab) {
        writeToInstance(instanceRef.current, '\t');
      } else if (key.ctrl && input) {
        // Convert ctrl+letter to control character (ctrl+a = 0x01, ctrl+c = 0x03, etc.)
        const code = input.charCodeAt(0) - 96;
        if (code > 0 && code < 27) {
          writeToInstance(instanceRef.current, String.fromCharCode(code));
        }
      } else if (input) {
        writeToInstance(instanceRef.current, input);
      }
    },
    { isActive: true },
  );

  return (
    <Box flexDirection="column" width={columns} height={rows}>
      <Box paddingX={1}>
        <Text bold color="white">
          nekode
        </Text>
        <Text color="gray"> — {INSTANCE_COUNT}x Claude Code multiplexer</Text>
      </Box>

      <TerminalPane instance={instance} offsetRow={2} rows={paneRows} cols={columns} />

      <Box paddingX={1} justifyContent="space-between">
        <Text color="gray">
          {tabs.map((n, i) => (
            <Text key={n} bold={i === 0} color={i === 0 ? 'cyan' : 'gray'}>
              {i > 0 ? ' ' : ''}[{n}]
            </Text>
          ))}
        </Text>
        <Text color="gray">
          {LEADER_KEY} → 1-{INSTANCE_COUNT} switch | Ctrl+D quit
        </Text>
      </Box>
    </Box>
  );
}
