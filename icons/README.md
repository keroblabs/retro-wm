# Extracted icon collection

This folder is intended to hold a preliminary palette of **266 individual 32 × 32 PNG icons** extracted from the 700 × 536 montage provided for Retro WM.

## File layout

- `png/icon_cXX_rYY.png`: individual sprites (zero-based column/row within each column).
- `manifest.json` and `manifest.csv`: source crop coordinates.
- `contact-sheet.png`: visual catalogue of the icon IDs.
- `source-sprite.png`: original montage, for reproducible extraction.
- `../tools/extract_icons.py`: Python/Pillow extraction utility.

The image consists of 14 central columns of 17 icons each and 2 outer columns of 14 icons each. **The outer columns are shifted down half a row.** A naive regular grid cuts through some icons.

Run from the repository root after placing the original source image at `icons/source-sprite.png`:

```sh
python3 -m pip install Pillow
python3 tools/extract_icons.py icons/source-sprite.png icons/png
```

The input collage is **RGB with grey backgrounds**, not RGBA. To avoid destructive edits or false transparency, the extraction preserves those background pixels and does not resize or interpolate.

This is an initial icon library for evaluation, not yet a semantic resource API for the window manager. Before general use, map selected icons to descriptive names and determine which backgrounds/transparency and target pixel formats (RGB565 / RGB888) are appropriate.

**Provenance:** Source icon licenses were not included in the montage. Check original attribution and redistribution permissions before publicly committing the PNG artwork.
