import hashlib
import json
import os
from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from audit_rendering import FormatError
from custom_ui_assets import BANK, SOURCE, inventory, install, prepare, repair, runtime_sources, verify
from custom_ui_test_support import custom_ui_fixture, custom_ui_menu_fixture


class CustomUiAssets(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='custom-ui-', dir=os.environ.get('HALO2_TEST_SCRATCH'))
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name) / 'source'
        custom_ui_fixture(self.root)
        self.cache = Path(self.temp.name) / 'cache'
        self.cache.mkdir()
        custom_ui_menu_fixture(self.cache, self.root)
        self.provenance = self.cache / 'provenance.json'
        self.provenance.write_text(json.dumps(dict(schema='H2_MCC_TU15_PREPARATION_V1',
            external_installation_requirement='Halo MCC only', unrelated='retained')))

    def test_source_copy_preserves_masters_and_celestial_binding(self):
        stage = Path(self.temp.name) / 'package'
        stage.mkdir()
        report = install(self.root, stage)
        self.assertEqual(report['source_pages'], 14)
        self.assertEqual(inventory(self.root), inventory(stage))
        self.assertFalse(report['original_fonts_included'])
        entry, payloads = runtime_sources(stage)
        self.assertEqual(len(entry['files']), 15)
        pages = json.loads(payloads[BANK + '/manifest.json'])['pages']
        self.assertEqual({page['frame'] for page in pages}, set(range(43, 50)))
        with self.assertRaisesRegex(FormatError, 'already exists'):
            install(self.root, stage)

    def test_source_art_tampering_and_missing_slot_reject(self):
        path = self.root / SOURCE / 'masters.json'
        metadata = json.loads(path.read_bytes())
        page = next(iter(metadata['pages'].values()))
        (self.root / SOURCE / page['dds']).write_bytes(b'customized')
        with self.assertRaisesRegex(FormatError, 'differs'):
            runtime_sources(self.root)
        metadata['pages'].pop(next(iter(metadata['pages'])))
        path.write_text(json.dumps(metadata))
        with self.assertRaises(FormatError):
            runtime_sources(self.root)

    def test_fresh_install_repair_and_private_cache_verification(self):
        entry = prepare(self.root, self.cache, progress=lambda _: None)
        self.assertTrue(verify(self.cache, entry))
        _, report = repair(self.root, self.cache, progress=lambda _: None)
        self.assertEqual(report['custom_ui_assets'], entry)
        self.assertEqual(report['unrelated'], 'retained')
        name = next(name for name in entry['files'] if name.endswith('.dds'))
        data = (self.cache / name).read_bytes()
        (self.cache / name).unlink()
        with self.assertRaisesRegex(FormatError, 'differs'):
            verify(self.cache, entry)
        repair(self.root, self.cache, progress=lambda _: None)
        self.assertEqual((self.cache / name).read_bytes(), data)
        self.assertFalse(list(self.cache.rglob('.custom-rank-*')))

    def test_custom_cache_art_and_wrong_source_owners_are_retained(self):
        _, report = repair(self.root, self.cache, progress=lambda _: None)
        name = next(name for name in report['custom_ui_assets']['files'] if name.endswith('.dds'))
        path = self.cache / name
        path.write_bytes(b'user modified art')
        before = self.provenance.read_bytes()
        with self.assertRaisesRegex(FormatError, 'Customized'):
            repair(self.root, self.cache, progress=lambda _: None)
        self.assertEqual(path.read_bytes(), b'user modified art')
        self.assertEqual(self.provenance.read_bytes(), before)
        path.unlink()
        metadata_path = self.cache / 'ui/mcc-menu-assets.json'
        metadata = json.loads(metadata_path.read_bytes())
        metadata['pages'][0]['dds_sha256'] = '0' * 64
        metadata_path.write_text(json.dumps(metadata))
        with self.assertRaisesRegex(FormatError, 'frame owner'):
            repair(self.root, self.cache, progress=lambda _: None)
        self.assertEqual(self.provenance.read_bytes(), before)

    def test_real_celestial_art_has_all_slots_and_original_geometry(self):
        entry, payloads = runtime_sources(ROOT)
        pages = json.loads(payloads[BANK + '/manifest.json'])['pages']
        self.assertEqual(len(pages), 14)
        self.assertEqual({tuple(p['logical']) for p in pages}, {(28, 26), (17, 17)})
        self.assertEqual(len(entry['files']), 15)
        self.assertTrue(all(p['sha256'] != p['source_dds_sha256'] for p in pages))


if __name__ == '__main__':
    unittest.main()
