#!/usr/bin/env python3
"""Convert GIFs in gifs/ to C header files for SSD1306 OLED (1-bit PROGMEM bitmaps)."""

import os
import sys
import math
from pathlib import Path
from PIL import Image

GIFS_DIR = Path(__file__).parent / "gifs"
OUT_DIR = Path(__file__).parent / "include"
BYTES_PER_LINE = 9  # visual grouping in output


def gif_to_frames(gif_path: Path) -> list[Image.Image]:
    """Extract all frames from a GIF as monochrome (1-bit) images."""
    img = Image.open(gif_path)
    frames = []
    try:
        while True:
            # Convert to grayscale then threshold to pure black/white
            frame = img.convert("L").point(lambda x: 255 if x > 127 else 0, mode="1")
            frames.append(frame)
            img.seek(img.tell() + 1)
    except EOFError:
        pass
    return frames


def frame_to_bytes(frame: Image.Image) -> bytes:
    """Convert a 1-bit PIL image to packed MSB-first bitmap bytes (matches drawScaledBitmap)."""
    w, h = frame.size
    bytes_per_row = (w + 7) // 8
    data = bytearray(bytes_per_row * h)

    for y in range(h):
        for x in range(w):
            pixel = frame.getpixel((x, y))
            if pixel:  # white pixel = bit set
                byte_idx = y * bytes_per_row + x // 8
                bit_idx = 7 - (x % 8)  # MSB first: 0x80 >> (x & 7)
                data[byte_idx] |= 1 << bit_idx

    return bytes(data)


def format_c_array(name: str, data: bytes, bytes_per_row: int) -> str:
    """Format a byte array as a C PROGMEM constant."""
    lines = [f"const uint8_t PROGMEM {name}[] = {{"]
    for i in range(0, len(data), bytes_per_row):
        row = data[i : i + bytes_per_row]
        hex_vals = ", ".join(f"0x{b:02X}" for b in row)
        lines.append(f"  {hex_vals},")
    lines.append("};")
    return "\n".join(lines)


def convert_gif(gif_path: Path) -> None:
    """Convert a single GIF file to a C header file."""
    stem = gif_path.stem  # e.g. "idle", "typing"
    prefix = "k" + stem.replace("_", " ").title().replace(" ", "")  # e.g. "kIdle", "kTyping"

    print(f"  {gif_path.name}:")
    frames = gif_to_frames(gif_path)
    if not frames:
        print("    (no frames, skipping)")
        return

    w, h = frames[0].size
    bytes_per_row = (w + 7) // 8
    total_bytes = bytes_per_row * h
    print(f"    {len(frames)} frame(s), {w}x{h}, {bytes_per_row} bytes/row, {total_bytes} bytes/frame")

    header_name = f"{stem}.h"
    header_path = OUT_DIR / header_name

    parts = []
    parts.append(f"// Auto-generated from {gif_path.name} by gif_to_header.py")
    parts.append(f"// {len(frames)} frame(s), {w}x{h} pixels, 1-bit monochrome")
    parts.append(f"#pragma once")
    parts.append(f"#include <Arduino.h>\n")

    # Dimensions
    parts.append(f"constexpr uint8_t {prefix}W = {w};")
    parts.append(f"constexpr uint8_t {prefix}H = {h};")
    parts.append(f"constexpr uint8_t {prefix}FrameCount = {len(frames)};\n")

    # Individual frame arrays
    frame_names = []
    for i, frame in enumerate(frames):
        name = f"{prefix}Frame{i}"
        frame_names.append(name)
        data = frame_to_bytes(frame)
        parts.append(format_c_array(name, data, bytes_per_row))
        parts.append("")

    # Pointer array for easy indexing: frames[i]
    parts.append(f"const uint8_t * const PROGMEM {prefix}Frames[] = {{")
    for name in frame_names:
        parts.append(f"  {name},")
    parts.append("};\n")

    header_path.write_text("\n".join(parts))
    total_kb = len(frames) * total_bytes / 1024
    print(f"    -> {header_path} ({total_kb:.1f} KB flash)")


def main():
    if not GIFS_DIR.is_dir():
        print(f"Error: {GIFS_DIR} not found. Create it and add .gif files.")
        sys.exit(1)

    gifs = sorted(GIFS_DIR.glob("*.gif"))
    if not gifs:
        print(f"No .gif files found in {GIFS_DIR}")
        sys.exit(1)

    OUT_DIR.mkdir(exist_ok=True)

    print(f"Converting {len(gifs)} GIF(s) from {GIFS_DIR}/\n")
    for gif_path in gifs:
        convert_gif(gif_path)
    print("\nDone!")


if __name__ == "__main__":
    main()
