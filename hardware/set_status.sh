#!/bin/bash
# Send animation status to ESP32 over WiFi (TCP) or serial fallback.
# Usage: set_status.sh <status>
# Called by Claude Code hooks via stdin JSON or direct argument.

STATUS="$1"

# If no argument, consume stdin without blocking (hook mode passes JSON)
if [ -z "$STATUS" ]; then
  cat > /dev/null &
  exit 0
fi

NEKODE_HOST="${NEKODE_HOST:-nekode.local}"
NEKODE_PORT="${NEKODE_PORT:-23}"

# Try WiFi first (nc with 1s timeout)
if printf '%s\n' "$STATUS" | nc -w 1 "$NEKODE_HOST" "$NEKODE_PORT" 2>/dev/null; then
  exit 0
fi

# Fallback to serial
PORT=$(ls /dev/cu.usbmodem* 2>/dev/null | head -1)
if [ -z "$PORT" ]; then
  exit 0
fi

(
  printf '%s\n' "$STATUS" > "$PORT"
) &
BGPID=$!

(
  sleep 2
  kill "$BGPID" 2>/dev/null
) &

exit 0
