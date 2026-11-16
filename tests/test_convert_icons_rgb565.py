"""SDL-free tests for tools/convert_icons_rgb565.py; requires Pillow."""

import importlib.util
import json
import struct
import tempfile
import unittest
from pathlib import Path

from PIL import Image

TOOL = Path(__file__).resolve().parents[1] / 'tools' / 'convert_icons_rgb565.py'
spec = importlib.util.spec_from_file_location('convert_icons_rgb565', TOOL)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class IconConversionTests(unittest.TestCase):
    def test_rgb565_word_and_endian(self):
        pixels = Image.new('RGB', (3, 1))
        pixels.putdata([(255, 0, 0), (0, 255, 0), (0, 0, 255)])
        self.assertEqual(module.encode_rgb565(pixels, 'little'),
                         bytes.fromhex('00f8e0071f00'))
        self.assertEqual(module.encode_rgb565(pixels, 'big'),
                         bytes.fromhex('f80007e0001f'))
        back = module.decode_rgb565(module.encode_rgb565(pixels, 'little'),
                                    pixels.size, 'little')
        self.assertEqual(list(back.getdata()), list(pixels.getdata()))

    def test_detect_crop_and_aspect_ratio(self):
        image = Image.new('RGB', (88, 88), 'white')
        for y in range(20, 75):
            for x in range(31, 43):
                image.putpixel((x, y), (0, 0, 0))
        box = module.detect_bbox(image, (255, 255, 255), 12)
        self.assertEqual(box, [31, 20, 43, 75])
        padded = module.padded_box(box, 88, 88, 2)
        self.assertEqual(padded, [29, 18, 45, 77])
        result, placement = module.layout(image, padded, 64, 3, (255, 255, 255))
        self.assertEqual(result.size, (64, 64))
        self.assertEqual(placement[3] - placement[1], 58)
        self.assertLess(placement[2] - placement[0], 25)
        self.assertEqual(result.getpixel((0, 0)), (255, 255, 255))

    def test_crop_validation(self):
        module.validate_box([0, 0, 88, 88], 88, 88, 'source')
        for bad in ([0, 0, 89, 88], [0, 0, 0, 88], [1, 2, 3], [1, 1, 5.5, 6]):
            with self.assertRaises(ValueError):
                module.validate_box(bad, 88, 88, 'source')

    def test_rgba_matte_and_batch_output(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source = root / 'input'
            source.mkdir()
            icon = Image.new('RGBA', (16, 16), (255, 0, 0, 0))
            for y in range(4, 12):
                for x in range(3, 13):
                    icon.putpixel((x, y), (0, 0, 0, 255))
            icon.save(source / 'sample.png')
            overrides = root / 'crops.json'
            overrides.write_text(json.dumps({'sample.png': [2, 3, 14, 13]}))
            args = type('Args', (), dict(input_dir=source, output_dir=root / 'generated',
                  size=32, threshold=12, source_padding=2, canvas_padding=2,
                  background=(255, 255, 255), endian='little',
                  colour_key=0xF81F, flood_tolerance=24, alpha_cutoff=128,
                  opaque=False, crop_config=overrides, analyse_only=False))()
            module.run(args)
            data = (args.output_dir / 'raw' / 'sample.rgb565').read_bytes()
            self.assertEqual(len(data), 32 * 32 * 2)
            metadata = json.loads((args.output_dir / 'manifest.json').read_text())
            self.assertEqual(metadata['icons'][0]['crop_box_exclusive'], [2, 3, 14, 13])
            self.assertTrue(metadata['icons'][0]['manual_crop'])
            self.assertTrue((args.output_dir / 'preview' / 'sample.png').exists())
            self.assertTrue((args.output_dir / 'crop-contact-sheet.png').exists())
            self.assertEqual(data[:2], struct.pack('<H', 0xf81f))
            self.assertEqual(metadata['colour_key_rgb565'], '0xF81F')
            self.assertGreater(metadata['icons'][0]['transparent_pixels'], 0)
            preview = Image.open(args.output_dir / 'preview' / 'sample.png')
            self.assertEqual(preview.mode, 'RGBA')
            self.assertEqual(preview.getpixel((0, 0))[3], 0)


    def test_flood_preserves_enclosed_white_and_distinct_opaque_white(self):
        image = Image.new('RGB', (11, 11), (255, 255, 255))
        for y in range(2, 9):
            for x in range(2, 9):
                if x in (2, 8) or y in (2, 8):
                    image.putpixel((x, y), (0, 0, 0))
        mask = module.flood_exterior(image, (255, 255, 255), 24)
        self.assertEqual(mask.getpixel((0, 0)), 0)
        self.assertEqual(mask.getpixel((5, 5)), 255)  # enclosed white
        self.assertEqual(mask.getpixel((2, 5)), 255)  # black contour
        canvas, _ = module.layout(image, [0, 0, 11, 11], 11, 0,
                                  (255, 255, 255), mask)
        raw = module.encode_rgb565(canvas, 'little', 0xf81f)
        words = struct.unpack('<121H', raw)
        self.assertEqual(words[0], 0xf81f)
        self.assertEqual(words[5 * 11 + 5], 0xffff)
        self.assertEqual(words[5 * 11 + 2], 0x0000)
        preview = module.decode_rgb565(raw, (11, 11), 'little', 0xf81f)
        self.assertEqual(preview.getpixel((0, 0))[3], 0)
        self.assertEqual(preview.getpixel((5, 5)), (255, 255, 255, 255))

    def test_flood_near_white_and_opaque_flag(self):
        image = Image.new('RGB', (7, 7), (247, 247, 247))
        image.putpixel((3, 3), (0, 0, 0))
        exterior = module.flood_exterior(image, (255, 255, 255), 12)
        self.assertEqual(exterior.getpixel((0, 0)), 0)
        self.assertEqual(exterior.getpixel((3, 3)), 255)
        strict = module.flood_exterior(image, (255, 255, 255), 4)
        self.assertEqual(strict.getpixel((0, 0)), 255)
        self.assertEqual(module.encode_rgb565(Image.new('RGB', (1, 1), 'white'),
                                               'little'), b'\\xff\\xff'.decode('unicode_escape').encode('latin1'))

    def test_big_endian_key_and_reserved_colour_collision(self):
        image = Image.new('RGBA', (2, 1))
        image.putdata([(123, 12, 4, 0), (255, 255, 255, 255)])
        data = module.encode_rgb565(image, 'big', 0xf81f)
        self.assertEqual(data, bytes.fromhex('f81fffff'))
        preview = module.decode_rgb565(data, (2, 1), 'big', 0xf81f)
        self.assertEqual(preview.getpixel((0, 0))[3], 0)
        self.assertEqual(preview.getpixel((1, 0)), (255, 255, 255, 255))
        with self.assertRaises(ValueError):
            module.encode_rgb565(Image.new('RGB', (1, 1), (255, 0, 255)),
                                  'little', 0xf81f)
        self.assertEqual(module.parse_colour_key('0xf81f'), 0xf81f)
        with self.assertRaises(Exception):
            module.parse_colour_key('0x10000')


if __name__ == '__main__':
    unittest.main()
