"""Real compiled launcher and verifier admission; no game rendering is claimed."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path[:0] = [str(ROOT/'tools'), str(ROOT/'tests')]
import test_native_renderer_aot_capability as fixtures

LAUNCHER = None


class NativeRendererLauncherAdmissionTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        fixtures.AotCapabilityTests.setUpClass()

    def setUp(self):
        self.fixture = fixtures.AotCapabilityTests()
        self.fixture.setUp()
        self.addCleanup(self.fixture.doCleanups)
        self.f = self.fixture.f
        self.launcher = self.f.root/'Halo2-Launcher'
        shutil.copy2(LAUNCHER, self.launcher)
        tools = self.f.root/'tools'
        tools.mkdir()
        (tools/'setup_assets.py').write_text('# Isolated installation sentinel.\n')
        for name in ('native_renderer_shaders.py', 'audit_native_renderer_kernels.py'):
            shutil.copy2(ROOT/'tools'/name, tools/name)

    def info(self):
        result = subprocess.run([str(self.launcher), '--launcher-info'], cwd=self.f.root,
            env={**os.environ, 'SDL_VIDEODRIVER':'dummy', 'SDL_AUDIODRIVER':'dummy',
                 'PYTHONDONTWRITEBYTECODE':'1'}, capture_output=True, text=True, timeout=30)
        self.assertEqual(result.returncode, 0, result.stdout+result.stderr)
        return result.stdout

    def test_complete_aot_bank_is_selected_by_the_actual_verifier(self):
        self.assertIn('Runtime: '+str(self.f.runtime), self.info())

    def test_changed_aot_module_cannot_downgrade_to_presence_checks(self):
        (self.f.shaders/'ui.frag.spv').write_bytes(b'changed')
        self.assertIn('Runtime: not built', self.info())

    def test_generic_probe_bank_without_aot_descriptor_is_rejected(self):
        (self.f.shaders/'production-shaders.json').unlink()
        for name in ('world.frag.spv', 'hud.frag.spv'):
            (self.f.shaders/name).write_bytes(self.fixture.code['frag'])
        self.assertIn('Runtime: not built', self.info())


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--launcher', type=Path, required=True)
    args, remaining = parser.parse_known_args()
    LAUNCHER = args.launcher.resolve(strict=True)
    unittest.main(argv=[sys.argv[0], *remaining])
