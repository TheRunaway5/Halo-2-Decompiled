"""Exercise the launcher actions and their menu admission."""
import os
from pathlib import Path
import sys
import tempfile
import unittest
import threading
import time
from unittest.mock import patch

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

    def open(self, ready=True, check=None, wait=True):
        import tkinter as tk
        self.window = tk.Tk()
        self.home = LauncherHome(self.window, self.root, frontend_function=check or (lambda: ready))
        if wait:
            self.wait_for_check()

    def wait_for_check(self):
        deadline = time.monotonic() + 3
        while self.status_text() == 'Checking game files…' and time.monotonic() < deadline:
            self.window.update()
            time.sleep(.005)
        self.assertNotEqual(self.status_text(), 'Checking game files…')

    def status_text(self):
        return self.home.canvas.itemcget(self.home.status, 'text')

    def destroy(self):
        self.home.close()

    def test_play_and_rebuild_are_exposed(self):
        buttons = [child for child in self.window.winfo_children()
                   if child.winfo_class() == "Button" and child.winfo_ismapped()]
        self.assertEqual([str(button.cget("text")) for button in buttons], ["Play", "Rebuild Cache"])
        self.assertGreater(self.home.rebuild_button.winfo_x(), self.home.play_button.winfo_x())
        self.assertFalse(any(child.winfo_class() == "TCombobox" for child in self.window.winfo_children()))
        self.assertEqual(self.status_text(), 'Ready to play')

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

    def test_unavailable_frontend_exposes_repair(self):
        self.home.close()
        self.open(ready=False)
        self.assertTrue(self.home.repair_button.winfo_ismapped())
        self.assertEqual(str(self.home.repair_button.cget('state')), 'normal')

    def open_repair(self):
        class Child:
            def __init__(self, root, events, **options):
                self.events, self.options, self.stopped = events, options, False
            def start(self, selected):
                self.selected = selected
            def poll(self):
                pass
            def shutdown(self):
                self.stopped = True
        with patch('launcher_repair_ui.SetupProcess', Child):
            self.home.repair_button.invoke()
        self.window.update()
        return self.home.repair_dialog

    def test_rebuild_is_available_when_ready_and_never_launches(self):
        class Child:
            def __init__(self, root, events, **options):
                self.options, self.stopped = options, False
            def start(self, selected):
                pass
            def poll(self):
                pass
            def shutdown(self):
                self.stopped = True
        with patch('launcher_repair_ui.SetupProcess', Child):
            self.home.rebuild_button.invoke()
        dialog = self.home.repair_dialog
        self.assertEqual(dialog.process.options['tool'], 'rebuild_asset_cache.py')
        self.assertEqual(str(self.home.play_button.cget('state')), 'disabled')
        self.home.play()
        self.assertFalse(self.home.closed)
        dialog.events.put(('done', None))
        dialog.window.after_cancel(dialog.poll_id)
        dialog.poll()
        self.wait_for_check()
        self.assertTrue(dialog.process.stopped)
        self.assertEqual(str(self.home.rebuild_button.cget('state')), 'normal')
        self.assertEqual(str(self.home.play_button.cget('state')), 'normal')
        self.assertEqual(self.home.result, CANCELLED)

    def test_repair_success_rechecks_without_automatically_playing(self):
        self.home.close()
        ready = [False]
        self.open(check=lambda: ready[0])
        dialog = self.open_repair()
        self.assertEqual(dialog.process.options['tool'], 'repair_launcher.py')
        ready[0] = True
        dialog.events.put(('done', None))
        dialog.window.after_cancel(dialog.poll_id)
        dialog.poll()
        self.wait_for_check()
        self.assertTrue(dialog.process.stopped)
        self.assertIsNone(self.home.repair_dialog)
        self.assertFalse(self.home.repair_button.winfo_ismapped())
        self.assertEqual(str(self.home.play_button.cget('state')), 'normal')
        self.assertFalse(self.home.closed)
        self.assertEqual(self.home.result, CANCELLED)

    def test_failed_repair_keeps_play_disabled_and_can_return(self):
        self.home.close()
        self.open(ready=False)
        dialog = self.open_repair()
        dialog.events.put(('error', 'Repair failed: missing SDL2.dll'))
        dialog.window.after_cancel(dialog.poll_id)
        dialog.poll()
        self.window.update()
        self.assertIn('missing SDL2.dll', dialog.status.cget('text'))
        self.assertEqual(dialog.back.cget('text'), 'Back')
        self.assertTrue(dialog.choose.winfo_ismapped())
        dialog.back.invoke()
        self.wait_for_check()
        self.assertTrue(dialog.process.stopped)
        self.assertEqual(str(self.home.play_button.cget('state')), 'disabled')
        self.assertFalse(self.home.closed)

    def test_opening_home_does_not_create_game_data(self):
        self.assertEqual(list(self.root.iterdir()), [])

    def test_slow_readiness_does_not_block_the_window_or_allow_early_play(self):
        self.home.close()
        release = threading.Event()
        self.addCleanup(release.set)
        self.open(check=lambda: release.wait(3), wait=False)
        self.window.update()
        self.assertEqual(str(self.home.play_button.cget('state')), 'disabled')
        self.home.play()
        self.assertFalse(self.home.closed)
        release.set()
        self.wait_for_check()
        self.assertEqual(str(self.home.play_button.cget('state')), 'normal')

    def test_closing_during_readiness_does_not_launch(self):
        self.home.close()
        release = threading.Event()
        self.addCleanup(release.set)
        self.open(check=lambda: release.wait(3), wait=False)
        self.home.close()
        release.set()
        self.home.readiness.join(1)
        self.assertEqual(self.home.result, CANCELLED)


if __name__ == "__main__":
    unittest.main()
