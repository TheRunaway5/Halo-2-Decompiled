"""The strict comparator must reject even one changed channel or shifted pixel."""
import importlib.util
from pathlib import Path
import unittest

import numpy as np

path = Path(__file__).resolve().parents[1]/'tools/compare_held_pose_pixels.py'
spec = importlib.util.spec_from_file_location('held_pose_pixels', path)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class HeldPosePixels(unittest.TestCase):
    def test_exact_and_single_channel_failure(self):
        original = np.zeros((3, 4, 3), dtype=np.uint8)
        original[1, 2] = [70, 90, 120]
        mask = np.zeros((3, 4), dtype=bool)
        mask[1, 2] = True
        self.assertTrue(module.compare(original, original.copy(), mask)[1]['rgb_component_passed'])
        changed = original.copy()
        changed[1, 2, 1] += 1
        result = module.compare(original, changed, mask)[1]
        self.assertFalse(result['rgb_component_passed'])
        self.assertEqual(result['differing_pixels'], 1)
        self.assertEqual(result['maximum_channel_difference'], 1)
        self.assertFalse(module.compare(original, np.roll(original, 1, axis=1), mask)[1]['rgb_component_passed'])

    def test_empty_or_mismatched_input_rejected(self):
        pixels = np.zeros((3, 4, 3), dtype=np.uint8)
        with self.assertRaises(ValueError):
            module.compare(pixels, pixels, np.zeros((3, 4), dtype=bool))
        with self.assertRaises(ValueError):
            module.compare(pixels, pixels[:2], np.ones((3, 4), dtype=bool))
        with self.assertRaises(ValueError):
            module.compare(pixels, pixels, np.ones((3, 4), dtype=np.uint8))


if __name__ == '__main__':
    unittest.main()
