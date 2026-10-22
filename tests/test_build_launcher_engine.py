"""Promotion policy tests. Synthetic recorders establish no gameplay parity."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import build_launcher_engine as engine


class EnginePromotionTests(unittest.TestCase):
    def setUp(self):
        self.owner = tempfile.TemporaryDirectory()
        self.addCleanup(self.owner.cleanup)
        self.root = Path(self.owner.name) / 'checkout'
        self.root.mkdir()
        (self.root / 'CMakeLists.txt').write_text('Synthetic policy fixture')
        (self.root / 'tests').mkdir()
        self.cache = self.root / 'assets/imported/mcc-test'
        self.maps = self.cache / 'halo2/h2_maps_win64_dx11'
        self.maps.mkdir(parents=True)
        (self.maps / 'lockout.map').write_bytes(b'synthetic map')
        self.scratch = Path(self.owner.name) / 'scratch'
        self.scratch.mkdir()
        self.runtime = self.scratch / 'recorder'
        self.args = argparse.Namespace(root=self.root, output=self.root / 'reports', jobs=1, timeout=1)
        self.selection = self.root / 'gameplay-runtime-selection.json'
        self.selection.write_text('existing selection retained verbatim')
        self.original_selection = self.selection.read_bytes()
        for name in ('Halo2-Launcher', 'Halo2-Title-Launcher'):
            (self.root / name).write_text('previous launcher')

    def record(self, text):
        self.runtime.write_text('#!' + sys.executable + '\n' + text)
        self.runtime.chmod(0o755)

    def test_exit_success_without_four_submitted_frames_is_rejected(self):
        for text in ('pass', 'print("Multiplayer frames=0 ticks=4")',
                     'print("Multiplayer frames=3 ticks=4")'):
            self.record(text)
            result = engine.smoke(self.runtime, self.maps, 'lockout.map', 1, self.scratch, 1)
            self.assertFalse(result['passed'])

    def test_nonzero_exit_and_timeout_never_qualify_even_with_a_frame_line(self):
        for text, timeout in (('print("Multiplayer frames=4 ticks=4"); raise SystemExit(1)', 1),
                              ('import time; time.sleep(5)', .05)):
            self.record(text)
            self.assertFalse(engine.smoke(self.runtime, self.maps, 'lockout.map', 1,
                                          self.scratch, timeout)['passed'])

    def test_frame_submission_also_requires_advancing_state_and_all_local_players(self):
        for ticks, players, expected in ((0, 4, False), (4, 1, False), (4, 4, True)):
            self.record(f'print("Multiplayer frames=4 ticks={ticks} players=4 connected=1 peak_players={players}")')
            self.assertEqual(engine.smoke(self.runtime, self.maps, 'lockout.map', 4,
                                          self.scratch, 1)['passed'], expected)

    def build(self, command, **kwargs):
        build = self.scratch / 'build'
        build.mkdir(exist_ok=True)
        for name in ('halo2_native', 'Halo2-Launcher', 'Halo2-Title-Launcher'):
            (build / name).write_text('synthetic new binary')
            (build / name).chmod(0o755)
        shader_directory = next(v.split('=', 2)[2] for v in command
                                if v.startswith('--cmake-arg=-DHALO2_RUNTIME_SHADER_DIR='))
        (build / 'halo2_native').write_bytes(b'Synthetic admission input: ' + shader_directory.encode())
        (build / 'shaders').mkdir(exist_ok=True)
        # Use the same shader-name inventory as the actual compiled launcher.
        text = (Path(__file__).resolve().parents[1] / 'src/app/launcher.cpp').read_text()
        block = engine.re.search(r'shader_names\{(.*?)\};', text, engine.re.S)[1]
        for name in engine.re.findall(r'"([^"\n]+\.spv)"', block):
            (build / 'shaders' / name).write_bytes(b'synthetic shader')
        (build / 'engine-targets.txt').write_text('halo2_native|halo2_systems||src/app/main.cpp\n'
                                                'halo2_systems|||src/engine.cpp\n')
        return subprocess.CompletedProcess(command, 0)

    def attempt(self, case, build=None):
        with patch.dict(os.environ, HALO2_TEST_SCRATCH=str(self.scratch)), \
             patch('gameplay_runtime_selection.selected_gameplay_runtime', return_value=None), \
             patch('mcc_onboarding.selected', return_value=(self.cache, {})), \
             patch.object(engine.subprocess, 'run', side_effect=build or self.build), \
             patch.object(engine, 'smoke', return_value=case):
            engine.worker(self.args)

    def test_failed_frame_check_preserves_both_launchers_selection_and_removes_candidate(self):
        with self.assertRaisesRegex(RuntimeError, 'frame check'):
            self.attempt(dict(passed=False, submitted_frames=0))
        self.assertEqual(self.selection.read_bytes(), self.original_selection)
        for name in ('Halo2-Launcher', 'Halo2-Title-Launcher'):
            self.assertEqual((self.root / name).read_text(), 'previous launcher')
        self.assertEqual(list((self.root / 'bin/engine-builds').iterdir()), [])
        report = json.loads((self.args.output / 'report.json').read_text())
        self.assertFalse(report['passed'])
        self.assertEqual(len(report['requested_smoke_cases']), 3)
        self.assertEqual(len(report['smoke']), 3, 'One failed case must not hide the rest of the requested cohort')

    def test_failed_weapon_audio_preserves_selection_and_both_launchers(self):
        self.args.weapon_audio_reference_xbe = self.root / 'reference.xbe'
        self.args.weapon_audio_reference_xbe.write_bytes(b'synthetic reference identity')
        with patch.object(engine, 'weapon_audio', return_value=dict(passed=False)) as audio:
            with self.assertRaisesRegex(RuntimeError, 'weapon audio checks failed'):
                self.attempt(dict(passed=True, submitted_frames=4))
        audio.assert_called_once()
        self.assertEqual(self.selection.read_bytes(), self.original_selection)
        for name in ('Halo2-Launcher', 'Halo2-Title-Launcher'):
            self.assertEqual((self.root / name).read_text(), 'previous launcher')
        self.assertEqual(list((self.root / 'bin/engine-builds').iterdir()), [])
        self.assertFalse(json.loads((self.args.output / 'report.json').read_text())['weapon_audio']['passed'])

    def test_frozen_source_change_cannot_promote_a_mixed_build(self):
        def changed(command, **kwargs):
            result = self.build(command, **kwargs)
            (self.scratch / 'source/CMakeLists.txt').write_text('changed while compiler was running')
            return result
        with self.assertRaisesRegex(RuntimeError, 'source changed'):
            self.attempt(dict(passed=True), changed)
        self.assertEqual(self.selection.read_bytes(), self.original_selection)

    def test_eos_release_refuses_a_missing_runtime_library(self):
        self.args.eos_client_config = self.root / 'private-client.env'
        self.args.eos_client_config.write_text('synthetic shared client input')
        with self.assertRaisesRegex(RuntimeError, 'EOS runtime library'):
            self.attempt(dict(passed=True))
        self.assertEqual(self.selection.read_bytes(), self.original_selection)

    def test_eos_library_and_client_settings_reach_the_runtime_before_smoke(self):
        self.args.eos_client_config = self.root / 'private-client.env'
        self.args.eos_client_config.write_text('synthetic shared client input')
        def build(command, **kwargs):
            self.assertIn('--cmake-arg=-DHALO2_EOS_CLIENT_CONFIG=' + str(self.args.eos_client_config), command)
            result = self.build(command, **kwargs)
            for name in ('libEOSSDK-Linux-Shipping.so', 'eos-client.env'):
                (self.scratch / 'build' / name).write_text('synthetic packaged dependency')
            return result
        def smoke(runtime, *_args, **_kwargs):
            for name in ('libEOSSDK-Linux-Shipping.so', 'eos-client.env'):
                self.assertEqual((runtime.parent / name).read_text(), 'synthetic packaged dependency')
            return dict(passed=False)
        with patch.dict(os.environ, HALO2_TEST_SCRATCH=str(self.scratch)), \
             patch('gameplay_runtime_selection.selected_gameplay_runtime', return_value=None), \
             patch('mcc_onboarding.selected', return_value=(self.cache, {})), \
             patch.object(engine.subprocess, 'run', side_effect=build), \
             patch.object(engine, 'smoke', side_effect=smoke):
            with self.assertRaisesRegex(RuntimeError, 'frame check'):
                engine.worker(self.args)
        self.assertEqual(self.selection.read_bytes(), self.original_selection)

    def test_frozen_build_is_independent_of_later_engine_edits(self):
        destination = self.scratch / 'frozen'
        pins = engine.freeze_source(self.root, destination)
        (self.root / 'CMakeLists.txt').write_text('next engine revision')
        self.assertEqual(engine.source_hashes(destination), pins)
        self.assertNotEqual(engine.source_hashes(self.root), pins)

    def test_another_tasks_selection_is_not_overwritten(self):
        def changed(command, **kwargs):
            result = self.build(command, **kwargs)
            self.selection.write_text('another task selected its runtime')
            return result
        with self.assertRaisesRegex(RuntimeError, 'Another task'):
            self.attempt(dict(passed=True), changed)
        self.assertEqual(self.selection.read_text(), 'another task selected its runtime')

    def test_success_checks_the_actual_admission_reader_before_selecting_both_launchers(self):
        from gameplay_runtime_selection import selected_gameplay_runtime
        def reader(root, **kwargs):
            return selected_gameplay_runtime(root, **kwargs) if kwargs else None
        with patch.dict(os.environ, HALO2_TEST_SCRATCH=str(self.scratch)), \
             patch('gameplay_runtime_selection.selected_gameplay_runtime', side_effect=reader), \
             patch('mcc_onboarding.selected', return_value=(self.cache, {})), \
             patch.object(engine.subprocess, 'run', side_effect=self.build), \
             patch.object(engine, 'smoke', return_value=dict(passed=True, submitted_frames=4)):
            engine.worker(self.args)
        record = json.loads(self.selection.read_text())
        runtime = self.root / record['runtime']
        self.assertEqual(record['runtime_sha256'], engine.digest(runtime))
        for name in ('Halo2-Launcher', 'Halo2-Title-Launcher'):
            self.assertEqual((self.root / name).read_bytes(), (runtime.parent / name).read_bytes())
        self.assertTrue((runtime.parent / 'source-hashes.json').is_file())

    def test_status_ignores_test_only_snapshot_files_and_detects_new_engine_modules(self):
        (self.scratch / 'source-hashes.json').write_text(json.dumps(
            dict(engine.source_hashes(self.root), **{'tests/legacy.cpp': 'old test hash'})))
        self.assertIn('matches', engine.source_status(self.root, self.runtime))
        (self.root / 'src').mkdir()
        (self.root / 'src/new.cpp').write_text('new engine code')
        self.assertIn('1 files differ', engine.source_status(self.root, self.runtime))

    def test_new_source_must_reach_the_native_engine_through_the_link_graph(self):
        (self.root / 'src').mkdir()
        (self.root / 'src/new.cpp').write_text('new engine code')
        graph = self.scratch / 'graph.txt'
        for graph_text, expected in (
                ('halo2_native|||src/app/main.cpp\n', False),
                ('halo2_native|||src/app/main.cpp\nnew_library|||src/new.cpp\n', False),
                ('halo2_native|new_library||src/app/main.cpp\nnew_library|||src/new.cpp\n', True)):
            graph.write_text(graph_text)
            self.assertEqual(engine.wiring(graph, self.root)['passed'], expected)


if __name__ == '__main__':
    unittest.main()
