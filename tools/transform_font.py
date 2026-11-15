#!/usr/bin/env python3
"""Transform font data from vertical to horizontal bit representation.

Reads src/retro_font_vert.c, where each glyph is stored as 8 bytes, one per
column. In that vertical layout bit 0 of a byte is the top pixel of the column.

Writes src/retro_font.c with a horizontal layout: each glyph is stored as 8
bytes, one per row, where bit 0 of a byte is the left-most pixel of the row.

The transform is a simple 8x8 bit transpose per glyph:
    out[row] bit[col] = in[col] bit[row]

All comments and surrounding structure are preserved; only the glyph data
bytes are rewritten.
"""

import os
import re

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_DIR = os.path.join(SCRIPT_DIR, "..", "src")
IN_PATH = os.path.join(SRC_DIR, "retro_font_vert.c")
OUT_PATH = os.path.join(SRC_DIR, "retro_font.c")

# Matches a data line consisting only of hex byte literals (e.g. "0x00,").
HEX_LINE_RE = re.compile(r"^\s*(?:0x[0-9A-Fa-f]{2}\s*,\s*)+$")
HEX_BYTE_RE = re.compile(r"0x[0-9A-Fa-f]{2}")


def transpose_glyph(columns):
    """Transpose an 8-byte vertical glyph into an 8-byte horizontal glyph."""
    rows = [0] * 8
    for col_index, col_byte in enumerate(columns):
        for row_index in range(8):
            if col_byte & (1 << row_index):
                rows[row_index] |= (1 << col_index)
    return rows


def transform_line(line):
    bytes_in = [int(b, 16) for b in HEX_BYTE_RE.findall(line)]
    if len(bytes_in) != 8:
        # Not a single-glyph data line; leave untouched.
        return line
    rows = transpose_glyph(bytes_in)
    indent = line[: len(line) - len(line.lstrip())]
    body = "\t".join("0x{:02X},".format(v) for v in rows)
    return "{}{} \n".format(indent, body)


def main():
    with open(IN_PATH, "r") as f:
        lines = f.readlines()

    out_lines = []
    for line in lines:
        if HEX_LINE_RE.match(line):
            out_lines.append(transform_line(line))
        else:
            out_lines.append(line)

    with open(OUT_PATH, "w") as f:
        f.writelines(out_lines)

    print("Wrote {}".format(os.path.normpath(OUT_PATH)))


if __name__ == "__main__":
    main()
