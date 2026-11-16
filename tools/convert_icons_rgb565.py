#!/usr/bin/env python3
"""Analyse, crop and export PNG icons as headerless colour-keyed RGB565.

A four-connected flood from source image edges classifies near-background
pixels as transparent; enclosed white details remain opaque. RGB565 has no
alpha, so transparent pixels use a reserved 16-bit colour key (default F81F).
Previews expose the key as PNG alpha. Requires Pillow; sources are unchanged.
"""

import argparse
from collections import deque
import hashlib
import json
import struct
from pathlib import Path

from PIL import Image, ImageChops, ImageDraw


def parse_rgb(value: str) -> tuple[int, int, int]:
    parts = value.split(",")
    if len(parts) != 3:
        raise argparse.ArgumentTypeError("RGB colour must be R,G,B")
    try:
        rgb = tuple(int(part, 0) for part in parts)
    except ValueError as exc:
        raise argparse.ArgumentTypeError("RGB values must be integers") from exc
    if any(channel < 0 or channel > 255 for channel in rgb):
        raise argparse.ArgumentTypeError("RGB channels must be in 0..255")
    return rgb


def to_rgb(image: Image.Image, background: tuple[int, int, int]) -> Image.Image:
    if "A" in image.getbands() or "transparency" in image.info:
        rgba = image.convert("RGBA")
        matte = Image.new("RGBA", rgba.size, (*background, 255))
        return Image.alpha_composite(matte, rgba).convert("RGB")
    return image.convert("RGB")


def detect_bbox(image: Image.Image, background: tuple[int, int, int], threshold: int):
    """Return [left, top, right, bottom] (exclusive), or None for no content."""
    width, height = image.size
    left, top, right, bottom = width, height, 0, 0
    pixels = image.load()
    for y in range(height):
        for x in range(width):
            p = pixels[x, y]
            if max(abs(p[i] - background[i]) for i in range(3)) >= threshold:
                left = min(left, x)
                top = min(top, y)
                right = max(right, x + 1)
                bottom = max(bottom, y + 1)
    return [left, top, right, bottom] if right > left and bottom > top else None


def flood_exterior(image: Image.Image, background: tuple[int, int, int],
                   tolerance: int) -> Image.Image:
    """Binary L mask: 0 for edge-connected background, 255 for icon.

    Only pixels within max per-channel distance 'tolerance' of background
    can be flooded. Four-neighbour connectivity avoids leaking diagonally
    through a one-pixel barrier. Interior white regions are not erased.
    """
    width, height = image.size
    rgb_image = image.convert("RGB")
    pixels = rgb_image.load()
    visited = bytearray(width * height)
    exterior = bytearray(width * height)
    queue = deque()

    def visit(x: int, y: int) -> None:
        index = y * width + x
        if visited[index]:
            return
        visited[index] = 1
        rgb = pixels[x, y]
        if max(abs(rgb[i] - background[i]) for i in range(3)) <= tolerance:
            exterior[index] = 1
            queue.append((x, y))

    for x in range(width):
        visit(x, 0)
        visit(x, height - 1)
    for y in range(height):
        visit(0, y)
        visit(width - 1, y)
    while queue:
        x, y = queue.popleft()
        if x:
            visit(x - 1, y)
        if x + 1 < width:
            visit(x + 1, y)
        if y:
            visit(x, y - 1)
        if y + 1 < height:
            visit(x, y + 1)
    mask = Image.new("L", (width, height))
    mask.putdata([0 if marked else 255 for marked in exterior])
    return mask


def parse_colour_key(value: str) -> int:
    try:
        key = int(value, 0)
    except ValueError as exc:
        raise argparse.ArgumentTypeError("colour key must be a 16-bit integer, e.g. 0xF81F") from exc
    if not 0 <= key <= 0xFFFF:
        raise argparse.ArgumentTypeError("colour key must be within 0x0000..0xFFFF")
    return key


def padded_box(box, width: int, height: int, pad: int):
    l, t, r, b = box
    return [max(0, l - pad), max(0, t - pad), min(width, r + pad), min(height, b + pad)]


def validate_box(box, width: int, height: int, source: str):
    if not isinstance(box, list) or len(box) != 4 or any(type(n) is not int for n in box):
        raise ValueError(f"{source}: crop must be four integers [left, top, right, bottom]")
    l, t, r, b = box
    if not (0 <= l < r <= width and 0 <= t < b <= height):
        raise ValueError(f"{source}: invalid crop {box} for {width}x{height}")


def layout(image: Image.Image, box, size: int, inset: int, background,
           alpha: Image.Image | None = None):
    if alpha is not None:
        if alpha.mode != "L" or alpha.size != image.size:
            raise ValueError("alpha mask must be L and match source dimensions")
        image = image.convert("RGBA")
        image.putalpha(alpha)
    cropped = image.crop(tuple(box))
    available = size - 2 * inset
    if available <= 0:
        raise ValueError("canvas padding leaves no drawable area")
    scale = min(available / cropped.width, available / cropped.height)
    width = max(1, min(available, round(cropped.width * scale)))
    height = max(1, min(available, round(cropped.height * scale)))
    resampling = getattr(Image, "Resampling", Image).LANCZOS
    resized = cropped.resize((width, height), resampling)
    canvas = (Image.new("RGBA", (size, size), (0, 0, 0, 0))
              if alpha is not None else Image.new("RGB", (size, size), background))
    x, y = (size - width) // 2, (size - height) // 2
    canvas.paste(resized, (x, y))
    return canvas, [x, y, x + width, y + height]


def encode_rgb565(image: Image.Image, endian: str,
                  colour_key: int | None = None,
                  alpha_cutoff: int = 128) -> bytes:
    order = "<" if endian == "little" else ">"
    pixels = image.get_flattened_data() if hasattr(image, "get_flattened_data") else list(image.getdata())
    words = []
    for pixel in pixels:
        r, g, b = pixel[:3]
        a = pixel[3] if len(pixel) == 4 else 255
        if colour_key is not None and a < alpha_cutoff:
            words.append(colour_key)
        else:
            value = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
            if colour_key is not None and value == colour_key:
                raise ValueError("opaque pixel quantizes to reserved RGB565 colour key; "
                                 "choose another --colour-key")
            words.append(value)
    return struct.pack(f"{order}{len(words)}H", *words)


def decode_rgb565(data: bytes, size: tuple[int, int], endian: str,
                  colour_key: int | None = None) -> Image.Image:
    order = "<" if endian == "little" else ">"
    count = size[0] * size[1]
    if len(data) != count * 2:
        raise ValueError("invalid RGB565 byte count")
    words = struct.unpack(f"{order}{count}H", data)
    rgb = []
    for value in words:
        r, g, b = (value >> 11) & 31, (value >> 5) & 63, value & 31
        colour = ((r << 3) | (r >> 2), (g << 2) | (g >> 4), (b << 3) | (b >> 2))
        rgb.append((*colour, 0 if value == colour_key else 255)
                   if colour_key is not None else colour)
    image = Image.new("RGBA" if colour_key is not None else "RGB", size)
    image.putdata(rgb)
    return image


def pixel_map_rows(data: bytes, size: tuple[int, int], endian: str,
                   colour_key: int) -> list[str]:
    """One # per opaque pixel and one . per RGB565 colour-key pixel."""
    width, height = size
    if len(data) != width * height * 2:
        raise ValueError("RGB565 data length does not match the pixel-map dimensions")
    words = struct.unpack(("<" if endian == "little" else ">") +
                          f"{width * height}H", data)
    return ["".join("." if word == colour_key else "#" for word in
                    words[y * width:(y + 1) * width]) for y in range(height)]


def pixel_map_paths(directory: Path, name: str) -> tuple[Path, Path]:
    return directory / f"{name}.txt", directory / f"{name}.json"


def map_metadata(data: bytes, size: tuple[int, int],
                 endian: str, colour_key: int) -> dict:
    # Hash the unedited RGB565 result so manual masks cannot silently be
    # applied to a different crop, source image, scale or flood result.
    return {
        "size": list(size),
        "endian": endian,
        "colour_key_rgb565": f"0x{colour_key:04X}",
        "baseline_sha256": hashlib.sha256(data).hexdigest(),
    }


def export_pixel_map(directory: Path, name: str, data: bytes,
                     size: tuple[int, int], endian: str,
                     colour_key: int, reset: bool = False) -> bool:
    """Create editable mask and validation metadata; never overwrite by default."""
    mask_path, meta_path = pixel_map_paths(directory, name)
    if (mask_path.exists() or meta_path.exists()) and not reset:
        if not mask_path.is_file() or not meta_path.is_file():
            raise ValueError(f"{name}: incomplete mask pair; restore the missing file "
                             "or use --reset-maps to regenerate")
        return False
    directory.mkdir(parents=True, exist_ok=True)
    rows = pixel_map_rows(data, size, endian, colour_key)
    mask_path.write_text("\n".join(rows) + "\n", encoding="ascii")
    meta_path.write_text(
        json.dumps(map_metadata(data, size, endian, colour_key), indent=2) + "\n",
        encoding="utf-8")
    return True


def apply_pixel_cleanup(data: bytes, size: tuple[int, int],
                        endian: str, colour_key: int,
                        mask_path: Path, meta_path: Path) -> tuple[bytes, int]:
    """Replace selected opaque pixels with key; never change opaque colours."""
    if not mask_path.is_file() or not meta_path.is_file():
        raise ValueError(f"{mask_path}: missing mask or metadata; run conversion "
                         "once without --cleanup to export initial maps")
    metadata = json.loads(meta_path.read_text(encoding="utf-8"))
    if metadata != map_metadata(data, size, endian, colour_key):
        raise ValueError(f"{mask_path}: source/crop/options changed since mask export; "
                         "review and regenerate with --reset-maps (overwrites edits)")
    width, height = size
    rows = mask_path.read_text(encoding="ascii").splitlines()
    if len(rows) != height or any(len(row) != width for row in rows):
        raise ValueError(f"{mask_path}: expected exactly {height} rows of {width} pixels")
    if any(character not in "#." for row in rows for character in row):
        raise ValueError(f"{mask_path}: only # (opaque) and . (transparent) are valid")
    original = pixel_map_rows(data, size, endian, colour_key)
    for y, (edited, source) in enumerate(zip(rows, original)):
        for x, (want, was) in enumerate(zip(edited, source)):
            if want == "#" and was == ".":
                raise ValueError(f"{mask_path}: cannot restore transparent pixel "
                                 f"at ({x}, {y}); cleanup only changes # to .")
    output = bytearray(data)
    key_bytes = struct.pack("<H" if endian == "little" else ">H", colour_key)
    cleared = 0
    for index, character in enumerate("".join(rows)):
        if character == "." and output[index * 2:index * 2 + 2] != key_bytes:
            output[index * 2:index * 2 + 2] = key_bytes
            cleared += 1
    return bytes(output), cleared


def contact_sheet(records, source_images, previews, target: Path, background):
    row_height, width = 120, 400
    sheet = Image.new("RGB", (width, row_height * len(records) + 24), (235, 235, 235))
    pen = ImageDraw.Draw(sheet)
    pen.text((10, 5), "SOURCE + CROP (red)                  RGB565 PREVIEW", fill=(0, 0, 0))
    for row, record in enumerate(records):
        top = row_height * row + 24
        pen.text((8, top + 2), record["name"], fill=(0, 0, 0))
        original = source_images[row]
        offset = (10, top + 23)
        sheet.paste(original, offset)
        box = record["crop_box_exclusive"]
        pen.rectangle((offset[0] + box[0], offset[1] + box[1],
                       offset[0] + box[2] - 1, offset[1] + box[3] - 1),
                      outline=(220, 20, 20), width=1)
        # Checkerboard exposes transparent pixels in the quantized PNG preview.
        for py in range(previews[row].height):
            for px in range(previews[row].width):
                shade = 205 if ((px // 8 + py // 8) & 1) else 245
                sheet.putpixel((220 + px, top + 23 + py), (shade, shade, shade))
        sheet.paste(previews[row], (220, top + 23),
                    previews[row] if previews[row].mode == "RGBA" else None)
        pen.text((300, top + 28), f"{record['output_size'][0]}x{record['output_size'][1]}", fill=(0, 0, 0))
    sheet.save(target)


def run(args):
    if not args.input_dir.is_dir():
        raise ValueError(f"input directory missing: {args.input_dir}")
    files = sorted(path for path in args.input_dir.iterdir() if path.is_file()
                   and path.suffix.lower() == ".png")
    if not files:
        raise ValueError(f"no PNG files in {args.input_dir}")
    if not 1 <= args.threshold <= 255:
        raise ValueError("threshold must be in 1..255")
    if not 0 <= args.flood_tolerance <= 255:
        raise ValueError("flood tolerance must be in 0..255")
    if not 1 <= args.alpha_cutoff <= 255:
        raise ValueError("alpha cutoff must be in 1..255")
    colour_key = None if args.opaque else args.colour_key
    if args.cleanup and args.opaque:
        raise ValueError("--cleanup requires colour-key mode; remove --opaque")
    if args.cleanup and args.reset_maps:
        raise ValueError("--cleanup and --reset-maps cannot be used together")
    if args.reset_maps and args.opaque:
        raise ValueError("--reset-maps requires colour-key mode")
    if args.source_padding < 0 or args.canvas_padding < 0 or args.size < 1:
        raise ValueError("sizes and padding must be non-negative, with size >= 1")
    if args.size <= args.canvas_padding * 2:
        raise ValueError("canvas padding must be less than half the canvas size")
    overrides = {}
    if args.crop_config is not None:
        with args.crop_config.open(encoding="utf-8") as source:
            overrides = json.load(source)
        if not isinstance(overrides, dict):
            raise ValueError("crop config must be a JSON object: filename -> [l,t,r,b]")
        unknown = set(overrides) - {p.name for p in files}
        if unknown:
            raise ValueError(f"crop config mentions missing PNGs: {sorted(unknown)}")
    if not args.analyse_only:
        (args.output_dir / "raw").mkdir(parents=True, exist_ok=True)
        (args.output_dir / "preview").mkdir(parents=True, exist_ok=True)
    records, sources, previews = [], [], []
    for path in files:
        with Image.open(path) as loaded:
            source = to_rgb(loaded, args.background)
            source_alpha = (loaded.convert("RGBA").getchannel("A")
                            if "A" in loaded.getbands() or "transparency" in loaded.info
                            else None)
        alpha = None
        if colour_key is not None:
            alpha = flood_exterior(source, args.background, args.flood_tolerance)
            if source_alpha is not None:
                alpha = ImageChops.multiply(alpha, source_alpha)
        detected = detect_bbox(source, args.background, args.threshold)
        if detected is None and path.name not in overrides:
            raise ValueError(f"{path}: no content detected; review background/threshold")
        crop = overrides.get(path.name)
        if crop is None:
            crop = padded_box(detected, *source.size, args.source_padding)
        validate_box(crop, *source.size, path.name)
        rendered, placement = layout(source, crop, args.size, args.canvas_padding,
                                     args.background, alpha)
        binary = encode_rgb565(rendered, args.endian, colour_key, args.alpha_cutoff)
        cleaned_pixels = 0
        mask_file = None
        if colour_key is not None and not args.analyse_only:
            mask_path, meta_path = pixel_map_paths(args.mask_dir, path.stem)
            mask_file = str(mask_path)
            if args.cleanup:
                binary, cleaned_pixels = apply_pixel_cleanup(
                    binary, rendered.size, args.endian, colour_key,
                    mask_path, meta_path)
            else:
                exported = export_pixel_map(
                    args.mask_dir, path.stem, binary, rendered.size,
                    args.endian, colour_key, reset=args.reset_maps)
                if exported:
                    print(f"  wrote editable pixel map: {mask_path}")
        preview = decode_rgb565(binary, rendered.size, args.endian, colour_key)
        transparent_count = (sum(1 for pixel in preview.getdata() if pixel[3] == 0)
                             if colour_key is not None else 0)
        item = {
            "name": path.name, "source_size": list(source.size),
            "detected_content_box_exclusive": detected,
            "crop_box_exclusive": crop, "manual_crop": path.name in overrides,
            "output_size": list(rendered.size), "placement_box_exclusive": placement,
            "raw_file": f"raw/{path.stem}.rgb565", "byte_count": len(binary),
            "transparent_pixels": transparent_count,
            "cleanup_removed_pixels": cleaned_pixels,
            "editable_pixel_map": mask_file,
        }
        records.append(item)
        sources.append(source)
        previews.append(preview)
        print(f"{path.name:24} detected={detected} crop={crop} "
              f"placed={placement} raw={len(binary)} bytes")
        if not args.analyse_only:
            (args.output_dir / item["raw_file"]).write_bytes(binary)
            preview.save(args.output_dir / "preview" / path.name)
    if not args.analyse_only:
        manifest = {
            "format": "RGB565", "layout": "row-major, no header, no alpha",
            "endianness": args.endian, "background_rgb": list(args.background),
            "colour_key_rgb565": f"0x{colour_key:04X}" if colour_key is not None else None,
            "flood_connectivity": 4 if colour_key is not None else None,
            "flood_tolerance": args.flood_tolerance if colour_key is not None else None,
            "alpha_cutoff": args.alpha_cutoff if colour_key is not None else None,
            "cleanup_applied": args.cleanup,
            "threshold": args.threshold, "source_padding": args.source_padding,
            "canvas_padding": args.canvas_padding, "size": args.size,
            "icons": records,
        }
        (args.output_dir / "manifest.json").write_text(
            json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
        contact_sheet(records, sources, previews,
                      args.output_dir / "crop-contact-sheet.png", args.background)
        print(f"Wrote {len(records)} icons and crop report to {args.output_dir}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input_dir", nargs="?", type=Path, default=Path("icons/images"))
    parser.add_argument("--output-dir", type=Path, default=Path("icons/converted"))
    parser.add_argument("--size", type=int, default=64, help="square output canvas (default: 64)")
    parser.add_argument("--threshold", type=int, default=12,
                        help="minimum RGB distance from background (default: 12)")
    parser.add_argument("--source-padding", type=int, default=2,
                        help="extra source pixels around detected content (default: 2)")
    parser.add_argument("--canvas-padding", type=int, default=3,
                        help="blank border inside output canvas (default: 3)")
    parser.add_argument("--background", type=parse_rgb, default=(255, 255, 255),
                        help="background/matte RGB triplet, e.g. 255,255,255")
    parser.add_argument("--endian", choices=("little", "big"), default="little")
    parser.add_argument("--colour-key", type=parse_colour_key, default=0xF81F,
                        help="reserved transparent RGB565 word (default: 0xF81F)")
    parser.add_argument("--flood-tolerance", type=int, default=24,
                        help="maximum per-channel difference from background for exterior flood (default: 24)")
    parser.add_argument("--alpha-cutoff", type=int, default=128,
                        help="resampled alpha below this becomes colour-key transparent (default: 128)")
    parser.add_argument("--opaque", action="store_true",
                        help="disable flood/key and emit original opaque RGB565 behaviour")
    parser.add_argument("--mask-dir", type=Path, default=Path("icons/masks"),
                        help="editable pixel map directory (default: icons/masks)")
    parser.add_argument("--cleanup", action="store_true",
                        help="apply edited #/. text maps, clearing marked pixels to the RGB565 key")
    parser.add_argument("--reset-maps", action="store_true",
                        help="explicitly overwrite existing pixel maps and metadata; destroys manual edits")
    parser.add_argument("--crop-config", type=Path, default=None,
                        help="JSON map of PNG name to manual [left,top,right,bottom) crop")
    parser.add_argument("--analyse-only", action="store_true",
                        help="print proposed bounding boxes; write no files")
    args = parser.parse_args()
    try:
        run(args)
    except (OSError, ValueError, json.JSONDecodeError) as exc:
        parser.error(str(exc))


if __name__ == "__main__":
    main()
