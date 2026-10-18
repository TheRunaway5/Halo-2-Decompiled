#!/usr/bin/env python3
import copy
import importlib.util
import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("class_profile", ROOT / "tools/generate_mcc_sound_class_reference.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class ClassProfileTests(unittest.TestCase):
    def setUp(self):
        self.data = json.loads((ROOT / "tools/data/mcc-sound-class-reference.json").read_text())

    def reject(self, edit):
        data = copy.deepcopy(self.data)
        edit(data)
        with self.assertRaises(ValueError):
            module.generate(json.dumps(data).encode())

    def test_exact_generated_file(self):
        raw = (ROOT / "tools/data/mcc-sound-class-reference.json").read_bytes()
        self.assertEqual(module.generate(raw), (ROOT / "src/mcc_sound_class_reference.inc").read_text())

    def test_mutated_authority_and_layout(self):
        for key, value in [("schema", "other"), ("original_xbe_sha256", "0" * 64),
                           ("original_class_name_table", 0), ("class_count", 53),
                           ("source_stride", 92), ("native_stride", 120),
                           ("unsupported_class_indices", [])]:
            with self.subTest(key=key):
                self.reject(lambda d: d.__setitem__(key, value))

    def test_mutated_named_word(self):
        for key, value in [("class_index", 15), ("name", "alias"), ("offset", 92),
                           ("kind", "Float64"), ("field", "unknown"),
                           ("before_bits", 0), ("after_bits", 1)]:
            with self.subTest(key=key):
                self.reject(lambda d: d["deltas"][0].__setitem__(key, value))

    def test_mutated_complete_context(self):
        for field in ["sound_block", "sound_globals", "class_root", "source_bank_sha256"]:
            with self.subTest(field=field):
                self.reject(lambda d: d["contexts"][0].__setitem__(field, "0" * len(d["contexts"][0][field])))
        self.reject(lambda d: d["contexts"][0]["legacy"].__setitem__("datum", 1234))
        self.reject(lambda d: d["contexts"][0]["globals"].__setitem__("address", 16))
        self.reject(lambda d: d["contexts"].pop())
        self.reject(lambda d: d["contexts"].append(copy.deepcopy(d["contexts"][0])))

    def test_incomplete_and_broadened_proof(self):
        self.reject(lambda d: d["deltas"].pop())
        self.reject(lambda d: d["deltas"].append(copy.deepcopy(d["deltas"][0])))
        self.reject(lambda d: d["evidence"].__setitem__("class_names", "not-a-digest"))


if __name__ == "__main__":
    unittest.main()
