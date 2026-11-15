"""Exercise cropping, character order, grayscale scaling and nibble layout.

Run from the repository root:
    python3 -m unittest discover -s tests -p 'test_convert_font.py'
"""

from pathlib import Path
import re
import tempfile
import unittest

from PIL import Image

from tools.convert_font import convert_font, pack_glyph


def bitmap_from_c(path):
    text = path.read_text(encoding="utf-8")
    bitmap = text.split("[] = {", 1)[1].split("};", 1)[0]
    bitmap = re.sub(r"/\*.*?\*/", "", bitmap, flags=re.DOTALL)
    return bytes(int(value, 16) for value in re.findall(r"0x[0-9A-F]{2}", bitmap))


class ConvertFontTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.source = Path(self.directory.name) / "source.png"
        self.output = Path(self.directory.name) / "font.c"

    def test_crop_grid_order_and_shared_normalization(self):
        # Bright pixels outside the crops must not affect normalization.
        image = Image.new("L", (16, 14), 255)
        glyphs = [
            [20, 35, 50, 65, 80, 95, 110, 125],
            [140, 155, 170, 185, 200, 215, 230, 245],
            [20] * 8,
            [35, 20, 245, 20, 65, 50, 95, 80],
        ]
        for index, values in enumerate(glyphs):
            left = 3 + (index % 2) * 6
            top = 4 + (index // 2) * 4
            glyph = Image.new("L", (4, 2))
            glyph.putdata(values)
            image.paste(glyph, (left, top))
        image.save(self.source)
        convert_font(self.source, self.output, origin=(2, 3), cell=(6, 4),
                     crop=(1, 1, 4, 2), columns=2, rows=2, first_character=0x30)
        self.assertEqual(bitmap_from_c(self.output), bytes([
            0x10, 0x32, 0x54, 0x76,
            0x98, 0xBA, 0xDC, 0xFE,
            0x00, 0x00, 0x00, 0x00,
            0x01, 0x0F, 0x23, 0x45,
        ]))
        text = self.output.read_text(encoding="utf-8")
        self.assertIn("font_c4x2", text)
        self.assertIn("FONT_C4x2", text)
        self.assertIn("0x30,\t// first character", text)
        self.assertIn("4,\t\t// bytes per glyph", text)

    def test_odd_width_starts_each_row_on_a_byte_boundary(self):
        self.assertEqual(pack_glyph(bytes([1, 2, 3, 4, 5, 6]), 3, 2),
                         bytes([0x21, 0x03, 0x54, 0x06]))

    def test_quantization_rounds_to_nearest_level(self):
        image = Image.new("L", (4, 1))
        image.putdata([20, 27, 28, 245])
        image.save(self.source)
        convert_font(self.source, self.output, origin=(0, 0), cell=(4, 1),
                     crop=(0, 0, 4, 1), columns=1, rows=1)
        self.assertEqual(bitmap_from_c(self.output), bytes([0x00, 0xF1]))

    def test_uniform_crop_produces_zero_pixels(self):
        Image.new("L", (2, 2), 123).save(self.source)
        convert_font(self.source, self.output, origin=(0, 0), cell=(2, 2),
                     crop=(0, 0, 2, 2), columns=1, rows=1)
        self.assertEqual(bitmap_from_c(self.output), bytes([0, 0]))

    def test_transparency_is_composited_before_normalization(self):
        image = Image.new("RGBA", (2, 1))
        image.putdata([(255, 255, 255, 0), (255, 255, 255, 255)])
        image.save(self.source)
        convert_font(self.source, self.output, origin=(0, 0), cell=(2, 1),
                     crop=(0, 0, 2, 1), columns=1, rows=1)
        self.assertEqual(bitmap_from_c(self.output), bytes([0xF0]))

    def test_invalid_geometry_does_not_overwrite_output(self):
        Image.new("L", (8, 8)).save(self.source)
        self.output.write_text("existing output", encoding="utf-8")
        defaults = dict(origin=(0, 0), cell=(2, 2), crop=(0, 0, 2, 2),
                        columns=1, rows=1)
        invalid = [
            {"origin": (-1, 0)},
            {"cell": (0, 2)},
            {"crop": (0, 0, 0, 2)},
            {"crop": (1, 0, 2, 2)},
            {"origin": (7, 0)},
            {"columns": 0},
            {"first_character": -1},
            {"first_character": 0xFFFF, "columns": 2},
        ]
        for options in invalid:
            with self.subTest(options=options):
                with self.assertRaises(ValueError):
                    convert_font(self.source, self.output, **{**defaults, **options})
                self.assertEqual(self.output.read_text(encoding="utf-8"), "existing output")

    def test_source_cannot_be_overwritten(self):
        Image.new("L", (2, 2)).save(self.source)
        original = self.source.read_bytes()
        with self.assertRaisesRegex(ValueError, "different paths"):
            convert_font(self.source, self.source, origin=(0, 0), cell=(2, 2),
                         crop=(0, 0, 2, 2), columns=1, rows=1)
        self.assertEqual(self.source.read_bytes(), original)


if __name__ == "__main__":
    unittest.main()
