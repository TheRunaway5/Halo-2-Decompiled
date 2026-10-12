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
        environment = {**os.environ, "SDL_VIDEODRIVER": "dummy"}
        environment.pop("DISPLAY", None)
        environment.pop("WAYLAND_DISPLAY", None)
        return subprocess.run(
            [str(self.launcher), *arguments], cwd=self.temporary.name,
            env=environment,
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

    def test_source_flags_require_paths_before_running_setup(self):
        for flag in ("--vista-source", "--xbox-source", "--mcc-source"):
            for value in ((), ("",), ("--setup-only",), ("--xbox-source",)):
                with self.subTest(flag=flag, value=value):
                    result = self.run_launcher("--setup-only", flag, *value)
                    self.assertEqual(result.returncode, 1, result.stderr)
                    self.assertIn("missing value for " + flag, result.stderr)
                    self.assertIn("file or folder path", result.stderr)
                    self.assertFalse((self.root / "tools/setup-call.json").exists())

    def test_explicit_source_paths_skip_the_gui(self):
        vista = str(Path(self.temporary.name) / "Vista installer.zip")
        xbox = str(Path(self.temporary.name) / "Halo 2.iso")
        result = self.run_launcher(
            "--setup-only", "--vista-source", vista, "--xbox-source", xbox,
        )
        self.assertEqual(result.returncode, 23, result.stderr)
        arguments = json.loads((self.root / "tools/setup-call.json").read_text())
        self.assertEqual(arguments, ["--root", str(self.root), "--vista", vista, "--xbox", xbox])

    def ready_assets(self):
        for name in ("xbox/default.xbe", "xbox/maps/mainmenu.map", "xbox/maps/shared.map",
                     "xbox/maps/lockout.map", "xbox/maps/fonts/font_table.txt",
                     "vista/pack.tsv", "vista/hud.tsv", "vista/maps/mainmenu.map",
                     "vista/maps/shared.map", "vista/maps/lockout.map"):
            path = self.root / "assets" / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.touch()
        (self.root / "assets/vista/fonts").mkdir()
        (self.root / "assets/asset-sources.tsv").write_text(
            "H2_ASSET_SOURCES_V1\nxbox\tassets/xbox\nvista\tassets/vista\n"
        )

    def successful_setup(self):
        self.ready_assets()
        registry = self.root / "assets/asset-sources.tsv"
        saved = registry.read_text()
        registry.unlink()
        (self.root / "tools/setup_assets.py").write_text(
            "from pathlib import Path\n"
            f"Path({str(registry)!r}).write_text({saved!r})\n"
            f"Path({str(self.root / 'setup-finished')!r}).touch()\n"
        )

    def runtime_fixture(self):
        script = (
            "#!/usr/bin/env python3\n"
            "import json, pathlib, sys\n"
            "pathlib.Path('runtime-call.json').write_text(json.dumps(sys.argv[1:]))\n"
        )
        names = ["world.vert", "world.frag", "world_emblem.geom", "world_emblem.frag",
                 "fog.vert", "fog.frag", "water_copy.frag", "bsp_patchy_noise.frag",
                 "bsp_patchy_composite.frag", "patchy_noise.frag", "patchy_composite.frag",
                 "bloom.vert", "bloom.frag", "crt_oled.frag", "display.frag", "overlay.vert",
                 "overlay.frag", "ui.vert", "ui.frag", "hud.frag"]
        return (
            "import json, pathlib, sys\n"
            f"root = pathlib.Path({str(self.root)!r})\n"
            "(root / 'build-call.json').write_text(json.dumps(sys.argv[1:]))\n"
            "build = root / 'build'\n"
            "(build / 'shaders').mkdir(parents=True, exist_ok=True)\n"
            f"for name in {names!r}: (build / 'shaders' / (name + '.spv')).touch()\n"
            "runtime = build / 'halo2_native'\n"
            f"runtime.write_text({script!r})\n"
            "runtime.chmod(0o755)\n"
        )

    def test_successful_setup_builds_and_launches_automatically(self):
        self.successful_setup()
        (self.root / "tools/build_runtime.py").write_text(
            f"from pathlib import Path\nassert Path({str(self.root / 'setup-finished')!r}).exists()\n"
            + self.runtime_fixture()
        )
        result = self.run_launcher()
        self.assertEqual(result.returncode, 0, result.stderr)
        build_arguments = json.loads((self.root / "build-call.json").read_text())
        self.assertEqual(build_arguments, ["--root", str(self.root), "--build-dir", str(self.root / "build")])
        runtime_arguments = json.loads((self.root / "runtime-call.json").read_text())
        self.assertIn("--menu-screen", runtime_arguments)

    def test_prepared_checkout_builds_and_forwards_runtime_arguments(self):
        self.ready_assets()
        (self.root / "tools/build_runtime.py").write_text(self.runtime_fixture())
        result = self.run_launcher("--inspect")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("--inspect", json.loads((self.root / "runtime-call.json").read_text()))

    def test_build_failure_stops_launch_and_preserves_exit_status(self):
        self.ready_assets()
        (self.root / "tools/build_runtime.py").write_text("raise SystemExit(17)\n")
        result = self.run_launcher()
        self.assertEqual(result.returncode, 17, result.stderr)
        self.assertFalse((self.root / "runtime-call.json").exists())

    def test_setup_only_prepares_without_building_or_launching(self):
        self.successful_setup()
        result = self.run_launcher("--setup-only")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertTrue((self.root / "setup-finished").exists())
        self.assertFalse((self.root / "build-call.json").exists())
        self.assertFalse((self.root / "runtime-call.json").exists())

    def test_moved_build_is_rebuilt_instead_of_launching_the_old_runtime(self):
        self.ready_assets()
        script = self.root / "tools/build_runtime.py"
        script.write_text(self.runtime_fixture())
        subprocess.run([sys.executable, str(script)], check=True)
        (self.root / "build/CMakeCache.txt").write_text(
            "CMAKE_HOME_DIRECTORY:INTERNAL=/old/project\n"
            "CMAKE_CACHEFILE_DIR:INTERNAL=/old/project/build\n"
        )
        (self.root / "build-call.json").unlink()
        script.write_text("raise SystemExit(17)\n")
        result = self.run_launcher()
        self.assertEqual(result.returncode, 17, result.stderr)
        self.assertFalse((self.root / "runtime-call.json").exists())


if __name__ == "__main__":
    unittest.main()
