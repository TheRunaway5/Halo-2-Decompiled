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
        self.launcher_modes = {name: (self.root / name).stat().st_mode & 0o777
                               for name in ('Halo2-Launcher', 'Halo2-Title-Launcher')}

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
        for name in ('halo2_native', 'Halo2-Launcher', 'Halo2-Title-Launcher', 'eos_sdk_worker'):
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

    def attempt(self, case, build=None, *, reader=None):
        with patch.dict(os.environ, HALO2_TEST_SCRATCH=str(self.scratch)), \
             patch('gameplay_runtime_selection.selected_gameplay_runtime', side_effect=reader, return_value=None), \
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

    def test_missing_sdk_worker_cannot_publish_or_reach_smoke(self):
        def missing(command, **kwargs):
            result = self.build(command, **kwargs)
            (self.scratch / 'build/eos_sdk_worker').unlink()
            return result
        with self.assertRaises(FileNotFoundError):
            self.attempt(dict(passed=True), missing)
        self.assertEqual(self.selection.read_bytes(), self.original_selection)
        self.assertEqual(list((self.root / 'bin/engine-builds').iterdir()), [])

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

    def test_another_tasks_launcher_is_not_adopted_as_a_publication_preimage(self):
        def changed(command, **kwargs):
            result = self.build(command, **kwargs)
            (self.root / 'Halo2-Launcher').write_text('another task installed its launcher')
            return result
        with self.assertRaisesRegex(RuntimeError, 'Another task changed publication file'):
            self.attempt(dict(passed=True), changed)
        self.assertEqual((self.root / 'Halo2-Launcher').read_text(), 'another task installed its launcher')
        self.assertEqual(self.selection.read_bytes(), self.original_selection)
        self.assertEqual(list((self.root / 'bin/engine-builds').iterdir()), [])

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
        report = json.loads((self.args.output / 'report.json').read_text())
        self.assertTrue(report['passed'])
        self.assertTrue(report['qualified_candidate'])
        self.assertTrue(report['runtime_installed'])
        self.assertEqual(report['status'], 'published')
        self.assertNotEqual(report['scope'], 'qualified_candidate_only')

    def test_prepare_only_qualifies_retains_candidate_without_root_publication(self):
        from gameplay_runtime_selection import selected_gameplay_runtime
        self.args.prepare_only = True
        (self.maps / 'mainmenu.map').write_bytes(b'synthetic menu')
        (self.root / 'frontend-runtime-selection.json').write_text('previous menu owner')
        names = ('Halo2-Launcher', 'Halo2-Title-Launcher', self.selection.name,
                 'frontend-runtime-selection.json')
        original = {name: engine.publication_identity(self.root / name) for name in names}
        admitted = []

        def reader(root, **kwargs):
            if not kwargs:
                return None
            value = selected_gameplay_runtime(root, **kwargs)
            admitted.append(value)
            return value

        def menu(root, runtime, *_):
            return dict(runtime=runtime.relative_to(root).as_posix(), runtime_sha256=engine.digest(runtime))

        with patch.object(engine, 'publish_runtime_files') as publish, \
             patch('qualify_frontend_runtime.qualify', side_effect=menu):
            self.attempt(dict(passed=True), reader=reader)
        publish.assert_not_called()
        self.assertEqual({name: engine.publication_identity(self.root / name) for name in names}, original)
        self.assertEqual(len(admitted), 1, 'The actual candidate admission reader must still run')
        report = json.loads((self.args.output / 'report.json').read_text())
        self.assertTrue(report['passed'])
        self.assertTrue(report['qualified_candidate'])
        self.assertFalse(report['runtime_installed'])
        self.assertEqual(report['status'], 'prepared')
        self.assertEqual(report['scope'], 'qualified_candidate_only')
        runtime = self.root / report['runtime']
        self.assertTrue(runtime.is_file())
        self.assertEqual(report['runtime_sha256'], engine.digest(runtime))
        for name in ('selection.json', 'qualification.json', 'runtime-files.json',
                     'source-hashes.json', 'frontend-selection.json'):
            self.assertTrue((runtime.parent / name).is_file(), name)
        self.assertEqual(json.loads((runtime.parent / 'qualification.json').read_text()), report)

    def test_prepare_only_preserves_foreign_root_changes_during_validation(self):
        self.args.prepare_only = True
        (self.maps / 'mainmenu.map').write_bytes(b'synthetic menu')
        frontend = self.root / 'frontend-runtime-selection.json'
        frontend.write_text('previous menu owner')

        def changed(root, runtime, *_):
            self.selection.write_text('foreign gameplay owner')
            frontend.write_text('foreign frontend owner')
            (self.root / 'Halo2-Launcher').write_text('foreign launcher owner')
            return dict(runtime=runtime.relative_to(root).as_posix(), runtime_sha256=engine.digest(runtime))

        with patch('qualify_frontend_runtime.qualify', side_effect=changed):
            self.attempt(dict(passed=True))
        self.assertEqual(self.selection.read_text(), 'foreign gameplay owner')
        self.assertEqual(frontend.read_text(), 'foreign frontend owner')
        self.assertEqual((self.root / 'Halo2-Launcher').read_text(), 'foreign launcher owner')
        self.assertEqual((self.root / 'Halo2-Title-Launcher').read_text(), 'previous launcher')
        report = json.loads((self.args.output / 'report.json').read_text())
        self.assertTrue(report['qualified_candidate'])
        self.assertFalse(report['runtime_installed'])
        self.assertTrue((self.root / report['runtime']).is_file())

    def test_prepare_only_failed_frames_remove_unqualified_candidate(self):
        self.args.prepare_only = True
        with self.assertRaisesRegex(RuntimeError, 'frame check'):
            self.attempt(dict(passed=False))
        self.assertEqual(self.selection.read_bytes(), self.original_selection)
        self.assertEqual((self.root / 'Halo2-Launcher').read_text(), 'previous launcher')
        self.assertEqual(list((self.root / 'bin/engine-builds').iterdir()), [])
        report = json.loads((self.args.output / 'report.json').read_text())
        self.assertFalse(report['passed'])
        self.assertFalse(report['qualified_candidate'])
        self.assertFalse(report['runtime_installed'])
        self.assertEqual(report['status'], 'failed')

    def test_prepare_only_reader_rejection_removes_candidate(self):
        self.args.prepare_only = True

        def reject(root, **kwargs):
            if kwargs:
                raise ValueError('candidate admission rejected')
            return None

        with self.assertRaisesRegex(ValueError, 'candidate admission rejected'):
            self.attempt(dict(passed=True), reader=reject)
        self.assertEqual(self.selection.read_bytes(), self.original_selection)
        self.assertEqual(list((self.root / 'bin/engine-builds').iterdir()), [])
        report = json.loads((self.args.output / 'report.json').read_text())
        self.assertFalse(report['passed'])
        self.assertFalse(report['qualified_candidate'])
        self.assertFalse(report['runtime_installed'])

    def test_cache_candidate_preparation_uses_real_reader_without_selecting_cache(self):
        from gameplay_runtime_selection import selected_gameplay_runtime
        import mcc_onboarding
        self.args.prepare_only = True
        candidate = self.root / 'assets/imported/mcc-candidate'
        maps = candidate / 'halo2/h2_maps_win64_dx11'
        maps.mkdir(parents=True)
        for name in ('lockout.map', 'coagulation.map', 'mainmenu.map'):
            (maps / name).write_bytes(('qualified candidate ' + name).encode())
        self.args.cache_candidate = candidate
        registry = self.root / mcc_onboarding.REGISTRY
        registry.write_text(mcc_onboarding.SCHEMA + '\ncache\t' + self.cache.relative_to(self.root).as_posix() + '\n')
        (self.root / 'frontend-runtime-selection.json').write_text('previous menu owner')
        names = ('Halo2-Launcher', 'Halo2-Title-Launcher', self.selection.name,
                 'frontend-runtime-selection.json', mcc_onboarding.REGISTRY)
        before = {name: engine.publication_identity(self.root / name) for name in names}
        prior = dict(maps={'lockout.map': 'old', 'coagulation.map': 'old'},
                     default_map='coagulation.map', local_players=3, video_driver='x11')

        def reader(root, **kwargs):
            return selected_gameplay_runtime(root, **kwargs) if kwargs else prior

        def menu(root, runtime, chosen, *_):
            self.assertEqual(chosen, candidate)
            return dict(runtime=runtime.relative_to(root).as_posix(), runtime_sha256=engine.digest(runtime))

        with patch('mcc_onboarding.private_cache', return_value=(candidate, {})) as verify, \
             patch('qualify_frontend_runtime.qualify', side_effect=menu), \
             patch.object(engine, 'publish_runtime_files') as publish:
            self.attempt(dict(passed=True), reader=reader)
        publish.assert_not_called()
        self.assertEqual({name: engine.publication_identity(self.root / name) for name in names}, before)
        self.assertEqual(verify.call_count, 2)
        self.assertTrue(all(call.kwargs['verify'] is True for call in verify.call_args_list))
        report = json.loads((self.args.output / 'report.json').read_text())
        selection = self.root / report['runtime']
        record_path = selection.parent / 'selection.json'
        record = json.loads(record_path.read_text())
        self.assertEqual(record['cache'], candidate.relative_to(self.root).as_posix())
        self.assertEqual(set(record['maps']), set(prior['maps']))
        for name in ('default_map', 'local_players', 'video_driver'):
            self.assertEqual(record[name], prior[name])
        self.assertIn(dict(map='coagulation.map', local_players=3), report['requested_smoke_cases'])
        self.assertIn('eos_worker', record)
        with patch('mcc_onboarding.selected', return_value=(self.cache, {})):
            with self.assertRaisesRegex(ValueError, 'qualified prepared MCC cache'):
                selected_gameplay_runtime(self.root, selection_name=record_path.relative_to(self.root).as_posix())
        with patch('mcc_onboarding.private_cache', return_value=(self.cache, {})):
            with self.assertRaisesRegex(ValueError, 'qualified prepared MCC cache'):
                selected_gameplay_runtime(self.root, selection_name=record_path.relative_to(self.root).as_posix(),
                                          cache_candidate=self.cache)

    def test_invalid_cache_candidate_does_not_build_or_publish(self):
        self.args.cache_candidate = self.root / 'unverified-cache'
        with patch('mcc_onboarding.private_cache', side_effect=ValueError('cache verification failed')), \
             patch.object(engine, 'publish_runtime_files') as publish:
            with self.assertRaisesRegex(ValueError, 'cache verification failed'):
                self.attempt(dict(passed=True))
        publish.assert_not_called()
        self.assertEqual(self.selection.read_bytes(), self.original_selection)

    def test_cache_candidate_publication_passes_the_real_normal_reader(self):
        from gameplay_runtime_selection import selected_gameplay_runtime
        import mcc_onboarding
        candidate = self.root / 'assets/imported/mcc-candidate'
        maps = candidate / 'halo2/h2_maps_win64_dx11'
        maps.mkdir(parents=True)
        for name in ('lockout.map', 'mainmenu.map'):
            (maps / name).write_bytes(('qualified candidate ' + name).encode())
        self.args.cache_candidate = candidate
        registry = self.root / mcc_onboarding.REGISTRY
        registry.write_text(mcc_onboarding.SCHEMA + '\ncache\t' + self.cache.relative_to(self.root).as_posix() + '\n')
        (self.root / 'frontend-runtime-selection.json').write_text('previous menu owner')
        reads = []

        def reader(root, **kwargs):
            reads.append(kwargs)
            # The synthetic previous selector intentionally has no admitted
            # runtime; all candidate and final admissions use the real reader.
            return None if len(reads) == 1 else selected_gameplay_runtime(root, **kwargs)

        def menu(root, runtime, chosen, *_):
            self.assertEqual(chosen, candidate)
            return dict(runtime=runtime.relative_to(root).as_posix(), runtime_sha256=engine.digest(runtime))

        def verified(root, chosen, **kwargs):
            self.assertIn(chosen, (self.cache, candidate))
            return chosen, {}

        with patch.dict(os.environ, HALO2_TEST_SCRATCH=str(self.scratch)), \
             patch('gameplay_runtime_selection.selected_gameplay_runtime', side_effect=reader), \
             patch('mcc_onboarding.private_cache', side_effect=verified), \
             patch('qualify_frontend_runtime.qualify', side_effect=menu), \
             patch.object(engine.subprocess, 'run', side_effect=self.build), \
             patch.object(engine, 'smoke', return_value=dict(passed=True, submitted_frames=4)):
            engine.worker(self.args)
            record = selected_gameplay_runtime(self.root)
        self.assertEqual(record['cache'], candidate.relative_to(self.root).as_posix())
        self.assertEqual(reads[-1], {}, 'Final admission must use the normal selected-cache reader')
        self.assertEqual(registry.read_text(), mcc_onboarding.SCHEMA + '\ncache\t' + record['cache'] + '\n')
        report = json.loads((self.args.output / 'report.json').read_text())
        self.assertTrue(report['passed'])
        self.assertTrue(report['publication']['complete'])
        self.assertEqual(report['publication']['files'][-1]['name'], mcc_onboarding.REGISTRY)

    def publication_fault(self, target_name, error=OSError, foreign=None):
        (self.maps / 'mainmenu.map').write_bytes(b'synthetic menu')
        (self.root / 'frontend-runtime-selection.json').write_text('previous menu owner')
        replace = os.replace
        injected = False

        def menu(root, runtime, *_):
            return dict(runtime=runtime.relative_to(root).as_posix(), runtime_sha256=engine.digest(runtime))

        def fail_after_replace(source, target):
            nonlocal injected
            replace(source, target)
            if Path(target) == self.root / target_name and not injected:
                injected = True
                if foreign:
                    foreign(replace)
                raise error('injected publication failure')

        with patch.object(engine.os, 'replace', side_effect=fail_after_replace), \
             patch('qualify_frontend_runtime.qualify', side_effect=menu):
            with self.assertRaisesRegex(error, 'injected publication failure'):
                self.attempt(dict(passed=True))
        self.assertTrue(injected)
        return json.loads((self.args.output / 'report.json').read_text())

    def assert_publication_restored(self, report):
        self.assertFalse(report['passed'])
        self.assertEqual(self.selection.read_bytes(), self.original_selection)
        self.assertEqual((self.root / 'frontend-runtime-selection.json').read_text(), 'previous menu owner')
        for name in ('Halo2-Launcher', 'Halo2-Title-Launcher'):
            self.assertEqual((self.root / name).read_text(), 'previous launcher')
            self.assertEqual((self.root / name).stat().st_mode & 0o777, self.launcher_modes[name])
        self.assertFalse(report['publication']['rollback_failed'])
        self.assertFalse(report['publication']['candidate_retained'])
        self.assertEqual(list((self.root / 'bin/engine-builds').iterdir()), [])

    def test_interruption_after_first_launcher_restores_its_preimage(self):
        self.assert_publication_restored(self.publication_fault('Halo2-Launcher', SystemExit))

    def test_failure_after_second_launcher_restores_both_preimages(self):
        self.assert_publication_restored(self.publication_fault('Halo2-Title-Launcher'))

    def test_failure_after_gameplay_selector_restores_launchers_and_selector(self):
        self.assert_publication_restored(self.publication_fault('gameplay-runtime-selection.json'))

    def test_failure_after_frontend_selector_restores_the_whole_publication(self):
        self.assert_publication_restored(self.publication_fault('frontend-runtime-selection.json'))

    def test_foreign_same_bytes_launcher_is_preserved_and_its_candidate_retained(self):
        foreign_identity = None

        def foreign(replace):
            nonlocal foreign_identity
            target = self.root / 'Halo2-Launcher'
            temporary = self.root / 'foreign-launcher'
            temporary.write_bytes(target.read_bytes())
            temporary.chmod(0o755)
            replace(temporary, target)
            foreign_identity = engine.publication_identity(target)

        report = self.publication_fault('Halo2-Title-Launcher', foreign=foreign)
        self.assertEqual(engine.publication_identity(self.root / 'Halo2-Launcher'), foreign_identity)
        self.assertEqual((self.root / 'Halo2-Title-Launcher').read_text(), 'previous launcher')
        self.assertEqual(self.selection.read_bytes(), self.original_selection)
        self.assertTrue((self.root / report['runtime']).is_file())
        self.assertTrue(report['publication']['candidate_retained'])
        self.assertIn(dict(name='Halo2-Launcher', status='candidate_reference'),
                      report['publication']['candidate_references'])
        self.assertEqual(report['publication']['files'][0]['outcome'], 'foreign_or_missing_preserved')

    def test_foreign_selector_referencing_candidate_is_preserved_with_candidate(self):
        foreign_bytes = None

        def foreign(replace):
            nonlocal foreign_bytes
            value = json.loads(self.selection.read_text())
            value['foreign_owner'] = True
            temporary = self.root / 'foreign-selection'
            temporary.write_text(json.dumps(value))
            replace(temporary, self.selection)
            foreign_bytes = self.selection.read_bytes()

        report = self.publication_fault('gameplay-runtime-selection.json', foreign=foreign)
        self.assertEqual(self.selection.read_bytes(), foreign_bytes)
        self.assertEqual((self.root / 'Halo2-Launcher').read_text(), 'previous launcher')
        self.assertTrue((self.root / report['runtime']).is_file())
        self.assertTrue(report['publication']['candidate_retained'])
        self.assertIn(dict(name='gameplay-runtime-selection.json', status='candidate_reference'),
                      report['publication']['candidate_references'])

    def test_failed_rollback_never_deletes_the_still_referenced_candidate(self):
        replace = os.replace
        attempted = 0

        def fail_root_launcher(source, target):
            nonlocal attempted
            if Path(target) == self.root / 'Halo2-Launcher':
                attempted += 1
                if attempted == 1:
                    replace(source, target)
                    raise OSError('injected publication failure')
                raise PermissionError('injected rollback failure')
            replace(source, target)

        with patch.object(engine.os, 'replace', side_effect=fail_root_launcher):
            with self.assertRaisesRegex(OSError, 'injected publication failure'):
                self.attempt(dict(passed=True))
        report = json.loads((self.args.output / 'report.json').read_text())
        self.assertEqual(attempted, 2)
        self.assertTrue(report['publication']['rollback_failed'])
        self.assertTrue(report['publication']['candidate_retained'])
        self.assertTrue((self.root / report['runtime']).is_file())
        self.assertEqual(self.selection.read_bytes(), self.original_selection)
        self.assertEqual((self.root / 'Halo2-Title-Launcher').read_text(), 'previous launcher')

    def test_publication_identity_ignores_read_access_time_only(self):
        path = self.root / 'Halo2-Launcher'
        before = path.lstat()
        values = list(before)
        values[7] += 1  # st_atime advances on a read; all content metadata stays fixed.
        after = os.stat_result(values, dict(st_atime_ns=before.st_atime_ns + 1_000_000_000,
                                           st_mtime_ns=before.st_mtime_ns, st_ctime_ns=before.st_ctime_ns))
        self.assertNotEqual(before, after)
        with patch.object(Path, 'lstat', side_effect=[before, after]):
            identity = engine.publication_identity(path)
        self.assertEqual(identity[-1], engine.digest(path))
        self.assertEqual(identity[:5], (before.st_dev, before.st_ino, before.st_size,
                                      before.st_mtime_ns, before.st_mode & 0o777))

    def test_menu_failure_preserves_the_installed_launcher_and_selection(self):
        (self.maps / 'mainmenu.map').write_bytes(b'synthetic menu')
        with patch('qualify_frontend_runtime.qualify', side_effect=RuntimeError('menu check failed')):
            with self.assertRaisesRegex(RuntimeError, 'menu check failed'):
                self.attempt(dict(passed=True))
        self.assertEqual(self.selection.read_bytes(), self.original_selection)
        self.assertEqual((self.root / 'Halo2-Launcher').read_text(), 'previous launcher')
        self.assertEqual(list((self.root / 'bin/engine-builds').iterdir()), [])

    def test_future_engine_builds_select_the_same_qualified_menu_owner(self):
        (self.maps / 'mainmenu.map').write_bytes(b'synthetic menu')
        frontend = self.root / 'frontend-runtime-selection.json'
        frontend.write_text('previous menu owner')
        def menu(root, runtime, *_):
            return dict(runtime=runtime.relative_to(root).as_posix(), runtime_sha256=engine.digest(runtime))
        with patch('qualify_frontend_runtime.qualify', side_effect=menu):
            self.attempt(dict(passed=True))
        game = json.loads(self.selection.read_text())
        menu = json.loads(frontend.read_text())
        self.assertEqual(menu['runtime'], game['runtime'])
        self.assertEqual(menu['runtime_sha256'], game['runtime_sha256'])

    def test_concurrent_menu_selection_is_preserved_during_engine_validation(self):
        (self.maps / 'mainmenu.map').write_bytes(b'synthetic menu')
        frontend = self.root / 'frontend-runtime-selection.json'
        frontend.write_text('previous menu owner')
        def changed(*_):
            frontend.write_text('another task selected its menu')
            return dict(runtime='candidate')
        with patch('qualify_frontend_runtime.qualify', side_effect=changed):
            with self.assertRaisesRegex(RuntimeError, 'Another task changed the frontend'):
                self.attempt(dict(passed=True))
        self.assertEqual(frontend.read_text(), 'another task selected its menu')
        self.assertEqual(self.selection.read_bytes(), self.original_selection)

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


class CachePublicationTests(unittest.TestCase):
    def setUp(self):
        fixture = EnginePromotionTests()
        fixture.setUp()
        self.addCleanup(fixture.doCleanups)
        self.root, self.scratch = fixture.root, fixture.scratch
        self.destination = self.root / 'bin/engine-builds/candidate'
        self.destination.mkdir(parents=True)
        for name in ('Halo2-Launcher', 'Halo2-Title-Launcher'):
            (self.destination / name).write_text('new launcher')
        (self.root / 'frontend-runtime-selection.json').write_text('previous menu')
        self.registry = self.root / 'assets/mcc-source.tsv'
        self.registry.write_text('H2_MCC_ASSET_SOURCE_V1\ncache\tassets/imported/mcc-test\n')
        self.candidate = self.root / 'assets/imported/mcc-candidate'
        self.candidate.mkdir()
        self.record = dict(runtime='bin/engine-builds/candidate/halo2_native',
                           cache=self.candidate.relative_to(self.root).as_posix())
        self.frontend = dict(runtime=self.record['runtime'])
        self.names = ('Halo2-Launcher', 'Halo2-Title-Launcher', 'gameplay-runtime-selection.json',
                      'frontend-runtime-selection.json', 'assets/mcc-source.tsv')
        self.previous = {name: (self.root / name).read_bytes() for name in self.names}
        self.baseline = {name: engine.publication_identity(self.root / name) for name in self.names}
        self.report = {}

    def publish(self, reader=None):
        def admitted(root):
            self.assertEqual(self.registry.read_text(),
                'H2_MCC_ASSET_SOURCE_V1\ncache\tassets/imported/mcc-candidate\n')
            return json.loads((root / 'gameplay-runtime-selection.json').read_text())
        with patch('mcc_onboarding.private_cache', return_value=(self.candidate, {})) as verify, \
             patch('gameplay_runtime_selection.selected_gameplay_runtime', side_effect=reader or admitted) as admission:
            engine.publish_runtime_files(self.root, self.destination, self.scratch, self.record,
                                         self.frontend, self.report, self.baseline, cache_choice=self.candidate)
        verify.assert_called_once_with(self.root, self.candidate, verify=True)
        admission.assert_called_once_with(self.root)

    def restored(self):
        self.assertEqual({name: (self.root / name).read_bytes() for name in self.names}, self.previous)
        self.assertFalse(self.report['publication']['complete'])
        self.assertFalse(self.report['publication']['rollback_failed'])

    def test_success_publishes_raw_tsv_fifth_and_checks_normal_reader(self):
        self.publish()
        self.assertTrue(self.report['publication']['complete'])
        self.assertEqual([row['name'] for row in self.report['publication']['files']], list(self.names))
        self.assertEqual(json.loads((self.root / 'gameplay-runtime-selection.json').read_text()), self.record)

    def test_fifth_file_failure_before_and_after_committed_rename_rolls_back_all_files(self):
        replace = os.replace
        for committed in (False, True):
            with self.subTest(committed=committed):
                self.baseline = {name: engine.publication_identity(self.root / name) for name in self.names}
                injected = False

                def fail(source, target):
                    nonlocal injected
                    if Path(target) == self.registry and not injected:
                        injected = True
                        if committed:
                            replace(source, target)
                        raise OSError('fifth rename failed')
                    replace(source, target)

                with patch.object(engine.os, 'replace', side_effect=fail):
                    with self.assertRaisesRegex(OSError, 'fifth rename failed'):
                        self.publish()
                self.assertTrue(injected)
                self.restored()

    def test_foreign_registry_inode_survives_rollback_even_with_candidate_bytes(self):
        replace = os.replace
        foreign_identity = None
        injected = False

        def fail(source, target):
            nonlocal foreign_identity, injected
            replace(source, target)
            if Path(target) == self.registry and not injected:
                injected = True
                foreign = self.registry.with_name('foreign-choice')
                foreign.write_bytes(self.registry.read_bytes())
                replace(foreign, self.registry)
                foreign_identity = engine.publication_identity(self.registry)
                raise OSError('fifth rename interrupted')

        with patch.object(engine.os, 'replace', side_effect=fail):
            with self.assertRaisesRegex(OSError, 'fifth rename interrupted'):
                self.publish()
        self.assertEqual(engine.publication_identity(self.registry), foreign_identity)
        for name in self.names[:-1]:
            self.assertEqual((self.root / name).read_bytes(), self.previous[name])
        self.assertEqual(self.report['publication']['files'][-1]['outcome'], 'foreign_or_missing_preserved')

    def test_changed_registry_baseline_prevents_any_publication(self):
        self.registry.write_text('foreign cache choice')
        with self.assertRaisesRegex(RuntimeError, 'Another task changed publication file: assets/mcc-source.tsv'):
            self.publish()
        self.assertEqual(self.registry.read_text(), 'foreign cache choice')
        for name in self.names[:-1]:
            self.assertEqual((self.root / name).read_bytes(), self.previous[name])

    def test_failed_normal_reader_rolls_back_all_five_files(self):
        def reject(root):
            raise ValueError('normal admission failed')
        with self.assertRaisesRegex(ValueError, 'normal admission failed'):
            self.publish(reject)
        self.restored()


if __name__ == '__main__':
    unittest.main()
