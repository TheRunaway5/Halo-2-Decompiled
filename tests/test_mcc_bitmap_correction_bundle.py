"""Pixel correction admission must fail before a private MCC bank is changed."""
import hashlib
import json
from pathlib import Path
import sys
import unittest
import zlib
import struct
import tempfile
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from audit_rendering import FormatError
from mcc_bitmap_correction_bundle import BUNDLE, decode_delta, validate_profile
import mcc_bitmap_correction_bundle as bundle
import mcc_bitmap_correction_bundle_v3 as bundle_v3
import mcc_graphics_corrections as graphics
from mcc_content import MccCache, texture_resource


def sha(value):
    return hashlib.sha256(value).hexdigest()


class BitmapCorrectionBundleTests(unittest.TestCase):
    def setUp(self):
        self.source = bytes(range(256)) * 5
        self.target = self.source[::-1]
        self.packed = zlib.compress(bytes(a ^ b for a, b in zip(self.source, self.target, strict=True)))

    def decode(self, source=None, packed=None, target=None):
        packed = self.packed if packed is None else packed
        return decode_delta(self.source if source is None else source, packed,
                            source_sha256=sha(self.source),
                            target_sha256=sha(self.target if target is None else target),
                            delta_sha256=sha(packed))

    def test_exact_donor_reconstructs_without_resampling(self):
        self.assertEqual(self.decode(), self.target)
        self.assertEqual(self.source, bytes(range(256)) * 5)

    def test_changed_donor_and_wrong_target_fail(self):
        with self.assertRaisesRegex(FormatError, "donor"):
            self.decode(source=self.target)
        with self.assertRaisesRegex(FormatError, "reconstructed"):
            self.decode(target=self.source)

    def test_compressed_extent_and_stream_ownership(self):
        for bad in (self.packed[:-1], self.packed + b"extra", self.packed + self.packed,
                    zlib.compress(bytes(len(self.source) - 1)),
                    zlib.compress(bytes(len(self.source) + 1)),
                    zlib.compress(bytes(len(self.source) * 100))):
            with self.subTest(bytes=len(bad)), self.assertRaises(FormatError):
                self.decode(packed=bad)

    def test_delta_fingerprint_is_checked_before_decompression(self):
        with self.assertRaisesRegex(FormatError, "fingerprint"):
            decode_delta(self.source, b"corrupted", source_sha256=sha(self.source),
                         target_sha256=sha(self.target), delta_sha256=sha(self.packed))

    def test_bundled_profile_pins_complete_mip_and_reference_identity(self):
        root = Path(__file__).resolve().parents[1]
        manifest = json.loads((root / BUNDLE).with_suffix("").read_bytes())
        profile = manifest["profile"]
        validate_profile(profile)
        for key, value in (("original_map_sha256", "0" * 64), ("rgba_sha256", "0" * 64),
                           ("mips", profile["mips"][:-1]), ("tag", profile["tag"] + "_custom")):
            with self.subTest(key=key), self.assertRaises(FormatError):
                validate_profile(dict(profile, **{key: value}))
        packed = (root / BUNDLE).read_bytes()
        self.assertEqual(len(packed), manifest["delta_bytes"])
        self.assertEqual(sha(packed), manifest["delta_sha256"])

    def test_shipped_v2_bundle_has_both_complete_pinned_formats(self):
        root = Path(__file__).resolve().parents[1]
        path, manifest, packed = bundle.read_v2(root)
        self.assertEqual(path, root / bundle.BUNDLE_V2)
        self.assertEqual(sum(p["profile"]["mip_count"] for p in manifest["profiles"]), 17)
        self.assertEqual([p["profile"]["format"] for p in manifest["profiles"]], [11, 16])
        self.assertEqual(sum(p["delta_bytes"] for p in manifest["profiles"]), len(packed))

    def test_shipped_v3_bundle_has_three_complete_metropolis_images(self):
        root = Path(__file__).resolve().parents[1]
        path, manifest, packed = bundle_v3.read_v3(root)
        self.assertEqual(path, root / bundle_v3.BUNDLE_V3)
        self.assertEqual(sum(p["profile"]["mip_count"] for p in manifest["profiles"]), 23)
        self.assertEqual([p["profile"]["format"] for p in manifest["profiles"]], [16, 11, 16])
        self.assertEqual(sum(p["delta_bytes"] for p in manifest["profiles"]), len(packed))


class BitmapCorrectionAtomicApplicationTests(unittest.TestCase):
    """Real binary-cache parser, resource decoder and save seam; small pixel fixtures."""
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.pack = self.root / "private-pack"
        maps = self.pack / "halo2/h2_maps_win64_dx11"
        maps.mkdir(parents=True)
        self.map_path, self.bank_path = maps / "zanzibar.map", maps / "textures.dat"
        self.bundle_path = self.root / bundle.BUNDLE_V2
        self.profiles, self.donors, self.targets = [], {}, {}
        for base, encoding, width, mips in ((bundle.PROFILES_V2[0], "rgba8", 8, 4),
                                             (bundle.PROFILES_V2[1], "bc3", 8, 3)):
            size = 340 if encoding == "rgba8" else 96
            donor = bytes((i * 29 + 19) % 256 for i in range(size))
            target = bytes(x ^ 0x75 for x in donor)
            profile = dict(base, width=width, height=width, mip_count=mips, pixel_bytes=size,
                           donor_sha256=sha(donor), target_sha256=sha(target))
            self.profiles.append(profile)
            self.donors[profile["tag"]], self.targets[profile["tag"]] = donor, target
        self.addCleanup(patch.stopall)
        patch.object(bundle, "PROFILES_V2", tuple(self.profiles)).start()
        data = bytearray(4096)
        def word(at, value):
            struct.pack_into("<I", data, at, value)
        for at, value in ((0, 0x68656164), (4, 13), (8, len(data)), (16, 1024),
                          (32, 2), (36, 2048), (40, 192), (44, 2240),
                          (720, 1024), (1024, 1056), (1032, 1056), (1048, 2)):
            word(at, value)
        names, bank = bytearray(), bytearray(b"padding!")
        self.frames = []
        for index, profile in enumerate(self.profiles):
            word(2240 + 4 * index, len(names))
            names.extend(profile["tag"].encode() + b"\0")
            root, frame = 2304 + index * 256, 3000 + index * 168
            self.frames.append(frame)
            struct.pack_into("<4I", data, 1056 + index * 16,
                             int.from_bytes(b"bitm", "big"), 0xe1740000 + index, root, 256)
            word(root + 68, 1)
            word(root + 72, frame)
            struct.pack_into("<HHBBHH", data, frame + 4,
                             profile["width"], profile["height"], 1, 0, 0, profile["format"])
            struct.pack_into("<H", data, frame + 20, profile["mip_count"] - 1)
            native = bytearray(self.donors[profile["tag"]])
            if profile["encoding"] == "rgba8":
                native[0::4], native[2::4] = native[2::4], native[0::4]
            resource = struct.pack("<Ii", 1, -len(native)) + native
            word(frame + 56, len(bank))
            word(frame + 80, len(resource))
            bank.extend(resource)
        data[2048:2048 + len(names)] = names
        self.map_path.write_bytes(data)
        self.bank_path.write_bytes(bank)
        patch.object(graphics, "SOURCE_MAP", sha(data)).start()
        bundle.create_v2(self.donors, self.targets, self.bundle_path)
        self.metropolis_path = maps / bundle_v3.MAP_NAME
        self.bundle_v3_path = self.root / bundle_v3.BUNDLE_V3
        self.metropolis_profiles, self.metropolis_frames = [], []
        self.metropolis_donors, self.metropolis_targets = {}, {}
        data = bytearray(8192)
        for at, value in ((0, 0x68656164), (4, 13), (8, len(data)), (16, 1024),
                          (32, 3), (36, 2048), (40, 1024), (44, 3072),
                          (720, 1024), (1024, 1056), (1032, 1056), (1048, 3)):
            word(at, value)
        names, bank = bytearray(), bytearray(self.bank_path.read_bytes())
        for index, base in enumerate(bundle_v3.PROFILES_V3):
            rgba = base["encoding"] == "rgba8"
            donor = bytes((i * 13 + index + 9) % 256 for i in range(340 if rgba else 96))
            target = bytes(x ^ (index + 0x37) for x in donor)
            profile = dict(base, width=8, height=8, mip_count=4 if rgba else 3,
                           pixel_bytes=len(donor), donor_sha256=sha(donor), target_sha256=sha(target))
            self.metropolis_profiles.append(profile)
            self.metropolis_donors[profile["tag"]] = donor
            self.metropolis_targets[profile["tag"]] = target
            word(3072 + 4 * index, len(names))
            names.extend(profile["tag"].encode() + b"\0")
            root, frame = 4096 + index * 256, 6000 + index * 168
            self.metropolis_frames.append(frame)
            struct.pack_into("<4I", data, 1056 + index * 16,
                             int.from_bytes(b"bitm", "big"), 0xe1750000 + index, root, 256)
            word(root + 68, 1)
            word(root + 72, frame)
            struct.pack_into("<HHBBHH", data, frame + 4,
                             profile["width"], profile["height"], 1, 0, 0, profile["format"])
            struct.pack_into("<H", data, frame + 20, profile["mip_count"] - 1)
            native = bytearray(donor)
            if rgba:
                native[0::4], native[2::4] = native[2::4], native[0::4]
            resource = struct.pack("<Ii", 1, -len(native)) + native
            word(frame + 56, len(bank))
            word(frame + 80, len(resource))
            bank.extend(resource)
        data[2048:2048 + len(names)] = names
        self.metropolis_path.write_bytes(data)
        self.bank_path.write_bytes(bank)
        patch.object(bundle_v3, "SOURCE_MAP", sha(data)).start()
        patch.object(bundle_v3, "PROFILES_V3", tuple(self.metropolis_profiles)).start()
        bundle_v3.create_v3(self.metropolis_donors, self.metropolis_targets, self.bundle_v3_path)

    def change_manifest(self, change):
        path = self.bundle_path.with_suffix("")
        manifest = json.loads(path.read_bytes())
        change(manifest)
        path.write_text(json.dumps(manifest))

    def assert_rejected_without_writes(self, message):
        paths = (self.map_path, self.metropolis_path, self.bank_path)
        before = [p.read_bytes() for p in paths]
        with self.assertRaisesRegex(FormatError, message):
            graphics.apply_selected(self.root, self.pack)
        self.assertEqual(before, [p.read_bytes() for p in paths])
        self.assertFalse((self.pack / "graphics-corrections").exists())
        self.assertFalse((self.pack / "graphics-conversion.json").exists())

    def test_both_maps_and_all_mips_commit_with_only_five_resource_bindings_changed(self):
        before_map, before_bank = self.map_path.read_bytes(), self.bank_path.read_bytes()
        before_metropolis = self.metropolis_path.read_bytes()
        report = graphics.apply_selected(self.root, self.pack)
        self.assertTrue(report["complete"])
        self.assertEqual(report["restored_mips"], 17)
        self.assertEqual(len(report["profiles"]), 5)
        self.assertEqual(len(report["changes"]), 3)
        self.assertEqual(self.bank_path.read_bytes()[:len(before_bank)], before_bank)
        cache = MccCache(self.map_path)
        try:
            expected = bytearray(before_map)
            # Save may rewrite compression header fields, so inspect the genuine
            # decoded metadata after the 896-byte cache header.
            for profile, at in zip(self.profiles, self.frames, strict=True):
                for offset in (56, 80):
                    expected[at + offset:at + offset + 4] = cache.data[at + offset:at + offset + 4]
                with self.bank_path.open("rb") as bank:
                    pixels = bytearray(texture_resource(bank, cache.u32(at + 56), cache.u32(at + 80)))
                if profile["encoding"] == "rgba8":
                    pixels[0::4], pixels[2::4] = pixels[2::4], pixels[0::4]
                self.assertEqual(pixels, self.targets[profile["tag"]])
                self.assertEqual(cache.u16(at + 12), profile["format"])
            self.assertEqual(cache.data[896:], expected[896:])
        finally:
            cache.close()
        cache = MccCache(self.metropolis_path)
        try:
            expected = bytearray(before_metropolis)
            for profile, at in zip(self.metropolis_profiles, self.metropolis_frames, strict=True):
                for offset in (56, 80):
                    expected[at + offset:at + offset + 4] = cache.data[at + offset:at + offset + 4]
                with self.bank_path.open("rb") as bank:
                    pixels = bytearray(texture_resource(bank, cache.u32(at + 56), cache.u32(at + 80)))
                if profile["encoding"] == "rgba8":
                    pixels[0::4], pixels[2::4] = pixels[2::4], pixels[0::4]
                self.assertEqual(pixels, self.metropolis_targets[profile["tag"]])
            self.assertEqual(cache.data[896:], expected[896:])
        finally:
            cache.close()
        self.assertFalse(list((self.pack / "graphics-corrections").glob("*.rgba")))

    def test_second_delta_failure_rejects_after_first_valid_reconstruction_without_writes(self):
        packed = bytearray(self.bundle_path.read_bytes())
        manifest = json.loads(self.bundle_path.with_suffix("").read_bytes())
        packed[manifest["profiles"][1]["delta_offset"]] ^= 1
        self.bundle_path.write_bytes(packed)
        self.change_manifest(lambda m: m.update(delta_sha256=sha(packed)))
        self.assert_rejected_without_writes("delta fingerprint")

    def test_published_receipt_uses_staged_bundle_bytes_without_reopening_inputs(self):
        from prepare_mcc_gameplay import save_changed_cache
        original = self.bundle_path.read_bytes()
        def save(cache):
            save_changed_cache(cache)
            self.bundle_path.write_bytes(b"changed after validated input snapshot")
            self.bundle_path.with_suffix("").write_text("{}")
        with patch("prepare_mcc_gameplay.save_changed_cache", side_effect=save):
            report = graphics.apply_selected(self.root, self.pack)
        self.assertTrue(report["complete"])
        copied = self.pack / "graphics-corrections" / self.bundle_path.name
        self.assertEqual(copied.read_bytes(), original)
        manifest = json.loads(copied.with_suffix("").read_bytes())
        self.assertEqual(manifest["delta_sha256"], sha(original))

    def test_second_donor_mip_failure_rejects_whole_transaction(self):
        bank = bytearray(self.bank_path.read_bytes())
        bank[-1] ^= 1
        self.bank_path.write_bytes(bank)
        self.assert_rejected_without_writes("donor mip")

    def test_second_descriptor_failure_rejects_before_any_write(self):
        data = bytearray(self.map_path.read_bytes())
        struct.pack_into("<H", data, self.frames[1] + 20, 1)
        self.map_path.write_bytes(data)
        with patch.object(graphics, "SOURCE_MAP", sha(data)):
            self.assert_rejected_without_writes("descriptor")

    def test_wrong_map_and_ambiguous_or_missing_identity_reject(self):
        with patch.object(graphics, "SOURCE_MAP", "0" * 64):
            self.assert_rejected_without_writes("source differs")
        data = bytearray(self.map_path.read_bytes())
        struct.pack_into("<I", data, 2244, 0)  # Same path, different salted datum.
        self.map_path.write_bytes(data)
        with patch.object(graphics, "SOURCE_MAP", sha(data)):
            self.assert_rejected_without_writes("bitmap identity")

    def test_profile_scope_mip_owners_and_delta_partition_ownership_are_complete(self):
        original = self.bundle_path.with_suffix("").read_bytes()
        cases = (
            lambda m: m["profiles"][1]["profile"].update(tag=bundle.GREY_TAG + "_custom"),
            lambda m: m["profiles"][1]["profile"].update(format=11),
            lambda m: m["profiles"][1]["mips"].pop(),
            lambda m: m["profiles"][1].update(delta_offset=0),
            lambda m: m["profiles"].pop(),
            lambda m: m.update(original_map_sha256="0" * 64),
        )
        for change in cases:
            self.bundle_path.with_suffix("").write_bytes(original)
            self.change_manifest(change)
            with self.subTest(change=change):
                self.assert_rejected_without_writes("identity|profile|owner")
        self.bundle_path.with_suffix("").write_bytes(original)

    def test_target_mip_digest_and_trailing_blob_bytes_reject(self):
        original = self.bundle_path.with_suffix("").read_bytes()
        self.change_manifest(lambda m: m["profiles"][1]["target_mip_sha256"].__setitem__(-1, "0" * 64))
        self.assert_rejected_without_writes("target mip")
        self.bundle_path.with_suffix("").write_bytes(original)
        self.bundle_path.write_bytes(self.bundle_path.read_bytes() + b"unowned")
        self.assert_rejected_without_writes("extent")

    def test_bounded_decompression_is_used_for_second_entry_too(self):
        manifest = json.loads(self.bundle_path.with_suffix("").read_bytes())
        packed = self.bundle_path.read_bytes()[:manifest["profiles"][1]["delta_offset"]]
        bomb = zlib.compress(bytes(100000))
        packed += bomb
        self.bundle_path.write_bytes(packed)
        def change(m):
            m.update(delta_bytes=len(packed), delta_sha256=sha(packed))
            m["profiles"][1].update(delta_bytes=len(bomb), delta_sha256=sha(bomb))
        self.change_manifest(change)
        self.assert_rejected_without_writes("delta extent")

    def test_missing_v2_bundle_cannot_report_concrete_only_as_complete(self):
        self.bundle_path.unlink()
        before = self.bank_path.read_bytes()
        report = graphics.apply_selected(self.root, self.pack)
        self.assertFalse(report["complete"])
        self.assertIn("v2", report["blocker"])
        self.assertEqual(before, self.bank_path.read_bytes())

    def test_missing_v3_bundle_cannot_publish_zanzibar_only_as_complete(self):
        self.bundle_v3_path.unlink()
        paths = (self.map_path, self.metropolis_path, self.bank_path)
        before = [p.read_bytes() for p in paths]
        report = graphics.apply_selected(self.root, self.pack)
        self.assertFalse(report["complete"])
        self.assertIn("v3", report["blocker"])
        self.assertEqual(before, [p.read_bytes() for p in paths])

    def test_last_metropolis_delta_failure_rejects_both_maps_without_writes(self):
        packed = bytearray(self.bundle_v3_path.read_bytes())
        packed[-1] ^= 1
        self.bundle_v3_path.write_bytes(packed)
        manifest_path = self.bundle_v3_path.with_suffix("")
        manifest = json.loads(manifest_path.read_bytes())
        manifest["delta_sha256"] = sha(packed)
        manifest_path.write_text(json.dumps(manifest))
        self.assert_rejected_without_writes("delta fingerprint")

    def test_metropolis_profile_overlap_target_mip_and_map_identity_reject(self):
        path = self.bundle_v3_path.with_suffix("")
        original = path.read_bytes()
        cases = (
            lambda m: m["profiles"][-1].update(delta_offset=0),
            lambda m: m["profiles"][-1]["mips"].pop(),
            lambda m: m["profiles"][-1]["target_mip_sha256"].__setitem__(-1, "0" * 64),
            lambda m: m["profiles"][-1]["profile"].update(tag="unverified"),
            lambda m: m.update(map="another.map"),
        )
        for change in cases:
            manifest = json.loads(original)
            change(manifest)
            path.write_text(json.dumps(manifest))
            with self.subTest(change=change):
                self.assert_rejected_without_writes("owner|profile|target mip|identity")
        path.write_bytes(original)


if __name__ == "__main__":
    unittest.main()
