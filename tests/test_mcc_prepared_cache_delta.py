"""Real filesystem transaction/ownership controls for prepared-cache deltas.

Profile semantics are independently exercised by the catalog/instant-charge
tests. These cases exercise actual FICLONE and atomic publication, without
retail assets, runtime execution, or a full import/extraction.
"""
from pathlib import Path
import errno
import json
import os
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import mcc_prepared_cache_delta as delta
from audit_rendering import FormatError, file_sha


class DerivativeTransactions(unittest.TestCase):
    def setUp(self):
        scratch = os.environ.get("HALO2_TEST_SCRATCH")
        self.temp = tempfile.TemporaryDirectory(prefix="mcc-delta-tests-", dir=scratch)
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.source = self.root / "original prepared cache"
        self.source.mkdir()
        (self.source / "immutable").mkdir()
        (self.source / "empty original folder").mkdir()
        (self.source / "immutable/pixels.bin").write_bytes(bytes(range(256)) * 16)
        (self.source / "changed.map").write_bytes(b"MCC fixture before")
        self.destination = self.root / "new derivative"
        self.before = {p.relative_to(self.source).as_posix(): (file_sha(p), p.stat().st_ino)
                       for p in self.source.rglob("*") if p.is_file()}

    def stages(self):
        return list(self.root.glob(".mcc-delta-*"))

    def assert_source_retained(self):
        self.assertEqual({p.relative_to(self.source).as_posix(): (file_sha(p), p.stat().st_ino)
                          for p in self.source.rglob("*") if p.is_file()}, self.before)

    def test_real_reflink_private_write_and_unchanged_resource_retention(self):
        def convert(stage, source):
            self.assertEqual(source, self.source)
            self.assertEqual((stage / "changed.map").read_bytes(), b"MCC fixture before")
            (stage / "changed.map").write_bytes(b"MCC fixture after")
            return {"metadata_store_blocks": 1}
        report = delta._transaction(self.source, self.destination, convert)
        self.assertTrue(report["original_cache_unchanged"])
        self.assertEqual(report["clone"]["files"], 2)
        self.assertFalse(report["clone"]["hardlinks"])
        self.assertTrue((self.destination / "empty original folder").is_dir())
        for name, (sha, inode) in self.before.items():
            clone = self.destination / name
            self.assertNotEqual(clone.stat().st_ino, inode)
            self.assertEqual(clone.stat().st_nlink, 1)
            self.assertFalse(clone.is_symlink())
            if name.startswith("immutable/"):
                self.assertEqual(file_sha(clone), sha)
        self.assertEqual((self.destination / "changed.map").read_bytes(), b"MCC fixture after")
        self.assert_source_retained()
        self.assertEqual(self.stages(), [])

    def test_converter_failure_removes_unpublished_partial_clone(self):
        def fail(stage, source):
            (stage / "changed.map").write_bytes(b"partial conversion")
            raise FormatError("deliberate converter rejection")
        with self.assertRaisesRegex(FormatError, "deliberate converter rejection"):
            delta._transaction(self.source, self.destination, fail)
        self.assertFalse(self.destination.exists())
        self.assertEqual(self.stages(), [])
        self.assert_source_retained()

    def test_existing_destination_is_never_replaced_including_empty_directory(self):
        self.destination.mkdir()
        inode = self.destination.stat().st_ino
        with self.assertRaisesRegex(FormatError, "destination already exists"):
            delta._transaction(self.source, self.destination, lambda *_: {})
        self.assertEqual(self.destination.stat().st_ino, inode)
        self.assertEqual(self.stages(), [])
        self.assert_source_retained()

    def test_late_destination_creation_cannot_be_overwritten_by_atomic_rename(self):
        def race(stage, source):
            self.destination.mkdir()
            (self.destination / "concurrent unrelated work.txt").write_text("keep")
            return {}
        with self.assertRaises(FileExistsError):
            delta._transaction(self.source, self.destination, race)
        self.assertEqual((self.destination / "concurrent unrelated work.txt").read_text(), "keep")
        self.assertEqual(self.stages(), [])
        self.assert_source_retained()

    def test_late_source_change_refuses_publication(self):
        def change_source(stage, source):
            (source / "changed.map").write_bytes(b"external concurrent update")
            return {}
        with self.assertRaisesRegex(FormatError, "original prepared cache changed"):
            delta._transaction(self.source, self.destination, change_source)
        self.assertFalse(self.destination.exists())
        self.assertEqual((self.source / "changed.map").read_bytes(), b"external concurrent update")
        self.assertEqual(self.stages(), [])

    def test_symbolic_source_file_and_symbolic_ancestor_reject_before_clone(self):
        (self.source / "alias.bin").symlink_to(self.source / "immutable/pixels.bin")
        with self.assertRaisesRegex(FormatError, "symbolic cache entry"):
            delta._transaction(self.source, self.destination, lambda *_: {})
        (self.source / "alias.bin").unlink()
        alias = self.root / "aliased original"
        alias.symlink_to(self.source, target_is_directory=True)
        with self.assertRaisesRegex(FormatError, "directory is absent or symbolic"):
            delta._transaction(alias, self.destination, lambda *_: {})
        self.assertFalse(self.destination.exists())
        self.assertEqual(self.stages(), [])
        self.assert_source_retained()

    def test_existing_hardlink_source_owner_is_rejected(self):
        other = self.root / "external shared owner"
        os.link(self.source / "changed.map", other)
        with self.assertRaisesRegex(FormatError, "shared hardlink owner"):
            delta._transaction(self.source, self.destination, lambda *_: {})
        self.assertEqual(other.read_bytes(), b"MCC fixture before")
        self.assertFalse(self.destination.exists())
        self.assertEqual(self.stages(), [])
        self.assert_source_retained()

    def test_converter_cannot_publish_symbolic_or_hardlinked_resources(self):
        for kind in ("symbolic", "hardlink"):
            with self.subTest(kind=kind):
                def link(stage, source):
                    target = stage / "injected owner"
                    if kind == "symbolic":
                        target.symlink_to(source / "changed.map")
                    else:
                        os.link(stage / "changed.map", target)
                    return {}
                with self.assertRaises(FormatError):
                    delta._transaction(self.source, self.destination, link)
                self.assertFalse(self.destination.exists())
                self.assertEqual(self.stages(), [])
                self.assert_source_retained()

    def test_reflink_failure_has_no_copy_or_hardlink_fallback(self):
        real = delta.fcntl.ioctl
        calls = []
        def fail_second(*args):
            calls.append(args)
            if len(calls) == 2:
                raise OSError(errno.EOPNOTSUPP, "fixture filesystem refuses reflink")
            return real(*args)
        with patch.object(delta.fcntl, "ioctl", side_effect=fail_second):
            with self.assertRaisesRegex(OSError, "refuses reflink"):
                delta._transaction(self.source, self.destination, lambda *_: {})
        self.assertEqual(len(calls), 2)
        self.assertFalse(self.destination.exists())
        self.assertEqual(self.stages(), [])
        self.assert_source_retained()

    def test_destination_inside_original_cache_is_rejected(self):
        with self.assertRaisesRegex(FormatError, "source and destination overlap"):
            delta._transaction(self.source, self.source / "nested derivative", lambda *_: {})
        self.assertEqual(self.stages(), [])
        self.assert_source_retained()

    def test_mismatched_prepared_map_manifest_leaves_no_derivative(self):
        maps = self.source / delta.MAPS
        maps.mkdir(parents=True)
        (maps / "lockout.map").write_bytes(b"not the authenticated prepared map")
        (self.source / "provenance.json").write_text(json.dumps({"schema": delta.PROFILE}))
        (self.source / "tag-conversion.json").write_text(json.dumps({
            "profile": delta.PROFILE,
            "maps": [{"map": "lockout.map", "output_sha256": "0" * 64}]}))
        before = delta._snapshot(self.source)
        with self.assertRaisesRegex(FormatError, "prepared map output hash differs"):
            delta.prepare_derivative(self.source, self.destination, progress=lambda _: None)
        self.assertEqual(delta._snapshot(self.source), before)
        self.assertFalse(self.destination.exists())
        self.assertEqual(self.stages(), [])


class InheritedMenuOwnership(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(dir=os.environ.get("HALO2_TEST_SCRATCH"))
        self.addCleanup(self.temp.cleanup)
        self.pack = Path(self.temp.name)
        self.pixels = b"retained copied pixel bytes"
        self.pixel_sha = delta.digest(self.pixels)
        pixel_path = "bitmap-resources/" + self.pixel_sha + ".bin"
        (self.pack / "bitmap-resources").mkdir()
        (self.pack / pixel_path).write_bytes(self.pixels)
        dds = b"retained already-qualified DDS"
        self.dds_sha = delta.digest(dds)
        dds_path = "ui/bitmaps/" + self.dds_sha + ".dds"
        (self.pack / "ui/bitmaps").mkdir(parents=True)
        (self.pack / dds_path).write_bytes(dds)
        (self.pack / "halo2/h2_fonts").mkdir(parents=True)
        font = self.pack / "halo2/h2_fonts/font_table.txt"
        font.write_bytes(b"font table fixture")
        self.page = dict(cache="mainmenu.map", name=r"ui\example", datum="12340000",
                         frame=0, dds_sha256=self.dds_sha)
        self.bitmap = dict(resources={"copied-key": dict(path=pixel_path,
                          sha256=self.pixel_sha, decoded_bytes=len(self.pixels))})
        fonts = {"h2_fonts/font_table.txt": dict(bytes=font.stat().st_size, sha256=file_sha(font))}
        self.inventory = dict(pages=[self.page], fonts=fonts)
        self.assets = dict(pages=[dict(self.page, path=dds_path, bytes=len(dds),
                           source_record_hex="00", source_partition="copied-key")],
                           fonts={"halo2/" + k: v for k, v in fonts.items()})

    def check(self):
        delta._validate_inherited_resources(self.pack, self.bitmap, self.inventory, self.assets)

    def test_complete_unchanged_resources_are_reused_without_decoding(self):
        with patch.object(delta, "dds_image", side_effect=AssertionError("must not rebuild inherited DDS")):
            self.check()

    def test_missing_unchanged_dds_rejects_even_when_both_changed_pages_could_exist(self):
        (self.pack / self.assets["pages"][0]["path"]).unlink()
        with self.assertRaisesRegex(FormatError, "private owner"):
            self.check()

    def test_truncated_resource_and_symbolic_font_reject(self):
        resource = self.pack / self.bitmap["resources"]["copied-key"]["path"]
        resource.write_bytes(b"short")
        with self.assertRaisesRegex(FormatError, "absent or truncated"):
            self.check()
        resource.write_bytes(self.pixels)
        font = self.pack / "halo2/h2_fonts/font_table.txt"
        font.unlink(); font.symlink_to(resource)
        with self.assertRaisesRegex(FormatError, "private owner"):
            self.check()

    def test_duplicate_partial_and_retargeted_page_owners_reject(self):
        self.assets["pages"].append(dict(self.assets["pages"][0]))
        with self.assertRaisesRegex(FormatError, "page identity differs"):
            self.check()
        self.assets["pages"].pop()
        self.assets["pages"][0]["datum"] = "56780000"
        with self.assertRaisesRegex(FormatError, "page identity differs"):
            self.check()
        self.assets["pages"].clear()
        with self.assertRaisesRegex(FormatError, "catalogue is partial"):
            self.check()


if __name__ == "__main__":
    unittest.main()
