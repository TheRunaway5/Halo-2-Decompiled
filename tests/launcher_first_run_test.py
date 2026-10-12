"""Exercise a relocated launcher with setup tools and no built runtime."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest


LAUNCHER = Path(sys.argv.pop(1)).resolve()


class LauncherFirstRunTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="halo2-first-run-")
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name) / "fresh checkout"
        (self.root / "tools").mkdir(parents=True)
        self.launcher = self.root / LAUNCHER.name
        shutil.copy2(LAUNCHER, self.launcher)
        (self.root / "tools/setup_assets.py").write_text(
            "import json, pathlib, sys\n"
            "pathlib.Path(__file__).with_name('setup-call.json').write_text(json.dumps(sys.argv[1:]))\n"
            "raise SystemExit(23)\n"
        )

    def run_launcher(self, *arguments):
        return subprocess.run(
            [str(self.launcher), *arguments], cwd=self.temporary.name,
            env={**os.environ, "SDL_VIDEODRIVER": "dummy"},
            capture_output=True, text=True, timeout=10,
        )

    def test_info_uses_relocated_project_without_another_checkouts_runtime(self):
        result = self.run_launcher("--launcher-info")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("Project: " + str(self.root), result.stdout)
        self.assertIn("Runtime: not built", result.stdout)

    def test_first_launch_opens_setup_before_runtime_exists(self):
        for supplied in ((), ("--setup-only",)):
            with self.subTest(arguments=supplied):
                result = self.run_launcher(*supplied)
                self.assertEqual(result.returncode, 23, result.stderr)
                arguments = json.loads((self.root / "tools/setup-call.json").read_text())
                self.assertEqual(arguments, ["--root", str(self.root), "--interactive"])


if __name__ == "__main__":
    unittest.main()
