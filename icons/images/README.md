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
folder). For a consistent resource size we propose a **64×64 canvas**
with **3 pixels of destination padding**. Each crop is scaled uniformly to
fit the 58×58 available area and is centred, without stretching its aspect
ratio. This standardizes dimensions, not physical shape. Crops and scaling
are **working proposals**, subject to reviewing the generated contact sheet.

## Convert to headerless, colour-keyed RGB565

The converter now uses **four-connected flood fill** from the outer edges of each
source PNG. Only pixels close to the configured source background colour are
eligible, with a default per-channel tolerance of 24. Pixels enclosed by an
unbroken contour, including genuinely white interior details, are preserved.
The mask is calculated **before cropping or resizing**, then resized with the
colours; this reduces white-fringe artifacts on antialiased edges.

A pixel with RGB565 value **0xF81F** (magenta, little-endian bytes `1f f8`)
represents transparency. It is not white, so an opaque white pixel remains
**0xFFFF**. This is a proposed icon-specific convention: the future RWM icon
renderer must skip pixels whose *16-bit value* equals the colour key. An opaque
pixel that would quantize to the reserved key causes an error rather than
silently disappearing. Choose a different key for such assets.

There is no per-pixel alpha in the raw RGB565 data. Resampled alpha values below
`--alpha-cutoff` (128 by default) become transparent; others become fully
opaque. Soft shadows and antialiased contours therefore have **binary**
transparency at runtime, with no fractional blending.

Requires Python 3 and Pillow (already installed on the development machine).

Commands from the repository root:

    # Inspect detected crop bounds without writing files:
    python3 tools/convert_icons_rgb565.py --analyse-only

    # Convert with current crop proposals and automatic exterior transparency:
    python3 tools/convert_icons_rgb565.py \
        --crop-config icons/images/crop-proposals.json

    # Make exterior segmentation more aggressive (review the result carefully):
    python3 tools/convert_icons_rgb565.py \
        --crop-config icons/images/crop-proposals.json \
        --flood-tolerance 32

    # Select a different reserved RGB565 key, if needed:
    python3 tools/convert_icons_rgb565.py \
        --crop-config icons/images/crop-proposals.json \
        --colour-key 0xF81F

    # Restore previous opaque conversion (no key/flood):
    python3 tools/convert_icons_rgb565.py \
        --crop-config icons/images/crop-proposals.json --opaque

The converter writes to the ignored `icons/converted/` folder:

- `raw/*.rgb565`: 64×64, headerless, row-major 16-bit RGB565, little-endian
  by default; **8192 bytes per icon**, including transparent key pixels.
- `preview/*.png`: **RGBA** PNG previews of the quantized RGB565 pixels.
  Pixels equal to the key are rendered with alpha=0 for inspection.
- `manifest.json`: input, crop, placement, transparent pixel count,
  colour-key encoding and flood threshold.
- `crop-contact-sheet.png`: original source/crop rectangles next to output
  previews on a **checkerboard**, making holes and transparency visible.

Use `xdg-open icons/converted/crop-contact-sheet.png` to inspect the results.

The adjustable crop rectangles are in `crop-proposals.json`.
The tool also supports `--size`, `--threshold` (crop detection),
`--source-padding`, `--canvas-padding`, `--background R,G,B`,
`--endian big`, `--alpha-cutoff` and `--output-dir`.
Run `--help` for the complete set. Source PNGs are never modified.

## Editable pixel maps and manual cleanup

Automatic flood fill is conservative, but isolated near-white source artifacts
may survive. To remove individual unwanted pixels **without changing any other
RGB565 values**, every normal colour-key conversion now exports an editable
pixel map for each icon under `icons/masks/`:

- `icon_drive.txt` etc.: plain-text **64 rows × 64 columns** by default.
  `#` means the pixel is opaque; `.` means RGB565 transparency key `0xF81F`.
  There are no headers, row numbers, or spaces.
- `icon_drive.json` etc.: reference dimensions, byte order, key and SHA-256
  of the baseline RGB565 data. These files prevent accidental use of a stale
  mask after a crop, image, resize or threshold change.

**First pass** (creates maps if missing, but never overwrites manual edits):

    python3 tools/convert_icons_rgb565.py \
        --crop-config icons/images/crop-proposals.json

**Edit** `icons/masks/icon_drive.txt` (and the other maps) in any plain-text
editor. Change **only unwanted opaque `#` pixels to `.`**, keeping the grid
size intact. This is a *removal-only* operation; you cannot turn existing
transparent dots back into opaque artwork.

**Second pass**, apply your edited maps:

    python3 tools/convert_icons_rgb565.py \
        --crop-config icons/images/crop-proposals.json --cleanup

This updates `icons/converted/raw/*.rgb565` and the RGBA previews/contact
sheet. It **does not rewrite the text maps**. The manifest reports how many
pixels were manually cleared per icon. Unchanged `#` pixels retain exactly
their original RGB565 values; the script only writes the reserved key at
selected `.` positions. There is no extra alpha buffer in the binary file.

The editable `icons/masks/` directory is deliberately **not gitignored**:
after approving a cleaned mask you may commit its text and JSON metadata to
the `icon-devel` branch for reproducible conversion.

If a source image, crop rectangle or conversion setting changes the baseline,
`--cleanup` reports a hash mismatch instead of silently shifting your edits
onto different pixels. Review any changes, back up your edited masks, and
regenerate them **explicitly** with `--reset-maps` (this overwrites edits):

    python3 tools/convert_icons_rgb565.py \
        --crop-config icons/images/crop-proposals.json --reset-maps

After regenerating, re-apply any desired manual cleanup. The raw and preview
outputs remain ignored as reproducible build artifacts.

## Important contour limitations

- Flood fill only removes near-background pixels **connected to a source
  image edge**. A white interior region connected to the outside through a
  gap may also be removed; use a smaller tolerance or repair the source mask.
- A white enclosed area is kept even when it is pure white.
- The threshold cannot perfectly distinguish pale highlights, shadows and
  JPEG-derived white noise. A binary RGB565 key also cannot reproduce
  fractional opacity. Review the checkerboard contact sheet before adoption.
- Manual cleanup edits individual **output** pixels at the selected final
  resolution. Re-export the maps before changing icon size, crop or scale;
  otherwise coordinates will no longer refer to the same artwork.
- A transparent key in RGB565 is an **asset encoding convention**, not
  automatic transparency in the framebuffer or SDL texture. The renderer
  implementation remains tracked by `RWM-ICO-0001` / `RWM-ICO-0002`.

The current source artwork may have licensing restrictions; check provenance
and redistributability before publishing derivative assets. No converted
binary files are committed here.

Run the independent synthetic tests with:

    python3 -m unittest discover -s tests -p 'test_convert_icons_rgb565.py' -v

This work is an initial implementation toward **RWM-ICO-0003**, not completion
of the broader icon-import pipeline, semantic naming, or runtime rendering.
