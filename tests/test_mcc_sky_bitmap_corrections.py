"""Whole-donor and whole-owner guards for exact native sky texture storage."""
import json
import os
from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from audit_rendering import FormatError
from mcc_content import MccCache
from mcc_bitmap_correction_bundle import digest
from mcc_bitmap_correction_bundle_v7 import (BUNDLE_V7, read_v7, reconstruct_v7,
    validate_owner_v7, hashes)
from prepare_mcc_elongation_glass import bitmap, rgba


class NativeSkyBitmapManifestTests(unittest.TestCase):
    def test_shipped_native_chains_and_every_local_owner_are_pinned(self):
        _, manifest, packed = read_v7(ROOT)
        self.assertEqual(len(manifest["owners"]), 11)
        self.assertEqual(len({(o["map"], o["tag"], o["frame"]) for o in manifest["owners"]}), 11)
        self.assertEqual([e["profile"]["format"] for e in manifest["profiles"]], [16, 16, 14, 16, 16])
        self.assertEqual([e["profile"]["mip_count"] for e in manifest["profiles"]], [6, 8, 1, 1, 8])
        self.assertEqual(sum(e["delta_bytes"] for e in manifest["profiles"]), len(packed))
        self.assertEqual(sum(next(e["profile"]["mip_count"] for e in manifest["profiles"]
                                  if e["profile_id"] == o["profile_id"]) for o in manifest["owners"]), 35)

    def test_complete_manifest_and_delta_reject_mutation(self):
        original = ROOT / BUNDLE_V7
        for field in ("manifest", "delta"):
            with self.subTest(field=field), tempfile.TemporaryDirectory() as temp:
                target = Path(temp) / BUNDLE_V7
                target.parent.mkdir(parents=True)
                raw, packed = original.with_suffix("").read_bytes(), original.read_bytes()
                if field == "manifest":
                    metadata = json.loads(raw)
                    metadata["owners"][0]["descriptor_sha256"] = "0" * 64
                    raw = (json.dumps(metadata, indent=2) + "\n").encode()
                else:
                    packed = packed[:-1] + bytes([packed[-1] ^ 1])
                target.write_bytes(packed)
                target.with_suffix("").write_bytes(raw)
                with self.assertRaisesRegex(FormatError, "fingerprint"):
                    read_v7(Path(temp))

    def test_empty_and_foreign_donor_sets_fail_before_reconstruction(self):
        for donors in ({}, {"unknown": b"untrusted"}):
            with self.subTest(donors=list(donors)), self.assertRaisesRegex(FormatError, "donor set"):
                reconstruct_v7(ROOT, donors)


class ActualNativeSkyBitmapTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source = Path(os.environ.get("HALO2_SKY_BITMAP_DONOR_ROOT",
            ROOT / "assets/imported/mcc-sky-water-tu15-20261024-v3"))
        if not (cls.source / "provenance.json").is_file():
            raise unittest.SkipTest("Authenticated prepared V3 donor cache unavailable")
        _, cls.manifest, _ = read_v7(ROOT)

    def test_actual_complete_mcc_donors_reconstruct_every_original_mip(self):
        seen = set()
        for owner in self.manifest["owners"]:
            entry = next(e for e in self.manifest["profiles"] if e["profile_id"] == owner["profile_id"])
            profile = entry["profile"]
            cache = MccCache(self.source / "halo2/h2_maps_win64_dx11" / owner["map"])
            try:
                at = bitmap(cache, profile)
                validate_owner_v7(ROOT, owner["map"], cache, cache.tags[owner["datum"]], at)
                donor = rgba(cache, self.source / "halo2/h2_maps_win64_dx11/textures.dat", profile)
                images, _, _, _ = reconstruct_v7(ROOT, {profile["tag"]: donor}, map_name=owner["map"])
                self.assertEqual(len(images), 1)
                self.assertEqual(digest(images[0][1]), profile["target_sha256"])
                self.assertEqual(hashes(images[0][1], profile), entry["target_mip_sha256"])
                if owner["profile_id"] not in seen:
                    # A changed donor suffix outside the XOR prefix must fail.
                    # This catches a partial-donor implementation that otherwise
                    # reconstructs the same target from a corrupted resource.
                    changed = bytearray(donor)
                    changed[-1] ^= 1
                    with self.assertRaisesRegex(FormatError, "complete donor mips"):
                        reconstruct_v7(ROOT, {profile["tag"]: changed}, map_name=owner["map"])
                    seen.add(owner["profile_id"])
            finally:
                cache.close()
        self.assertEqual(len(seen), 5)

    def test_actual_owner_and_every_descriptor_byte_are_required(self):
        owner = self.manifest["owners"][0]
        profile = next(e["profile"] for e in self.manifest["profiles"] if e["profile_id"] == owner["profile_id"])
        path = self.source / "halo2/h2_maps_win64_dx11" / owner["map"]
        before = path.stat().st_mtime_ns, path.stat().st_size
        cache = MccCache(path)
        try:
            at = bitmap(cache, profile)
            tag = cache.tags[owner["datum"]]
            for delta in (0, 14, 56, 80, 167):
                with self.subTest(descriptor_byte=delta):
                    cache.data[at + delta] ^= 1
                    try:
                        with self.assertRaisesRegex(FormatError, "complete local owner"):
                            validate_owner_v7(ROOT, owner["map"], cache, tag, at)
                    finally:
                        cache.data[at + delta] ^= 1
            with self.assertRaisesRegex(FormatError, "unique authenticated owner"):
                validate_owner_v7(ROOT, "foreign.map", cache, tag, at)
        finally:
            cache.close()
        self.assertEqual(before, (path.stat().st_mtime_ns, path.stat().st_size))


if __name__ == "__main__":
    unittest.main()
