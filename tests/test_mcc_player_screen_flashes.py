"""Import ownership and exact TU1.5 flash admission boundary regressions."""
from pathlib import Path
import struct
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from audit_rendering import FormatError
from mcc_player_screen_flashes import FLASH_GATE, inventory_map


class Bank:
    def __init__(self, path):
        self.path, self.magic = path, 0x1000
        self.data = bytearray(0x200)
        struct.pack_into("<II", self.data, 0x6c, 1, 0x1100)
        # Explicit offsets retain Player Responses padding at +02 and +0E.
        struct.pack_into("<h", self.data, 0x100, 2)
        struct.pack_into("<hh", self.data, 0x104, 0, 1)
        struct.pack_into("<f", self.data, 0x108, .4)
        struct.pack_into("<h", self.data, 0x10c, 1)
        struct.pack_into("<5f", self.data, 0x110, .5, 1, 0, .6, .8)
        self.tags = {
            0: dict(datum=0, group="jpt!", name="damage", address=0x1000, size=0xc8),
            1: dict(datum=1, group="jpt!", name="damage_alias", address=0x1000, size=0),
            2: dict(datum=2, group="bitm", name="interned_rows", address=0x1100, size=0x4c),
        }
        path.write_bytes(self.data)

    def pointer(self, address, magic, size):
        offset = address - magic
        if offset < 0 or offset + size > len(self.data):
            raise FormatError("Synthetic bank escape")
        return offset


class FlashImportTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="mcc-flash-import-")
        self.addCleanup(self.temporary.cleanup)
        self.bank = Bank(Path(self.temporary.name) / "local.map")

    def test_none_flash_still_passes_original_duration_intensity_gates(self):
        result = inventory_map(self.bank)
        self.assertEqual((result["damage_tags"], result["flash_tags"], result["aliases"]), (2, 2, 1))
        self.assertEqual(result["tags"][0]["rows"], result["tags"][1]["rows"])
        self.assertEqual(result["tags"][1]["backing_datum"], 0)
        self.assertEqual(result["tags"][0]["rows"][0]["fields"][:3], [2, 0, 1])

    def test_exact_binary32_gate_is_exclusive(self):
        for offset in (0x108, 0x110):
            with self.subTest(offset=offset):
                old = bytes(self.bank.data[offset:offset + 4])
                struct.pack_into("<f", self.bank.data, offset, FLASH_GATE)
                self.assertEqual(inventory_map(self.bank)["flash_tags"], 0)
                struct.pack_into("<I", self.bank.data, offset, 0x3d4cccce)
                self.assertEqual(inventory_map(self.bank)["flash_tags"], 2)
                self.bank.data[offset:offset + 4] = old

    def test_alias_requires_same_address_and_group(self):
        self.bank.tags[1]["address"] += 4
        with self.assertRaisesRegex(FormatError, "exact typed owner"):
            inventory_map(self.bank)
        self.bank.tags[1]["address"] -= 4
        self.bank.tags[0]["group"] = "bitm"
        with self.assertRaisesRegex(FormatError, "exact typed owner"):
            inventory_map(self.bank)

    def test_response_pointer_cannot_escape_owned_allocation(self):
        struct.pack_into("<I", self.bank.data, 0x70, 0x1104)
        with self.assertRaisesRegex(FormatError, "escapes owned metadata"):
            inventory_map(self.bank)

    def test_shared_merge_requires_complete_salted_identity(self):
        shared = Bank(Path(self.temporary.name) / "shared.map")
        self.bank.tags[0]["address"] = 0
        shared.tags[0]["datum"] = 0x10000
        with self.assertRaisesRegex(FormatError, "datum/group/path collision"):
            inventory_map(self.bank, shared)


if __name__ == "__main__":
    unittest.main()
