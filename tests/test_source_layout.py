"""Regressions for tooling that consumes categorized implementation paths."""
import json
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import build_launcher_engine as engine
from source_layout import source_file


class SourceLayoutTests(unittest.TestCase):
    def setUp(self):
        self.owner = tempfile.TemporaryDirectory()
        self.addCleanup(self.owner.cleanup)
        self.root = Path(self.owner.name)

    def source(self, relative):
        path = self.root / 'src' / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text('// Synthetic module inventory input.\n')
        return path

    def test_module_lookup_rejects_missing_and_ambiguous_owners(self):
        with self.assertRaises(FileNotFoundError):
            source_file(self.root, 'module.cpp')
        first = self.source('physics/module.cpp')
        self.assertEqual(source_file(self.root, 'module.cpp'), first)
        self.source('rendering/module.cpp')
        with self.assertRaises(ValueError):
            source_file(self.root, 'module.cpp')
        self.assertEqual(source_file(self.root, 'physics/module.cpp'), first)
        with self.assertRaises(ValueError):
            source_file(self.root, '../module.cpp')

    def test_native_partitions_and_specializations_keep_their_subsystem_owners(self):
        self.source('physics/world_generated.cpp')
        self.source('audio/sound_generated.cpp')
        graph = self.root / 'engine-targets.txt'
        graph.write_text(
            'halo2_native|world;sound||\n'
            'world|||/build/native-translations/world_generated/router.cpp\n'
            'sound|||/build/native-operands/audio/sound_generated.cpp\n')
        report = engine.wiring(graph, self.root)
        self.assertTrue(report['passed'], json.dumps(report))
        self.assertEqual(report['engine_sources'], [
            'src/audio/sound_generated.cpp', 'src/physics/world_generated.cpp'])

    def test_nested_unwired_module_is_still_rejected(self):
        self.source('rendering/effects/new_effect.cpp')
        graph = self.root / 'engine-targets.txt'
        graph.write_text('halo2_native|||\n')
        report = engine.wiring(graph, self.root)
        self.assertFalse(report['passed'])
        self.assertEqual(report['unexpected_unwired_sources'], [
            'src/rendering/effects/new_effect.cpp'])


if __name__ == '__main__':
    unittest.main()
