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
        (self.root / "tools/launcher_ui.py").write_text(
            "import json, pathlib, sys\n"
            "pathlib.Path(__file__).with_name('home-call.json').write_text(json.dumps(sys.argv[1:]))\n"
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

    def test_mcc_preparation_works_before_normal_first_run_without_an_iso(self):
        for supplied in ((), ("--mcc-source", str(Path(self.temporary.name) / "MCC"))):
            with self.subTest(arguments=supplied):
                result = self.run_launcher("--prepare-mcc-gameplay", "--setup-only", *supplied)
                self.assertEqual(result.returncode, 23, result.stderr)
                arguments = json.loads((self.root / "tools/setup-call.json").read_text())
                expected = ["--root", str(self.root), "--prepare-mcc-gameplay"]
                if supplied:
                    expected += ["--mcc-root", supplied[1]]
                self.assertEqual(arguments, expected)

    def test_mcc_preparation_does_not_accept_runtime_or_other_source_options(self):
        for supplied in (("--launch-game",), ("--import-mcc-hud",), ("--xbox-source", "donor"), ("--menu",)):
            with self.subTest(arguments=supplied):
                result = self.run_launcher("--prepare-mcc-gameplay", *supplied)
                self.assertEqual(result.returncode, 1, result.stderr)
                self.assertIn("by itself", result.stderr)
                self.assertFalse((self.root / "tools/setup-call.json").exists())

    def ready_assets(self):
        for name in ("xbox/default.xbe", "xbox/maps/mainmenu.map", "xbox/maps/shared.map",
                     "xbox/maps/lockout.map", "xbox/maps/fonts/font_table.txt",
                     "vista/pack.tsv", "vista/hud.tsv", "vista/maps/mainmenu.map",
                     "vista/maps/shared.map", "vista/maps/lockout.map"):
            path = self.root / "assets" / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.touch()
        (self.root / "assets/vista/fonts").mkdir(exist_ok=True)
        (self.root / "assets/asset-sources.tsv").write_text(
            "H2_ASSET_SOURCES_V1\nxbox\tassets/xbox\nvista\tassets/vista\n"
        )
        self.loading_assets(self.root / "assets/vista/loading")

    def loading_assets(self, directory):
        directory.mkdir(parents=True, exist_ok=True)
        (directory / "loading-textures.tsv").write_text("H2V_LOADING_TEXTURES_V1\n" + "row\n" * 12)
        (directory / "loading-tone.xbox-adpcm").write_bytes(bytes(18396))
        for index in range(12):
            (directory / f"loading-{index:02d}.dds").write_bytes(bytes(132))

    def test_selected_vista_match_loading_survives_gameplay_source_changes(self):
        self.ready_assets()
        loading=self.root/"assets/imported/mcc-owned/loading"
        self.loading_assets(loading)
        registry=self.root/"assets/loading-source.tsv"
        registry.write_text("H2_LOADING_SOURCE_V1\nvista\t"+str(loading)+"\n")
        result=self.run_launcher("--launcher-info")
        self.assertEqual(result.returncode,0,result.stderr)
        self.assertIn("Vista loading assets: "+str(loading),result.stdout)
        (loading/"loading-00.dds").unlink()
        result=self.run_launcher("--launcher-info")
        self.assertEqual(result.returncode,1,result.stderr)
        self.assertIn("match-loading bank is incomplete",result.stderr)

    def test_missing_loading_resources_do_not_launch_an_existing_runtime(self):
        self.ready_assets()
        script = self.root / "tools/create_runtime.py"
        script.write_text(self.runtime_fixture())
        subprocess.run([sys.executable, str(script)], check=True)
        (self.root / "assets/vista/loading/loading-11.dds").unlink()
        result = self.run_launcher()
        self.assertEqual(result.returncode, 23, result.stderr)
        self.assertFalse((self.root / "runtime-call.json").exists())
        call = json.loads((self.root / "tools/setup-call.json").read_text())
        self.assertIn("--interactive", call)
        self.assertIn(str(self.root / "assets/xbox"), call)

    def test_old_import_repairs_loading_before_launch_without_rebuilding(self):
        self.ready_assets()
        script = self.root / "tools/create_runtime.py"
        script.write_text(self.runtime_fixture())
        subprocess.run([sys.executable, str(script)], check=True)
        target = self.root / "assets/vista/loading/loading-11.dds"
        target.unlink()
        (self.root / "build-call.json").unlink()
        (self.root / "tools/setup_assets.py").write_text(
            "import pathlib,sys\nassert sys.argv[1:] == "
            f"{['--root', str(self.root), '--repair-loading']!r}\n"
            f"pathlib.Path({str(target)!r}).write_bytes(bytes(132))\n")
        result = self.run_launcher("--skip-intro")
        self.assertEqual(result.returncode, 0, result.stderr)
        args = json.loads((self.root / "runtime-call.json").read_text())
        self.assertEqual(args[args.index("--menu-loading-assets") + 1], str(target.parent))
        self.assertFalse((self.root / "build-call.json").exists())

    def test_complete_legacy_loading_export_remains_usable(self):
        self.ready_assets()
        shutil.rmtree(self.root / "assets/vista/loading")
        legacy = self.root / "assets/menu-ui/vista/loading"
        self.loading_assets(legacy)
        script = self.root / "tools/create_runtime.py"
        script.write_text(self.runtime_fixture())
        subprocess.run([sys.executable, str(script)], check=True)
        result = self.run_launcher()
        self.assertEqual(result.returncode, 0, result.stderr)
        args = json.loads((self.root / "runtime-call.json").read_text())
        self.assertEqual(args[args.index("--menu-loading-assets") + 1], str(legacy))

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
        self.assertEqual(runtime_arguments[runtime_arguments.index("--menu-loading-assets") + 1],
                         str(self.root / "assets/vista/loading"))

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

    def test_existing_runtime_launches_without_requiring_build_tools(self):
        self.ready_assets()
        script = self.root / "tools/create_runtime.py"
        script.write_text(self.runtime_fixture())
        subprocess.run([sys.executable, str(script)], check=True)
        (self.root / "build-call.json").unlink()
        result = self.run_launcher("--inspect")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertFalse((self.root / "build-call.json").exists())
        self.assertTrue((self.root / "runtime-call.json").exists())

    def existing_runtime(self):
        self.ready_assets()
        script = self.root / "tools/create_runtime.py"
        script.write_text(self.runtime_fixture())
        subprocess.run([sys.executable, str(script)], check=True)
        (self.root / "build-call.json").unlink()

    def test_installed_launcher_opens_home_before_play(self):
        self.existing_runtime()
        result = self.run_launcher()
        self.assertEqual(result.returncode, 0, result.stderr)
        arguments = json.loads((self.root / "tools/home-call.json").read_text())
        self.assertEqual(arguments, ["--root", str(self.root), "--build-dir", str(self.root / "build")])
        self.assertTrue((self.root / "runtime-call.json").exists())
        self.assertFalse((self.root / "build-call.json").exists())

    def test_closing_home_does_not_launch_the_runtime(self):
        self.existing_runtime()
        (self.root / "tools/launcher_ui.py").write_text("raise SystemExit(20)\n")
        result = self.run_launcher()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertFalse((self.root / "runtime-call.json").exists())

    def test_home_failure_does_not_launch_the_runtime(self):
        self.existing_runtime()
        (self.root / "tools/launcher_ui.py").write_text("raise SystemExit(19)\n")
        result = self.run_launcher()
        self.assertEqual(result.returncode, 19, result.stderr)
        self.assertFalse((self.root / "runtime-call.json").exists())

    def test_explicit_launch_skips_home_and_does_not_forward_launcher_flag(self):
        self.existing_runtime()
        result = self.run_launcher("--launch-game")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertFalse((self.root / "tools/home-call.json").exists())
        self.assertNotIn("--launch-game", json.loads((self.root / "runtime-call.json").read_text()))

    def test_update_works_before_setup_and_does_not_launch(self):
        (self.root / "tools/update_runtime.py").write_text(
            "import json, pathlib, sys\n"
            "pathlib.Path(__file__).with_name('update-call.json').write_text(json.dumps(sys.argv[1:]))\n"
        )
        result = self.run_launcher("--update")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(json.loads((self.root / "tools/update-call.json").read_text()),
                         ["--root", str(self.root), "--build-dir", str(self.root / "build")])
        self.assertFalse((self.root / "tools/setup-call.json").exists())
        self.assertFalse((self.root / "runtime-call.json").exists())

    def test_update_failure_is_returned_without_launching(self):
        self.existing_runtime()
        (self.root / "tools/update_runtime.py").write_text("raise SystemExit(18)\n")
        result = self.run_launcher("--update")
        self.assertEqual(result.returncode, 18, result.stderr)
        self.assertFalse((self.root / "tools/home-call.json").exists())
        self.assertFalse((self.root / "runtime-call.json").exists())

    def test_pending_update_is_rebuilt_before_a_direct_launch(self):
        self.existing_runtime()
        (self.root / "launcher-update.pending").touch()
        (self.root / "tools/build_runtime.py").write_text(self.runtime_fixture())
        result = self.run_launcher("--launch-game")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertTrue((self.root / "build-call.json").exists())
        self.assertFalse((self.root / "launcher-update.pending").exists())
        self.assertTrue((self.root / "runtime-call.json").exists())

    def test_pending_build_failure_stops_a_direct_launch(self):
        self.existing_runtime()
        (self.root / "launcher-update.pending").touch()
        (self.root / "tools/build_runtime.py").write_text("raise SystemExit(17)\n")
        result = self.run_launcher("--launch-game")
        self.assertEqual(result.returncode, 17, result.stderr)
        self.assertTrue((self.root / "launcher-update.pending").exists())
        self.assertFalse((self.root / "runtime-call.json").exists())

    def test_play_reloads_sources_changed_while_the_home_was_open(self):
        self.existing_runtime()
        selected = self.root / "assets/selected-update.xbe"
        registry = self.root / "assets/gameplay-source.tsv"
        (self.root / "tools/launcher_ui.py").write_text(
            "from pathlib import Path\n"
            f"Path({str(selected)!r}).touch()\n"
            f"Path({str(registry)!r}).write_text('H2_GAMEPLAY_SOURCE_V1\\nxbe\\tassets/selected-update.xbe\\n')\n")
        result = self.run_launcher()
        self.assertEqual(result.returncode, 0, result.stderr)
        arguments = json.loads((self.root / "runtime-call.json").read_text())
        self.assertEqual(arguments[arguments.index("--gameplay-xbe") + 1], str(selected))

    def test_selecting_sources_rebuilds_an_existing_runtime(self):
        self.ready_assets()
        script = self.root / "tools/build_runtime.py"
        script.write_text(self.runtime_fixture())
        subprocess.run([sys.executable, str(script)], check=True)
        (self.root / "build-call.json").unlink()
        self.successful_setup()
        result = self.run_launcher("--setup")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertTrue((self.root / "build-call.json").exists())
        self.assertTrue((self.root / "runtime-call.json").exists())

    @unittest.skipUnless(shutil.which("cmake") and shutil.which("ninja"), "CMake and Ninja required")
    def test_real_cmake_build_is_followed_by_runtime_launch(self):
        self.ready_assets()
        source = Path(__file__).resolve().parents[1] / "tools/build_runtime.py"
        shutil.copy2(source, self.root / "tools/build_runtime.py")
        (self.root / "tools/create_runtime.py").write_text(self.runtime_fixture())
        (self.root / "CMakeLists.txt").write_text(
            "cmake_minimum_required(VERSION 3.24)\nproject(LauncherFixture LANGUAGES NONE)\n"
            "find_package(Python3 REQUIRED COMPONENTS Interpreter)\n"
            'add_custom_target(halo2_launcher COMMAND "${Python3_EXECUTABLE}" '
            '"${CMAKE_SOURCE_DIR}/tools/create_runtime.py" VERBATIM)\n'
            "add_custom_target(halo2_title_launcher)\n"
        )
        result = self.run_launcher("--inspect")
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertTrue((self.root / "build/CMakeCache.txt").is_file())
        self.assertTrue((self.root / "build/launcher-build.log").is_file())
        self.assertIn("--inspect", json.loads((self.root / "runtime-call.json").read_text()))

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
