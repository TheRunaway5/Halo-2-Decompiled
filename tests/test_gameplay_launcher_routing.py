"""Actual compiled launcher routing; synthetic files grant no gameplay parity."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path[:0] = [str(ROOT/'tools'), str(ROOT/'tests')]
import gameplay_runtime_selection as selection
import test_frontend_launcher_routing as prior


class GameplayLauncherRoutingTests(unittest.TestCase):
    def setUp(self):
        base = prior.FrontendLauncherRoutingTests()
        base.setUp()
        self.addCleanup(base.doCleanups)
        self.base = base
        self.root, self.cache, self.maps = base.root, base.cache, base.maps
        tools = self.root/'tools'
        shutil.copyfile(ROOT/'tools/gameplay_runtime_selection.py', tools/'gameplay_runtime_selection.py')
        (tools/'mcc_onboarding.py').write_text('from pathlib import Path\ndef selected(root):\n return Path(root)/'+repr(self.cache.relative_to(self.root).as_posix())+', {}\n')
        self.runtime = self.root/'delivery/halo2_native'
        self.runtime.parent.mkdir()
        shaders = self.runtime.parent/'shaders'
        shutil.copytree(base.game_shaders, shaders)
        base.capture(self.runtime, shaders, 'selected-gameplay')
        self.runtime.write_text(self.runtime.read_text().replace(
            'import json,sys', 'import json,sys,os').replace(
            'argv=sys.argv)', 'argv=sys.argv,video_driver=os.environ.get("SDL_VIDEODRIVER"))'))
        for name in ('coagulation.map', 'beavercreek.map'):
            (self.maps/name).write_bytes(('Synthetic route input '+name).encode())
        self.proof = self.root/'qualified.json'
        self.pin = base.f.digest(self.runtime)
        self.proof.write_text(json.dumps(dict(passed=True, runtime='delivery/halo2_native',
            runtime_sha256=self.pin, unresolved_case_failures=[])))
        self.inventory = self.runtime.parent/'runtime-files.json'
        self.inventory.write_text(json.dumps({
            'shaders/'+p.name:dict(sha256=base.f.digest(p)) for p in shaders.iterdir()}))
        self.record = dict(schema=selection.SCHEMA, runtime='delivery/halo2_native', runtime_sha256=self.pin,
            qualification='qualified.json', qualification_sha256=base.f.digest(self.proof),
            inventory='delivery/runtime-files.json', inventory_sha256=base.f.digest(self.inventory),
            cache=self.cache.relative_to(self.root).as_posix(), default_map='coagulation.map', local_players=2,
            maps={name:base.f.digest(self.maps/name) for name in ('coagulation.map','beavercreek.map')},
            video_driver='x11')
        self.manifest = self.root/selection.SELECTION
        self.publish()
        ui = self.cache/'ui'
        ui.mkdir()
        for name in ('pack.tsv', 'hud.tsv', 'pause.tsv'):
            (ui/name).write_text('Unqualified menu-only sampling input\n')

    def publish(self):
        self.manifest.write_text(json.dumps(self.record))

    def run_launcher(self, *args):
        return subprocess.run([str(self.base.launcher), *args], cwd=self.root,
            env=self.base.env, capture_output=True, text=True, timeout=30)

    def routed(self, *args):
        result = self.run_launcher(*args)
        self.assertEqual(result.returncode, 0, result.stdout+result.stderr)
        rows = [json.loads(line[7:]) for line in result.stdout.splitlines() if line.startswith('ROUTED ')]
        self.assertEqual(len(rows), 1)
        self.assertEqual(rows[0]['owner'], 'selected-gameplay')
        return rows[0]['argv']

    def test_play_opens_intro_and_menus_even_when_multiplayer_is_qualified(self):
        # Menu assets must belong to the admitted frontend, just as at launch.
        for path in (self.cache/'ui').iterdir(): path.unlink()
        (self.cache/'ui').rmdir()
        result = self.run_launcher('--launch-game', '--frames', '6', '--mute')
        self.assertEqual(result.returncode, 0, result.stderr)
        row = next(json.loads(line[7:]) for line in result.stdout.splitlines() if line.startswith('ROUTED '))
        self.assertEqual(row['owner'], 'frontend')
        self.assertIn('--menu-screen', row['argv'])
        self.assertNotIn('--skip-intro', row['argv'])
        self.assertNotIn('--multiplayer', row['argv'])

    def test_explicit_multiplayer_uses_qualified_defaults(self):
        argv = self.routed('--multiplayer', '--frames', '6', '--mute')
        self.assertEqual(argv[0], str(self.runtime))
        self.assertEqual(argv[argv.index('--map')+1], str(self.maps/'coagulation.map'))
        self.assertEqual(argv[argv.index('--local-players')+1], '2')
        self.assertNotIn('--menu-screen', argv)
        self.assertNotIn('--hud-ui-assets', argv)

    def test_explicit_map_and_split_count_reach_the_selected_owner(self):
        argv = self.routed('--multiplayer', '--map', 'beavercreek.map', '--local-players', '4')
        self.assertEqual(argv.count('--map'), 1)
        self.assertEqual(argv[argv.index('--map')+1], str(self.maps/'beavercreek.map'))
        self.assertEqual(argv[argv.index('--local-players')+1], '4')

    def test_info_names_gameplay_owner_without_granting_frontend_or_full_parity(self):
        result = self.run_launcher('--launcher-info')
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn('Runtime: '+str(self.runtime), result.stdout)
        self.assertIn('Play: unavailable', result.stdout)
        self.assertIn('Native MCC gameplay ready: no', result.stdout)
        self.assertNotIn('ROUTED ', result.stdout)

    def test_explicit_menu_keeps_its_independently_admitted_frontend(self):
        for path in (self.cache/'ui').iterdir():
            path.unlink()
        (self.cache/'ui').rmdir()
        result = self.run_launcher('--menu-screen', '9', '--frames', '1')
        self.assertEqual(result.returncode, 0, result.stderr)
        row = next(json.loads(line[7:]) for line in result.stdout.splitlines() if line.startswith('ROUTED '))
        self.assertEqual(row['owner'], 'frontend')
        self.assertNotIn('--multiplayer', row['argv'])

    def test_changed_owner_proof_shader_and_map_fail_without_old_runtime_fallback(self):
        for path in (self.runtime, self.proof, self.runtime.parent/'shaders/world.frag.spv', self.maps/'coagulation.map'):
            with self.subTest(path=path):
                old = path.read_bytes()
                path.write_bytes(old+b'changed')
                result = self.run_launcher('--multiplayer')
                self.assertNotEqual(result.returncode, 0)
                self.assertNotIn('ROUTED ', result.stdout)
                path.write_bytes(old)

    def test_unsafe_selection_paths_and_unqualified_arguments_reject(self):
        for name in ('../escape', str(self.runtime), 'delivery/../delivery/halo2_native'):
            with self.subTest(name=name):
                self.manifest.write_text(json.dumps(dict(self.record, runtime=name)))
                self.assertNotEqual(self.run_launcher('--multiplayer').returncode, 0)
        self.publish()
        for args in (('--map', str(self.root/'foreign.map')), ('--campaign',),
                     ('--gameplay-xbe', '/original/default.xbe'),
                     ('--map', 'coagulation.map', '--map', 'beavercreek.map')):
            with self.subTest(args=args):
                result = self.run_launcher('--multiplayer', *args)
                self.assertNotEqual(result.returncode, 0)
                self.assertNotIn('ROUTED ', result.stdout)

    @unittest.skipUnless(sys.platform.startswith('linux'), 'Linux display preference')
    def test_display_preference_is_applied_and_explicit_override_is_preserved(self):
        self.base.env.pop('SDL_VIDEODRIVER', None)
        for override, expected in ((None, 'x11'), ('dummy', 'dummy')):
            if override is not None:
                self.base.env['SDL_VIDEODRIVER'] = override
            result = self.run_launcher('--multiplayer', '--frames', '1')
            self.assertEqual(result.returncode, 0, result.stderr)
            row = next(json.loads(line[7:]) for line in result.stdout.splitlines() if line.startswith('ROUTED '))
            self.assertEqual(row['video_driver'], expected)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--launcher', type=Path, required=True)
    args, remaining = parser.parse_known_args()
    prior.LAUNCHER = args.launcher.resolve(strict=True)
    unittest.main(argv=[sys.argv[0], *remaining])
