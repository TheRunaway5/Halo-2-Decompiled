"""Onboarding must work from shipped source files without local reference tools."""
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
import json
import hashlib
import zlib

ROOT = Path(__file__).resolve().parents[1]


class MccSourceDistributionTests(unittest.TestCase):
    def test_shader_preservation_has_no_development_registry_dependency(self):
        with tempfile.TemporaryDirectory(prefix='halo2-clean-shaders-') as directory:
            root = Path(directory)
            shutil.copytree(ROOT / 'tools', root / 'tools',
                            ignore=shutil.ignore_patterns('reference', '__pycache__'))
            code = '''from pathlib import Path
import json
from preserve_native_shaders import copy_selected
root = Path.cwd()
out = root / 'prepared'
out.mkdir()
result = copy_selected(root, out)
assert result['complete'], result
library = out / 'native-shaders/lockout'
manifest = json.loads((library / 'library.json').read_bytes())
assert manifest['map'] == 'lockout'
assert len((library / 'function-tables.bin').read_bytes()) == 28 * 1024
assert any(t['name'].endswith('composite_block') for t in manifest['shader_tags'])
policy = json.loads((root / 'tools/data/mcc-native-library-closure.json').read_bytes())
from native_shader_bundle import canonical_digest
cairo = out / 'native-shaders' / policy['source_map'] / 'library.json'
assert canonical_digest(json.loads(cairo.read_bytes())) == policy['source_canonical_sha256']
assert not list(root.rglob('*.xbe')) and not list(root.rglob('*.map'))
print(json.dumps(result))
'''
            result = subprocess.run([sys.executable, '-c', code], cwd=root,
                                    env=dict(__import__('os').environ, PYTHONPATH=str(root / 'tools')),
                                    capture_output=True, text=True, timeout=30)
            self.assertEqual(result.returncode, 0, result.stderr)
            report = json.loads(result.stdout)
            self.assertEqual(len(report['maps']), 39)
            self.assertFalse((root / 'assets/native-shaders-source.tsv').exists())

    def test_menu_verification_opens_its_inventory_without_reference_checkout(self):
        with tempfile.TemporaryDirectory(prefix='halo2-clean-setup-') as directory:
            root = Path(directory)
            tools = root / 'tools'
            tools.mkdir()
            for path in (ROOT / 'tools').glob('*.py'):
                shutil.copy2(path, tools / path.name)
            data = ROOT / 'tools/data'
            if data.is_dir():
                shutil.copytree(data, tools / 'data')
            # Drive the actual default call used by prepare(), without overriding
            # its inventory. Missing MCC bytes stop the minimal probe only after
            # the complete bundled inventory has opened and parsed successfully.
            code = '''from pathlib import Path
import mcc_menu_inventory
try:
    mcc_menu_inventory.verify(Path('missing-MCC/halo2'))
except FileNotFoundError as error:
    assert Path(error.filename) == Path('missing-MCC/halo2/h2_maps_win64_dx11/textures.dat'), (
        'MCC setup has an unshipped dependency: ' + str(error))
else:
    raise AssertionError('Inventory verification was skipped')
'''
            result = subprocess.run([sys.executable, '-c', code], cwd=root,
                                    env=dict(__import__('os').environ, PYTHONPATH=str(tools)),
                                    capture_output=True, text=True, timeout=10)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertFalse((tools / 'reference').exists())

    def test_corrupt_shader_bundle_is_rejected_before_cache_publication(self):
        with tempfile.TemporaryDirectory(prefix='halo2-corrupt-shaders-') as directory:
            root = Path(directory)
            data = root / 'tools/data'
            data.mkdir(parents=True)
            for name in ('native-materials.json', 'native-materials.json.zlib'):
                shutil.copyfile(ROOT / 'tools/data' / name, data / name)
            payload = data / 'native-materials.json.zlib'
            damaged = bytearray(payload.read_bytes())
            damaged[len(damaged) // 2] ^= 1
            payload.write_bytes(damaged)
            code = '''from pathlib import Path
from native_shader_bundle import copy_bundled
from audit_rendering import FormatError
root = Path.cwd()
out = root / 'prepared'
out.mkdir()
try:
    copy_bundled(root, out)
except FormatError as error:
    assert 'fingerprint differs' in str(error), error
else:
    raise AssertionError('Changed native materials were admitted')
assert not (out / 'native-shaders').exists()
'''
            result = subprocess.run([sys.executable, '-c', code], cwd=root,
                                    env=dict(__import__('os').environ, PYTHONPATH=str(ROOT / 'tools')),
                                    capture_output=True, text=True, timeout=10)
            self.assertEqual(result.returncode, 0, result.stderr)

    def test_material_bundle_cannot_carry_gameplay_payloads(self):
        with tempfile.TemporaryDirectory(prefix='halo2-material-scope-') as directory:
            root = Path(directory)
            data = root / 'tools/data'
            data.mkdir(parents=True)
            bundle = json.loads(zlib.decompress((ROOT / 'tools/data/native-materials.json.zlib').read_bytes()))
            bundle['maps']['lockout']['shader_tags'][0]['group'] = 'weap'
            raw = json.dumps(bundle).encode()
            packed = zlib.compress(raw, 1)
            (data / 'native-materials.json.zlib').write_bytes(packed)
            report = json.loads((ROOT / 'tools/data/native-materials.json').read_bytes())
            report.update(bytes=len(packed), decoded_bytes=len(raw), sha256=hashlib.sha256(packed).hexdigest())
            (data / 'native-materials.json').write_text(json.dumps(report))
            code = '''from pathlib import Path
from native_shader_bundle import copy_bundled
from audit_rendering import FormatError
root = Path.cwd()
out = root / 'prepared'
out.mkdir()
try:
    copy_bundled(root, out)
except FormatError as error:
    assert 'Non-shader payload' in str(error), error
else:
    raise AssertionError('Gameplay bytes admitted as port shader resources')
assert not (out / 'native-shaders').exists()
'''
            result = subprocess.run([sys.executable, '-c', code], cwd=root,
                                    env=dict(__import__('os').environ, PYTHONPATH=str(ROOT / 'tools')),
                                    capture_output=True, text=True, timeout=10)
            self.assertEqual(result.returncode, 0, result.stderr)


if __name__ == '__main__':
    unittest.main()
