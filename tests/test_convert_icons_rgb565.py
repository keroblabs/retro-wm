"""SDL-free tests for tools/convert_icons_rgb565.py; requires Pillow."""

import importlib.util
import json
import shutil
import struct
import subprocess
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
                  opaque=False, crop_config=overrides, analyse_only=False,
                  cleanup=False, reset_maps=False, mask_dir=root / 'masks',
                  no_c=False))()
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
            pixel_map = args.mask_dir / 'sample.txt'
            self.assertTrue(pixel_map.exists())
            rows = pixel_map.read_text(encoding='ascii').splitlines()
            self.assertEqual(len(rows), 32)
            self.assertTrue(all(len(row) == 32 and set(row) <= set('#.')
                                for row in rows))
            self.assertEqual(rows[0][0], '.')
            c_header = args.output_dir / 'c' / 'sample.h'
            c_source = args.output_dir / 'c' / 'sample.c'
            self.assertTrue(c_header.is_file())
            self.assertTrue(c_source.is_file())
            self.assertIn('extern const icon_t sample;', c_header.read_text())
            self.assertIn('.pixel_type = WM_ICON_PIXEL_RGB565', c_source.read_text())
            self.assertIn('UINT32_C(0x0000F81F)', c_source.read_text())
            self.assertIn('0xF81Fu', c_source.read_text())


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
                                               'little'), bytes.fromhex('ffff'))

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


    def test_manual_cleanup_only_changes_selected_pixels(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            incoming = root / 'input'
            incoming.mkdir()
            source = Image.new('RGB', (8, 8), 'white')
            for y in range(2, 6):
                for x in range(2, 6):
                    source.putpixel((x, y), (0, 0, 0))
            source.save(incoming / 'sample.png')
            settings = type('Args', (), dict(
                input_dir=incoming, output_dir=root / 'generated',
                size=8, threshold=12, source_padding=0, canvas_padding=0,
                background=(255, 255, 255), endian='little',
                colour_key=0xF81F, flood_tolerance=24, alpha_cutoff=128,
                opaque=False, crop_config=None, analyse_only=False,
                cleanup=False, reset_maps=False, mask_dir=root / 'masks',
                no_c=False))()
            # Use a manual full-image crop to obtain exact pixel correspondence.
            cfg = root / 'crops.json'
            cfg.write_text(json.dumps({'sample.png': [0, 0, 8, 8]}))
            settings.crop_config = cfg
            module.run(settings)
            rawpath = settings.output_dir / 'raw' / 'sample.rgb565'
            baseline = rawpath.read_bytes()
            rows = (settings.mask_dir / 'sample.txt').read_text().splitlines()
            self.assertEqual(rows[3][3], '#')
            self.assertEqual(rows[0][0], '.')
            rows[3] = rows[3][:3] + '.' + rows[3][4:]
            rows[0] = '#' + rows[0][1:]  # restore previously transparent white
            (settings.mask_dir / 'sample.txt').write_text(
                '\n'.join(rows) + '\n', encoding='ascii')
            settings.cleanup = True
            module.run(settings)
            cleaned = rawpath.read_bytes()
            pos = (3 * 8 + 3) * 2
            self.assertEqual(cleaned[:2], bytes.fromhex('ffff'))
            self.assertEqual(cleaned[2:pos], baseline[2:pos])
            self.assertEqual(cleaned[pos:pos + 2], bytes.fromhex('1ff8'))
            self.assertEqual(cleaned[pos + 2:], baseline[pos + 2:])
            self.assertEqual((settings.mask_dir / 'sample.txt').read_text().splitlines(),
                             rows)
            manifest = json.loads((settings.output_dir / 'manifest.json').read_text())
            self.assertTrue(manifest['cleanup_applied'])
            self.assertEqual(manifest['icons'][0]['cleanup_removed_pixels'], 1)
            self.assertEqual(manifest['icons'][0]['cleanup_restored_pixels'], 1)
            c_source = (settings.output_dir / 'c' / 'sample.c').read_text()
            self.assertEqual(manifest['icons'][0]['c_symbol'], 'sample')
            self.assertIn('0xFFFFu', c_source)  # restored white at (0, 0)
            preview = Image.open(settings.output_dir / 'preview' / 'sample.png')
            self.assertEqual(preview.getpixel((3, 3))[3], 0)
            self.assertEqual(preview.getpixel((0, 0)), (255, 255, 255, 255))

    def test_cleanup_rejects_invalid_maps_and_stale_geometry(self):
        with tempfile.TemporaryDirectory() as temp:
            folder = Path(temp)
            rgb = Image.new('RGBA', (4, 4), (0, 0, 0, 255))
            rgb.putpixel((0, 0), (0, 0, 0, 0))
            baseline = module.encode_rgb565(rgb, 'little', 0xf81f)
            module.export_pixel_map(folder, 'icon', baseline, (4, 4),
                                    'little', 0xf81f)
            path, metadata = module.pixel_map_paths(folder, 'icon')
            original = path.read_text()
            self.assertEqual(original.splitlines()[0][0], '.')
            restoration_image = Image.new('RGB', (4, 4), (0, 0, 0))
            restoration_image.putpixel((0, 0), (255, 255, 255))
            restoration = module.encode_rgb565(restoration_image, 'little')
            path.write_text('#' + original[1:])
            restored, removed, added = module.apply_pixel_cleanup(
                baseline, restoration, (4, 4), 'little', 0xf81f,
                path, metadata)
            self.assertEqual((removed, added), (0, 1))
            self.assertEqual(restored[:2], bytes.fromhex('ffff'))
            self.assertEqual(restored[2:], baseline[2:])
            path.write_text('###\n')
            with self.assertRaisesRegex(ValueError, 'expected exactly'):
                module.apply_pixel_cleanup(baseline, restoration, (4, 4),
                                           'little', 0xf81f, path, metadata)
            path.write_text(original)
            changed = bytearray(baseline)
            changed[2] ^= 1
            with self.assertRaisesRegex(ValueError, 'changed since mask export'):
                module.apply_pixel_cleanup(bytes(changed), restoration, (4, 4),
                                           'little', 0xf81f, path, metadata)
            # A normal re-export may not silently erase user edits.
            path.write_text('manual edit\n')
            self.assertFalse(module.export_pixel_map(folder, 'icon', baseline,
                                                     (4, 4), 'little', 0xf81f))
            self.assertEqual(path.read_text(), 'manual edit\n')
            self.assertTrue(module.export_pixel_map(folder, 'icon', baseline,
                                                    (4, 4), 'little', 0xf81f,
                                                    reset=True))
            self.assertEqual(path.read_text(), original)


    def test_restoring_original_nonwhite_colour_and_reserved_key_collision(self):
        with tempfile.TemporaryDirectory() as temp:
            folder = Path(temp)
            baseline = bytes.fromhex('1ff8 0000')  # transparent, black
            module.export_pixel_map(folder, 'item', baseline, (2, 1),
                                    'little', 0xf81f)
            mask, metadata = module.pixel_map_paths(folder, 'item')
            mask.write_text('##\n', encoding='ascii')
            original_red = bytes.fromhex('00f8 0000')  # red, black
            result, removed, restored = module.apply_pixel_cleanup(
                baseline, original_red, (2, 1), 'little', 0xf81f,
                mask, metadata)
            self.assertEqual(result, original_red)
            self.assertEqual((removed, restored), (0, 1))
            collision = bytes.fromhex('1ff8 0000')
            with self.assertRaisesRegex(ValueError, 'collide'):
                module.apply_pixel_cleanup(
                    baseline, collision, (2, 1), 'little', 0xf81f,
                    mask, metadata)

    def test_big_endian_restoration_and_bad_backup_length(self):
        with tempfile.TemporaryDirectory() as temp:
            folder = Path(temp)
            baseline = bytes.fromhex('f81f 07e0')
            module.export_pixel_map(folder, 'be', baseline, (2, 1),
                                    'big', 0xf81f)
            mask, metadata = module.pixel_map_paths(folder, 'be')
            mask.write_text('##\n', encoding='ascii')
            result, removed, restored = module.apply_pixel_cleanup(
                baseline, bytes.fromhex('001f 07e0'), (2, 1),
                'big', 0xf81f, mask, metadata)
            self.assertEqual(result, bytes.fromhex('001f 07e0'))
            self.assertEqual((removed, restored), (0, 1))
            with self.assertRaisesRegex(ValueError, 'restoration RGB565'):
                module.apply_pixel_cleanup(
                    baseline, b'\x00', (2, 1), 'big', 0xf81f,
                    mask, metadata)


    def test_generate_rgb565_c_arrays_and_raw_byte_order(self):
        red_green = Image.new('RGB', (2, 1))
        red_green.putdata([(255, 0, 0), (0, 255, 0)])
        for endian in ('little', 'big'):
            raw = module.encode_rgb565(red_green, endian)
            header, source = module.generate_c_icon(
                'icon-test', raw, (2, 1), endian, 0xf81f)
            self.assertIn('extern const icon_t icon_test;', header)
            self.assertIn('#include "icon_test.h"', source)
            self.assertIn('0xF800u, 0x07E0u', source)
            self.assertIn('.width = 2u', source)
            self.assertIn('.height = 1u', source)
            self.assertIn('.pixel_type = WM_ICON_PIXEL_RGB565', source)
            self.assertIn('.colour_key = UINT32_C(0x0000F81F)', source)
            no_key_header, no_key_source = module.generate_c_icon(
                'opaque', raw, (2, 1), endian, None)
            self.assertIn('.colour_key = WM_ICON_COLOUR_KEY_NONE',
                          no_key_source)
            self.assertIn('extern const icon_t opaque;', no_key_header)
        self.assertEqual(module.icon_symbol('12-name'), '_12_name')
        with self.assertRaisesRegex(ValueError, 'must be packed RGB565'):
            module.generate_c_icon('bad', b'\x00', (2, 1), 'little', 0xf81f)
        with self.assertRaisesRegex(ValueError, 'fit into uint16_t'):
            module.generate_c_icon('large', b'', (65536, 1), 'little', 0xf81f)



    def test_generated_icon_c_compiles_with_portable_resource_header(self):
        compiler = shutil.which('cc')
        if compiler is None:
            self.skipTest('C compiler unavailable')
        source_image = Image.new('RGB', (2, 2), (255, 255, 255))
        raw = module.encode_rgb565(source_image, 'little')
        header, source_code = module.generate_c_icon(
            'icon_test', raw, (2, 2), 'little', 0xf81f)
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / 'icon_test.h').write_text(header, encoding='ascii')
            (root / 'icon_test.c').write_text(source_code, encoding='ascii')
            include_dir = Path(__file__).resolve().parents[1] / 'include'
            subprocess.run(
                [compiler, '-std=c11', '-Wall', '-Wextra', '-Wpedantic',
                 '-Werror', '-I', str(include_dir), '-I', str(root),
                 '-c', str(root / 'icon_test.c'), '-o', str(root / 'icon_test.o')],
                check=True, capture_output=True, text=True)
            self.assertTrue((root / 'icon_test.o').is_file())


if __name__ == '__main__':
    unittest.main()
