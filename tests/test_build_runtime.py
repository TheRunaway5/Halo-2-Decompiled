"""Exercise automatic configuration and stale-cache recovery with real CMake."""
from pathlib import Path
import shutil
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from build_runtime import build_runtime


@unittest.skipUnless(shutil.which("cmake") and shutil.which("ninja"), "CMake and Ninja required")
class RuntimeBuildTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="halo2-build-")
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name) / "fresh checkout"
        self.root.mkdir()
        self.messages = []
        self.project()

    def project(self, fail=False):
        command = "false" if fail else "touch ${CMAKE_BINARY_DIR}/compiled"
        (self.root / "CMakeLists.txt").write_text(
            "cmake_minimum_required(VERSION 3.24)\nproject(LauncherFixture LANGUAGES NONE)\n"
            f"add_custom_target(halo2_launcher COMMAND ${{CMAKE_COMMAND}} -E {command})\n"
            "add_custom_target(halo2_title_launcher)\n"
        )

    def run_build(self):
        build_runtime(self.root, progress=self.messages.append)

    def test_fresh_checkout_configures_and_builds_both_launchers(self):
        self.run_build()
        self.assertTrue((self.root / "build/compiled").is_file())
        log = (self.root / "build/launcher-build.log").read_text()
        self.assertIn("Configuring Halo 2", log)
        self.assertIn("halo2_title_launcher", log)
        self.assertFalse((self.root / "build.previous").exists())

    def test_moved_checkout_preserves_old_build_and_creates_a_working_cache(self):
        self.run_build()
        original_cache = (self.root / "build/CMakeCache.txt").read_bytes()
        (self.root / "build/keep-me").write_text("preserve local files")
        moved = self.root.with_name("moved checkout")
        self.root.rename(moved)
        self.root = moved
        self.run_build()
        self.assertEqual((moved / "build.previous/CMakeCache.txt").read_bytes(), original_cache)
        self.assertEqual((moved / "build.previous/keep-me").read_text(), "preserve local files")
        self.assertIn(str(moved), (moved / "build/CMakeCache.txt").read_text())
        self.assertTrue((moved / "build/compiled").is_file())

    def test_valid_build_is_reused(self):
        self.run_build()
        (self.root / "build/keep-me").touch()
        self.run_build()
        self.assertTrue((self.root / "build/keep-me").is_file())
        self.assertFalse((self.root / "build.previous").exists())

    def test_backup_names_do_not_replace_previous_builds(self):
        build = self.root / "build"
        build.mkdir()
        (build / "CMakeCache.txt").write_text("invalid relocated cache")
        old = self.root / "build.previous"
        old.mkdir()
        (old / "keep-me").touch()
        self.run_build()
        self.assertTrue((old / "keep-me").is_file())
        self.assertEqual((self.root / "build.previous-2/CMakeCache.txt").read_text(), "invalid relocated cache")

    def test_symlink_to_another_build_is_preserved_without_moving_its_target(self):
        external = Path(self.temporary.name) / "another build"
        external.mkdir()
        (external / "CMakeCache.txt").write_text("another project's cache")
        (self.root / "build").symlink_to(external, target_is_directory=True)
        self.run_build()
        self.assertTrue((self.root / "build.previous").is_symlink())
        self.assertEqual((external / "CMakeCache.txt").read_text(), "another project's cache")
        self.assertFalse((self.root / "build").is_symlink())

    def test_build_failure_records_the_compiler_output(self):
        self.project(fail=True)
        with self.assertRaisesRegex(RuntimeError, "Building Halo 2 failed.*launcher-build.log"):
            self.run_build()
        self.assertIn("FAILED", (self.root / "build/launcher-build.log").read_text())
        self.assertFalse((self.root / "build/compiled").exists())

    def test_configuration_failure_stops_before_build(self):
        (self.root / "CMakeLists.txt").write_text("this is not CMake")
        with self.assertRaisesRegex(RuntimeError, "Configuring Halo 2 failed"):
            self.run_build()
        self.assertIn("CMake Error", (self.root / "build/launcher-build.log").read_text())
        self.assertFalse((self.root / "build/compiled").exists())

    def test_missing_dependency_leaves_the_old_build_in_place(self):
        (self.root / "build").mkdir()
        (self.root / "build/keep-me").touch()
        with patch("build_runtime.shutil.which", return_value=None):
            with self.assertRaisesRegex(RuntimeError, "CMake is required"):
                self.run_build()
        self.assertTrue((self.root / "build/keep-me").is_file())
        self.assertFalse((self.root / "build.previous").exists())

    def test_source_directory_cannot_be_replaced_by_a_build(self):
        for directory in (self.root, self.root.parent):
            with self.subTest(directory=directory):
                with self.assertRaisesRegex(RuntimeError, "separate build directory"):
                    build_runtime(self.root, directory, self.messages.append)
                self.assertTrue((self.root / "CMakeLists.txt").is_file())


if __name__ == "__main__":
    unittest.main()
