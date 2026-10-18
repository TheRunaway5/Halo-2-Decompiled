"""Owned MCC menu-resource assembly is separate from runtime menu admission."""
import hashlib
import json
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from audit_rendering import FormatError
from export_vista_menu_ui import dds_image
from mcc_menu_assets import prepare


class MenuAssetsTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="halo2-mcc-ui-")
        self.addCleanup(self.temp.cleanup)
        self.pack = Path(self.temp.name) / "cache"
        self.pack.mkdir()
        self.pixels = bytes(range(16)) + b"\x80\x40\x20\xff"  # 2x2 plus complete 1x1 mip.
        resource = self.pack / "bitmap-resources/one.bin"
        resource.parent.mkdir()
        resource.write_bytes(self.pixels)
        self.frame = dict(dimensions=[2, 2, 1], type=0, partitions=["one", None, None],
            format=11, additional_mips=1, record_hex="actual-168-byte-record")
        self.bitmaps = dict(schema="H2_MCC_BITMAP_RESOURCES_V1",
            banks=[dict(map="mainmenu.map", datum="e1740000", tag="ui\\test", frames=[self.frame])],
            resources={"one": dict(path="bitmap-resources/one.bin", decoded_bytes=20,
                sha256=hashlib.sha256(self.pixels).hexdigest())})
        self.dds = dds_image(2, 2, 1, 11, self.pixels)[0]
        font = self.pack / "halo2/h2_fonts/CaseSensitiveFont"
        font.parent.mkdir(parents=True)
        font.write_bytes(b"original-font")
        self.inventory = dict(schema="H2_MCC_VISTA_MENU_INVENTORY_V1", complete=True,
            pages=[dict(cache="mainmenu.map", name="ui\\test", datum="e1740000", frame=0,
                width=2, height=2, dds_sha256=hashlib.sha256(self.dds).hexdigest())],
            fonts={"h2_fonts/CaseSensitiveFont": dict(bytes=13,
                sha256=hashlib.sha256(b"original-font").hexdigest())},
            unavailable_in_mcc=[dict(name="missing-source-owner")])
        self.save()

    def save(self):
        (self.pack / "bitmap-resources.json").write_text(json.dumps(self.bitmaps))
        (self.pack / "vista-menu-inventory.json").write_text(json.dumps(self.inventory))

    def rejected(self):
        with self.assertRaises(FormatError):
            prepare(self.pack)
        self.assertFalse((self.pack / "ui").exists())
        self.assertEqual(list(self.pack.glob(".mcc-menu-resources-*")), [])

    def test_owned_full_mips_fonts_and_missing_roles_without_sampling_activation(self):
        report = prepare(self.pack, self.inventory)
        self.assertEqual((self.pack / report["pages"][0]["path"]).read_bytes(), self.dds)
        self.assertEqual(report["pages"][0]["source_record_hex"], self.frame["record_hex"])
        self.assertIn("halo2/h2_fonts/CaseSensitiveFont", report["fonts"])
        self.assertEqual(report["unavailable_in_mcc"], self.inventory["unavailable_in_mcc"])
        self.assertFalse(report["runtime_activation"])
        self.assertFalse(report["external_vista_or_xbox_required"])
        self.assertFalse(report["sampling_pack"])
        self.assertFalse((self.pack / "ui/pack.tsv").exists())
        self.assertFalse((self.pack / "ui/menu-ui-source.tsv").exists())

    def test_conflicting_source_page_hash_publishes_nothing(self):
        self.inventory["pages"][0]["dds_sha256"] = "0" * 64
        self.save()
        self.rejected()

    def test_changed_source_pixels_and_changed_font_publish_nothing(self):
        (self.pack / "bitmap-resources/one.bin").write_bytes(b"x" * 20)
        self.rejected()
        (self.pack / "bitmap-resources/one.bin").write_bytes(self.pixels)
        (self.pack / "halo2/h2_fonts/CaseSensitiveFont").write_bytes(b"changed-font!")
        self.rejected()

    def test_stale_fullsalt_missing_frame_and_split_storage_reject(self):
        self.inventory["pages"][0]["datum"] = "e1750000"
        self.save()
        self.rejected()
        self.inventory["pages"][0]["datum"] = "e1740000"
        self.inventory["pages"][0]["frame"] = 1
        self.save()
        self.rejected()
        self.inventory["pages"][0]["frame"] = 0
        self.frame["partitions"][1] = "one"
        self.save()
        self.rejected()

    def test_foreign_and_internal_symlink_resource_parents_reject(self):
        link = self.pack / "borrowed"
        link.symlink_to(self.pack / "bitmap-resources", target_is_directory=True)
        self.bitmaps["resources"]["one"]["path"] = "borrowed/one.bin"
        self.save()
        self.rejected()
        self.bitmaps["resources"]["one"]["path"] = "../foreign.bin"
        self.save()
        self.rejected()

    def test_existing_ui_is_never_replaced(self):
        (self.pack / "ui").mkdir()
        sentinel = self.pack / "ui/custom.txt"
        sentinel.write_bytes(b"retained-custom-ui")
        with self.assertRaisesRegex(FormatError, "retained"):
            prepare(self.pack)
        self.assertEqual(sentinel.read_bytes(), b"retained-custom-ui")


if __name__ == "__main__":
    unittest.main()
