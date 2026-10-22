"""Actual resource-bank packaging, tamper rejection and private cleanup."""
import hashlib
import json
import os
from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from controller_prompt_assets import FAMILIES, RUNTIME_FILES, install, inventory, prepare, repair


def controller_prompt_fixture(root):
    """Tiny metadata/copy inputs; never presented as rendered DDS evidence."""
    for family, schema in FAMILIES.items():
        bank = Path(root) / 'resources/presentation' / ('prompts-' + family)
        files = {}
        for name in sorted(RUNTIME_FILES | {'reconstruction.json'}):
            data = ('synthetic controller prompt fixture ' + family + ' ' + name).encode()
            path = bank / name;path.parent.mkdir(parents=True, exist_ok=True);path.write_bytes(data)
            files[name] = dict(bytes=len(data), sha256=hashlib.sha256(data).hexdigest())
        (bank / 'manifest.json').write_text(json.dumps(dict(schema=schema, files=files)))


class Packaging(unittest.TestCase):
    def test_actual_pair(self):
        with tempfile.TemporaryDirectory(prefix='controller-prompts-', dir=os.environ.get('HALO2_TEST_SCRATCH')) as scratch:
            stage = Path(scratch)
            source = inventory(ROOT)
            copied = install(ROOT, stage)
            self.assertEqual(copied['families'], ['xbox', 'playstation'])
            self.assertEqual(inventory(stage), source)
            with self.assertRaises(ValueError):
                install(ROOT, stage)
            path = stage / 'resources/presentation/prompts-playstation/atlases/1.dds'
            data = bytearray(path.read_bytes());data[-1] ^= 1;path.write_bytes(data)
            with self.assertRaisesRegex(ValueError, 'differs'):
                inventory(stage)
        self.assertFalse(stage.exists())


class VanillaOnboarding(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='vanilla-controller-prompts-', dir=os.environ.get('HALO2_TEST_SCRATCH'))
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name) / 'port';controller_prompt_fixture(self.root)
        self.cache = self.root / 'assets/imported/mcc-fixture';self.cache.mkdir(parents=True)
        self.font = self.cache / 'halo2/h2_fonts/keep.font';self.font.parent.mkdir(parents=True)
        self.font.write_bytes(b'keep MCC font bytes exactly')
        self.report = self.cache / 'provenance.json'
        self.report.write_text(json.dumps(dict(schema='H2_MCC_TU15_PREPARATION_V1',
            external_installation_requirement='Halo MCC only', unrelated='preserve')))

    def test_fresh_vanilla_install_and_missing_asset_repair(self):
        entry = prepare(self.root, self.cache, progress=lambda _: None)
        self.assertEqual(len(entry['files']), 18)
        self.assertTrue(all((self.cache / name).is_file() for name in entry['files']))
        self.assertFalse(list(self.cache.rglob('*.svg')))
        _, report = repair(self.root, self.cache, progress=lambda _: None)
        self.assertEqual(report['controller_prompts'], entry)
        self.assertEqual(report['unrelated'], 'preserve')
        prior = self.report.read_bytes()
        missing = self.cache / 'presentation/prompts-playstation/atlases/1.dds';expected = missing.read_bytes();missing.unlink()
        repair(self.root, self.cache, progress=lambda _: None)
        self.assertEqual(missing.read_bytes(), expected)
        self.assertEqual(self.report.read_bytes(), prior)
        self.assertEqual(self.font.read_bytes(), b'keep MCC font bytes exactly')

    def test_custom_assets_and_provenance_are_preserved(self):
        repair(self.root, self.cache, progress=lambda _: None)
        path = self.cache / 'presentation/prompts-playstation/atlases/1.dds';path.write_bytes(b'custom user artwork')
        prior = self.report.read_bytes()
        with self.assertRaisesRegex(ValueError, 'Customized'):
            repair(self.root, self.cache, progress=lambda _: None)
        self.assertEqual(path.read_bytes(), b'custom user artwork')
        self.assertEqual(self.report.read_bytes(), prior)
        self.assertFalse(list(self.cache.rglob('.controller-*')))

    def test_authenticated_default_upgrade(self):
        repair(self.root, self.cache, progress=lambda _: None)
        bank = self.root / 'resources/presentation/prompts-xbox'
        source = bank / 'atlases/0.dds';source.write_bytes(b'updated synthetic default')
        metadata = json.loads((bank / 'manifest.json').read_text())
        metadata['files']['atlases/0.dds'] = dict(bytes=source.stat().st_size,
            sha256=hashlib.sha256(source.read_bytes()).hexdigest())
        (bank / 'manifest.json').write_text(json.dumps(metadata))
        repair(self.root, self.cache, progress=lambda _: None)
        self.assertEqual((self.cache / 'presentation/prompts-xbox/atlases/0.dds').read_bytes(),source.read_bytes())
        self.assertEqual(self.font.read_bytes(), b'keep MCC font bytes exactly')


if __name__ == '__main__':
    unittest.main()
