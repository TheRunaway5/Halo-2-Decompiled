"""Custom map source delivery retains compiled owners and omits authoring work."""
import hashlib
import copy
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import custom_map_assets as assets
from audit_rendering import FormatError


def custom_map_fixture(root, *, populated=False):
    root = Path(root)
    manifest = dict(schema=assets.SCHEMA, maps=[])
    if populated:
        row = dict(id='fixture', source='assets/custom-maps/fixture',
                   map='halo2/Mods/fixture/fixture.map', profile='menu/profile.json',
                   resource_aliases=['halo2/h2_maps_win64_dx11/textures.dat'])
        manifest['maps'].append(row)
        package = root / row['source']
        files = {
            row['map']: b'compiled-map-fixture',
            row['resource_aliases'][0]: b'compiled-map-fixture',
            'provenance.json': b'{}',
            'tag-conversion.json': b'{}',
            'native-shaders/map-owners.json': b'{}',
            'native-shaders/maps/fixture/library.json': b'preserved-native-material-bank',
            'custom-owner-receipt.json': json.dumps(dict(map=row['map'],
                input_bytes=len(b'compiled-map-fixture'),
                input_sha256=hashlib.sha256(b'compiled-map-fixture').hexdigest())).encode(),
        }
        for name, raw in files.items():
            path = package / 'content' / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(raw)
        menu = package / 'menu'
        menu.mkdir()
        (menu / 'profile.json').write_text(json.dumps(dict(map_id=1234,
            name='Fixture', description='Fixture map', scenario_path=r'scenarios\multi\fixture\fixture',
            preview='preview.png')))
        (menu / 'preview.png').write_bytes(b'preview-fixture')
        (package / 'authoring').mkdir()
        (package / 'authoring/not-a-runtime.bin').write_bytes(b'authoring-data')
        (package / 'engine-selection.json').write_bytes(b'local runtime selection')
    path = root / assets.MANIFEST
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(manifest))
    return manifest


class CustomMapAssetTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix='custom-map-assets-')
        self.addCleanup(temporary.cleanup)
        self.work = Path(temporary.name)
        self.root = self.work / 'resources'
        self.destination = self.work / 'package'
        custom_map_fixture(self.root, populated=True)
        # These fixtures exercise copying and path policy with synthetic map
        # bytes. Actual owner-index authentication has an unmocked source probe
        # and the dedicated metadata publication tests.
        patcher = patch.object(assets, '_verify_source_index', return_value={})
        self.authenticator = patcher.start()
        self.addCleanup(patcher.stop)

    def test_source_inventory_and_copy_preserve_complete_runtime_only(self):
        pins = assets.source_bundle(self.root)
        self.assertEqual(len(pins), 10)
        self.assertFalse(any('authoring' in name or 'engine-selection' in name for name in pins))
        report = assets.copy_sources(self.root, self.destination, pins)
        self.assertEqual(report['maps'], ['fixture'])
        self.assertEqual(assets.source_bundle(self.destination), pins)
        before = {p: (p.stat().st_ino, p.stat().st_mtime_ns)
                  for p in self.destination.rglob('*') if p.is_file()}
        assets.copy_sources(self.root, self.destination)
        self.assertEqual(before, {p: (p.stat().st_ino, p.stat().st_mtime_ns) for p in before})

    def test_changed_existing_package_retained_before_copy(self):
        self.destination.mkdir()
        target = self.destination / assets.MANIFEST
        target.parent.mkdir()
        target.write_bytes(b'custom-map-delivery')
        with self.assertRaisesRegex(FormatError, 'existing packaged custom asset retained'):
            assets.copy_sources(self.root, self.destination)
        self.assertEqual(target.read_bytes(), b'custom-map-delivery')
        self.assertFalse((self.destination / 'assets').exists())

    def test_missing_registered_map_refuses_partial_delivery(self):
        (self.root / 'assets/custom-maps/fixture/content/halo2/Mods/fixture/fixture.map').unlink()
        with self.assertRaisesRegex(FormatError, 'missing prepared custom owner'):
            assets.copy_sources(self.root, self.destination)
        self.assertFalse(self.destination.exists())

    def test_source_map_and_resource_alias_are_hash_bound(self):
        path = self.root / 'assets/custom-maps/fixture/content/halo2/h2_maps_win64_dx11/textures.dat'
        path.write_bytes(b'different-resource-offset-owner')
        with self.assertRaisesRegex(FormatError, 'resource alias differs'):
            assets.source_bundle(self.root)

    def test_source_index_must_authenticate_before_any_package_is_written(self):
        self.authenticator.side_effect = FormatError('custom source library differs from its owner index')
        with self.assertRaisesRegex(FormatError, 'library differs from its owner index'):
            assets.copy_sources(self.root, self.destination)
        self.authenticator.assert_called_once()
        self.assertFalse(self.destination.exists())

    def test_source_and_destination_symlinks_are_rejected(self):
        outside = self.work / 'outside'
        outside.mkdir()
        self.destination.mkdir()
        (self.destination / 'resources').symlink_to(outside, target_is_directory=True)
        with self.assertRaises(FormatError):
            assets.copy_sources(self.root, self.destination)
        self.assertEqual(list(outside.iterdir()), [])
        path = self.root / 'assets/custom-maps/fixture/menu/preview.png'
        path.unlink()
        path.symlink_to(self.root / assets.MANIFEST)
        with self.assertRaises(FormatError):
            assets.source_bundle(self.root)

    def test_bound_read_only_resources_remain_the_source(self):
        data = self.work / 'data'
        binding = json.dumps(dict(schema='H2_INSTALLATION_LAYOUT_V1', data=str(data),
                                  resources=str(self.root)))
        with patch.dict('os.environ', H2_INSTALLATION_LAYOUT=binding):
            result = assets.copy_sources(data, self.destination)
        self.assertEqual(result['files'], assets.source_bundle(self.root))

    def test_missing_binding_requests_fresh_preparation_without_writing(self):
        self.assertEqual(assets.check(self.root, self.destination), ['fixture'])
        self.assertFalse(assets.compatible(self.root, self.destination))
        self.assertFalse(self.destination.exists())

    def test_existing_different_binding_and_directory_remain_untouched(self):
        self.destination.mkdir()
        path = self.destination / assets.BINDINGS
        path.write_text(json.dumps(dict(schema='H2_FRONTEND_MAP_BINDINGS_V1',
            maps=[dict(map_id=1234, cache_root='custom-maps/my-authored-version')])) )
        before = path.read_bytes()
        with self.assertRaisesRegex(FormatError, 'existing map binding differs'):
            assets.check(self.root, self.destination)
        self.assertEqual(path.read_bytes(), before)

    def test_explicit_empty_catalog_has_no_cache_write(self):
        empty = self.work / 'empty'
        custom_map_fixture(empty)
        self.assertTrue(assets.compatible(empty, self.destination))
        self.assertEqual(assets.prepare(empty, self.destination)['maps'], [])
        self.assertFalse(self.destination.exists())

    def test_historical_source_migration_changes_only_derived_stock_receipt(self):
        from test_mcc_native_map_owners import Fixture
        fixture = Fixture()
        fixture.add_map('h2_maps_win64_dx11/shared.map', role='resources')
        fixture.authority['producer'] = 'mcc_native_map_owner_publication'
        old = fixture.build()
        current = copy.deepcopy(old)
        current['maps'][0]['compatibility_authority']['receipt_sha256'] = 'a' * 64
        before = copy.deepcopy(old)
        changes = assets._normalize_source_index(old, current)
        self.assertEqual(changes, [dict(map='halo2/h2_maps_win64_dx11/shared.map',
            old_sha256='f' * 64, new_sha256='a' * 64)])
        self.assertEqual(old, before)
        for field, value in [('input_sha256', 'b' * 64), ('input_bytes', 1234)]:
            corrupt = copy.deepcopy(current)
            corrupt['maps'][0][field] = value
            with self.assertRaises(FormatError):
                assets._normalize_source_index(old, corrupt)


if __name__ == '__main__':
    unittest.main()
