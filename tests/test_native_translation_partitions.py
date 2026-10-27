#!/usr/bin/env python3
"""Check cross-body calls and shared helper state after native partitioning."""
import importlib.util
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('native_partition', ROOT/'tools/split_native_translation_unit.py')
partition = importlib.util.module_from_spec(spec)
spec.loader.exec_module(partition)
SOURCE = '''#include <stdexcept>
namespace halo2 {
namespace {
struct WorldState { int value{}; };
int native_count() { static int count{}; return ++count; }
static void run_1(WorldState &);
static void run_2(WorldState &);
static void dispatch(WorldState &s,unsigned target) { if(target==1)run_1(s);else run_2(s); }
static void run_1(WorldState &s) {
s.value+=native_count();run_2(s);return;
throw std::runtime_error("Original solver fell outside authenticated CFG");
}

static void run_2(WorldState &s) {
s.value+=native_count();return;
throw std::runtime_error("Original solver fell outside authenticated CFG");
}
} // namespace
int native_test() { WorldState s;dispatch(s,1);return s.value; }
} // namespace halo2
int main() { if(halo2::native_test()!=3)return 1;if(halo2::native_test()!=7)return 2; }
'''


class Partitions(unittest.TestCase):
    def test_stable_writer_keeps_platform_text_output_bytes(self):
        with tempfile.TemporaryDirectory(dir=os.environ.get('HALO2_TEST_SCRATCH')) as folder:
            path = Path(folder)
            value = 'native body\nshared helper\n'
            (path/'reference').write_text(value)
            partition.write_if_changed(path/'actual', value)
            self.assertEqual((path/'actual').read_bytes(), (path/'reference').read_bytes())

    def test_unchanged_partition_keeps_output_mtimes_and_hashes(self):
        with tempfile.TemporaryDirectory(dir=os.environ.get('HALO2_TEST_SCRATCH')) as folder:
            path = Path(folder); source = path/'native.cpp'; source.write_text(SOURCE)
            original = partition.split(source, path/'split', 2)
            files = sorted((path/'split').iterdir())
            # A fixed old timestamp makes this deterministic even on file
            # systems whose clock cannot distinguish consecutive writes.
            for output in files:
                os.utime(output, ns=(1_000_000_000, 1_000_000_000))
            before = {output.name: (output.read_bytes(), output.stat().st_mtime_ns) for output in files}
            self.assertEqual(partition.split(source, path/'split', 2), original)
            self.assertEqual({output.name: (output.read_bytes(), output.stat().st_mtime_ns)
                              for output in files}, before)

    def test_local_body_edit_only_republishes_its_chunk_and_manifest(self):
        with tempfile.TemporaryDirectory(dir=os.environ.get('HALO2_TEST_SCRATCH')) as folder:
            path = Path(folder); source = path/'native.cpp'; source.write_text(SOURCE)
            original = partition.split(source, path/'split', 2)
            files = sorted((path/'split').iterdir())
            for output in files:
                os.utime(output, ns=(1_000_000_000, 1_000_000_000))
            before = {output.name: (output.read_bytes(), output.stat().st_mtime_ns) for output in files}
            source.write_text(SOURCE.replace('s.value+=native_count();run_2', 's.value-=native_count();run_2'))
            changed = partition.split(source, path/'split', 2)
            chunk = next(row['chunk'] for row in original['entries'] if row['entry'] == 'run_1')
            expected = {f'body_{chunk:03d}.cpp', 'manifest.json'}
            self.assertEqual({output.name for output in files if output.read_bytes() != before[output.name][0]}, expected)
            self.assertEqual({output.name for output in files if output.stat().st_mtime_ns != before[output.name][1]}, expected)
            for old, new in zip(original['entries'], changed['entries']):
                self.assertEqual(old['chunk'], new['chunk'])
                self.assertEqual(old['body_sha256'] == new['body_sha256'], old['entry'] != 'run_1')
                self.assertTrue(new['instruction_body_unchanged'])

    def test_cmake_incremental_generation_only_rebuilds_changed_body(self):
        generators = [('Ninja', 'ninja'), ('Unix Makefiles', 'make')]
        generators = [name for name, tool in generators if shutil.which(tool)]
        self.assertTrue(shutil.which('cmake') and generators, 'CMake and a native build tool are required')
        for generator in generators:
            with self.subTest(generator=generator), tempfile.TemporaryDirectory(
                    dir=os.environ.get('HALO2_TEST_SCRATCH')) as folder:
                path = Path(folder); project = path/'project'; project.mkdir()
                (project/'tools').mkdir(); (project/'cmake').mkdir()
                for name in ('NativeTranslationUnits.cmake', 'NativeOperandSpecialization.cmake'):
                    shutil.copyfile(ROOT/'cmake'/name, project/'cmake'/name)
                # Count actual invocations while executing the real splitter.
                # This fixture does not change its generated source bytes.
                (project/'tools/split_native_translation_unit.py').write_text(
                    'from pathlib import Path\nimport runpy\n'
                    'with Path(__file__).with_suffix(".runs").open("a") as counter: counter.write("run\\n")\n'
                    f'runpy.run_path({str(ROOT/"tools/split_native_translation_unit.py")!r}, run_name="__main__")\n')
                (project/'CMakeLists.txt').write_text('''cmake_minimum_required(VERSION 3.20)
project(native_partition_incremental LANGUAGES CXX)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(HALO2_SPECIALIZE_NATIVE_OPERANDS OFF CACHE BOOL "" FORCE)
include(cmake/NativeTranslationUnits.cmake)
add_executable(partitioned)
target_compile_features(partitioned PRIVATE cxx_std_20)
halo2_partition_native_translation(partitioned "${CMAKE_CURRENT_SOURCE_DIR}/native.cpp" 2)
''')
                source = project/'native.cpp'; source.write_text(SOURCE)
                build = path/'build'
                def command(*args):
                    result = subprocess.run(args, capture_output=True, text=True)
                    self.assertEqual(result.returncode, 0, result.stdout+result.stderr)
                def build_once():
                    command('cmake', '--build', str(build), '--parallel', '2')
                def run_count():
                    return len((project/'tools/split_native_translation_unit.runs').read_text().splitlines())
                def object_mtimes():
                    return {p.name: p.stat().st_mtime_ns for p in build.rglob('*.cpp.o')}
                command('cmake', '-S', str(project), '-B', str(build), '-G', generator,
                        f'-DPython3_EXECUTABLE={sys.executable}')
                build_once(); self.assertEqual(run_count(), 1)
                command(str(build/'partitioned'))
                before = object_mtimes(); self.assertEqual(len(before), 3)
                build_once(); self.assertEqual(run_count(), 1)
                self.assertEqual(object_mtimes(), before)
                # An input timestamp change must complete generation once,
                # without compiling byte-identical generated sources again.
                source.write_text(SOURCE)
                build_once(); self.assertEqual(run_count(), 2)
                self.assertEqual(object_mtimes(), before)
                build_once(); self.assertEqual(run_count(), 2)
                self.assertEqual(object_mtimes(), before)
                manifest = partition.split(source, path/'reference', 2)
                chunk = next(row['chunk'] for row in manifest['entries'] if row['entry'] == 'run_1')
                source.write_text(SOURCE.replace('s.value+=native_count();run_2', 's.value-=native_count();run_2'))
                build_once(); self.assertEqual(run_count(), 3)
                self.assertEqual({name for name, mtime in object_mtimes().items() if before[name] != mtime},
                                 {f'body_{chunk:03d}.cpp.o'})
                # The linked program now observes the changed native body.
                self.assertEqual(subprocess.run([str(build/'partitioned')]).returncode, 1)
                build_once(); self.assertEqual(run_count(), 3)

    def test_cross_body_calls_keep_one_helper_static(self):
        compilers = [x for x in ('clang++', 'g++') if shutil.which(x)]
        self.assertTrue(compilers, 'A native C++ compiler is required')
        with tempfile.TemporaryDirectory(dir=os.environ.get('HALO2_TEST_SCRATCH')) as folder:
            path = Path(folder); source = path/'native.cpp'; source.write_text(SOURCE)
            report = partition.split(source, path/'split', 2)
            self.assertEqual(report['body_count'], 2)
            chunks = [path/'split/router.cpp', *sorted((path/'split').glob('body_*.cpp'))]
            for compiler in compilers:
                for label, files in (('canonical', [source]), ('partitioned', chunks)):
                    exe = path/(compiler.replace('+', 'p')+'-'+label)
                    command = [compiler, '-std=c++20', '-O1', *map(str, files), '-o', str(exe)]
                    result = subprocess.run(command, capture_output=True, text=True)
                    self.assertEqual(result.returncode, 0, result.stdout+result.stderr)
                    result = subprocess.run([str(exe)], capture_output=True, text=True)
                    self.assertEqual(result.returncode, 0, compiler+' '+label+' lost shared helper state')
            # Each body keeps its exact original instruction lines. C++
            # declarations/linkage are the only source text that changes.
            for match in partition.BODY.finditer(SOURCE):
                instruction_lines = match[0].split('\n', 1)[1]
                self.assertEqual(sum(p.read_text().count(instruction_lines) for p in chunks), 1)

    def test_namespace_static_state_is_rejected(self):
        with tempfile.TemporaryDirectory(dir=os.environ.get('HALO2_TEST_SCRATCH')) as folder:
            path = Path(folder); source = path/'native.cpp'
            source.write_text(SOURCE.replace('struct WorldState', 'static int unowned;\nstruct WorldState'))
            with self.assertRaisesRegex(ValueError, 'namespace static state'):
                partition.split(source, path/'split', 2)


if __name__ == '__main__':
    unittest.main()
