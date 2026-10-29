"""Failure controls for renderer parity admission; no actual GPU is involved."""
import contextlib
import copy
import hashlib
import io
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools' / 'perf_4k60'))
import parity


def image(value=0, size=(2, 2)):
    return f'P6\n{size[0]} {size[1]}\n255\n'.encode() + bytes([value]) * (size[0] * size[1] * 3)


def run(role, value=0, valid=True):
    payload = image(value)
    return dict(role=role, valid=valid, exit_code=0, seconds=0,
                sha256=hashlib.sha256(payload).hexdigest(), _payload=payload)


class ParityAdmissionTests(unittest.TestCase):
    def compare(self, *runs):
        entry = dict(case='4:90:0:0', runs=list(runs))
        with contextlib.redirect_stdout(io.StringIO()):
            parity.compare_runs(entry)
        # Reports never retain raw frame data.
        json.dumps(entry)
        return entry

    def test_valid_equal_captures_and_repeats_pass(self):
        entry = self.compare(run('reference'), run('reference-repeat'), run('candidate'), run('candidate-repeat'))
        self.assertTrue(entry['passed'])

    def test_matching_invalid_images_cannot_establish_parity(self):
        entry = self.compare(run('reference', valid=False), run('reference-repeat', valid=False), run('candidate', valid=False))
        self.assertTrue(entry['identical'])
        self.assertFalse(entry['passed'])

    def test_reference_repeat_mismatch_fails_matching_candidate(self):
        entry = self.compare(run('reference'), run('reference-repeat', 1), run('candidate'))
        self.assertTrue(entry['identical'])
        self.assertFalse(entry['reference_deterministic'])
        self.assertFalse(entry['passed'])

    def test_candidate_repeat_mismatch_fails(self):
        entry = self.compare(run('reference'), run('reference-repeat'), run('candidate'), run('candidate-repeat', 1))
        self.assertTrue(entry['identical'])
        self.assertFalse(entry['candidate_deterministic'])
        self.assertFalse(entry['passed'])

    def test_candidate_difference_retains_pixel_diagnostics(self):
        entry = self.compare(run('reference'), run('reference-repeat'), run('candidate', 1))
        self.assertFalse(entry['passed'])
        self.assertEqual(entry['difference']['candidate']['pixels'], 4)

    def test_ppm_requires_native_dimensions_complete_rgb_and_valid_header(self):
        self.assertIsNone(parity.validate_image(image(), (2, 2)))
        for invalid in (image(size=(1, 1)), image()[:-1], image() + b'X',
                        image().replace(b'P6', b'P3'), image().replace(b'255', b'256'), b'garbage'):
            with self.subTest(invalid=invalid):
                self.assertIsNotNone(parity.validate_image(invalid, (2, 2)))
        self.assertIsNotNone(parity.validate_image(image()))

    def test_main_returns_nonzero_and_saves_failed_repeat_report(self):
        with tempfile.TemporaryDirectory() as directory:
            report = Path(directory) / 'report.json'
            captures = [run('reference'), run('reference-repeat', 1), run('candidate')]
            with patch.dict(os.environ, {'HALO2_TEST_SCRATCH': directory, 'H2_REPEAT_CANDIDATE': ''}), \
                 patch.object(parity, 'capture', side_effect=copy.deepcopy(captures)), \
                 contextlib.redirect_stdout(io.StringIO()):
                code = parity.main([str(report), 'reference', 'candidate', '4:90:0:0'])
            self.assertEqual(code, 1)
            self.assertFalse(json.loads(report.read_text())['passed'])

    def test_main_requests_candidate_repeat_and_returns_success(self):
        with tempfile.TemporaryDirectory() as directory:
            report = Path(directory) / 'report.json'
            captures = [run('reference'), run('reference-repeat'), run('candidate'), run('candidate-repeat')]
            with patch.dict(os.environ, {'HALO2_TEST_SCRATCH': directory, 'H2_REPEAT_CANDIDATE': '1'}), \
                 patch.object(parity, 'capture', side_effect=copy.deepcopy(captures)) as capture, \
                 contextlib.redirect_stdout(io.StringIO()):
                code = parity.main([str(report), 'reference', 'candidate', '4:90:0:0'])
            self.assertEqual(code, 0)
            self.assertEqual(capture.call_count, 4)
            self.assertTrue(json.loads(report.read_text())['passed'])

    def test_main_forwards_explicit_or_environment_maps_and_pins_selected_file(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            env_maps, explicit_maps = root / 'environment-cache', root / 'explicit-cache'
            for maps in (env_maps, explicit_maps):
                maps.mkdir(); (maps / 'fixture.map').write_bytes(maps.name.encode())
            for explicit in (False, True):
                selected = explicit_maps if explicit else env_maps
                report = root / 'report.json'
                argv = [str(report), 'reference', 'candidate']
                if explicit:
                    argv.extend(['--maps', str(explicit_maps)])
                argv.append('4:90:0:0')
                captures = [run('reference'), run('reference-repeat'), run('candidate')]
                with patch.dict(os.environ, {'HALO2_TEST_SCRATCH': directory, 'H2_REPEAT_CANDIDATE': '',
                                             'H2_MAPS': str(env_maps), 'H2_MAP': 'fixture'}), \
                     patch.object(parity, 'capture', side_effect=captures) as capture, \
                     contextlib.redirect_stdout(io.StringIO()):
                    self.assertEqual(parity.main(argv), 0)
                actual = json.loads(report.read_text())
                self.assertEqual(actual['maps'], str(selected.resolve()))
                self.assertEqual(actual['map_path'], str(selected / 'fixture.map'))
                self.assertEqual(actual['map_sha256'], hashlib.sha256(selected.name.encode()).hexdigest())
                for call in capture.call_args_list:
                    self.assertEqual(call.kwargs['maps'], selected.resolve())
                    self.assertEqual(call.kwargs['map_name'], 'fixture')

    def test_main_keeps_legacy_maps_without_override(self):
        with tempfile.TemporaryDirectory() as directory:
            report = Path(directory) / 'report.json'
            captures = [run('reference'), run('reference-repeat'), run('candidate')]
            with patch.dict(os.environ, {'HALO2_TEST_SCRATCH': directory, 'H2_MAPS': '',
                                         'H2_REPEAT_CANDIDATE': ''}), \
                 patch.object(parity, 'capture', side_effect=captures), \
                 patch.object(parity, 'file_sha256', return_value='fixture-pin'), \
                 contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(parity.main([str(report), 'reference', 'candidate', '4:90:0:0']), 0)
            self.assertEqual(json.loads(report.read_text())['maps'], str(parity.MAPS.resolve()))


class CaptureTests(unittest.TestCase):
    def capture(self, directory, name='fixture'):
        return parity.capture(Path(directory) / 'runtime/halo2_native', name, 4, 1, 0, 0,
                              scratch=directory, size=(2, 2))

    def test_capture_uses_runtime_directory_and_absolute_input_paths(self):
        with tempfile.TemporaryDirectory() as directory:
            def render(command, **kwargs):
                runtime = Path(directory) / 'runtime/halo2_native'
                self.assertEqual(kwargs['cwd'], runtime.parent)
                self.assertIn(str(runtime), command)
                for option in ('--map', '--shared-maps', '--mp-input-script', '--screenshot'):
                    self.assertTrue(Path(command[command.index(option) + 1]).is_absolute())
                self.assertNotIn('WAYLAND_DISPLAY', kwargs['env'])
                self.assertEqual(command[command.index('--graphics-mode') + 1], 'Original')
                for key in ('HALO2_PIPELINE_CACHE_DIRECTORY', 'MESA_SHADER_CACHE_DIR',
                            '__GL_SHADER_DISK_CACHE_PATH', 'XDG_DATA_HOME', 'XDG_CONFIG_HOME'):
                    self.assertTrue(Path(kwargs['env'][key]).is_relative_to(directory))
                Path(command[command.index('--screenshot') + 1]).write_bytes(image())
                return subprocess.CompletedProcess(command, 0)
            with patch.object(parity.subprocess, 'run', side_effect=render):
                result = self.capture(directory)
            self.assertTrue(result['valid'])
            self.assertFalse((Path(directory) / 'fixture.ppm').exists())

    def test_explicit_maps_reach_both_native_map_arguments(self):
        with tempfile.TemporaryDirectory() as directory:
            maps = Path(directory) / 'selected-cache/h2_maps_win64_dx11'
            def render(command, **kwargs):
                self.assertEqual(command[command.index('--map') + 1], str(maps / 'fixture.map'))
                self.assertEqual(command[command.index('--shared-maps') + 1], str(maps))
                Path(command[command.index('--screenshot') + 1]).write_bytes(image())
                return subprocess.CompletedProcess(command, 0)
            with patch.object(parity.subprocess, 'run', side_effect=render):
                result = parity.capture(Path(directory) / 'runtime/halo2_native', 'explicit-maps',
                                        4, 1, 0, 0, scratch=directory, maps=maps,
                                        map_name='fixture', size=(2, 2))
            self.assertTrue(result['valid'])

    def test_failed_runtime_cannot_pass_with_a_valid_image(self):
        with tempfile.TemporaryDirectory() as directory:
            def render(command, **kwargs):
                Path(command[command.index('--screenshot') + 1]).write_bytes(image())
                kwargs['stdout'].write('runtime failed after capture\n')
                return subprocess.CompletedProcess(command, 2)
            with patch.object(parity.subprocess, 'run', side_effect=render):
                result = self.capture(directory)
            self.assertFalse(result['valid'])
            self.assertIn('runtime failed after capture', result['log_tail'])
            self.assertIn('sha256', result)
            self.assertFalse((Path(directory) / 'fixture.ppm').exists())

    def test_capture_timeout_returns_failure_diagnostics(self):
        with tempfile.TemporaryDirectory() as directory:
            def timeout(command, **kwargs):
                kwargs['stdout'].write('renderer was waiting\n')
                raise subprocess.TimeoutExpired(command, 600)
            with patch.object(parity.subprocess, 'run', side_effect=timeout):
                result = self.capture(directory)
            self.assertFalse(result['valid'])
            self.assertTrue(result['timed_out'])
            self.assertIsNone(result['exit_code'])
            self.assertIn('renderer was waiting', result['log_tail'])

    @unittest.skipUnless(os.name == 'posix', 'POSIX process-group cleanup')
    def test_timeout_stops_xvfb_child_before_next_capture(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            commands = root / 'commands'; commands.mkdir()
            runtime = root / 'runtime/halo2_native'; runtime.parent.mkdir(); runtime.touch()
            heartbeat, pid_file = root / 'heartbeat', root / 'child.pid'
            child = ('import os,sys,time\nfrom pathlib import Path\n'
                     'Path(sys.argv[2]).write_text(str(os.getpid()))\n'
                     'while True:\n'
                     ' Path(sys.argv[1]).write_text(str(time.monotonic_ns()));time.sleep(.01)\n')
            launcher = commands / 'xvfb-run'
            launcher.write_text('#!' + sys.executable + '\nimport subprocess,sys,time\n'
                                'subprocess.Popen([sys.executable,"-c",' + repr(child) + ',' +
                                repr(str(heartbeat)) + ',' + repr(str(pid_file)) + '])\n'
                                'time.sleep(60)\n')
            launcher.chmod(0o755)
            with patch.dict(os.environ, {'PATH': str(commands) + os.pathsep + os.environ['PATH']}):
                result = parity.capture(runtime, 'timeout-tree', 4, 1, 0, 0, scratch=root,
                                        size=(2, 2), timeout_seconds=.4)
            self.assertFalse(result['valid'])
            self.assertTrue(result['timed_out'])
            self.assertTrue(pid_file.exists(), 'fixture never launched its runtime child')
            last = heartbeat.read_bytes()
            time.sleep(.05)
            self.assertEqual(heartbeat.read_bytes(), last, 'runtime child survived capture timeout')
            proc = Path('/proc') / pid_file.read_text() / 'stat'
            if proc.exists():
                self.assertEqual(proc.read_text().split(') ')[1].split()[0], 'Z')
            self.assertEqual(list(root.glob('halo2-test-*')), [], 'nested capture scratch survived')

    def test_missing_capture_is_a_failure(self):
        with tempfile.TemporaryDirectory() as directory:
            with patch.object(parity.subprocess, 'run', return_value=subprocess.CompletedProcess([], 0)):
                result = self.capture(directory)
            self.assertFalse(result['valid'])
            self.assertIn('not produced', result['error'])

    def test_wrong_size_and_truncated_captures_are_rejected_and_removed(self):
        with tempfile.TemporaryDirectory() as directory:
            for payload in (image(size=(1, 1)), image()[:-1], b'garbage'):
                def render(command, **kwargs):
                    Path(command[command.index('--screenshot') + 1]).write_bytes(payload)
                    return subprocess.CompletedProcess(command, 0)
                with patch.object(parity.subprocess, 'run', side_effect=render):
                    result = self.capture(directory)
                self.assertFalse(result['valid'])
                self.assertIn('sha256', result)
                self.assertFalse((Path(directory) / 'fixture.ppm').exists())


class ProfileMapSelectionTests(unittest.TestCase):
    @unittest.skipUnless(os.name == 'posix', 'POSIX profiling wrapper')
    def test_profile_admits_only_recorded_4k_render_and_presentation(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            maps = root / 'maps'; maps.mkdir()
            runtime = root / 'runtime/halo2_native'; runtime.parent.mkdir()
            scratch = root / 'scratch'; scratch.mkdir()
            reports = root / 'reports'
            observation = root / 'observation.json'
            fields = ['render_width', 'render_height', 'present_width', 'present_height', 'interval_ms']
            script = Path(parity.__file__).with_name('profile_run.sh')
            for case in ('wrong_render', 'wrong_presentation', 'wrong_warmup_row', 'valid_4k',
                         'valid_gpu', 'missing_gpu', 'wrong_gpu_view', 'missing_gpu_frame', 'early_exit'):
                with self.subTest(case=case):
                    # The failed GPU retry must clear the successful cohort's
                    # files, so readers cannot mistake stale data for its result.
                    label = 'gpu_retry' if case in ('valid_gpu', 'missing_gpu') else case
                    rows = [dict(zip(fields, (3840, 2160, 1280, 720, 16.6667))) for _ in range(35)]
                    if case == 'wrong_render':
                        for row in rows:
                            row.update(render_width=1280, render_height=720)
                    elif case == 'wrong_presentation':
                        for row in rows:
                            row.update(present_width=3840, present_height=2160)
                    elif case == 'wrong_warmup_row':
                        rows[0].update(render_width=1280, render_height=720)
                    elif case == 'early_exit':
                        rows.pop()
                    gpu_rows = [dict(frame=i, gpu_total_ms=4, gpu_compute_ms=1, gpu_scene_ms=2,
                                     gpu_present_copy_ms=1, render_width=3840, render_height=2160, views=4)
                                for i in range(35)]
                    if case == 'wrong_gpu_view':
                        gpu_rows[0]['views'] = 1
                    elif case == 'missing_gpu_frame':
                        gpu_rows.pop()
                    gpu_script = '' if case == 'missing_gpu' else (
                        'if os.environ.get("HALO2_RENDER_GPU_TIMINGS"):\n'
                        ' with open(os.environ["HALO2_RENDER_GPU_TIMINGS"],"w") as stream:\n'
                        '  writer=csv.DictWriter(stream,fieldnames=' + repr(list(gpu_rows[0])) + ',delimiter="\\t")\n'
                        '  writer.writeheader();writer.writerows(' + repr(gpu_rows) + ')\n')
                    runtime.write_text('#!' + sys.executable + '\nimport csv,json,os,sys\n'
                                       'from pathlib import Path\n'
                                       'Path(' + repr(str(observation)) + ').write_text(json.dumps({"args":sys.argv[1:],'
                                       '"xdg_data":os.environ.get("XDG_DATA_HOME"),'
                                       '"xdg_config":os.environ.get("XDG_CONFIG_HOME")}))\n'
                                       'with open(os.environ["HALO2_RENDER_TIMINGS"],"w") as stream:\n'
                                       ' writer=csv.DictWriter(stream,fieldnames=' + repr(fields) + ',delimiter="\\t")\n'
                                       ' writer.writeheader();writer.writerows(' + repr(rows) + ')\n' + gpu_script)
                    runtime.chmod(0o755)
                    env = {**os.environ, 'HALO2_TEST_SCRATCH': str(scratch), 'H2_MAPS': str(maps),
                           'H2_MAP': 'fixture', 'H2_CPUMON': '',
                           'H2_GPU_TIMINGS': '1' if 'gpu' in case else ''}
                    result = subprocess.run([str(script), str(runtime), str(reports), label, '4', '35', ''],
                                            env=env, capture_output=True, text=True, timeout=10)
                    observed = json.loads(observation.read_text())
                    args = observed['args']
                    self.assertEqual(args[args.index('--viewport') + 1:args.index('--viewport') + 3], ['1280', '720'])
                    self.assertEqual(args[args.index('--render-size') + 1:args.index('--render-size') + 3], ['3840', '2160'])
                    self.assertEqual(args[args.index('--graphics-mode') + 1], 'Original')
                    self.assertEqual(observed['xdg_data'], str(scratch / 'xdg-data'))
                    self.assertEqual(observed['xdg_config'], str(scratch / 'xdg-config'))
                    if case in ('valid_4k', 'valid_gpu'):
                        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                        self.assertIn('frames=5 fps=60.00', result.stdout)
                        self.assertTrue((reports / (label + '.timing.tsv')).exists())
                        self.assertEqual((reports / (label + '.gpu.tsv')).exists(), case == 'valid_gpu')
                    else:
                        self.assertNotEqual(result.returncode, 0)
                        self.assertNotIn('fps=', result.stdout)
                        self.assertFalse((reports / (label + '.timing.txt')).exists())
                        self.assertFalse((reports / (label + '.timing.tsv')).exists())
                        diagnostic = json.loads((reports / (label + '.failure.log')).read_text())
                        self.assertFalse(diagnostic['passed'])
                        if 'gpu' in case:
                            self.assertIn('GPU', diagnostic['error'])
                            self.assertFalse((reports / (label + '.gpu.tsv')).exists())
                        elif case == 'early_exit':
                            self.assertEqual(diagnostic['total_rows'], 34)
                            self.assertEqual(diagnostic['requested_frames'], 35)
                        else:
                            self.assertEqual(diagnostic['total_rows'], 35)
                            self.assertEqual(diagnostic['mismatched_rows'], 1 if case == 'wrong_warmup_row' else 35)

    def test_ab_routes_all_assignments_only_to_candidate_and_interleaves(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            script = root / 'ab_env.sh'
            script.write_text(Path(parity.__file__).with_name('ab_env.sh').read_text())
            script.chmod(0o755)
            helper = root / 'profile_run.sh'
            helper.write_text('#!/bin/bash\n'
                              'printf "%s,%s,%s\\n" "$3" "${H2_TEST_A-}" "${H2_TEST_B-}" >> "$H2_TEST_OBSERVATIONS"\n'
                              'printf "frames=270 fps=60\\ncpu_ms_per_frame=1\\n"\n')
            helper.chmod(0o755)
            observations = root / 'observations'
            env = {**os.environ, 'H2_PERF_REPORTS': str(root / 'reports'),
                   'H2_TEST_OBSERVATIONS': str(observations)}
            env.pop('H2_TEST_A', None)
            env.pop('H2_TEST_B', None)
            result = subprocess.run([str(script), 'matrix', '2', 'runtime',
                                     'H2_TEST_A=1', 'H2_TEST_B=2'],
                                    env=env, capture_output=True, text=True, timeout=10)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertEqual(observations.read_text().splitlines(),
                             ['matrix-A-1,,', 'matrix-B-1,1,2',
                              'matrix-B-2,1,2', 'matrix-A-2,,'])
            missing = subprocess.run([str(script), 'matrix', '1', 'runtime'],
                                     env=env, capture_output=True, text=True, timeout=10)
            self.assertEqual(missing.returncode, 2)

    @unittest.skipUnless(os.name == 'posix', 'POSIX profiling wrapper')
    def test_environment_maps_reach_profile_runtime_from_its_directory(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            maps = root / 'selected-cache/h2_maps_win64_dx11'; maps.mkdir(parents=True)
            runtime = root / 'runtime/halo2_native'; runtime.parent.mkdir()
            runtime.write_text('#!' + sys.executable + '\nimport json,os,sys\n'
                               'print(json.dumps({"args":sys.argv[1:],"cwd":os.getcwd()}))\n'
                               'raise SystemExit(17)\n')
            runtime.chmod(0o755)
            scratch = root / 'scratch'; scratch.mkdir()
            reports = root / 'reports'
            env = {**os.environ, 'HALO2_TEST_SCRATCH': str(scratch), 'H2_MAPS': str(maps),
                   'H2_MAP': 'fixture', 'H2_CPUMON': ''}
            script = Path(parity.__file__).with_name('profile_run.sh')
            result = subprocess.run([str(script), str(runtime), str(reports), 'maps', '4', '1', ''],
                                    env=env, capture_output=True, text=True, timeout=10)
            self.assertEqual(result.returncode, 17, result.stdout + result.stderr)
            observation = json.loads((reports / 'maps.failure.log').read_text())
            args = observation['args']
            self.assertEqual(args[args.index('--map') + 1], str(maps / 'fixture.map'))
            self.assertEqual(args[args.index('--shared-maps') + 1], str(maps))
            self.assertEqual(args[args.index('--graphics-mode') + 1], 'Original')
            self.assertEqual(observation['cwd'], str(runtime.parent))


if __name__ == '__main__':
    unittest.main()
