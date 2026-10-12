import importlib.util
from pathlib import Path
import struct
import tempfile
import unittest

spec = importlib.util.spec_from_file_location("disc", Path(__file__).resolve().parents[1] / "tools/disc.py")
disc = importlib.util.module_from_spec(spec)
spec.loader.exec_module(disc)


def image(name=b"default.xbe", left=0, length=4):
    data = bytearray(36 * 2048)
    data[65536:65556] = disc.MAGIC
    data[65536 + 2048 - 20:65536 + 2048] = disc.MAGIC
    struct.pack_into("<II", data, 65556, 33, 64)
    struct.pack_into("<HHIIBB", data, 33 * 2048, left, 0, 34, length, 0x20, len(name))
    data[33 * 2048 + 14:33 * 2048 + 14 + len(name)] = name
    data[34 * 2048:34 * 2048 + 4] = b"XBEH"
    return data


class DiscTests(unittest.TestCase):
    def test_valid_listing_and_extraction(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "disc.iso"
            path.write_bytes(image())
            target = Path(directory) / "output"
            with disc.Disc(path) as reader:
                self.assertEqual(reader.entries()[0]["path"], "default.xbe")
                result = reader.extract(target)
                self.assertEqual(result["partition_offset"], 0)
                self.assertEqual((target / "default.xbe").read_bytes(), b"XBEH")
                with self.assertRaises(FileExistsError):
                    reader.extract(target)

    def test_unsafe_path(self):
        for name in (b"../bad", b"..", b"a\\b", b"c:bad", b"\0bad"):
            with self.subTest(name=name), tempfile.TemporaryDirectory() as directory:
                path = Path(directory) / "disc.iso"
                path.write_bytes(image(name))
                with disc.Disc(path) as reader, self.assertRaises(disc.FormatError):
                    reader.entries()

    def test_directory_cycles_and_truncation(self):
        for data in (image(left=1), image(length=0xFFFFFFFF), bytearray(2048)):
            with self.subTest(), tempfile.TemporaryDirectory() as directory:
                path = Path(directory) / "disc.iso"
                path.write_bytes(data)
                with self.assertRaises((disc.FormatError, UnicodeError)):
                    with disc.Disc(path) as reader:
                        reader.entries()

    def test_rejects_symlinks(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            path = root / "disc.iso"
            path.write_bytes(image())
            target = root / "output"
            target.mkdir()
            outside = root / "outside"
            outside.write_bytes(b"untouched")
            (target / "default.xbe").symlink_to(outside)
            with disc.Disc(path) as reader, self.assertRaises(disc.FormatError):
                reader.extract(target)
            self.assertEqual(outside.read_bytes(), b"untouched")

    def test_xbe_invalid_input(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "invalid.xbe"
            for data in (b"", b"XBEH", bytes(4096)):
                path.write_bytes(data)
                with self.assertRaises(disc.FormatError):
                    disc.inspect_xbe(path)


if __name__ == "__main__":
    unittest.main()
