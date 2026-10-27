"""Complete donor, target and local-owner guards for original decal mip inputs."""
import json
import os
from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from audit_rendering import FormatError
from mcc_bitmap_correction_bundle import digest, mip_hashes
from mcc_content import MccCache, texture_resource
from mcc_decal_bitmap_corrections import (BUNDLE, MANIFEST_SHA256, read_bundle,
    profiles_for_map, reconstruct, validate_owner)


class DecalBitmapBundleTests(unittest.TestCase):
    def test_complete_manifest_pins_four_profiles_and_all_local_copies(self):
        path, manifest, packed = read_bundle(ROOT)
        self.assertEqual(digest(path.with_suffix("").read_bytes()), MANIFEST_SHA256)
        self.assertEqual([e["profile_id"] for e in manifest["profiles"]], ["ice", "impact_scratch", "metal", "stone"])
        self.assertEqual(len(manifest["owners"]), 63)
        self.assertEqual(sum(e["delta_bytes"] for e in manifest["profiles"]), len(packed))
        self.assertFalse(manifest["provenance"]["original_runtime_input"])
        self.assertFalse(manifest["provenance"]["prepared_assets_modified"])
        shared = profiles_for_map(ROOT, "shared.map")
        self.assertEqual(len(shared), 4)
        self.assertEqual(profiles_for_map(ROOT, "foreign.map"), [])

    def test_manifest_or_delta_mutation_fails_before_reconstruction(self):
        original = ROOT / BUNDLE
        for field in ("manifest", "delta"):
            with self.subTest(field=field), tempfile.TemporaryDirectory() as temporary:
                target = Path(temporary) / BUNDLE
                target.parent.mkdir(parents=True)
                raw, packed = original.with_suffix("").read_bytes(), original.read_bytes()
                if field == "manifest":
                    value = json.loads(raw)
                    value["owners"][0]["descriptor_sha256"] = "0" * 64
                    raw = (json.dumps(value, indent=2) + "\n").encode()
                else:
                    packed = packed[:-1] + bytes([packed[-1] ^ 1])
                target.write_bytes(packed)
                target.with_suffix("").write_bytes(raw)
                with self.assertRaisesRegex(FormatError, "fingerprint"):
                    read_bundle(Path(temporary))

    def test_empty_foreign_and_unowned_map_donors_are_rejected(self):
        for donors, name in (({}, None), ({"untrusted": b"unknown"}, None), ({}, "foreign.map")):
            with self.subTest(map=name), self.assertRaisesRegex(FormatError, "donor set"):
                reconstruct(ROOT, donors, map_name=name)

    def test_preparation_freshness_keeps_the_independent_component_receipt(self):
        from mcc_bitmap_correction_bundle import BUNDLE_V2, PROFILES_V2
        from mcc_bitmap_correction_bundle_v3 import BUNDLE_V3, MAP_NAME, PROFILES_V3
        from mcc_onboarding import _needs_graphics_reimport
        _, manifest, _ = read_bundle(ROOT)
        legacy = [dict(map=name, tag=p["tag"], restored_mips=p["mip_count"], format=p["format"], target_sha256=p["target_sha256"])
                  for name, profiles in (("zanzibar.map", PROFILES_V2), (MAP_NAME, PROFILES_V3)) for p in profiles]
        graphics = dict(schema="H2_MCC_APPLIED_GRAPHICS_CORRECTIONS_V3", complete=True,
            restored_mips=sum(p["restored_mips"] for p in legacy), profiles=legacy)
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            for bundle in (BUNDLE_V2, BUNDLE_V3):
                path = root / bundle
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(b"installed legacy bundle sentinel")
            target = root / BUNDLE
            target.write_bytes((ROOT / BUNDLE).read_bytes())
            target.with_suffix("").write_bytes((ROOT / BUNDLE).with_suffix("").read_bytes())
            self.assertTrue(_needs_graphics_reimport(root, {"graphics_corrections": graphics}))
            rows = [dict(map=name, tag=p["tag"], restored_mips=p["mip_count"], format=p["format"], target_sha256=p["target_sha256"])
                    for name in dict.fromkeys(o["map"] for o in manifest["owners"])
                    for p in profiles_for_map(ROOT, name)]
            graphics["decal_bitmap_corrections"] = dict(schema=manifest["schema"], manifest_sha256=MANIFEST_SHA256,
                profiles=rows, restored_mips=sum(p["restored_mips"] for p in rows))
            self.assertFalse(_needs_graphics_reimport(root, {"graphics_corrections": graphics}))
            rows[0]["target_sha256"] = "0" * 64
            self.assertTrue(_needs_graphics_reimport(root, {"graphics_corrections": graphics}))
            # A partial older portable tool set preserves its verified legacy
            # receipt instead of dropping a component it cannot reconstruct.
            target.unlink()
            target.with_suffix("").unlink()
            self.assertFalse(_needs_graphics_reimport(root, {"graphics_corrections": graphics}))


class ActualDecalBitmapDonorTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source = Path(os.environ.get("HALO2_DECAL_BITMAP_DONOR_ROOT",
            ROOT / "assets/imported/mcc-sky-water-tu15-20261024-v3"))
        if not (cls.source / "provenance.json").is_file():
            raise unittest.SkipTest("Authenticated prepared MCC decal donors unavailable")
        _, cls.manifest, _ = read_bundle(ROOT)
        cls.maps = cls.source / "halo2/h2_maps_win64_dx11"

    def test_every_actual_local_owner_reconstructs_exact_original_mips(self):
        profiles = {e["profile_id"]: e for e in self.manifest["profiles"]}
        seen, count = set(), 0
        by_map = {}
        for owner in self.manifest["owners"]:
            by_map.setdefault(owner["map"], []).append(owner)
        for name, owners in by_map.items():
            cache = MccCache(self.maps / name)
            try:
                donors = {}
                with (self.maps / "textures.dat").open("rb") as stream:
                    for owner in owners:
                        tag = cache.tags[owner["datum"]]
                        rows = cache.block(tag["offset"] + 68, 168)
                        self.assertEqual(len(rows), 1)
                        at = rows[0]
                        validate_owner(ROOT, name, cache, tag, at)
                        pixels = bytearray(texture_resource(stream, cache.u32(at + 56), cache.u32(at + 80)))
                        pixels[0::4], pixels[2::4] = pixels[2::4], pixels[0::4]
                        donors[tag["name"]] = pixels
                        count += 1
                images, _, _, _ = reconstruct(ROOT, donors, map_name=name)
                self.assertEqual(len(images), len(owners))
                for profile, target in images:
                    entry = next(e for e in profiles.values() if e["profile"]["tag"] == profile["tag"])
                    self.assertEqual(digest(target), profile["target_sha256"])
                    self.assertEqual(mip_hashes(target, profile), entry["target_mip_sha256"])
                    seen.add(entry["profile_id"])
            finally:
                cache.close()
        self.assertEqual(count, 63)
        self.assertEqual(seen, {"ice", "impact_scratch", "metal", "stone"})

    def test_changed_donor_last_mip_is_rejected(self):
        cache = MccCache(self.maps / "shared.map")
        try:
            donors = {}
            with (self.maps / "textures.dat").open("rb") as stream:
                for owner in self.manifest["owners"]:
                    if owner["map"] != "shared.map":
                        continue
                    tag = cache.tags[owner["datum"]]
                    at = cache.block(tag["offset"] + 68, 168)[0]
                    pixels = bytearray(texture_resource(stream, cache.u32(at + 56), cache.u32(at + 80)))
                    pixels[0::4], pixels[2::4] = pixels[2::4], pixels[0::4]
                    donors[tag["name"]] = pixels
            for name in donors:
                with self.subTest(bitmap=name):
                    donors[name][-1] ^= 1
                    try:
                        with self.assertRaisesRegex(FormatError, "complete donor mips"):
                            reconstruct(ROOT, donors, map_name="shared.map")
                    finally:
                        donors[name][-1] ^= 1
        finally:
            cache.close()

    def test_wrong_identity_and_any_descriptor_byte_fail_without_asset_writes(self):
        path = self.maps / "shared.map"
        before = path.stat().st_size, path.stat().st_mtime_ns
        cache = MccCache(path)
        try:
            owner = next(o for o in self.manifest["owners"] if o["map"] == "shared.map")
            tag = cache.tags[owner["datum"]]
            at = cache.block(tag["offset"] + 68, 168)[0]
            for field in ("datum", "address", "size"):
                changed = dict(tag)
                changed[field] ^= 1
                with self.subTest(field=field), self.assertRaisesRegex(FormatError, "complete local owner"):
                    validate_owner(ROOT, "shared.map", cache, changed, at)
            for offset in (0, 8, 14, 56, 80, 167):
                with self.subTest(descriptor_byte=offset):
                    cache.data[at + offset] ^= 1
                    try:
                        with self.assertRaisesRegex(FormatError, "complete local owner"):
                            validate_owner(ROOT, "shared.map", cache, tag, at)
                    finally:
                        cache.data[at + offset] ^= 1
            with self.assertRaisesRegex(FormatError, "unique authenticated owner"):
                validate_owner(ROOT, "foreign.map", cache, tag, at)
        finally:
            cache.close()
        self.assertEqual(before, (path.stat().st_size, path.stat().st_mtime_ns))


if __name__ == "__main__":
    unittest.main()
