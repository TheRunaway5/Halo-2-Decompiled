#!/usr/bin/env python3
"""Finite metadata generation and malformed-authority rejection."""
import importlib.util
import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("hud_generator", ROOT / "tools/generate_mcc_hud_reference.py")
generator = importlib.util.module_from_spec(spec)
spec.loader.exec_module(generator)


class HudReferenceGeneratorTests(unittest.TestCase):
    def test_reviewed_outputs(self):
        raw = (ROOT / "tools/data/mcc-hud-reference.json").read_bytes()
        profile, hashes = generator.generate(raw)
        self.assertEqual(profile, (ROOT / "src/ui/hud/mcc_hud_reference.inc").read_text())
        self.assertEqual(hashes, (ROOT / "src/ui/hud/mcc_hud_reference_hashes.inc").read_text())

    def test_authored_curve_mutation_rejected(self):
        data = json.loads((ROOT / "tools/data/mcc-hud-reference.json").read_bytes())
        data["patches"][0][-1] ^= 1
        with self.assertRaisesRegex(ValueError, "authority"):
            generator.generate(json.dumps(data).encode())

    def test_truncated_authority_rejected(self):
        raw = (ROOT / "tools/data/mcc-hud-reference.json").read_bytes()
        with self.assertRaisesRegex(ValueError, "authority"):
            generator.generate(raw[:-1])

    def test_geometry_reviewed_output_and_mutation_rejection(self):
        raw = (ROOT / "tools/data/mcc-hud-geometry-reference.json").read_bytes()
        self.assertEqual(generator.generate_geometry(raw), (ROOT / "src/ui/hud/mcc_hud_geometry_reference.inc").read_text())
        data = json.loads(raw)
        self.assertEqual(len(data["profiles"]), 17)
        self.assertTrue(all(p["name"] != "ui\\hud\\scoreboard" for p in data["profiles"]))
        data["profiles"][0]["patches"][0][-1] ^= 1
        with self.assertRaisesRegex(ValueError, "authority"):
            generator.generate_geometry(json.dumps(data).encode())


if __name__ == "__main__":
    unittest.main()
