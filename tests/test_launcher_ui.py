"""Exercise the actual single-button launcher and its menu admission."""
import os
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from launcher_ui import CANCELLED, LauncherHome


@unittest.skipUnless(os.environ.get("DISPLAY"), "Tk display required (use xvfb-run)")
class LauncherHomeTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="halo2-home-")
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.addCleanup(self.destroy)
        self.open()

    def open(self, ready=True):
        import tkinter as tk
        self.window = tk.Tk()
        self.home = LauncherHome(self.window, self.root, frontend_function=lambda: ready)
        self.window.update()

    def destroy(self):
        self.home.close()

    def test_only_play_is_exposed(self):
        buttons = [child for child in self.window.winfo_children() if child.winfo_class() == "Button"]
        self.assertEqual([str(button.cget("text")) for button in buttons], ["Play"])
        self.assertFalse(any(child.winfo_class() == "TCombobox" for child in self.window.winfo_children()))
        self.assertIn("intro, title screen and main menu", self.home.status.cget("text"))

    def test_play_returns_to_native_launcher(self):
        self.home.play_button.invoke()
        self.assertTrue(self.home.closed)
        self.assertEqual(self.home.result, 0)

    def test_close_does_not_launch(self):
        self.home.close()
        self.assertEqual(self.home.result, CANCELLED)

    def test_unavailable_frontend_does_not_launch_an_unrelated_test(self):
        self.home.close()
        self.open(ready=False)
        self.assertEqual(str(self.home.play_button.cget("state")), "disabled")
        self.home.play()
        self.assertFalse(self.home.closed)
        self.assertEqual(self.home.result, CANCELLED)

    def test_opening_home_does_not_create_game_data(self):
        self.assertEqual(list(self.root.iterdir()), [])


if __name__ == "__main__":
    unittest.main()
