"""Exercise the updater against local Git repositories, without a network service."""
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from update_runtime import update_runtime


@unittest.skipUnless(shutil.which("git"), "Git required")
class UpdateRuntimeTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="halo2-update-")
        self.addCleanup(self.temporary.cleanup)
        self.base = Path(self.temporary.name)
        self.remote = self.base / "remote.git"
        self.source = self.base / "developer checkout"
        self.root = self.base / "testing checkout"
        self.git(self.base, "init", "--bare", str(self.remote))
        self.git(self.base, "clone", str(self.remote), str(self.source))
        self.identity(self.source)
        (self.source / "tools").mkdir()
        (self.source / ".gitignore").write_text("/assets/\n/build*/\n*.log\n")
        (self.source / "CMakeLists.txt").touch()
        self.build_helper("old")
        self.commit("initial")
        self.git(self.source, "push", "-u", "origin", "HEAD")
        self.git(self.base, "clone", str(self.remote), str(self.root))
        self.identity(self.root)
        (self.root / "assets").mkdir()
        (self.root / "assets/local-game-files").write_bytes(b"keep imported game data")
        self.messages = []

    def git(self, root, *arguments):
        return subprocess.run(["git", "-C", str(root), *arguments], check=True,
                              capture_output=True, text=True).stdout.strip()

    def identity(self, root):
        self.git(root, "config", "user.name", "Updater test")
        self.git(root, "config", "user.email", "updater@example.invalid")
        self.git(root, "config", "commit.gpgsign", "false")

    def build_helper(self, version, fail=False):
        (self.source / "tools/build_runtime.py").write_text(
            "import argparse\nfrom pathlib import Path\n"
            "p=argparse.ArgumentParser()\np.add_argument('--root', type=Path)\n"
            "p.add_argument('--build-dir', type=Path)\np.add_argument('--no-gui', action='store_true')\n"
            "a=p.parse_args()\nassert a.no_gui\na.build_dir.mkdir(parents=True, exist_ok=True)\n"
            f"(a.build_dir/'built-version').write_text({version!r})\n"
            f"raise SystemExit({9 if fail else 0})\n")

    def commit(self, message):
        self.git(self.source, "add", ".")
        self.git(self.source, "commit", "-m", message)

    def publish(self, version="new", fail=False):
        self.build_helper(version, fail)
        self.commit(version)
        self.git(self.source, "push")

    def update(self, build=None):
        update_runtime(self.root, build, progress=self.messages.append)

    def test_update_pulls_and_executes_the_new_build_helper_and_keeps_game_files(self):
        self.publish()
        build = self.root / "build release"
        self.update(build)
        self.assertEqual((build / "built-version").read_text(), "new")
        self.assertEqual(self.git(self.root, "rev-parse", "HEAD"), self.git(self.source, "rev-parse", "HEAD"))
        self.assertEqual((self.root / "assets/local-game-files").read_bytes(), b"keep imported game data")
        self.assertIn("Update complete", (self.root / "launcher-update.log").read_text())
        self.assertEqual(self.git(self.root, "status", "--porcelain"), "")

    def test_already_current_checkout_still_builds_changes_previously_pulled(self):
        self.publish()
        self.git(self.root, "pull", "--ff-only")
        self.update()
        self.assertEqual((self.root / "build/built-version").read_text(), "new")

    def test_dirty_checkout_is_preserved_without_pulling_or_building(self):
        self.publish()
        head = self.git(self.root, "rev-parse", "HEAD")
        (self.root / "CMakeLists.txt").write_text("my local changes")
        with self.assertRaisesRegex(RuntimeError, "local changes"):
            self.update()
        self.assertEqual(self.git(self.root, "rev-parse", "HEAD"), head)
        self.assertEqual((self.root / "CMakeLists.txt").read_text(), "my local changes")
        self.assertFalse((self.root / "build").exists())

    def test_diverged_branch_stops_without_merging_or_building(self):
        (self.root / "CMakeLists.txt").write_text("local commit")
        self.git(self.root, "add", ".")
        self.git(self.root, "commit", "-m", "local change")
        head = self.git(self.root, "rev-parse", "HEAD")
        self.publish()
        with self.assertRaisesRegex(RuntimeError, "Downloading updates failed"):
            self.update()
        self.assertEqual(self.git(self.root, "rev-parse", "HEAD"), head)
        self.assertFalse((self.root / "build").exists())

    def test_build_failure_is_reported_and_the_output_is_saved(self):
        self.publish(fail=True)
        with self.assertRaisesRegex(RuntimeError, "Building the updated game failed"):
            self.update()
        self.assertEqual((self.root / "build/built-version").read_text(), "new")
        self.assertNotIn("Update complete", (self.root / "launcher-update.log").read_text())

    def test_failed_build_remains_pending_until_a_successful_retry(self):
        self.publish(fail=True)
        with self.assertRaises(RuntimeError):
            self.update()
        self.assertTrue((self.root / "launcher-update.pending").exists())
        self.publish("fixed")
        self.update()
        self.assertFalse((self.root / "launcher-update.pending").exists())
        self.assertEqual((self.root / "build/built-version").read_text(), "fixed")

    @unittest.skipUnless(shutil.which("cmake") and shutil.which("ninja"), "CMake and Ninja required")
    def test_downloaded_sources_are_built_with_real_cmake(self):
        shutil.copy2(Path(__file__).resolve().parents[1] / "tools/build_runtime.py",
                     self.source / "tools/build_runtime.py")
        (self.source / "CMakeLists.txt").write_text(
            "cmake_minimum_required(VERSION 3.24)\nproject(UpdateFixture LANGUAGES NONE)\n"
            "add_custom_target(halo2_launcher COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_BINARY_DIR}/new-version)\n"
            "add_custom_target(halo2_title_launcher)\n")
        self.commit("CMake build fixture")
        self.git(self.source, "push")
        self.update()
        self.assertTrue((self.root / "build/new-version").is_file())
        self.assertIn("Build complete.", (self.root / "launcher-update.log").read_text())

    def test_missing_upstream_stops_before_build(self):
        self.git(self.root, "branch", "--unset-upstream")
        with self.assertRaisesRegex(RuntimeError, "no upstream"):
            self.update()
        self.assertFalse((self.root / "build").exists())

    def test_failed_download_stops_before_build(self):
        self.git(self.root, "remote", "set-url", "origin", str(self.base / "missing.git"))
        with self.assertRaisesRegex(RuntimeError, "Downloading updates failed"):
            self.update()
        self.assertFalse((self.root / "build").exists())


if __name__ == "__main__":
    unittest.main()
