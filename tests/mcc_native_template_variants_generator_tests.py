#!/usr/bin/env python3
"""Portable maintenance checks; installed game/reference caches are unnecessary."""
import copy
import importlib.util
import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("variants", ROOT / "tools/generate_mcc_native_template_variants.py")
GENERATOR = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(GENERATOR)
SOURCE = (ROOT / "tools/reference/mcc-native-template-variants.json").read_bytes()


class NativeVariants(unittest.TestCase):
    def test_committed_output_is_reproducible(self):
        self.assertEqual(GENERATOR.generate(SOURCE), (ROOT / "src/mcc_native_template_variants.inc").read_text())

    def test_changed_or_unowned_evidence_is_rejected(self):
        mutations = (
            lambda d: d.update(schema="unknown"),
            lambda d: d.update(exact_raw_variants=200),
            lambda d: d.update(existing_stem_instances=210),
            lambda d: d["profiles"][0].update(template="shaders\\unknown"),
            lambda d: d["profiles"][0]["native_shape_lod_layer_pass_impl_remap"].__setitem__(3, 33),
            lambda d: d["profiles"][0]["variants"].pop(),
            lambda d: d["profiles"][0]["variants"][0].update(payload="../unowned.bin"),
            lambda d: d["profiles"][0]["variants"][0].update(fnv64="invalid"),
            lambda d: d["profiles"][0]["variants"][0].update(bytes=95),
            lambda d: d["profiles"][0]["variants"][0].update(maps=[]),
            lambda d: d["profiles"][0]["variants"].__setitem__(1, copy.deepcopy(d["profiles"][0]["variants"][0])),
            lambda d: d["profiles"][0]["map_evidence"][0].update(library_sha256="invalid"),
            lambda d: d["profiles"][0]["map_evidence"][0].update(original_source_sha256="invalid"),
            lambda d: d["profiles"][0]["map_evidence"][0].update(payload="0" * 64 + ".bin"),
            lambda d: d["profiles"][0]["map_evidence"].__setitem__(1, copy.deepcopy(d["profiles"][0]["map_evidence"][0])),
        )
        for index, mutate in enumerate(mutations):
            with self.subTest(index=index):
                changed = json.loads(SOURCE)
                mutate(changed)
                with self.assertRaises(ValueError):
                    GENERATOR.generate(json.dumps(changed).encode())


if __name__ == "__main__":
    unittest.main()
