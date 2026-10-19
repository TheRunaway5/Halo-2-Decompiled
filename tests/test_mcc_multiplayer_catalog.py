"""Whole-owner MCC catalog conversion; no original runtime assets required."""
import io
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from audit_rendering import FormatError
from mcc_content import MccCache
import mcc_multiplayer_catalog as catalog

ROWS = (
    dict(path=r'scenarios\multi\halo\highplains\highplains', title='Tombstone',
         description='The UNSC decommissioned this munitions testing complex after safety concerns came to light.',
         map_id=1402, placeholder=1239, order=1500, source_bitmap=0xE9530113,
         target_bitmap=0xE89E05A8, pointer=0x1FCC8600, size=158459),
    dict(path=r'scenarios\multi\halo\derelict\derelict', title='Desolation',
         description='The jungle is slowly reclaiming this once pristine but now derelict structure.',
         map_id=1400, placeholder=1240, order=1400, source_bitmap=0xE8F70140,
         target_bitmap=0xE89F05A9, pointer=0x1F301C00, size=153929),
)


def record(row, source):
    # Complete literal C64 logical metadata reconstructed from the MCC donor;
    # production's independent full-record fingerprints verify these fixtures.
    data = bytearray(0xC64)
    struct.pack_into('<3I', data, 0, row['map_id'] if source else row['placeholder'],
                     0x6269746D, row['source_bitmap'] if source else row['target_bitmap'])
    if source:
        for i in range(9):
            for at, value in ((12 + i * 64, row['title']), (0x24C + i * 256, row['description'])):
                encoded = value.encode('utf-16le')
                data[at:at + len(encoded)] = encoded
        struct.pack_into('<i', data, 0xC4C, row['order'])
    encoded = row['path'].encode('ascii')
    data[0xB4C:0xB4C + len(encoded)] = encoded
    data[0xC54:0xC64] = bytes([0, 2, 8, 8, 8, 0, 0, 8, 8, 2, 0, 0, 0, 0, 0, 0]
                             if source else [0, 2] + [8] * 14)
    return data


def frame(row, source):
    data = bytearray(168)
    struct.pack_into('<I2HBB3H2hHBB', data, 0, 0x6269746D, 220, 207, 1, 0,
                     0, 11, 0x210, 110, 104 if source else 103, 0, 11, 16)
    struct.pack_into('<I', data, 28, row['source_bitmap'] if source else row['target_bitmap'])
    struct.pack_into('<I', data, 56, row['pointer'] if source else 0x02399A80)
    struct.pack_into('<3I', data, 68, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF)
    struct.pack_into('<I', data, 80, row['size'] if source else 1986)
    return data


def fixture(path, rows, source):
    primary, globals_id = 0xE1740000, 0xE1740001
    bitmap_ids = [r['source_bitmap' if source else 'target_bitmap'] for r in rows]
    count = max(v & 65535 for v in [primary, globals_id, *bitmap_ids]) + 1
    names = {primary: rows[0]['path'] if source else r'scenarios\ui\mainmenu\mainmenu',
             globals_id: r'globals\globals'}
    for row, datum in zip(rows, bitmap_ids):
        names[datum] = (r'ui\global_bitmaps\map_shots\map_mp_' + ('highplains' if row['map_id'] == 1402 else 'derelict')) if source else r'ui\code_global_bitmaps\multiplayer' + ('23' if row['map_id'] == 1402 else '24')
    name_bytes = bytearray()
    name_offsets = {}
    for datum, name in names.items():
        name_offsets[datum] = len(name_bytes)
        name_bytes += name.encode() + b'\0'
    names_at = 1056 + count * 16
    offsets_at = names_at + len(name_bytes)
    base = (offsets_at + count * 4 + 15) & ~15
    owner_size = (0x3A0 if source else 0x180) + 24 + len(rows) * 0xC64
    data = bytearray(base + owner_size + 248 * len(rows))
    for at, value in ((0, 0x68656164), (4, 13), (8, len(data)), (16, 1024),
                      (32, count), (36, names_at), (40, len(name_bytes)), (44, offsets_at),
                      (720, 1024), (1024, 1056), (1032, 1056), (1036, primary),
                      (1040, globals_id), (1048, count)):
        struct.pack_into('<I', data, at, value)
    data[208:208 + len(names[primary])] = names[primary].encode()
    data[names_at:names_at + len(name_bytes)] = name_bytes
    for index in range(count):
        struct.pack_into('<I', data, 1056 + index * 16 + 4, 0xFFFFFFFF)
    def tag(datum, group, at, size):
        struct.pack_into('<4I', data, 1056 + (datum & 65535) * 16,
                         int.from_bytes(group.encode(), 'big'), datum, at, size)
        struct.pack_into('<I', data, offsets_at + (datum & 65535) * 4, name_offsets[datum])
    tag(primary if source else globals_id, 'scnr' if source else 'matg', base, owner_size)
    if source:
        struct.pack_into('<H', data, base + 16, 1)
    level_at = base + (0x3A0 if source else 0x180)
    struct.pack_into('<2I', data, base + (0x398 if source else 0x178), 1, level_at)
    struct.pack_into('<2I', data, level_at + 16, len(rows), level_at + 24)
    for i, (row, datum) in enumerate(zip(rows, bitmap_ids)):
        at = level_at + 24 + i * 0xC64
        data[at:at + 0xC64] = record(row, source)
        bitmap_at = base + owner_size + i * 248
        tag(datum, 'bitm', bitmap_at, 248)
        struct.pack_into('<2I', data, bitmap_at + 68, 1, bitmap_at + 80)
        data[bitmap_at + 80:bitmap_at + 248] = frame(row, source)
    path.write_bytes(data)


class CatalogTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.maps = Path(self.temp.name)
        fixture(self.maps / 'mainmenu.map', ROWS, False)
        # Filenames deliberately cannot drive the source/path resolution.
        for i, row in enumerate(ROWS):
            fixture(self.maps / f'owned-source-{i}.map', [row], True)
        self.main = MccCache(self.maps / 'mainmenu.map')
        self.addCleanup(self.main.close)
        self.resources = {0x02399A80: bytes(220 * 207 * 4),
                          ROWS[0]['pointer']: b'\x12\x34\x56\xff' * (220 * 207),
                          ROWS[1]['pointer']: b'\x87\x65\x43\xff' * (220 * 207)}
        # Sparse fixture: actual declared compressed byte ranges are retained
        # independently from the explicit decoded-pixel routing fixture.
        with (self.maps / 'textures.dat').open('wb') as file:
            for pointer, size in ((0x02399A80, 1986), *((r['pointer'], r['size']) for r in ROWS)):
                file.seek(pointer)
                file.write(b'T' * size)
        self.stream = (self.maps / 'textures.dat').open('rb')
        self.addCleanup(self.stream.close)
        # Explicit decoder routing fixture. The existing resource-reader suite
        # tests compression; the real MCC scratch probe uses the actual decoder.
        mocked = patch.object(catalog, 'texture_resource',
                              side_effect=lambda stream, pointer, size: self.resources[pointer])
        self.decoder = mocked.start()
        self.addCleanup(mocked.stop)

    def plan(self):
        return catalog.plan_catalog(self.main, self.maps, self.stream)

    def row_offsets(self):
        tag, at = self.main.tag(self.main.u32(self.main.index + 16), 'matg')
        return catalog.catalog(self.main, tag, at + 0x178)

    def test_full_records_preview_ownership_and_registration_are_coherent(self):
        before = bytes(self.main.data)
        source_files = {p: p.read_bytes() for p in self.maps.glob('owned-source-*.map')}
        plan = self.plan()
        self.assertEqual(bytes(self.main.data), before, 'planning mutated input')
        self.assertEqual(len(plan.stores), 6)
        report = catalog.apply_catalog_plan(self.main, plan)
        self.assertEqual(report['metadata_store_bytes'], 2 * (0xC64 + 8))
        for at, row in zip(self.row_offsets(), ROWS):
            expected = record(row, True)
            struct.pack_into('<I', expected, 8, row['target_bitmap'])
            self.assertEqual(self.main.data[at:at + 0xC64], expected)
            _, _, preview = catalog.bitmap(self.main, row['target_bitmap'])
            self.assertEqual(struct.unpack_from('<2h', preview, 16), (110, 103))
            self.assertEqual(struct.unpack_from('<I', preview, 28)[0], row['target_bitmap'])
            self.assertEqual(struct.unpack_from('<I', preview, 56)[0], row['pointer'])
            self.assertEqual(struct.unpack_from('<I', preview, 80)[0], row['size'])
        touched = set()
        for store in plan.stores:
            touched.update(range(store['file_offset'], store['file_offset'] + len(bytes.fromhex(store['after']))))
        self.assertTrue(all(a == b for i, (a, b) in enumerate(zip(before, self.main.data)) if i not in touched))
        self.assertEqual(source_files, {p: p.read_bytes() for p in source_files})
        self.assertTrue(all(r['registration_point']['preserved'] for r in report['records']))
        self.assertEqual(self.decoder.call_count, 4)

    def test_complete_projection_is_idempotent(self):
        catalog.apply_catalog_plan(self.main, self.plan())
        before = bytes(self.main.data)
        second = self.plan()
        self.assertFalse(second.stores)
        self.assertTrue(all(r['state'] == 'already-converted' for r in second.records))
        catalog.apply_catalog_plan(self.main, second)
        self.assertEqual(bytes(self.main.data), before)

    def test_partial_title_or_id_or_counts_are_not_accepted(self):
        for relative in (12, 0, 0xC56):
            original = bytes(self.main.data)
            self.main.data[self.row_offsets()[0] + relative] ^= 1
            changed = bytes(self.main.data)
            with self.subTest(offset=relative), self.assertRaisesRegex(FormatError, 'partial/customized'):
                self.plan()
            self.assertEqual(bytes(self.main.data), changed)
            self.main.data[:] = original

    def test_changed_source_whole_record_is_rejected(self):
        path = self.maps / 'owned-source-0.map'
        source = MccCache(path)
        tag, at = source.tag(source.u32(source.index + 12), 'scnr')
        source.data[catalog.catalog(source, tag, at + 0x398)[0] + 12] ^= 1
        path.write_bytes(source.data)
        source.close()
        with self.assertRaisesRegex(FormatError, 'whole authored'):
            self.plan()

    def test_source_block_cannot_escape_its_scenario_owner(self):
        path = self.maps / 'owned-source-0.map'
        source = MccCache(path)
        _, at = source.tag(source.u32(source.index + 12), 'scnr')
        struct.pack_into('<I', source.data, at + 0x39C, 1024)
        path.write_bytes(source.data)
        source.close()
        with self.assertRaisesRegex(FormatError, 'complete tag owner'):
            self.plan()

    def test_partial_pixel_bank_does_not_mutate_metadata(self):
        before = bytes(self.main.data)
        self.resources[ROWS[1]['pointer']] = b'\0' * 4
        with self.assertRaisesRegex(FormatError, 'partial pixel bank'):
            self.plan()
        self.assertEqual(bytes(self.main.data), before)

    def test_stale_plan_does_not_write_any_other_bytes(self):
        plan = self.plan()
        self.main.data[-1] ^= 1
        before = bytes(self.main.data)
        with self.assertRaisesRegex(FormatError, 'changed after planning'):
            catalog.apply_catalog_plan(self.main, plan)
        self.assertEqual(bytes(self.main.data), before)

    def test_preview_registration_or_resource_partial_change_is_rejected(self):
        row = ROWS[0]
        _, at, _ = catalog.bitmap(self.main, row['target_bitmap'])
        for relative in (16, 56):
            original = bytes(self.main.data)
            self.main.data[at + relative] ^= 1
            with self.subTest(offset=relative), self.assertRaises(FormatError):
                self.plan()
            self.main.data[:] = original

    def test_metadata_only_conversion_does_not_qualify_stale_preview(self):
        plan = self.plan()
        catalog.apply_catalog_plan(self.main, catalog.CatalogPlan(plan.mainmenu_decoded_sha256,
            tuple(s for s in plan.stores if s['group'] == 'matg'), plan.records))
        with self.assertRaisesRegex(FormatError, 'stale preview'):
            self.plan()

    def test_ambiguous_header_identity_fails_closed(self):
        (self.maps / 'another-name.map').write_bytes((self.maps / 'owned-source-0.map').read_bytes())
        with self.assertRaisesRegex(FormatError, 'ambiguous cache header'):
            self.plan()

    def test_wrong_cache_family_or_missing_source_is_rejected(self):
        (self.maps / 'owned-source-0.map').unlink()
        with self.assertRaisesRegex(FormatError, 'missing'):
            self.plan()
        fixture(self.maps / 'owned-source-0.map', [ROWS[0]], True)
        data = bytearray((self.maps / 'owned-source-0.map').read_bytes())
        struct.pack_into('<I', data, 4, 8)
        (self.maps / 'owned-source-0.map').write_bytes(data)
        with self.assertRaisesRegex(FormatError, 'version13'):
            self.plan()


if __name__ == '__main__':
    unittest.main()
