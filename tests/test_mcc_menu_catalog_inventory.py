"""Strict converted-page qualification and unchanged stock inventory behavior."""
import copy
import csv
import hashlib
from pathlib import Path
import struct
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from audit_rendering import FormatError
from export_vista_menu_ui import dds_image
from mcc_content import MccCache
import mcc_multiplayer_catalog as catalog
import mcc_menu_catalog_inventory as admission
import mcc_menu_inventory as inventory
from test_mcc_multiplayer_catalog import ROWS, fixture, record


class ProjectionInventoryTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.halo = Path(self.temp.name) / 'halo2'
        self.maps = self.halo / 'h2_maps_win64_dx11'
        self.maps.mkdir(parents=True)
        fixture(self.maps / 'mainmenu.map', ROWS, False)
        self.cache = MccCache(self.maps / 'mainmenu.map')
        self.addCleanup(self.cache.close)
        tag, at = self.cache.tag(self.cache.u32(self.cache.index + 16), 'matg')
        offsets = catalog.catalog(self.cache, tag, at + 0x178)
        records = []
        for row, spec, at in zip(ROWS, admission.PAGES, offsets):
            before = bytes(record(row, False))
            after = record(row, True)
            struct.pack_into('<I', after, 8, row['target_bitmap'])
            self.cache.data[at:at + len(after)] = after
            bitmap, frame_at, _ = catalog.bitmap(self.cache, row['target_bitmap'])
            struct.pack_into('<I', self.cache.data, frame_at + 56, row['pointer'])
            struct.pack_into('<I', self.cache.data, frame_at + 80, row['size'])
            # Explicit provenance fixture: the production v533 audit supplies
            # these exact input hashes; this unit suite supplies no game assets.
            records.append(dict(state='stock-placeholder', before=catalog.summary(before), after=catalog.summary(after),
                before_record_sha256=catalog.digest(before), source_record_sha256=catalog.digest(record(row, True)),
                projected_record_sha256=catalog.digest(after), source_cache_sha256=spec['source_cache_sha256'],
                destination_bitmap=dict(datum=spec['datum'], tag=bitmap['name'], frame_offset=frame_at),
                source_bitmap=dict(datum=spec['source_datum'], tag=spec['source_name'], frame_offset=0),
                registration_point=dict(destination=[110, 103], source=[110, 104], preserved=True),
                prior_resource=dict(admission.BLANK_RESOURCE), authored_resource=dict(pointer=spec['pointer'],
                    stored_bytes=spec['stored_bytes'], stored_sha256=spec['stored_sha256'], decoded_bytes=182160,
                    sha256=spec['pixel_sha256'])))
        self.projection = dict(profile=catalog.PROFILE, serialized_decoded_sha256=catalog.digest(self.cache.data), records=records)

    def test_exact_complete_metadata_and_two_fixed_pages_are_admitted(self):
        pages = admission.qualified_pages(self.cache, self.projection)
        self.assertEqual(set(pages), {(p['name'], 0) for p in admission.PAGES})
        for spec in admission.PAGES:
            row = dict(mcc_cache='mainmenu.map', mcc_name=spec['name'], mcc_frame='0',
                       width='220', height='207', dds_sha256=admission.STOCK_DDS, status='dds-exact')
            page = admission.projected_page(spec, row, spec['dds_sha256'], spec['pixel_sha256'])
            self.assertEqual(page['original_stock_dds_sha256'], admission.STOCK_DDS)
            self.assertEqual(page['original_source_comparison'], 'dds-exact')
            self.assertEqual(page['catalog_projection']['map_id'], spec['map_id'])
            self.assertEqual(page['source_comparison'], 'mcc-authored-catalog-projection')

    def test_missing_stale_or_forged_projection_fails_closed(self):
        mutations = (
            lambda p: p.update(profile='arbitrary profile'),
            lambda p: p.update(serialized_decoded_sha256='0' * 64),
            lambda p: p['records'].pop(),
            lambda p: p['records'][0].update(after=None),
            lambda p: p['records'][0]['after'].update(map_id=1239),
            lambda p: p['records'][0].update(projected_record_sha256='0' * 64),
            lambda p: p['records'][0].update(source_cache_sha256='0' * 64),
            lambda p: p['records'][0]['authored_resource'].update(sha256='0' * 64),
            lambda p: p['records'][0]['destination_bitmap'].update(datum='e9530113'),
            lambda p: p['records'][0]['source_bitmap'].update(tag='unrelated authored bitmap'),
            lambda p: p['records'][0]['registration_point'].update(destination=[110, 104]),
            lambda p: p['records'][0]['before'].update(map_id=1401),
        )
        for i, change in enumerate(mutations):
            projected = copy.deepcopy(self.projection)
            change(projected)
            with self.subTest(case=i), self.assertRaises(FormatError):
                admission.qualified_pages(self.cache, projected)

    def test_current_record_or_frame_mutation_is_not_hidden_by_new_report_hash(self):
        globals_tag, at = self.cache.tag(self.cache.u32(self.cache.index + 16), 'matg')
        record_at = catalog.catalog(self.cache, globals_tag, at + 0x178)[0]
        _, frame_at, _ = catalog.bitmap(self.cache, ROWS[0]['target_bitmap'])
        for offset in (record_at, record_at + 0xC56, frame_at + 16, frame_at + 56):
            before = bytes(self.cache.data)
            self.cache.data[offset] ^= 1
            projected = dict(self.projection, serialized_decoded_sha256=catalog.digest(self.cache.data))
            with self.subTest(offset=offset), self.assertRaises(FormatError):
                admission.qualified_pages(self.cache, projected)
            self.cache.data[:] = before

    def test_arbitrary_dds_pixels_stock_row_or_frame_cannot_use_projection(self):
        spec = admission.PAGES[0]
        row = dict(mcc_cache='mainmenu.map', mcc_name=spec['name'], mcc_frame='0', width='220', height='207',
                   dds_sha256=admission.STOCK_DDS, status='dds-exact')
        for dds, pixels in (('0' * 64, spec['pixel_sha256']), (spec['dds_sha256'], '0' * 64),
                            (admission.STOCK_DDS, spec['pixel_sha256'])):
            with self.subTest(dds=dds), self.assertRaises(FormatError):
                admission.projected_page(spec, row, dds, pixels)
        for key, value in (('mcc_frame', '1'), ('dds_sha256', spec['dds_sha256']), ('mcc_name', 'another page'), ('mcc_cache', 'shared.map')):
            changed = dict(row, **{key: value})
            with self.subTest(key=key), self.assertRaises(FormatError):
                admission.projected_page(spec, changed, spec['dds_sha256'], spec['pixel_sha256'])

    def test_default_stock_inventory_remains_strict_and_has_no_projection_fields(self):
        spec = admission.PAGES[0]
        _, at, _ = catalog.bitmap(self.cache, ROWS[0]['target_bitmap'])
        struct.pack_into('<2H', self.cache.data, at + 4, 1, 1)
        payload = b'\x12\x34\x56\xff'
        packed = struct.pack('<ii', 1, -len(payload)) + payload
        struct.pack_into('<I', self.cache.data, at + 56, 8)
        struct.pack_into('<I', self.cache.data, at + 80, len(packed))
        self.cache.path.write_bytes(self.cache.data)
        (self.maps / 'textures.dat').write_bytes(b'padding!' + packed)
        fonts = self.halo / 'h2_fonts'
        fonts.mkdir()
        for name in inventory.PRIMARY_FONTS:
            (fonts / name).write_bytes(name.encode())
        dds, _ = dds_image(1, 1, 0, 11, payload)
        source = self.halo / 'stock-inventory.tsv'
        fields = ('status', 'mcc_cache', 'mcc_name', 'mcc_frame', 'width', 'height', 'dds_sha256', 'vista_name', 'vista_frame')
        with source.open('w', newline='') as file:
            writer = csv.DictWriter(file, fields, delimiter='\t')
            writer.writeheader()
            writer.writerow(dict(status='dds-exact', mcc_cache='mainmenu.map', mcc_name=spec['name'], mcc_frame=0,
                width=1, height=1, dds_sha256=hashlib.sha256(dds).hexdigest(), vista_name=spec['name'], vista_frame=0))
        result = inventory.verify(self.halo, source)
        self.assertEqual(result['inventory_sha256'], hashlib.sha256(source.read_bytes()).hexdigest())
        self.assertEqual(result['pages'][0]['source_comparison'], 'dds-exact')
        self.assertNotIn('catalog_projection', result['pages'][0])
        self.assertNotIn('original_stock_dds_sha256', result['pages'][0])
        (self.maps / 'textures.dat').write_bytes(b'padding!' + packed[:-1] + b'\x7f')
        with self.assertRaisesRegex(FormatError, 'DDS differs'):
            inventory.verify(self.halo, source)


if __name__ == '__main__':
    unittest.main()
