#!/usr/bin/env python3
import importlib.util
import json
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('hud_shader_generator',ROOT/'tools/generate_mcc_hud_shader_reference.py')
generator=importlib.util.module_from_spec(spec);spec.loader.exec_module(generator)
class GeneratorTests(unittest.TestCase):
    def test_exact_output(self):
        self.assertEqual(generator.generate((ROOT/'tools/data/mcc-hud-shader-reference.json').read_bytes()),(ROOT/'src/mcc_hud_shader_reference.inc').read_text())
    def test_authored_grey_mutation_rejected(self):
        d=json.loads((ROOT/'tools/data/mcc-hud-shader-reference.json').read_bytes());d['converted_grey_mappings'][0][1]^=1
        with self.assertRaisesRegex(ValueError,'authority'):generator.generate(json.dumps(d).encode())
    def test_truncation_rejected(self):
        d=(ROOT/'tools/data/mcc-hud-shader-reference.json').read_bytes()
        with self.assertRaisesRegex(ValueError,'authority'):generator.generate(d[:-1])
if __name__=='__main__':unittest.main()
