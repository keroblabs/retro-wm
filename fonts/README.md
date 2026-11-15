# Rebuilding the fonts

The converter requires Python 3.9+ and Pillow (`python3 -m pip install Pillow`).
Run these commands from the repository root:

```sh
python3 tools/convert_font.py fonts/source_font_8x14.png src/retro_font_c8x14.c \
    --origin 56 20 --cell 8 19 --crop 0 5 8 14

python3 tools/convert_font.py fonts/source_font_12x20.png src/retro_font_c12x20.c \
    --origin 107 29 --cell 12 28 --crop 0 7 12 20
```

All coordinates are zero-based. `--origin X Y` locates the first character
cell. `--cell WIDTH HEIGHT` gives the spacing between cells. `--crop X Y WIDTH
HEIGHT` selects the pixels within each cell. The grid defaults to 8 columns
and 12 rows; change these with `--columns` and `--rows`. The default first
character is `0x20`, configurable with `--first-character` (decimal or hex).

Glyphs follow character order from left to right, then top to bottom, covering
`0x20` through `0x7F` in these images. The converter calculates grayscale
luminance, finds the darkest and lightest pixels across all extracted glyphs,
and maps that range linearly to 0–15 with rounding. The dark background becomes
0 and the brightest foreground becomes 15. It uses one shared scale per font
to preserve relative tones between glyphs. Transparent pixels are composited
onto black; an image with no contrast produces all-zero pixels.

Each byte stores two pixels: bits `[3:0]` hold the left pixel and bits `[7:4]`
the right pixel. Rows run top to bottom and start on byte boundaries. For odd
widths, the last byte of each row has a zero high nibble. Glyphs have no trailing
padding.

The C symbols and enum names come from the extraction dimensions:

| Font | Definition | Enum | Bytes per glyph | Total bitmap bytes |
| --- | --- | --- | --- | --- |
| 8×14 | `font_c8x14` | `FONT_C8x14` | 56 | 5,376 |
| 12×20 | `font_c12x20` | `FONT_C12x20` | 120 | 11,520 |

The generated files include `retro_font.h`. For additional font dimensions,
add the corresponding enum to that header before compiling the generated C.
