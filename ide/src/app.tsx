import { Box, Text, useApp, useInput, useStdout } from 'ink';
import { DEFAULT_COLUMNS, DEFAULT_ROWS, INSTANCE_COUNT, LEADER_KEY } from './lib/constants.js';

export default function App() {
  const { exit } = useApp();
  const { stdout } = useStdout();
  const columns = stdout?.columns ?? DEFAULT_COLUMNS;
  const rows = stdout?.rows ?? DEFAULT_ROWS;

  const tabs = Array.from({ length: INSTANCE_COUNT }, (_, i) => i + 1);

  useInput((_input, key) => {
    if (key.ctrl && _input === 'd') {
      exit();
    }
  });

  return (
    <Box flexDirection="column" width={columns} height={rows}>
      <Box paddingX={1}>
        <Text bold color="white">
          nekode
        </Text>
        <Text color="gray"> — {INSTANCE_COUNT}x Claude Code multiplexer</Text>
      </Box>

      <Box flexGrow={1} />

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
