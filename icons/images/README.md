# Selected icon preparation (working assets)

The seven source PNG files in this directory are 88×88 **RGB**, with **opaque
white backgrounds**. They were cropped from a larger image and are retained
unchanged. This folder is not yet a runtime Retro WM icon API.

## Crop assessment, 2026-11-16

Visible-content bounds were measured on the seven actual source PNGs. With
RGB distance-from-white threshold **24**, inclusive coordinates were:

| PNG | Left | Top | Right | Bottom | Visible extent |
| --- | ---: | ---: | ---: | ---: | --- |
| icon_drive.png | 12 | 11 | 76 | 67 | 65×57 |
| icon_floppy_disk.png | 25 | 16 | 80 | 72 | 56×57 |
| icon_folder.png | 10 | 11 | 77 | 67 | 68×57 |
| icon_folder_file.png | 11 | 11 | 77 | 67 | 67×57 |
| icon_tools.png | 14 | 12 | 72 | 72 | 59×61 |
| icon_trash.png | 28 | 8 | 77 | 67 | 50×60 |
| icon_usb_drive.png | 29 | 13 | 60 | 72 | 32×60 |

These are **diagnostic**, not final crop rectangles. The softer shadows extend
beyond some measured bounds. The adjacent crop-proposals.json instead takes
threshold **12** plus **2 source pixels of padding**, preserving more of those
shadows. Its boxes use [left, top, right, bottom] with *exclusive* right/bottom
coordinates.

The original placements vary markedly (e.g. the narrow USB drive and wide
folder). For a consistent resource size we propose a **64×64 opaque canvas**
with **3 pixels of destination padding**. Each crop is scaled uniformly to
fit the 58×58 available area and is centred, without stretching its aspect
ratio. This standardizes dimensions, not physical shape. Crops and scaling
are **working proposals**, subject to reviewing the generated contact sheet.

## Convert to headerless RGB565

Requires Python 3 and Pillow, also used by the existing icon tools.

Commands (run from repository root):

    python3 -m pip install Pillow

    # Inspect source content boxes only; do not write files:
    python3 tools/convert_icons_rgb565.py --analyse-only

    # Convert using editable proposed crop rectangles:
    python3 tools/convert_icons_rgb565.py --crop-config icons/images/crop-proposals.json

    # Or omit --crop-config to detect and pad bounds automatically:
    python3 tools/convert_icons_rgb565.py

The converter writes to the ignored icons/converted/ folder:

- raw/*.rgb565: 64×64, headerless, row-major 16-bit RGB565 pixels,
  **little-endian** by default (8192 bytes per icon).
- preview/*.png: visualizations of the **quantized RGB565 output**, not
  simply unquantized source previews.
- manifest.json: input, crop, output placement and storage metadata.
- crop-contact-sheet.png: original source with red crop rectangles beside
  its processed preview. Review this image before accepting crop proposals.

Adjust each rectangle in crop-proposals.json to improve individual crops.
The tool also supports --size, --threshold, --source-padding,
--canvas-padding, --background R,G,B, --endian big, and a separate
--output-dir. Run --help for options. It does not alter source PNGs.

## Limits and follow-up

**No transparency is encoded.** The RGB sources already contain opaque white
backgrounds; RGB565 itself has no alpha bit. The converter composites any
future RGBA PNG onto the selected solid background. Background removal,
masking, and the runtime resource contract belong to RWM-ICO-0001/0002 and
must be agreed separately. Do not mistake automatic foreground detection for
a reliable alpha mask, especially for white details inside icons.

The current source artwork may have licensing restrictions; check provenance
and redistributability before publishing derivative assets. No converted
binary files are committed here.

Run the independent synthetic tests with:

    python3 -m unittest discover -s tests -p 'test_convert_icons_rgb565.py' -v

This work is an initial implementation toward **RWM-ICO-0003**, not completion
of the broader icon-import pipeline, semantic naming, or runtime rendering.
