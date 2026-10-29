"""Exercise real correction partition planning and its source-bound reservation."""
import hashlib
import io
import os
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest.mock import patch
import zlib

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from audit_rendering import FormatError
from mcc_bitmap_correction_bundle import decode_delta
from mcc_content import texture_resource
import mcc_graphics_corrections as graphics


def sha(data):
    return hashlib.sha256(data).hexdigest()


def authenticated_pixels(target):
    donor = bytes(value ^ 0x53 for value in target)
    packed = zlib.compress(bytes(a ^ b for a, b in zip(donor, target, strict=True)))
    return decode_delta(donor, packed, source_sha256=sha(donor),
                        target_sha256=sha(target), delta_sha256=sha(packed))


class ResourceLayoutFixture(unittest.TestCase):
    def setUp(self):
        self.prefix = bytes(range(16))
        # The first two profiles have different authored channel order but the
        # same native byte chain. Equal-sized later chains must remain distinct.
        self.staged = []
        for name, encoding, target in (
                ("first.map", "bc3", b"ABCD" * 4),
                ("same-native.map", "rgba8", b"CBAD" * 4),
                ("different.map", "bc3", b"ABCE" * 4),
                ("same-later.map", "bc3", b"ABCE" * 4)):
            profile = dict(tag="fixture-" + name, encoding=encoding,
                           target_sha256=sha(target), pixel_bytes=len(target))
            self.staged.append((name, profile, authenticated_pixels(target)))
        self.owners = {(name, profile["tag"]): (name, 128 + index * 168)
                       for index, (name, profile, _) in enumerate(self.staged)}
        self.boundary = len(self.prefix) + 8 + len(self.staged[0][2])
        self.reserved = 32

    def plan(self, *, staged=None, start=None, boundary=1):
        return graphics.allocate_partitions(self.staged if staged is None else staged,
            self.owners, len(self.prefix) if start is None else start,
            preview_boundary=boundary)

    def small_layout(self):
        return patch.multiple(graphics, CANONICAL_V3_END=self.boundary,
                              PREVIEW_RESERVED_BYTES=self.reserved)

    def materialize(self, writes):
        bank = io.BytesIO(self.prefix)
        bank.seek(0, 2)
        for address, payload in writes:
            self.assertEqual(bank.tell(), address)
            bank.write(payload)
        return bank


class ResourceLayoutTests(ResourceLayoutFixture):
    def test_authentic_targets_share_native_storage_and_reserve_has_no_image_owner(self):
        with self.small_layout():
            partitions, writes, end = self.plan()
        self.assertEqual(len(partitions), 4)
        self.assertEqual(len(writes), 3)
        self.assertEqual(writes[1], (self.boundary, bytes(self.reserved)))
        self.assertEqual([row[3] for row in partitions],
                         [16, 16, self.boundary + self.reserved,
                          self.boundary + self.reserved])
        self.assertNotEqual(partitions[0][3], partitions[2][3])
        self.assertEqual(end, self.boundary + self.reserved + 24)
        bank = self.materialize(writes)
        self.assertEqual(bank.getvalue()[:len(self.prefix)], self.prefix)
        for name, profile, frame, address, partition, native in partitions:
            with self.subTest(map=name):
                self.assertEqual(frame, self.owners[(name, profile["tag"])][1])
                self.assertEqual(struct.unpack_from("<Ii", partition), (1, -len(native)))
                self.assertEqual(texture_resource(bank, address, len(partition)), native)
                self.assertFalse(address < self.boundary + self.reserved and
                                 address + len(partition) > self.boundary)
        with self.assertRaisesRegex(FormatError, "segment count"):
            texture_resource(bank, self.boundary, 8)

    def test_legacy_layout_keeps_each_owner_allocation_and_has_no_reservation(self):
        partitions, writes, end = self.plan(boundary=None)
        self.assertEqual([row[3] for row in partitions], [16, 40, 64, 88])
        self.assertEqual(len(writes), len(partitions))
        self.assertEqual(end, 112)
        self.materialize(writes)

    def test_changed_boundary_rejects_the_cohort(self):
        with patch.multiple(graphics, CANONICAL_V3_END=self.boundary + 1,
                            PREVIEW_RESERVED_BYTES=self.reserved):
            with self.assertRaisesRegex(FormatError, "V3 resource boundary differs"):
                self.plan()

    def test_boundary_must_separate_two_real_staged_entries(self):
        for boundary in (-1, 0, len(self.staged), len(self.staged) + 1):
            with self.subTest(boundary=boundary):
                with self.assertRaisesRegex(FormatError, "cohort boundary differs"):
                    self.plan(boundary=boundary)

    def test_address_capacity_rejects_exact_limit_and_allows_last_in_range_extent(self):
        one = self.staged[:1]
        limit = 0x40000000
        with self.assertRaisesRegex(FormatError, "address capacity"):
            self.plan(staged=one, start=limit - 24, boundary=None)
        partitions, _, end = self.plan(staged=one, start=limit - 25, boundary=None)
        self.assertEqual(end, limit - 1)
        self.assertEqual(partitions[0][3], limit - 25)

    def test_reserved_span_also_consumes_the_address_capacity(self):
        start = 0x40000000 - 60
        staged = [self.staged[0], self.staged[2]]
        with patch.multiple(graphics, CANONICAL_V3_END=start + 24,
                            PREVIEW_RESERVED_BYTES=self.reserved):
            with self.assertRaisesRegex(FormatError, "address capacity"):
                self.plan(staged=staged, start=start)


class CanonicalPrefixTests(ResourceLayoutFixture):
    def setUp(self):
        super().setUp()
        self.directory = tempfile.TemporaryDirectory(dir=os.environ.get("HALO2_TEST_SCRATCH"))
        self.addCleanup(self.directory.cleanup)
        self.bank = Path(self.directory.name) / "textures.dat"
        self.bank.write_bytes(self.prefix)
        with self.small_layout():
            self.partitions, self.writes, self.end = self.plan()
        self.v3_prefix = self.prefix + self.writes[0][1]

    def authorities(self, **changes):
        values = dict(CANONICAL_SOURCE_BYTES=len(self.prefix),
                      CANONICAL_SOURCE_SHA=sha(self.prefix),
                      CANONICAL_V3_END=self.boundary,
                      CANONICAL_V3_SHA=sha(self.v3_prefix))
        values.update(changes)
        return patch.multiple(graphics, **values)

    def test_exact_source_and_pending_v3_bytes_admit_without_mutating_the_bank(self):
        with self.authorities():
            graphics.verify_canonical_prefix(self.bank, self.writes)
        self.assertEqual(self.bank.read_bytes(), self.prefix)

    def test_wrong_source_size_or_digest_rejects_without_writes(self):
        for changes in (dict(CANONICAL_SOURCE_BYTES=len(self.prefix) + 1),
                        dict(CANONICAL_SOURCE_SHA="0" * 64)):
            with self.subTest(changes=changes), self.authorities(**changes):
                with self.assertRaisesRegex(FormatError, "texture source differs"):
                    graphics.verify_canonical_prefix(self.bank, self.writes)
            self.assertEqual(self.bank.read_bytes(), self.prefix)

    def test_wrong_v3_digest_and_changed_pending_target_reject_without_writes(self):
        with self.authorities(CANONICAL_V3_SHA="0" * 64):
            with self.assertRaisesRegex(FormatError, "resource prefix differs"):
                graphics.verify_canonical_prefix(self.bank, self.writes)
        writes = list(self.writes)
        address, payload = writes[0]
        writes[0] = (address, payload[:-1] + bytes([payload[-1] ^ 1]))
        with self.authorities():
            with self.assertRaisesRegex(FormatError, "resource prefix differs"):
                graphics.verify_canonical_prefix(self.bank, writes)
        self.assertEqual(self.bank.read_bytes(), self.prefix)

    def test_discontiguous_pending_prefix_rejects_without_writes(self):
        writes = [(self.writes[0][0] + 1, self.writes[0][1]), *self.writes[1:]]
        with self.authorities():
            with self.assertRaisesRegex(FormatError, "prefix allocation differs"):
                graphics.verify_canonical_prefix(self.bank, writes)
        self.assertEqual(self.bank.read_bytes(), self.prefix)


class ApplicationBoundaryTests(unittest.TestCase):
    def test_real_delta_pipeline_rejects_invalid_layout_before_map_or_bank_writes(self):
        import test_mcc_bitmap_correction_bundle as existing
        fixture = existing.BitmapCorrectionAtomicApplicationTests()
        fixture.setUp()
        self.addCleanup(fixture.doCleanups)
        allocate = graphics.allocate_partitions

        def wrong_boundary(staged, owners, extent, **_):
            return allocate(staged, owners, extent, preview_boundary=1)

        # Keep its real two-map cache and authenticated deltas. Inject only an
        # invalid layout decision at the allocator boundary used by the caller.
        with patch.object(graphics, "allocate_partitions", side_effect=wrong_boundary), \
                patch.object(graphics, "CANONICAL_V3_END", 0x10000000):
            fixture.assert_rejected_without_writes("V3 resource boundary differs")


if __name__ == "__main__":
    unittest.main()
