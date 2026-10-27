"""Check native partition linkage without changing recovered function bodies."""
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import split_native_translation_unit as splitter


def fixture(declarations=('run_a', 'run_b'), definitions=('run_b', 'run_a')):
    text = '#include <stdexcept>\nnamespace halo2 {\nnamespace {\nstruct WorldState {int value=0;};\n'
    text += 'int bump() {static int count=0;return ++count;}\n'
    text += ''.join('static void '+name+'(WorldState &);\n' for name in declarations)
    for name in definitions:
        text += 'static void '+name+'(WorldState &s) {\n'
        text += 's.value += bump()*'+('2' if name=='run_a' else '4')+';\nreturn;\n'
        text += 'throw std::runtime_error("Original solver fell outside authenticated CFG");\n}\n'
    return text + '} // namespace\nint evaluate(){WorldState s;run_a(s);run_b(s);return s.value;}\n} // namespace halo2\n'


class NativePartitionTests(unittest.TestCase):
    def setUp(self):
        self.owner = tempfile.TemporaryDirectory(prefix='native-partition-', dir=os.environ.get('HALO2_TEST_SCRATCH'))
        self.addCleanup(self.owner.cleanup)
        self.root = Path(self.owner.name)
        self.source = self.root / 'canonical.cpp'

    def test_reordered_forward_declarations_preserve_bodies_and_cpp_linkage(self):
        self.source.write_text(fixture())
        output = self.root / 'parts'
        report = splitter.split(self.source, output, 2)
        self.assertTrue(report['body_bytes_unchanged'])
        self.assertEqual(report['body_count'], 2)
        self.assertEqual({row['entry'] for row in report['entries']}, {'run_a', 'run_b'})
        compiler = shutil.which('c++')
        if compiler:
            main = self.root / 'main.cpp'
            main.write_text('namespace halo2 {int evaluate();}\nint main(){return halo2::evaluate()==10?0:1;}\n')
            for name, sources in [('original', [self.source]), ('partitioned', sorted(output.glob('*.cpp')))]:
                binary = self.root / name
                subprocess.run([compiler, '-std=c++20', *map(str, sources), str(main), '-o', str(binary)], check=True)
                subprocess.run([str(binary)], check=True)

    def test_missing_or_duplicate_names_are_rejected_before_publication(self):
        for declarations, definitions in [(('run_a',), ('run_a', 'run_b')),
                                          (('run_a', 'run_a'), ('run_a', 'run_a'))]:
            with self.subTest(declarations=declarations, definitions=definitions):
                self.source.write_text(fixture(declarations, definitions))
                output = self.root / 'never-published'
                with self.assertRaises(ValueError):
                    splitter.split(self.source, output, 2)
                self.assertFalse(output.exists())


if __name__ == '__main__':
    unittest.main()
