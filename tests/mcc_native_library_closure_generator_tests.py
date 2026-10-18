import copy
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("native_closure_generator", ROOT / "tools/generate_mcc_native_library_closure.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


class NativeLibraryClosureGeneratorTests(unittest.TestCase):
    def test_checked_generated_policy(self):
        raw = (ROOT / "tools/reference/mcc-native-library-closure.json").read_bytes()
        self.assertEqual(MODULE.generate(raw), (ROOT / "src/mcc_native_library_closure.inc").read_text())
        self.assertEqual(raw, (ROOT / "tools/data/mcc-native-library-closure.json").read_bytes())

    def test_unowned_or_partial_policy_is_rejected(self):
        original = json.loads((ROOT / "tools/reference/mcc-native-library-closure.json").read_bytes())
        cases = []
        def changed(edit):
            data = copy.deepcopy(original)
            edit(data)
            cases.append(data)
        changed(lambda d: d.update(schema="unknown"))
        changed(lambda d: d.update(source_map="lockout"))
        changed(lambda d: d.update(source_canonical_sha256="0"))
        changed(lambda d: d["closure"].pop())
        changed(lambda d: d["closure"].__setitem__(0,d["closure"][1]))
        changed(lambda d: d["closure"].__setitem__(0,["shad","unknown"]))
        changed(lambda d: d["libraries"].pop())
        changed(lambda d: d["libraries"].__setitem__(0,d["libraries"][1]))
        changed(lambda d: d["libraries"][0].update(map="../unknown"))
        changed(lambda d: d["libraries"][0].update(canonical_sha256="unknown"))
        changed(lambda d: d["libraries"][0].update(existing_records=0))
        changed(lambda d: d["libraries"][0].update(missing=[["stem","unknown"]]))
        changed(lambda d: d["evidence"].update(semantic_audit_sha256="unknown"))
        for data in cases:
            with self.subTest(data=data), self.assertRaises(ValueError):
                MODULE.generate(json.dumps(data).encode())

    def test_cli_publishes_and_checks_the_shipped_policy_mirror(self):
        with tempfile.TemporaryDirectory(prefix="h2-native-closure-policy-") as temporary:
            output = Path(temporary) / "closure.inc"
            mirror = Path(temporary) / "data/closure.json"
            command = [sys.executable, str(ROOT / "tools/generate_mcc_native_library_closure.py"),
                       "--output", str(output), "--runtime-policy", str(mirror)]
            result = subprocess.run(command, capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(mirror.read_bytes(), (ROOT / "tools/reference/mcc-native-library-closure.json").read_bytes())
            result = subprocess.run([*command, "--check"], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            mirror.write_bytes(mirror.read_bytes() + b"\n")
            result = subprocess.run([*command, "--check"], capture_output=True, text=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("Shipped retained native library closure policy is out of date", result.stderr)


if __name__ == "__main__":
    unittest.main()
