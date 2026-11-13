#!/usr/bin/env python3
"""Extract 28 icon images from the user-provided 750x353 Newaita JPEG.

  python3 tools/extract_newaita_icons.py icons/newaita/source-sprite.jpg icons/newaita

Requires Pillow. Output includes source-resolution (88x88) PNGs, centered
32x32 RGB PNGs, manifests, and a labelled contact sheet. Both variants
preserve opaque white backgrounds: they are not transparency-masked.
"""
import argparse
import csv
import json
from pathlib import Path
from PIL import Image, ImageChops, ImageDraw, ImageFont

SOURCE_SIZE = (750, 353)
CELL_SIZE = 88
COLUMNS = (0, 105, 218, 332, 445, 559, 662)
ROWS = (0, 87, 176, 265)
LABELS = (
    ("star", "drive", "folder_file", "folder", "folder_settings", "folder_tray", "trash"),
    ("donkey", "monkey", "firefox", "tools", "cassette", "firewall", "color_wheel"),
    ("disc_document", "r_document", "play_document", "audio_document", "archive", "banana", "image_document"),
    ("aperture", "monitor", "usb_drive", "microphone", "gamepad", "mouse", "floppy_disk"),
)


def scaled_thumbnail(crop: Image.Image) -> tuple[Image.Image, tuple[int, int, int, int]]:
    """Center visible content for a 32x32 white-background preview."""
    white = Image.new("RGB", crop.size, (255, 255, 255))
    difference = ImageChops.difference(crop, white).convert("L")
    mask = difference.point(lambda p: 255 if p > 16 else 0)
    bbox = mask.getbbox()
    if bbox is None:
        raise ValueError("Empty icon crop")
    x0, y0, x1, y1 = bbox
    x0, y0 = max(0, x0 - 2), max(0, y0 - 2)
    x1, y1 = min(crop.width, x1 + 2), min(crop.height, y1 + 2)
    contents = crop.crop((x0, y0, x1, y1))
    contents.thumbnail((30, 30), Image.Resampling.LANCZOS)
    result = Image.new("RGB", (32, 32), (255, 255, 255))
    result.paste(contents, ((32 - contents.width) // 2, (32 - contents.height) // 2))
    return result, (x0, y0, x1, y1)


def contact_sheet(entries: list[dict], folder: Path) -> None:
    cell_w, cell_h, top = 132, 133, 61
    sheet = Image.new("RGB", (7 * cell_w + 12, 4 * cell_h + top + 12), "#e9eef3")
    pen = ImageDraw.Draw(sheet)
    font = ImageFont.load_default()
    pen.text((15, 10), "RETRO WM / NEWAITA ICON EXTRACTION", fill="#1f2f42", font=font)
    pen.text((15, 28), "28 icons - originals 88x88 / previews 32x32", fill="#42516c", font=font)
    for ent in entries:
        c, r = ent["column"] - 1, ent["row"] - 1
        x, y = 12 + c * cell_w, top + r * cell_h
        pen.rounded_rectangle((x, y, x + 121, y + 123), 8, fill="white", outline="#cdd4e0")
        sheet.paste(Image.open(folder / "png" / ent["filename"]), (x + 16, y + 6))
        pen.text((x + 8, y + 99), f"C{c+1:02d} R{r+1:02d} - {ent['label']}", fill="#182638", font=font)
    sheet.save(folder / "contact-sheet.png", optimize=True)


def extract(source: Path, folder: Path) -> list[dict]:
    img = Image.open(source).convert("RGB")
    if img.size != SOURCE_SIZE:
        raise ValueError(f"Expected {SOURCE_SIZE}, got {img.size}")
    (folder / "png").mkdir(parents=True, exist_ok=True)
    (folder / "png32").mkdir(parents=True, exist_ok=True)
    entries = []
    for r, y in enumerate(ROWS, 1):
        for c, x in enumerate(COLUMNS, 1):
            label = LABELS[r - 1][c - 1]
            name = f"newaita_c{c:02d}_r{r:02d}_{label}.png"
            box = (x, y, x + CELL_SIZE, y + CELL_SIZE)
            crop = img.crop(box)
            crop.save(folder / "png" / name, optimize=True)
            small, content_box = scaled_thumbnail(crop)
            small.save(folder / "png32" / name, optimize=True)
            entries.append({
                "filename": name, "label": label, "column": c, "row": r,
                "source_box_xyxy": list(box),
                "content_box_in_crop_xyxy": list(content_box),
                "original_size": [88, 88], "preview_size": [32, 32],
            })
    manifest = {
        "source": source.name, "source_size": list(SOURCE_SIZE),
        "extracted_count": len(entries),
        "license": "CC BY-NC-SA 3.0 (upstream Newaita)",
        "icons": entries,
    }
    (folder / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    with (folder / "manifest.csv").open("w", newline="", encoding="utf-8") as out:
        writer = csv.writer(out)
        writer.writerow(("filename", "label", "column", "row", "x0", "y0", "x1", "y1"))
        for ent in entries:
            writer.writerow((ent["filename"], ent["label"], ent["column"], ent["row"],
                             *ent["source_box_xyxy"]))
    contact_sheet(entries, folder)
    return entries


if __name__ == "__main__":
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("source", type=Path)
    p.add_argument("folder", type=Path)
    args = p.parse_args()
    print(f"Extracted {len(extract(args.source, args.folder))} icons into {args.folder}")
