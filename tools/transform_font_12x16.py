#!/usr/bin/env python3
"""Transform 12x16 font data from vertical to horizontal bit representation.

Reads src/retro_font_vert.c. Each glyph is stored as 24 bytes split across two
data lines of 12 bytes each:

  * The first 12 bytes are the upper 12x8 block. Byte `c` is the column `c`
    (left-most = 0), where bit 0 is row 0 (line 1) and bit 7 is row 7 (line 8).
  * The next 12 bytes are the lower 12x8 block. Byte `c` is column `c`, where
    bit 0 is row 8 (line 9) and bit 7 is row 15 (line 16).

Writes src/retro_font_12x16.c with a horizontal layout: each glyph becomes 16
rows, one 16-bit little-endian word per row. In each word bit `c` selects
column `c` (bit 0 -> pixel 0, bit 11 -> pixel 11). Each word is emitted as two
bytes, low byte first.

Only the glyph data bytes are rewritten; all comments and structure are kept,
and the identifier / type names are updated to the horizontal variant.
"""

import os
import re

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_DIR = os.path.join(SCRIPT_DIR, "..", "src")
IN_PATH = os.path.join(SRC_DIR, "retro_font_vert.c")
OUT_PATH = os.path.join(SRC_DIR, "retro_font_12x16.c")

HEX_LINE_RE = re.compile(r"^\s*(?:0x[0-9A-Fa-f]{2}\s*,\s*)+$")
HEX_BYTE_RE = re.compile(r"0x[0-9A-Fa-f]{2}")

WIDTH = 12
HEIGHT = 16


def convert_glyph(columns):
    """Convert 24 vertical column bytes into 16 horizontal little-endian words.

    columns[0:12]  -> upper block, bit 0 = row 0 .. bit 7 = row 7
    columns[12:24] -> lower block, bit 0 = row 8 .. bit 7 = row 15
    Returns a list of 32 bytes (16 words, low byte first).
    """
    rows = [0] * HEIGHT
    for col in range(WIDTH):
        upper = columns[col]
        lower = columns[WIDTH + col]
        for bit in range(8):
            if upper & (1 << bit):
                rows[bit] |= (1 << col)
            if lower & (1 << bit):
                rows[8 + bit] |= (1 << col)

    out = []
    for word in rows:
        out.append(word & 0xFF)
        out.append((word >> 8) & 0xFF)
    return out


def format_bytes(values, indent):
    """Format a list of bytes as two rows of 16 bytes (one glyph per block)."""
    lines = []
    for r in range(0, len(values), 16):
        chunk = values[r:r + 16]
        body = "\t".join("0x{:02X},".format(v) for v in chunk)
        lines.append("{}{} \n".format(indent, body))
    return "".join(lines)


def main():
    with open(IN_PATH, "r") as f:
        lines = f.readlines()

    out_lines = []
    pending = []          # accumulated vertical bytes for current glyph
    pending_indent = ""

    def flush():
        if not pending:
            return ""
        values = convert_glyph(pending)
        text = format_bytes(values, pending_indent)
        pending.clear()
        return text

    for line in lines:
        if HEX_LINE_RE.match(line):
            if not pending:
                pending_indent = line[: len(line) - len(line.lstrip())]
            pending.extend(int(b, 16) for b in HEX_BYTE_RE.findall(line))
            if len(pending) >= WIDTH * 2:
                out_lines.append(flush())
        else:
            out_lines.append(line)

    with open(OUT_PATH, "w") as f:
        f.write("".join(out_lines))

    print("Wrote {}".format(os.path.normpath(OUT_PATH)))


if __name__ == "__main__":
    main()
