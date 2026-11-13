#!/usr/bin/env python3
"""Extract 32x32 PNG icons from the supplied 700x536 montage.

Usage: python3 tools/extract_icons.py icons/source-sprite.png icons/png

Requires Pillow. Note: source contains opaque, baked-in grey backgrounds.
"""
import argparse
import csv
import json
from pathlib import Path
from PIL import Image

COLUMN_CENTERS = [16, 56, 102, 148, 194, 240, 286, 333,
                  379, 417, 464, 509, 556, 602, 648, 683]
CENTER_ROWS = [16 + (31.5 * n) for n in range(17)]
OUTER_ROWS = [63 + (31.45 * n) for n in range(14)]


def extract(source: Path, directory: Path):
    img = Image.open(source).convert("RGB")
    if img.size != (700, 536):
        raise ValueError(f"Expected 700x536 source montage, got {img.size}")
    directory.mkdir(parents=True, exist_ok=True)
    records = []
    for col, xc in enumerate(COLUMN_CENTERS):
        rows = OUTER_ROWS if col in (0, 15) else CENTER_ROWS
        for row, yc in enumerate(rows):
            left, top = round(xc - 16), round(yc - 16)
            box = (left, top, left + 32, top + 32)
            if box[0] < 0 or box[1] < 0 or box[2] > img.width or box[3] > img.height:
                raise ValueError(f"Crop outside source: col={col}, row={row}, box={box}")
            name = f"icon_c{col:02d}_r{row:02d}.png"
            img.crop(box).save(directory / name, optimize=True)
            records.append({"filename": name, "column": col, "row": row,
                            "box_xyxy": list(box), "size": [32, 32]})

    with (directory.parent / "manifest.json").open("w", encoding="utf8") as file:
        json.dump({"source": source.name, "source_size": [700, 536],
                   "crop_size": [32, 32], "icon_count": len(records),
                   "icons": records}, file, indent=2)
    with (directory.parent / "manifest.csv").open("w", newline="", encoding="utf8") as file:
        writer = csv.writer(file)
        writer.writerow(["filename", "column", "row", "x0", "y0", "x1", "y1"])
        for record in records:
            writer.writerow([record["filename"], record["column"], record["row"],
                             *record["box_xyxy"]])
    return records


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("directory", type=Path)
    args = parser.parse_args()
    print(f"Extracted {len(extract(args.source, args.directory))} icons")
