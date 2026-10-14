"""Exercise the real Tk launcher controls with an isolated update operation."""
import os
from pathlib import Path
import sys
import tempfile
import threading
import time
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from launcher_ui import CANCELLED, LauncherHome


@unittest.skipUnless(os.environ.get("DISPLAY"), "Tk display required (use xvfb-run)")
class LauncherHomeTests(unittest.TestCase):
    def setUp(self):
        import tkinter as tk
        self.window = tk.Tk()
        self.temporary = tempfile.TemporaryDirectory(prefix="halo2-home-")
        self.addCleanup(self.temporary.cleanup)
        self.addCleanup(self.destroy)
        self.root = Path(self.temporary.name)
        self.release = threading.Event()
        self.calls = []
        self.failure = False

        def update(root, build, progress):
            self.calls.append((root, build))
            progress("Downloading updates…")
            if not self.release.wait(5):
                raise RuntimeError("Test update timed out")
            if self.failure:
                raise RuntimeError("The compiler failed")
            (root / "updated-runtime").touch()

        self.home = LauncherHome(self.window, self.root, self.root / "build", update, availability_function=lambda: None)
        self.window.update()

    def destroy(self):
        self.release.set()
        try:
            self.window.after_cancel(self.home.poll_id)
            self.window.destroy()
        except Exception:
            pass

    def wait_until(self, condition):
        deadline = time.monotonic() + 5
        while not condition() and time.monotonic() < deadline:
            self.window.update()
            time.sleep(0.005)
        self.assertTrue(condition(), "Launcher did not reach the expected state")

    def test_update_disables_actions_and_success_waits_for_play(self):
        self.home.update_button.invoke()
        self.wait_until(lambda: bool(self.calls))
        self.assertEqual(self.calls, [(self.root, self.root / "build")])
        for button in (self.home.play_button, self.home.update_button, self.home.exit_button):
            self.assertEqual(str(button.cget("state")), "disabled")
        self.home.close()
        self.assertFalse(self.home.closed)
        self.release.set()
        self.wait_until(lambda: not self.home.working)
        self.assertTrue((self.root / "updated-runtime").exists())
        self.assertFalse(self.home.closed)
        self.assertEqual(self.home.result, CANCELLED)
        self.assertEqual(str(self.home.play_button.cget("state")), "normal")
        self.home.play_button.invoke()
        self.assertTrue(self.home.closed)
        self.assertEqual(self.home.result, 0)

    def test_failure_keeps_output_and_requires_a_successful_retry_before_play(self):
        self.failure = True
        self.release.set()
        self.home.update_button.invoke()
        self.wait_until(lambda: not self.home.working)
        self.assertEqual(str(self.home.play_button.cget("state")), "disabled")
        self.assertIn("The compiler failed", self.home.output.get("1.0", "end"))
        self.home.play_button.invoke()
        self.assertFalse(self.home.closed)
        self.failure = False
        self.home.update_button.invoke()
        self.wait_until(lambda: not self.home.working)
        self.assertEqual(str(self.home.play_button.cget("state")), "normal")
        self.assertIn("Update complete", self.home.status.cget("text"))

    def test_incomplete_mcc_conversion_stays_disabled_after_a_successful_update(self):
        self.home.availability = lambda: "MCC native gameplay conversion is incomplete."
        self.home.unavailable = self.home.availability()
        self.release.set()
        self.home.update_button.invoke()
        self.wait_until(lambda: not self.home.working)
        self.assertEqual(str(self.home.play_button.cget("state")), "disabled")
        self.assertIn("conversion is incomplete", self.home.status.cget("text"))
        self.home.play_button.invoke()
        self.assertFalse(self.home.closed)

    def test_exit_does_not_play_or_update(self):
        self.home.exit_button.invoke()
        self.assertEqual(self.home.result, CANCELLED)
        self.assertEqual(self.calls, [])

    def test_pointer_click_on_play_starts_game(self):
        button = self.home.play_button
        button.event_generate("<Enter>", x=10, y=10)
        self.window.update()
        button.event_generate("<ButtonPress-1>", x=10, y=10)
        button.event_generate("<ButtonRelease-1>", x=10, y=10)
        self.assertTrue(self.home.closed, "A real pointer click must hand off to the game")
        self.assertEqual(self.home.result, 0)

    def test_reopening_an_incomplete_update_requires_retry(self):
        import tkinter as tk
        self.home.close()
        (self.root / "launcher-update.pending").touch()
        self.window = tk.Tk()
        self.home = LauncherHome(self.window, self.root)
        self.window.update()
        self.assertEqual(str(self.home.play_button.cget("state")), "disabled")
        self.assertIn("Select Update", self.home.status.cget("text"))
        self.home.play_button.invoke()
        self.assertFalse(self.home.closed)

    def test_controls_have_separate_hit_regions_and_fit_the_window(self):
        widgets = [self.home.status, self.home.output.master, self.home.bar,
                   self.home.exit_button, self.home.update_button, self.home.play_button]
        rectangles = []
        for widget in widgets:
            x, y, w, h = widget.winfo_x(), widget.winfo_y(), widget.winfo_width(), widget.winfo_height()
            self.assertGreaterEqual(x, 0)
            self.assertGreaterEqual(y, 0)
            self.assertLessEqual(x + w, self.window.winfo_width())
            self.assertLessEqual(y + h, self.window.winfo_height())
            for ox, oy, ow, oh in rectangles:
                self.assertFalse(x < ox + ow and x + w > ox and y < oy + oh and y + h > oy)
            rectangles.append((x, y, w, h))


if __name__ == "__main__":
    unittest.main()
