"""Verify the clone-local Linux entry point without compiling the full game."""
import json
import os
from pathlib import Path
import platform
import shutil
import subprocess
import sys
import tempfile
import unittest

REPO = Path(__file__).resolve().parents[1]


@unittest.skipUnless(platform.system() == "Linux" and platform.machine().lower() in ("x86_64", "amd64"),
                     "Linux x86_64 entry point")
class LinuxBuildScriptTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="halo2-build-entry-")
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name) / "another clone with spaces"
        (self.root / "tools").mkdir(parents=True)
        self.script = self.root / "build-linux.sh"
        shutil.copy2(REPO / "build-linux.sh", self.script)
        (self.root / "tools/build_runtime.py").write_text(
            "import json, os, pathlib, sys\n"
            "pathlib.Path(os.environ['ENTRY_RECEIPT']).write_text(json.dumps({"
            "'argv':sys.argv[1:], 'jobs':os.environ.get('CMAKE_BUILD_PARALLEL_LEVEL')}))\n")
        self.receipt = Path(self.temp.name) / "args.json"

    def run_script(self, *args, **environment):
        return subprocess.run([str(self.script), *args], cwd=self.temp.name,
                              env={**os.environ, "HALO2_PYTHON": sys.executable,
                                   "ENTRY_RECEIPT": str(self.receipt), **environment},
                              capture_output=True, text=True, timeout=10)

    def test_any_working_directory_uses_the_actual_clone_and_default_build(self):
        result = self.run_script(CMAKE_BUILD_PARALLEL_LEVEL="3")
        self.assertEqual(result.returncode, 0, result.stderr)
        receipt = json.loads(self.receipt.read_text())
        self.assertEqual(receipt["argv"], ["--root", str(self.root), "--build-dir", str(self.root / "build"), "--windows-build-dir", str(self.root / "build-windows"), "--no-gui"])
        self.assertEqual(receipt["jobs"], "3")

    def test_relative_and_absolute_build_overrides_and_option_forwarding(self):
        for value, expected in (("custom build", self.root / "custom build"),
                                (str(Path(self.temp.name) / "external build"), Path(self.temp.name) / "external build")):
            with self.subTest(value=value):
                result = self.run_script("--no-gui", HALO2_BUILD_DIR=value)
                self.assertEqual(result.returncode, 0, result.stderr)
                args = json.loads(self.receipt.read_text())["argv"]
                self.assertEqual(args[args.index("--build-dir") + 1], str(expected))
                self.assertEqual(args[-1], "--no-gui")

    def test_help_does_not_start_the_build_and_missing_python_is_actionable(self):
        result = self.run_script("--help", HALO2_PYTHON="missing-python-for-test")
        self.assertEqual(result.returncode, 0)
        self.assertIn("dist/builds/linux-x86_64", result.stdout)
        self.assertFalse(self.receipt.exists())
        result = self.run_script(HALO2_PYTHON="missing-python-for-test")
        self.assertEqual(result.returncode, 1)
        self.assertIn("HALO2_PYTHON", result.stderr)
        self.assertFalse(self.receipt.exists())


if __name__ == "__main__":
    unittest.main()
