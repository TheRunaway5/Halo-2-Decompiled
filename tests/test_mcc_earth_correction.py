"""V6 authenticates complete donor/target owners and composes same-map bundles."""
import hashlib
import json
from pathlib import Path
import shutil
import struct
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import mcc_bitmap_correction_bundle_v6 as bundle
import mcc_bitmap_correction_bundle as v2
import mcc_bitmap_correction_bundle_v3 as v3
import mcc_bitmap_correction_bundle_v4 as v4
import mcc_bitmap_correction_bundle_v5 as v5
import mcc_graphics_corrections as graphics
from audit_rendering import FormatError
from mcc_bitmap_correction_bundle import mip_owners
from mcc_content import MccCache, texture_resource
from mcc_onboarding import _needs_graphics_reimport
import test_mcc_bitmap_correction_bundle as cache_fixture


class EarthCorrectionTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.path = self.root / bundle.BUNDLE_V6
        self.path.parent.mkdir(parents=True)
        self.reset()

    def reset(self):
        for suffix in ("", ".zlib"):
            source = (ROOT / bundle.BUNDLE_V6).with_suffix("")
            target = self.path.with_suffix("")
            if suffix:
                source, target = source.with_suffix(".json.zlib"), target.with_suffix(".json.zlib")
            shutil.copyfile(source, target)

    def change(self, function):
        path = self.path.with_suffix("")
        value = json.loads(path.read_text())
        function(value)
        path.write_text(json.dumps(value))

    def test_shipped_delta_owns_five_actual_maps_and_all_mips(self):
        _, manifest, packed = bundle.read_v6(ROOT)
        self.assertEqual(len(manifest["owners"]), 5)
        entry = manifest["profiles"][0]
        self.assertEqual(entry["mips"], mip_owners(bundle.PROFILE_V6))
        self.assertEqual(entry["donor_mips"], mip_owners(bundle.DONOR_PROFILE))
        self.assertEqual(sum(m["size"] for m in entry["mips"]), 349520)
        self.assertEqual(sum(m["size"] for m in entry["donor_mips"]), 5592404)
        self.assertEqual(entry["mips"][-1]["width"], 4)
        self.assertEqual(len(packed), 189333)
        self.assertEqual(hashlib.sha256(packed).hexdigest(), manifest["delta_sha256"])

    def test_incomplete_donor_or_renamed_identity_rejected(self):
        for donors in ({}, {bundle.PROFILE_V6["tag"]: bytes(349520)},
                       {bundle.PROFILE_V6["tag"]: bytes(5592404)},
                       {bundle.PROFILE_V6["tag"] + "_alias": bytes(5592404)}):
            with self.subTest(keys=list(donors)), self.assertRaises(FormatError):
                bundle.reconstruct_v6(self.root, donors)

    def test_metadata_cannot_change_owner_target_or_prefix(self):
        changes = (
            lambda m: m["owners"][0].update(map="guessed.map"),
            lambda m: m["owners"][0].update(source_map_sha256="0" * 64),
            lambda m: m["owners"][0].update(original_map_sha256="0" * 64),
            lambda m: m["profiles"][0]["profile"].update(target_sha256="0" * 64),
            lambda m: m["profiles"][0]["mips"][1].update(offset=0),
            lambda m: m["profiles"][0]["donor_mips"][1].update(offset=0),
            lambda m: m["profiles"][0].update(base_offset=1),
            lambda m: m["profiles"][0].update(base_bytes=349519),
            lambda m: m["profiles"][0].update(base_sha256="0" * 64),
        )
        for change in changes:
            self.reset();self.change(change)
            with self.subTest(change=change), self.assertRaises(FormatError):
                bundle.read_v6(self.root)

    def test_delta_extent_and_fingerprint_checked(self):
        raw = self.path.read_bytes()
        for bad in (raw[:-1], raw + b"extra", bytes([raw[0] ^ 1]) + raw[1:]):
            self.path.write_bytes(bad)
            with self.subTest(size=len(bad)), self.assertRaises(FormatError):
                bundle.read_v6(self.root)

    def test_creation_rejects_unpinned_images_without_output(self):
        output = self.root / "rejected.json.zlib"
        with self.assertRaises(FormatError):
            bundle.create_v6({bundle.PROFILE_V6["tag"]: bytes(5592404)},
                {bundle.PROFILE_V6["tag"]: bytes(349520)}, output)
        self.assertFalse(output.exists())
        self.assertFalse(output.with_suffix("").exists())

    def test_complete97mips_required_by_onboarding(self):
        for path in (v2.BUNDLE_V2, v3.BUNDLE_V3, v4.BUNDLE_V4, v5.BUNDLE_V5, bundle.BUNDLE_V6):
            target = self.root / path;target.parent.mkdir(parents=True, exist_ok=True);target.touch()
        sources = [("zanzibar.map", v2.PROFILES_V2), (v3.MAP_NAME, v3.PROFILES_V3),
                   (v4.MAP_NAME, v4.PROFILES_V4), (v5.MAP_NAME, v5.PROFILES_V5)]
        sources.extend((o["map"], bundle.PROFILES_V6) for o in bundle.OWNERS_V6)
        profiles = [dict(map=name, tag=p["tag"], restored_mips=p["mip_count"],
                         format=p["format"], target_sha256=p["target_sha256"])
                    for name, values in sources for p in values]
        current = dict(schema="H2_MCC_APPLIED_GRAPHICS_CORRECTIONS_V6", complete=True,
                       restored_mips=97, profiles=profiles)
        self.assertFalse(_needs_graphics_reimport(self.root, {"graphics_corrections": current}))
        for missing in range(4, 9):
            incomplete = dict(current, restored_mips=89, profiles=profiles[:missing] + profiles[missing+1:])
            self.assertTrue(_needs_graphics_reimport(self.root, {"graphics_corrections": incomplete}))

    def test_actual_owner_salted_identity_and_resource_tuple_required(self):
        owner = bundle.OWNERS_V6[-1]
        tag = dict(datum=owner["datum"], address=owner["address"], size=248)
        class Cache:
            def u16(self, at):
                return 513
            def u32(self, at):
                return {56: 119835648, 80: 2210131}[at]
        bundle.validate_owner_v6(owner["map"], Cache(), tag, 0)
        for bad in (dict(tag, datum=tag["datum"] ^ 65536), dict(tag, address=0), dict(tag, size=249)):
            with self.assertRaises(FormatError):
                bundle.validate_owner_v6(owner["map"], Cache(), bad, 0)
        with self.assertRaises(FormatError):
            bundle.validate_owner_v6("guessed.map", Cache(), tag, 0)

    def test_two_bundles_same_map_commit_once_and_preserve_both_targets(self):
        fixture = cache_fixture.BitmapCorrectionAtomicApplicationTests("test_both_maps_and_all_mips_commit_with_only_five_resource_bindings_changed")
        fixture.setUp();self.addCleanup(fixture.doCleanups)
        first, keep = fixture.profiles
        for path in (fixture.bundle_path, fixture.bundle_path.with_suffix("")):
            path.unlink()
        with patch.object(v2, "PROFILES_V2", (keep,)):
            v2.create_v2({keep["tag"]: fixture.donors[keep["tag"]]},
                {keep["tag"]: fixture.targets[keep["tag"]]}, fixture.bundle_path)
            target = bytes(range(16))
            profile = dict(first, encoding="bc3", format=16, width=4, height=4, mip_count=1,
                pixel_bytes=16, target_sha256=hashlib.sha256(target).hexdigest(),
                donor_width=8, donor_height=8, donor_mip_count=4, donor_pixel_bytes=340,
                donor_encoding="rgba8", donor_format=11, registration=[2, 2])
            path = fixture.root / bundle.BUNDLE_V6;path.write_bytes(b"fixture-delta")
            owner = dict(map="zanzibar.map", source_map_sha256=hashlib.sha256(fixture.map_path.read_bytes()).hexdigest())
            def reconstruct(root, donors):
                self.assertEqual(donors, {first["tag"]: fixture.donors[first["tag"]]})
                return [(profile, target)], path, {"schema": "fixture"}, b"fixture-delta"
            with patch.object(bundle, "PROFILES_V6", (profile,)), patch.object(bundle, "OWNERS_V6", (owner,)), \
                    patch.object(bundle, "reconstruct_v6", side_effect=reconstruct), \
                    patch.object(bundle, "validate_owner_v6") as owner_check:
                report = graphics.apply_selected(fixture.root, fixture.pack)
            owner_check.assert_called_once()
        self.assertEqual(len(report["changes"]), 3)
        cache = MccCache(fixture.map_path)
        try:
            for index, expected in ((0, target), (1, fixture.targets[keep["tag"]])):
                at = fixture.frames[index]
                with fixture.bank_path.open("rb") as stream:
                    pixels = texture_resource(stream, cache.u32(at+56), cache.u32(at+80))
                self.assertEqual(pixels, expected)
                self.assertEqual(cache.u16(at+12), 16)
            self.assertEqual((cache.u16(fixture.frames[0]+4), cache.u16(fixture.frames[0]+20)), (4, 0))
        finally:
            cache.close()


if __name__ == "__main__":
    unittest.main()
