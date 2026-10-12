import importlib.util
from pathlib import Path
import struct
import tempfile
import unittest

SPEC = importlib.util.spec_from_file_location("audit_rendering", Path(__file__).resolve().parents[1] / "tools/audit_rendering.py")
AUDIT = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(AUDIT)


class RenderingAuditBounds(unittest.TestCase):
    def setUp(self):
        self.cache = AUDIT.Cache.__new__(AUDIT.Cache)
        self.cache.data = bytearray(64)
        self.cache.magic = 0

    def block(self, count, address):
        struct.pack_into("<II", self.cache.data, 0, count, address)
        return self.cache.block(0, 4)

    def test_virtual_pointer_cannot_wrap_or_escape_bsp(self):
        with self.assertRaises(AUDIT.FormatError):
            self.cache.pointer(1, 2, 4)
        with self.assertRaises(AUDIT.FormatError):
            self.cache.require(63, 2)
        struct.pack_into("<II", self.cache.data, 0, 3, 12)
        with self.assertRaises(AUDIT.FormatError):
            self.cache.block(0, 4, 0, (16, 32))
        with self.assertRaises(AUDIT.FormatError):
            self.cache.block(0, 4, 0, (8, 20))

    def test_excessive_or_truncated_blocks_are_rejected(self):
        with self.assertRaises(AUDIT.FormatError):
            self.block(65537, 8)
        with self.assertRaises(AUDIT.FormatError):
            self.block(2, 60)
        self.assertEqual(self.block(0, 0xFFFFFFFF), [])
        self.assertEqual(self.block(2, 8), [8, 12])

    def test_packed_ranges_do_not_silently_truncate(self):
        records = ["a", "b", "c"]
        self.assertEqual(self.cache.packed_range((2 << 9) | 1, records), ["b", "c"])
        self.assertEqual(self.cache.packed_range(511, records), [])
        with self.assertRaises(AUDIT.FormatError):
            self.cache.packed_range((3 << 9) | 1, records)
        with self.assertRaises(AUDIT.FormatError):
            self.cache.packed_range((1 << 9) | 511, records)

    def test_short_and_wrong_platform_caches_are_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "bad.map"
            for data in (b"head", bytes(2048)):
                path.write_bytes(data)
                with self.assertRaises(AUDIT.FormatError):
                    AUDIT.Cache(path)
                path.unlink()


if __name__ == "__main__":
    unittest.main()
