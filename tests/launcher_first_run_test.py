"""Exercise MCC-only routing in a relocated launcher with isolated recorders.

Synthetic playable metadata tests arguments only, never actual gameplay parity.
"""
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import unittest

REPO = Path(__file__).resolve().parents[1]
LAUNCHER = Path(sys.argv.pop(1)).resolve()
REQUIRED = (
    'halo2/h2_maps_win64_dx11/mainmenu.map', 'halo2/h2_maps_win64_dx11/shared.map',
    'halo2/h2_maps_win64_dx11/single_player_shared.map', 'halo2/h2_maps_win64_dx11/lockout.map',
    'halo2/h2_maps_win64_dx11/textures.dat', 'halo2/h2_fonts/font_table.txt',
    'bitmap-resources.json', 'tag-conversion.json', 'loading/mcc-loading-assets.json',
)


class LauncherFirstRunTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix='halo2-mcc-first-run-')
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name) / 'fresh checkout'
        (self.root / 'tools').mkdir(parents=True)
        self.launcher = self.root / LAUNCHER.name
        shutil.copy2(LAUNCHER, self.launcher)
        self.cache = self.root / 'assets/imported/mcc-test'
        self.maps = self.cache / 'halo2/h2_maps_win64_dx11'
        (self.root / 'tools/setup_assets.py').write_text('''import json, os, pathlib, sys
root=pathlib.Path(sys.argv[sys.argv.index('--root')+1])
(root/'setup-call.json').write_text(json.dumps(sys.argv[1:]))
status=int(os.environ.get('H2_TEST_SETUP_STATUS','23'))
if status: raise SystemExit(status)
(root/'assets/mcc-source.tsv').write_text('H2_MCC_ASSET_SOURCE_V1\\ncache\\tassets/imported/mcc-test\\n')
''')
        (self.root / 'tools/launcher_ui.py').write_text('''import json, os, pathlib, sys
root=pathlib.Path(sys.argv[sys.argv.index('--root')+1])
(root/'home-call.json').write_text(json.dumps(sys.argv[1:]))
raise SystemExit(int(os.environ.get('H2_TEST_HOME_STATUS','20')))
''')
        (self.root / 'tools/build_runtime.py').write_text("raise RuntimeError('unexpected build')\n")
        self.environment = {**os.environ, 'SDL_VIDEODRIVER': 'dummy', 'SDL_AUDIODRIVER': 'dummy'}
        self.environment.pop('DISPLAY', None)
        self.environment.pop('WAYLAND_DISPLAY', None)

    def run_launcher(self, *arguments):
        return subprocess.run([str(self.launcher), *arguments], cwd=self.temporary.name,
                              env=self.environment, capture_output=True, text=True, timeout=10)

    def call(self, name):
        return json.loads((self.root / (name + '-call.json')).read_text())

    def ready_assets(self, playable=False):
        for name in REQUIRED:
            path = self.cache / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text('fixture')
        self.provenance(playable)
        (self.root / 'assets/mcc-source.tsv').write_text(
            'H2_MCC_ASSET_SOURCE_V1\ncache\tassets/imported/mcc-test\n')

    def provenance(self, playable):
        (self.cache / 'provenance.json').write_text(json.dumps(dict(
            schema='H2_MCC_TU15_PREPARATION_V1', external_installation_requirement='Halo MCC only',
            runtime_active=playable, full_tu15_parity=playable)))

    def runtime(self):
        build = self.root / 'build'
        (build / 'shaders').mkdir(parents=True)
        source = (REPO / 'src/launcher.cpp').read_text()
        block = re.search(r'shader_names\{(.*?)\};', source, re.S).group(1)
        for name in re.findall(r'"([^"\n]+\.spv)"', block):
            (build / 'shaders' / name).touch()
        binary = build / 'halo2_native'
        binary.write_text('#!' + sys.executable + '\n' + '''import json, os, pathlib, sys
pathlib.Path('runtime-call.json').write_text(json.dumps(dict(argv=sys.argv[1:], cwd=os.getcwd(), discord=os.environ.get('H2_DISCORD_LAUNCHER'))))
''')
        binary.chmod(0o755)
        return binary

    def legacy_registries(self):
        (self.root / 'assets').mkdir(exist_ok=True)
        for name in ('asset-sources', 'gameplay-source', 'hud-source', 'menu-source', 'loading-source'):
            (self.root / ('assets/' + name + '.tsv')).write_text('MALFORMED OLD SOURCE\n')

    def test_info_works_before_setup_without_a_runtime(self):
        result = self.run_launcher('--launcher-info')
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn('Runtime: not built', result.stdout)
        self.assertIn('Asset source: Halo MCC only', result.stdout)
        self.assertIn('MCC cache prepared: no', result.stdout)
        self.assertFalse((self.root / 'setup-call.json').exists())

    def test_first_launch_opens_mcc_detection_setup(self):
        result = self.run_launcher()
        self.assertEqual(result.returncode, 23, result.stderr)
        self.assertEqual(self.call('setup'), ['--root', str(self.root), '--interactive'])

    def test_explicit_mcc_path_uses_terminal_import(self):
        result = self.run_launcher('--mcc-source', 'MCC with spaces', '--setup-only')
        self.assertEqual(result.returncode, 23, result.stderr)
        self.assertEqual(self.call('setup'), ['--root', str(self.root), '--mcc-root', 'MCC with spaces'])

    def test_missing_mcc_argument_fails_before_setup(self):
        for args in (('--mcc-source',), ('--mcc-source', '--setup-only')):
            with self.subTest(args=args):
                result = self.run_launcher(*args)
                self.assertEqual(result.returncode, 1)
                self.assertIn('missing value for --mcc-source', result.stderr)
        self.assertFalse((self.root / 'setup-call.json').exists())

    def test_legacy_options_are_rejected_without_reading_sources(self):
        self.legacy_registries()
        for option in ('--vista-source', '--xbox-source', '--gameplay-xbe', '--menu-vista-settings',
                       '--import-mcc-hud', '--import-mcc-menus'):
            with self.subTest(option=option):
                result = self.run_launcher(option, 'old source')
                self.assertEqual(result.returncode, 1)
                self.assertIn('no longer a launcher option', result.stderr)
        self.assertFalse((self.root / 'setup-call.json').exists())

    def test_help_needs_no_assets_and_only_describes_mcc_setup(self):
        self.legacy_registries()
        result = self.run_launcher('--help')
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn('--mcc-source PATH', result.stdout)
        self.assertNotIn('--vista-source', result.stdout)
        self.assertNotIn('--xbox-source', result.stdout)
        self.assertFalse((self.root / 'setup-call.json').exists())

    def test_prepare_alias_selects_cache_and_exits_without_building(self):
        self.ready_assets()
        self.environment['H2_TEST_SETUP_STATUS'] = '0'
        result = self.run_launcher('--prepare-mcc-gameplay', '--mcc-source', 'MCC path')
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(self.call('setup'), ['--root', str(self.root), '--mcc-root', 'MCC path'])
        self.assertFalse((self.root / 'home-call.json').exists())
        self.assertFalse((self.root / 'runtime-call.json').exists())

    def test_prepare_alias_rejects_mixed_gameplay_arguments(self):
        for arg in ('--play', '--update', '--launch-game'):
            result = self.run_launcher('--prepare-mcc-gameplay', arg)
            self.assertEqual(result.returncode, 1)
            self.assertIn('by itself', result.stderr)
        self.assertFalse((self.root / 'setup-call.json').exists())

    def test_prepare_alias_uses_automatic_detection_without_a_window(self):
        result = self.run_launcher('--prepare-mcc-gameplay')
        self.assertEqual(result.returncode, 23)
        self.assertEqual(self.call('setup'), ['--root', str(self.root)])

    def test_inactive_mcc_blocks_runtime_and_shows_honest_home(self):
        self.ready_assets()
        self.runtime()
        self.legacy_registries()
        result = self.run_launcher('--launch-game', '--multiplayer')
        self.assertEqual(result.returncode, 1)
        self.assertIn('conversion are still being completed', result.stderr)
        self.assertFalse((self.root / 'runtime-call.json').exists())
        result = self.run_launcher()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn('--runtime-unavailable', self.call('home'))
        self.assertFalse((self.root / 'setup-call.json').exists())
        result = self.run_launcher('--launcher-info')
        self.assertIn('MCC cache prepared: yes', result.stdout)
        self.assertIn('Native MCC gameplay ready: no', result.stdout)
        self.assertNotIn('Default gameplay XBE:', result.stdout)

    def test_both_readiness_flags_are_required(self):
        self.ready_assets()
        self.runtime()
        for active, parity in ((True, False), (False, True), ('true', True)):
            report = json.loads((self.cache / 'provenance.json').read_text())
            report.update(runtime_active=active, full_tu15_parity=parity)
            (self.cache / 'provenance.json').write_text(json.dumps(report))
            self.assertEqual(self.run_launcher('--launch-game').returncode, 1)
        self.assertFalse((self.root / 'runtime-call.json').exists())

    def test_incomplete_selected_cache_reopens_mcc_setup(self):
        self.ready_assets()
        (self.maps / 'shared.map').unlink()
        result = self.run_launcher()
        self.assertEqual(result.returncode, 23)
        self.assertIn('--interactive', self.call('setup'))

    def test_cache_outside_owned_import_directory_is_rejected(self):
        self.ready_assets()
        (self.root / 'assets/mcc-source.tsv').write_text('H2_MCC_ASSET_SOURCE_V1\ncache\t' + str(self.root) + '\n')
        result = self.run_launcher('--launcher-info')
        self.assertIn('MCC cache prepared: no', result.stdout)
        self.assertIn('private MCC import', result.stdout)

    def test_cache_symlink_is_rejected(self):
        self.ready_assets()
        link = self.cache.parent / 'mcc-link'
        link.symlink_to(self.cache, target_is_directory=True)
        (self.root / 'assets/mcc-source.tsv').write_text('H2_MCC_ASSET_SOURCE_V1\ncache\tassets/imported/mcc-link\n')
        self.assertIn('MCC cache prepared: no', self.run_launcher('--launcher-info').stdout)

    def test_default_route_uses_only_private_mcc_map_paths(self):
        self.ready_assets(True)
        binary = self.runtime()
        self.legacy_registries()
        self.environment['H2_TEST_HOME_STATUS'] = '0'
        result = self.run_launcher()
        self.assertEqual(result.returncode, 0, result.stderr)
        record = self.call('runtime')
        args = record['argv']
        self.assertEqual(record['cwd'], str(self.root))
        self.assertEqual(record['discord'], str(self.launcher))
        self.assertEqual(args[:2], ['--menu-screen', '9'])
        self.assertEqual('--skip-intro' in args, 'Title' in LAUNCHER.name)
        for option, value in (('--menu-maps', self.maps), ('--map', self.maps / 'lockout.map'), ('--shared-maps', self.maps)):
            self.assertEqual(args[args.index(option) + 1], str(value))
        for option in ('--gameplay-xbe', '--menu-vista-settings', '--menu-loading-assets',
                       '--menu-ui-assets', '--hud-ui-assets', '--pause-ui-assets'):
            self.assertNotIn(option, args)
        self.assertNotIn('--runtime-unavailable', self.call('home'))
        self.assertIn(str(binary), self.run_launcher('--launcher-info').stdout)

    def test_explicit_mod_map_and_shared_directory_are_retained(self):
        self.ready_assets(True)
        self.runtime()
        result = self.run_launcher('--launch-game', '--multiplayer', '--map', 'mods/map name.map', '--shared-maps', 'mods/shared')
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(self.call('runtime')['argv'], ['--multiplayer', '--map', 'mods/map name.map', '--shared-maps', 'mods/shared'])

    def test_derived_ui_packs_must_come_from_selected_cache(self):
        self.ready_assets(True)
        self.runtime()
        self.legacy_registries()
        for name in ('pack.tsv', 'hud.tsv', 'pause.tsv'):
            path = self.cache / 'ui' / name
            path.parent.mkdir(exist_ok=True)
            path.touch()
        result = self.run_launcher('--launch-game', '--multiplayer')
        self.assertEqual(result.returncode, 0, result.stderr)
        args = self.call('runtime')['argv']
        for option in ('--menu-ui-assets', '--hud-ui-assets', '--pause-ui-assets'):
            self.assertEqual(args[args.index(option) + 1], str(self.cache / 'ui'))

    def test_update_does_not_require_or_import_game_sources(self):
        (self.root / 'tools/update_runtime.py').write_text('''import pathlib, sys
pathlib.Path(sys.argv[sys.argv.index('--root')+1], 'updated').touch()
''')
        result = self.run_launcher('--update')
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertTrue((self.root / 'updated').exists())
        self.assertFalse((self.root / 'setup-call.json').exists())


if __name__ == '__main__':
    unittest.main()
