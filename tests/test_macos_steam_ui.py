"""Exercise the actual setup controls with an account-free process fixture."""
import base64
import contextlib
import os
from pathlib import Path
import sys
import tempfile
import time
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from setup_assets_ui import SourceSelector


class ProcessFixture:
    def __init__(self, root, helper, events):
        self.root, self.events, self.started, self.reaped, self.downloads = root, events, False, False, 0
    def start(self): self.started = True
    def shutdown(self): self.reaped = True
    def poll(self): pass
    def download(self): self.downloads += 1


@unittest.skipUnless(os.environ.get('DISPLAY') or sys.platform == 'darwin', 'Tk display required')
class SteamUiTests(unittest.TestCase):
    def setUp(self):
        import tkinter as tk
        self.tk = tk
        self.owner = tempfile.TemporaryDirectory(prefix='steam-ui-', dir=os.environ.get('HALO2_TEST_SCRATCH'))
        self.addCleanup(self.owner.cleanup)
        self.root = Path(self.owner.name).resolve()
        self.window = tk.Tk()
        self.addCleanup(self.cleanup)
        self.imports = []
        self.parent = SourceSelector(self.window, self.root, auto_detect=False, steam_helper=self.root/'helper',
            setup_function=lambda root, selected, **kwargs: self.imports.append((selected, self.process.reaped)))
        self.patch = patch('steam_session.SteamDownloadProcess', ProcessFixture)
        self.patch.start(); self.addCleanup(self.patch.stop)
        self.parent.steam_button.invoke()
        self.dialog = self.parent.steam
        self.process = self.dialog.process
        self.window.update()

    def cleanup(self):
        if hasattr(self, 'parent') and self.parent.steam is not None: self.parent.steam.finish()
        if hasattr(self, 'parent'):
            self.parent.closed = True
            with contextlib.suppress(self.tk.TclError):
                self.window.after_cancel(self.parent.poll_id)
                self.parent.bar.stop()
        with contextlib.suppress(self.tk.TclError): self.window.destroy()

    def plan(self, available=100):
        self.dialog.event('steam', dict(event='plan', files=7, download_bytes=20, bytes=30,
            required_free_bytes=80, available_free_bytes=available))

    def test_qr_is_transient_and_cancel_reaps_before_restoring_manual_import(self):
        image = self.tk.PhotoImage(master=self.window, width=2, height=2)
        image.put('#ffffff', to=(0, 0, 2, 2)); image.write(str(self.root/'qr-fixture.png'), format='png')
        png = base64.b64encode((self.root/'qr-fixture.png').read_bytes()).decode()
        self.dialog.event('steam', dict(event='qr', png=png))
        self.assertIsNotNone(self.dialog.image)
        self.assertTrue(self.parent.working)
        self.assertEqual(str(self.parent.entry.cget('state')), 'disabled')
        self.dialog.cancel_button.invoke()
        self.assertTrue(self.process.reaped)
        self.assertIsNone(self.dialog.image)
        self.assertIsNone(self.parent.steam)
        self.assertFalse(self.parent.working)
        self.assertEqual(str(self.parent.entry.cget('state')), 'normal')
        self.assertEqual(self.imports, [])

    def test_plan_shows_sizes_and_requires_a_click_with_enough_space(self):
        self.plan(10)
        self.assertEqual(str(self.dialog.confirm.cget('state')), 'disabled')
        self.dialog.confirm.invoke(); self.assertEqual(self.process.downloads, 0)
        self.assertIn('free space required', self.dialog.status.cget('text'))
        self.plan(100)
        self.assertEqual(str(self.dialog.confirm.cget('state')), 'normal')
        self.assertEqual(self.process.downloads, 0)
        self.dialog.confirm.invoke(); self.dialog.confirm.invoke()
        self.assertEqual(self.process.downloads, 1)
        self.assertEqual(self.imports, [])

    def test_complete_hands_source_to_existing_import_only_after_reaping(self):
        selected = self.root/'downloads/owned/content/halo2'
        self.dialog.event('steam-finished', dict(source=str(selected)))
        deadline = time.monotonic()+3
        while not self.imports and time.monotonic()<deadline:
            self.window.update(); time.sleep(.01)
        self.assertEqual(self.imports, [(selected, True)])
        self.assertIsNone(self.parent.steam)


if __name__ == '__main__': unittest.main()
