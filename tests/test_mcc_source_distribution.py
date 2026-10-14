"""Onboarding must work from shipped source files without local reference tools."""
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class MccSourceDistributionTests(unittest.TestCase):
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


if __name__ == '__main__':
    unittest.main()
