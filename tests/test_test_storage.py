"""Exercise real test/build processes without building the Halo engine."""
import os
import json
import contextlib
from pathlib import Path
import shutil
import signal
import subprocess
import sys
import tempfile
import time
import unittest

ROOT = Path(__file__).resolve().parents[1]
RUNNER = ROOT / 'tools/run_test.py'


class TestStorage(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix='halo2-storage-regression-')
        self.addCleanup(self.temporary.cleanup)
        self.directory = Path(self.temporary.name)
        self.scratch = self.directory / 'scratch'
        self.scratch.mkdir()
        self.env = dict(os.environ, TMPDIR=str(self.scratch), TMP=str(self.scratch),
                        TEMP=str(self.scratch), HALO2_KEEP_TEST_ARTIFACTS='0')

    def command(self, code, *options):
        return [sys.executable, str(RUNNER), *options, '--', sys.executable, '-c', code]

    def run_probe(self, code, *options, expected=0):
        result = subprocess.run(self.command(code, *options), env=self.env,
                                capture_output=True, text=True, timeout=15)
        self.assertEqual(result.returncode, expected, result.stdout + result.stderr)
        return result

    def test_success_failure_and_skip_remove_scratch(self):
        fixture = self.directory / 'original.bin'
        fixture.write_bytes(b'original fixture')
        for code in (0, 1, 77):
            with self.subTest(code=code):
                self.run_probe('import pathlib,tempfile,sys; '
                               'p=pathlib.Path(tempfile.gettempdir()); '
                               '(p/"dump.bin").write_bytes(b"x"*1048576); '
                               'print("retained diagnostic",flush=True); '
                               f'sys.exit({code})', expected=code)
                self.assertEqual(list(self.scratch.iterdir()), [])
                self.assertEqual(fixture.read_bytes(), b'original fixture')

    def test_keep_is_explicit_and_placeholders_use_owned_directory(self):
        self.env['CAPTURE'] = '@HALO2_TEST_SCRATCH@/capture'
        self.run_probe('import os,pathlib; '
                       'p=pathlib.Path(os.environ["HALO2_TEST_SCRATCH"]); '
                       'assert os.environ["CAPTURE"]==str(p/"capture"); '
                       '(p/"kept.bin").write_bytes(b"evidence")', '--keep-artifacts')
        directories = list(self.scratch.iterdir())
        self.assertEqual(len(directories), 1)
        self.assertEqual((directories[0] / 'kept.bin').read_bytes(), b'evidence')

    def test_parallel_runs_do_not_share_scratch(self):
        code = ('import pathlib,tempfile,time; '
                'p=pathlib.Path(tempfile.gettempdir()); '
                '(p/"same-name.bin").write_bytes(b"x"); time.sleep(.4); '
                'print(p,flush=True)')
        processes = [subprocess.Popen(self.command(code), env=self.env,
                                     stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                     text=True) for _ in range(2)]
        output = [p.communicate(timeout=15)[0].strip() for p in processes]
        self.assertTrue(all(p.returncode == 0 for p in processes))
        self.assertNotEqual(output[0], output[1])
        self.assertEqual(list(self.scratch.iterdir()), [])

    def test_build_logs_remain_bounded_when_a_command_floods_output(self):
        sys.path.insert(0, str(ROOT / 'tools'))
        from run_tests import LOG_LIMIT, run_step
        log = self.directory / 'build.log'
        command = [sys.executable, '-c',
                   'import sys; sys.stdout.buffer.write(b"x"*(10*1024*1024)+b"tail marker")']
        with open(os.devnull, 'w') as sink, contextlib.redirect_stdout(sink):
            self.assertEqual(run_step(command, log), 0)
        self.assertLessEqual(log.stat().st_size, 2 * LOG_LIMIT)
        self.assertTrue(log.read_bytes().endswith(b'tail marker'))

    @unittest.skipUnless(os.name == 'posix', 'POSIX process-group cancellation')
    def test_hard_killed_runner_cleans_scratch_and_descendants(self):
        marker = self.directory / 'escaped-child'
        code = ('import pathlib,tempfile,subprocess,sys,time; '
                'p=pathlib.Path(tempfile.gettempdir()); (p/"ready").touch(); '
                f'subprocess.Popen([sys.executable,"-c",'
                f'"import time,pathlib; time.sleep(2); pathlib.Path({str(marker)!r}).touch()"]); '
                'time.sleep(30)')
        process = subprocess.Popen(self.command(code), env=self.env,
                                   stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        try:
            deadline = time.monotonic() + 8
            while not list(self.scratch.glob('*/ready')) and time.monotonic() < deadline:
                time.sleep(.02)
            self.assertTrue(list(self.scratch.glob('*/ready')))
            os.kill(process.pid, signal.SIGKILL)
            process.communicate(timeout=8)
            self.assertEqual(list(self.scratch.iterdir()), [])
            time.sleep(2.1)
            self.assertFalse(marker.exists())
        finally:
            if process.poll() is None:
                process.kill()
                process.communicate(timeout=8)

    @unittest.skipUnless(shutil.which('cmake') and shutil.which('c++'), 'CMake and C++ required')
    def test_cmake_default_build_and_ctest_cleanup(self):
        source = self.directory / 'source'
        source.mkdir()
        (source / 'main.cpp').write_text('int main() { return 0; }\n')
        (source / 'probe.py').write_text(
            'import pathlib,tempfile,os,sys\n'
            'p=pathlib.Path(tempfile.gettempdir())\n'
            '(p/"leaked-test-dump.bin").write_bytes(b"x"*1048576)\n'
            'assert sys.argv[1:]==["space argument", "literal;semicolon"]\n'
            'assert pathlib.Path.cwd()==pathlib.Path(os.environ["EXPECTED_CWD"])\n')
        (source / 'CMakeLists.txt').write_text(f'''
cmake_minimum_required(VERSION 3.24)
project(StorageRegression LANGUAGES CXX)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
include("{ROOT / 'cmake/TestStorage.cmake'}" OPTIONAL)
enable_testing()
add_executable(storage_tests main.cpp)
add_executable(unregistered_tests main.cpp)
add_executable(game_runtime main.cpp)
add_test(NAME executable COMMAND storage_tests)
add_test(NAME scratch COMMAND ${{Python3_EXECUTABLE}} "${{CMAKE_CURRENT_SOURCE_DIR}}/probe.py"
    "space argument" "literal;semicolon")
set_tests_properties(scratch PROPERTIES WORKING_DIRECTORY "${{CMAKE_CURRENT_SOURCE_DIR}}"
    ENVIRONMENT "EXPECTED_CWD=${{CMAKE_CURRENT_SOURCE_DIR}}")
''')
        build = self.directory / 'build'
        def run(*args):
            result = subprocess.run(args, env=self.env, text=True, capture_output=True, timeout=30)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            return result
        run('cmake', '-S', str(source), '-B', str(build))
        run('cmake', '--build', str(build))
        self.assertFalse((build / 'storage_tests').exists(),
                         'Ordinary game builds must not build every test executable')
        self.assertFalse((build / 'unregistered_tests').exists())
        self.assertTrue((build / 'game_runtime').exists())
        run('cmake', '--build', str(build), '--target', 'halo2_tests')
        self.assertTrue((build / 'storage_tests').exists())
        self.assertTrue((build / 'unregistered_tests').exists())
        run('ctest', '--test-dir', str(build), '--output-on-failure')
        self.assertFalse(list(build.rglob('leaked-test-dump.bin')))
        self.assertEqual(list(self.scratch.iterdir()), [])

    @unittest.skipUnless(shutil.which('cmake'), 'CMake required')
    def test_disposable_build_success_and_failure_keep_only_reports(self):
        source = self.directory / 'source'
        source.mkdir()
        (source / 'probe.py').write_text('print("test diagnostic")\n')
        (source / 'CMakeLists.txt').write_text(f'''
cmake_minimum_required(VERSION 3.24)
project(DisposableRegression NONE)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
include("{ROOT / 'cmake/TestStorage.cmake'}")
enable_testing()
add_custom_target(generated ALL COMMAND ${{CMAKE_COMMAND}} -E touch "${{CMAKE_BINARY_DIR}}/compiled.o")
add_custom_target(selected DEPENDS generated)
add_test(NAME probe COMMAND ${{Python3_EXECUTABLE}} "${{CMAKE_CURRENT_SOURCE_DIR}}/probe.py")
''')
        output = self.directory / 'reports'
        command = [sys.executable, str(ROOT / 'tools/run_tests.py'), '--source', str(source),
                   '--output', str(output), '--scratch-root', str(self.scratch),
                   '--target', 'selected', '--filter', '^probe$']
        for expected in (0, 8):
            if expected:
                (source / 'probe.py').write_text('import sys; print("failure diagnostic"); sys.exit(1)\n')
            result = subprocess.run(command, env=self.env, capture_output=True, text=True, timeout=30)
            self.assertEqual(result.returncode, expected, result.stdout + result.stderr)
            report = json.loads((output / 'report.json').read_text())
            self.assertEqual(report['passed'], expected == 0)
            self.assertFalse(Path(report['build']).exists())
            self.assertEqual(list(self.scratch.iterdir()), [])
            if expected:
                self.assertIn('failure diagnostic', (output / 'tests.log').read_text())

    @unittest.skipUnless(os.name == 'posix' and shutil.which('cmake'), 'POSIX CTest timeout')
    def test_ctest_timeout_cleans_owned_storage(self):
        source = self.directory / 'source'
        source.mkdir()
        (source / 'probe.py').write_text(
            'import tempfile,pathlib,time\n'
            '(pathlib.Path(tempfile.gettempdir())/"timeout-dump.bin").write_bytes(b"x"*1048576)\n'
            'time.sleep(30)\n')
        (source / 'CMakeLists.txt').write_text(f'''
cmake_minimum_required(VERSION 3.24)
project(TimeoutRegression NONE)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
include("{ROOT / 'cmake/TestStorage.cmake'}")
enable_testing()
add_test(NAME timeout COMMAND ${{Python3_EXECUTABLE}} "${{CMAKE_CURRENT_SOURCE_DIR}}/probe.py")
set_tests_properties(timeout PROPERTIES TIMEOUT 1)
''')
        build = self.directory / 'build'
        result = subprocess.run(['cmake', '-S', str(source), '-B', str(build)],
                                env=self.env, capture_output=True, text=True, timeout=15)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        result = subprocess.run(['ctest', '--test-dir', str(build), '--output-on-failure'],
                                env=self.env, capture_output=True, text=True, timeout=15)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('Timeout', result.stdout)
        deadline = time.monotonic() + 5
        while list(build.rglob('timeout-dump.bin')) and time.monotonic() < deadline:
            time.sleep(.02)
        self.assertFalse(list(build.rglob('timeout-dump.bin')))

    @unittest.skipUnless(shutil.which('cmake'), 'CMake required')
    def test_selected_reports_survive_pass_and_failure_without_build_or_external_copies(self):
        import hashlib
        source = self.directory / 'source'
        source.mkdir()
        external = self.directory / 'original-reference.json'
        external.write_bytes(b'{"original":true}\n')
        report_bytes = b'{"completed":true}\n'
        (source / 'probe.py').write_text(
            'import pathlib,sys\n'
            'build=pathlib.Path(sys.argv[1]); (build/"selected").mkdir(exist_ok=True)\n'
            f'(build/"selected/report.json").write_bytes({report_bytes!r})\n'
            '(build/"large.json").write_bytes(b"x"*(1024*1024+1))\n'
            '(build/"binary.json").write_bytes(b"\\x7fELFpayload")\n'
            '(build/"compiled.o").write_bytes(b"temporary object")\n'
            f'(build/"outside.json").symlink_to({str(external)!r})\n'
            'sys.exit(int(sys.argv[2]))\n')
        (source / 'CMakeLists.txt').write_text(f'''
cmake_minimum_required(VERSION 3.24)
project(SelectedReportRegression NONE)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
include("{ROOT / 'cmake/TestStorage.cmake'}")
enable_testing()
add_custom_target(selected)
add_test(NAME probe COMMAND ${{Python3_EXECUTABLE}} "${{CMAKE_CURRENT_SOURCE_DIR}}/probe.py"
    "${{CMAKE_BINARY_DIR}}" "${{PROBE_EXIT}}")
''')
        for exit_code in (0, 1):
            output = self.directory / f'reports-{exit_code}'
            command = [sys.executable, str(ROOT / 'tools/run_tests.py'), '--source', str(source),
                       '--output', str(output), '--scratch-root', str(self.scratch), '--target', 'selected',
                       '--filter', '^probe$', f'--cmake-arg=-DPROBE_EXIT={exit_code}']
            for name in ('selected/report.json', 'missing.json', 'large.json', 'binary.json', 'outside.json', 'compiled.o'):
                command.extend(['--retain-report', name])
            result = subprocess.run(command, env=self.env, capture_output=True, text=True, timeout=30)
            self.assertEqual(result.returncode, 0 if exit_code == 0 else 8, result.stdout + result.stderr)
            report = json.loads((output / 'report.json').read_text())
            records = {r['path']: r for r in report['retained_small_reports']}
            retained = output / 'evidence/selected/report.json'
            self.assertEqual(retained.read_bytes(), report_bytes)
            self.assertEqual(records['selected/report.json']['sha256'], hashlib.sha256(report_bytes).hexdigest())
            self.assertEqual(records['missing.json']['status'], 'not produced')
            self.assertIn('limit', records['large.json']['status'])
            self.assertIn('executable', records['binary.json']['status'])
            self.assertIn('outside', records['outside.json']['status'])
            self.assertIn('suffix', records['compiled.o']['status'])
            self.assertFalse(Path(report['build']).exists())
            self.assertEqual(list(self.scratch.iterdir()), [])
            self.assertEqual(external.read_bytes(), b'{"original":true}\n')
            self.assertEqual(list((output / 'evidence').rglob('*.json')), [retained])


if __name__ == '__main__':
    unittest.main()
