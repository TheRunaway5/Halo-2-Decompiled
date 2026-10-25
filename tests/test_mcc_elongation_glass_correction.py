"""V5 must reject changed pixel/metadata owners and preserve COW sources."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from audit_rendering import FormatError
import mcc_bitmap_correction_bundle_v5 as bundle
from mcc_bitmap_correction_bundle import mip_owners
from mcc_onboarding import _needs_graphics_reimport
import mcc_bitmap_correction_bundle as old
import mcc_bitmap_correction_bundle_v3 as v3
import mcc_bitmap_correction_bundle_v4 as v4
from prepare_mcc_elongation_glass import atomic_json, bitmap, unshare


class GlassCorrectionTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.path = self.root / bundle.BUNDLE_V5
        self.path.parent.mkdir(parents=True)
        shutil.copyfile(ROOT / bundle.BUNDLE_V5, self.path)
        shutil.copyfile((ROOT / bundle.BUNDLE_V5).with_suffix(""), self.path.with_suffix(""))

    def change(self, function):
        path = self.path.with_suffix("")
        value = json.loads(path.read_text())
        function(value)
        path.write_text(json.dumps(value))

    def test_shipped_delta_owns_all_nine_mips(self):
        _, manifest, packed = bundle.read_v5(ROOT)
        self.assertEqual(manifest["profiles"][0]["mips"], mip_owners(bundle.PROFILE_V5))
        self.assertEqual(sum(m["size"] for m in manifest["profiles"][0]["mips"]), 349524)
        self.assertEqual(len(packed), 1123)
        self.assertEqual(hashlib.sha256(packed).hexdigest(), manifest["delta_sha256"])

    def test_changed_donor_or_missing_tuple_cannot_reconstruct(self):
        before = self.path.read_bytes()
        for donor in ({}, {bundle.PROFILE_V5["tag"]: bytes(349524)},
                      {bundle.PROFILE_V5["tag"]: bytes(349523)},
                      {bundle.PROFILE_V5["tag"] + "_custom": bytes(349524)}):
            with self.subTest(keys=list(donor)), self.assertRaises(FormatError):
                bundle.reconstruct_v5(self.root, donor)
        self.assertEqual(before, self.path.read_bytes())

    def test_changed_target_identity_cannot_be_admitted(self):
        self.change(lambda value: value["profiles"][0]["profile"].update(target_sha256="0" * 64))
        with self.assertRaisesRegex(FormatError, "ownership"):
            bundle.read_v5(self.root)

    def test_source_identity_and_mip_ownership_are_required(self):
        self.change(lambda value: value.update(source_map_sha256="0" * 64))
        with self.assertRaisesRegex(FormatError, "identity"):
            bundle.read_v5(self.root)
        shutil.copyfile((ROOT / bundle.BUNDLE_V5).with_suffix(""), self.path.with_suffix(""))
        self.change(lambda value: value["profiles"][0]["mips"][1].update(offset=0))
        with self.assertRaisesRegex(FormatError, "ownership"):
            bundle.read_v5(self.root)

    def test_delta_fingerprint_and_stream_extent_are_required(self):
        raw = self.path.read_bytes()
        self.path.write_bytes(bytes([raw[0] ^ 1]) + raw[1:])
        with self.assertRaisesRegex(FormatError, "fingerprint"):
            bundle.read_v5(self.root)
        self.path.write_bytes(raw + b"extra")
        with self.assertRaisesRegex(FormatError, "ownership"):
            bundle.read_v5(self.root)

    def test_creation_rejects_changed_inputs_without_output(self):
        output = self.root / "rejected.json.zlib"
        with self.assertRaisesRegex(FormatError, "images"):
            bundle.create_v5({bundle.PROFILE_V5["tag"]: bytes(349524)},
                             {bundle.PROFILE_V5["tag"]: bytes(349524)}, output)
        self.assertFalse(output.exists())
        self.assertFalse(output.with_suffix("").exists())

    def test_cow_detaches_inode_and_metadata_replacements_preserve_source(self):
        source = self.root / "source"
        target = self.root / "target"
        source.write_bytes(bytes(range(256)) * 128)
        before = source.read_bytes()
        os.link(source, target)
        self.assertEqual(source.stat().st_ino, target.stat().st_ino)
        try:
            unshare(target)
        except subprocess.CalledProcessError as error:
            if "Operation not supported" in error.stderr:
                self.skipTest("This temporary filesystem cannot clone COW extents")
            raise
        self.assertNotEqual(source.stat().st_ino, target.stat().st_ino)
        with target.open("ab") as stream:
            stream.write(b"new pixel partition")
        self.assertEqual(source.read_bytes(), before)
        metadata = self.root / "metadata"
        os.link(source, metadata)
        atomic_json(metadata, {"new": "provenance"})
        self.assertEqual(source.read_bytes(), before)

    def test_setup_requires_the_complete_v5_profile(self):
        for path in (old.BUNDLE_V2, v3.BUNDLE_V3, bundle.BUNDLE_V5):
            target = self.root / path
            target.parent.mkdir(parents=True, exist_ok=True)
            target.touch()
        profiles = [dict(map=name, tag=p["tag"], restored_mips=p["mip_count"],
                         format=p["format"], target_sha256=p["target_sha256"])
                    for name, values in (("zanzibar.map", old.PROFILES_V2),
                                         (v3.MAP_NAME, v3.PROFILES_V3)) for p in values]
        previous = dict(schema="H2_MCC_APPLIED_GRAPHICS_CORRECTIONS_V3", complete=True,
                        profiles=profiles, restored_mips=40)
        self.assertTrue(_needs_graphics_reimport(self.root, {"graphics_corrections": previous}))
        current = dict(previous, schema="H2_MCC_APPLIED_GRAPHICS_CORRECTIONS_V5",
                       profiles=profiles + [dict(map=bundle.MAP_NAME, tag=bundle.PROFILE_V5["tag"],
                           restored_mips=9, format=11, target_sha256=bundle.PROFILE_V5["target_sha256"])],
                       restored_mips=49)
        self.assertFalse(_needs_graphics_reimport(self.root, {"graphics_corrections": current}))
        current["profiles"] = current["profiles"][:-1]
        self.assertTrue(_needs_graphics_reimport(self.root, {"graphics_corrections": current}))

    def test_descriptor_changes_are_rejected_before_pixel_access(self):
        class Cache:
            tags = {1: dict(group="bitm", name=bundle.PROFILE_V5["tag"], address=1, offset=0)}
            def __init__(self):
                self.data = bytearray(512)
                struct.pack_into("<HHBBHH", self.data, 128 + 4, 256, 256, 1, 0, 0, 11)
                struct.pack_into("<hh", self.data, 128 + 16, 128, 128)
                struct.pack_into("<H", self.data, 128 + 20, 8)
            def block(self, *_):
                return [128]
            def u16(self, at):
                return struct.unpack_from("<H", self.data, at)[0]
            def u32(self, at):
                return struct.unpack_from("<I", self.data, at)[0]
        cache = Cache()
        self.assertEqual(bitmap(cache), 128)
        for field, value in ((4, 128), (12, 16), (20, 7), (84, 1)):
            other = Cache()
            struct.pack_into("<H" if field != 84 else "<I", other.data, 128 + field, value)
            with self.subTest(field=field), self.assertRaisesRegex(FormatError, "descriptor"):
                bitmap(other)

    def test_complete_v4_v5_cohort_reuses_and_missing_gradient_reimports(self):
        for path in (old.BUNDLE_V2, v3.BUNDLE_V3, v4.BUNDLE_V4):
            target = self.root / path
            target.parent.mkdir(parents=True, exist_ok=True)
            target.touch()
        profiles = [dict(map=name, tag=p["tag"], restored_mips=p["mip_count"],
                         format=p["format"], target_sha256=p["target_sha256"])
                    for name, values in (("zanzibar.map", old.PROFILES_V2),
                        (v3.MAP_NAME, v3.PROFILES_V3), (v4.MAP_NAME, v4.PROFILES_V4),
                        (bundle.MAP_NAME, bundle.PROFILES_V5)) for p in values]
        current = dict(schema="H2_MCC_APPLIED_GRAPHICS_CORRECTIONS_V5", complete=True,
                       restored_mips=57, profiles=profiles)
        self.assertFalse(_needs_graphics_reimport(self.root, {"graphics_corrections": current}))
        without_gradient = dict(current, restored_mips=49,
                                profiles=[p for p in profiles if p["map"] != v4.MAP_NAME])
        self.assertTrue(_needs_graphics_reimport(self.root, {"graphics_corrections": without_gradient}))


if __name__ == "__main__":
    unittest.main()
