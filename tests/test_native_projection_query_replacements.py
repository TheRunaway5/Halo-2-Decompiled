import sys, unittest
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]; sys.path.insert(0, str(ROOT / 'tools'))
import native_projection_query_replacements as binding
UNIT = ROOT / 'src/rendering/shadows/source_shadow_frame_tu15_generated.cpp'


def strip(source):
    for prefix in binding.BRIDGES.values():
        source = source.replace(prefix, '', 1)
    return source.replace(binding.INCLUDE, '', 1)


class Binding(unittest.TestCase):
    def test_installed_and_idempotent(self):
        source = UNIT.read_text()
        self.assertEqual(binding.replace_projection_query(source), source)
        for entry, prefix in binding.BRIDGES.items():
            self.assertEqual(source.count(prefix), 1, hex(entry))
            self.assertIn(f'static void run_{entry:x}(WorldState &s) {{' + prefix, source)
        self.assertEqual(source.count(binding.INCLUDE), 1)

    def test_reinstall_from_pristine(self):
        source = UNIT.read_text()
        pristine = strip(source)
        self.assertNotIn('native_tu15_projection_query', pristine)
        reinstalled = binding.replace_projection_query(pristine)
        # Other generated-unit owners may prepend their own includes later;
        # the bridges and everything else must reproduce exactly.
        self.assertEqual(reinstalled.count(binding.INCLUDE), 1)
        self.assertEqual(reinstalled.replace(binding.INCLUDE, '', 1), source.replace(binding.INCLUDE, '', 1))

    def test_moved_bridge_rejected(self):
        source = UNIT.read_text()
        prefix = binding.BRIDGES[0x2e79a0]
        moved = source.replace(prefix, '', 1).replace('static void run_2e6290(WorldState &s) {',
                                                       'static void run_2e6290(WorldState &s) {' + prefix, 1)
        with self.assertRaises(ValueError):
            binding.replace_projection_query(moved)

    def test_duplicate_bridge_rejected(self):
        source = UNIT.read_text()
        prefix = binding.BRIDGES[0x2e7c40]
        doubled = source.replace('static void run_2e6ed0(WorldState &s) {', 'static void run_2e6ed0(WorldState &s) {' + prefix, 1)
        with self.assertRaises(ValueError):
            binding.replace_projection_query(doubled)

    def test_absent_owner_untouched(self):
        self.assertEqual(binding.replace_projection_query('int x;\n'), 'int x;\n')


if __name__ == '__main__':
    unittest.main()
