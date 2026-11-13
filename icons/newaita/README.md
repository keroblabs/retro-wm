# Newaita screenshot-derived icons (evaluation library)

This directory is reserved for a preliminary **28-icon** collection, extracted from a supplied 750x353 JPEG preview of the Newaita icon theme (seven columns, four rows). Download the complete ZIP pack from the Retro WM project conversation, or place the source JPG here as source-sprite.jpg and run the extractor:

    python3 -m pip install Pillow
    python3 tools/extract_newaita_icons.py icons/newaita/source-sprite.jpg icons/newaita

The extraction produces:
- png/: 28 88x88 RGB PNGs, using unmodified source-image pixels.
- png32/: 28 32x32 RGB PNGs, centered/rescaled for preliminary WM testing.
- contact-sheet.png, manifest.csv, and manifest.json.

Every image retains the JPEG's original **opaque white background**. PNG conversion is not a recovery of original vector artwork or alpha transparency. Prefer Newaita's upstream vector assets for production quality.

## Licensing

The Newaita source project states that its theme is licensed **CC BY-NC-SA 3.0** (Attribution–NonCommercial–ShareAlike). Extracting/resizing these icons does not change their licence. They must not be treated as covered by Retro WM's source-code licence. See ATTRIBUTION.md and verify third-party logo/trademark rights before publication or commercial use. We have not pushed image binaries to this repository.
