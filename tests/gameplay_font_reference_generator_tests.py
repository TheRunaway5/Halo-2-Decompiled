#!/usr/bin/env python3
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
TOOL = ROOT / "tools/generate_gameplay_font_reference.py"
DATA = ROOT / "tools/data/gameplay-font-reference.json"


class FontReferenceGeneratorTests(unittest.TestCase):
    def test_committed_output(self):
        subprocess.run([sys.executable, str(TOOL), "--check"], check=True)

    def test_idempotent_generation(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "font.inc"
            command = [sys.executable, str(TOOL), "--output", str(output)]
            subprocess.run(command, check=True)
            expected = output.read_bytes()
            self.assertEqual(expected, (ROOT / "src/ui/gameplay_font_reference.inc").read_bytes())
            subprocess.run(command, check=True)
            self.assertEqual(output.read_bytes(), expected)
            subprocess.run(command + ["--check"], check=True)

    def test_unknown_partial_or_mutated_authority_never_writes(self):
        canonical = json.loads(DATA.read_bytes())
        mutations = []
        changed = json.loads(DATA.read_bytes())
        changed["profiles"][0]["glyph_metrics"][0][4] += 1
        mutations.append(changed)
        changed = json.loads(DATA.read_bytes())
        changed["mcc_names"][5] = "conduit-12"
        mutations.append(changed)
        changed = json.loads(DATA.read_bytes())
        changed["profiles"].pop()
        mutations.append(changed)
        mutations.append({"schema": canonical["schema"]})
        with tempfile.TemporaryDirectory() as directory:
            data = Path(directory) / "data.json"
            output = Path(directory) / "font.inc"
            output.write_bytes(b"unchanged")
            for mutation in mutations:
                data.write_text(json.dumps(mutation, separators=(",", ":")) + "\n")
                result = subprocess.run([sys.executable, str(TOOL), "--data", str(data), "--output", str(output)], capture_output=True)
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(output.read_bytes(), b"unchanged")
            output.write_bytes(b"stale")
            result = subprocess.run([sys.executable, str(TOOL), "--output", str(output), "--check"], capture_output=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual(output.read_bytes(), b"stale")


if __name__ == "__main__":
    unittest.main()
