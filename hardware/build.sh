#!/bin/bash
# Build and optionally upload nekode firmware.
# Usage:
#   ./build.sh          # generate headers + build
#   ./build.sh upload   # generate headers + build + flash

set -e
cd "$(dirname "$0")"

echo "=== Generating headers from GIFs ==="
python3 gif_to_header.py

echo ""
echo "=== Building firmware ==="
if [ "$1" = "upload" ]; then
  ~/.platformio/penv/bin/pio run -e glyph -t upload
else
  ~/.platformio/penv/bin/pio run -e glyph
fi
