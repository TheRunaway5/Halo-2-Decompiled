#!/usr/bin/env python3
"""Check cross-body calls and shared helper state after native partitioning."""
import importlib.util
import os
from pathlib import Path
import shutil
import subprocess
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
