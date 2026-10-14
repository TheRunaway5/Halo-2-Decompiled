"""Exercise actual MCC setup controls under a desktop or xvfb-run."""
import os
from pathlib import Path
import sys
import tempfile
import threading
import time
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from setup_assets_ui import SourceSelector


@unittest.skipUnless(os.environ.get('DISPLAY'), 'Tk display required (use xvfb-run)')
class MccSetupUiTests(unittest.TestCase):
    def setUp(self):
        import tkinter as tk
        self.window = tk.Tk()
        self.temporary = tempfile.TemporaryDirectory(prefix='halo2-mcc-ui-')
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.calls = []
        self.release = threading.Event()
        self.scan_release = threading.Event()
        self.failure = None
        self.ui = None
        self.addCleanup(self.destroy)

    def destroy(self):
        self.release.set()
        self.scan_release.set()
        try:
            if self.ui and not self.ui.closed:
                self.window.after_cancel(self.ui.poll_id)
                self.window.destroy()
        except Exception:
            pass

    def wait_until(self, condition):
        end = time.monotonic() + 5
        while not condition() and time.monotonic() < end:
            self.window.update()
            time.sleep(.005)
        self.assertTrue(condition(), 'Setup did not reach the expected state')

    def setup(self, root, source, progress):
        self.calls.append((root, source))
        progress('Checking copied MCC assets…')
        if not self.release.wait(5):
            raise RuntimeError('test timeout')
        if self.failure:
            raise RuntimeError(self.failure)

    def make(self, **kwargs):
        self.ui = SourceSelector(self.window, self.root, setup_function=self.setup, **kwargs)
        self.window.update()
        return self.ui

    def test_startup_detects_mcc_off_ui_thread_and_updates_single_path(self):
        main = threading.get_ident()
        threads = []
        def discover():
            threads.append(threading.get_ident())
            self.scan_release.wait(5)
            return self.root / 'Halo MCC/halo2'
        ui = self.make(discover_function=discover)
        self.wait_until(lambda: bool(threads))
        self.assertTrue(ui.scanning)
        self.assertNotEqual(threads[0], main)
        self.assertEqual(str(ui.prepare.cget('state')), 'disabled')
        ui.start()
        self.assertEqual(self.calls, [])
        self.scan_release.set()
        self.wait_until(lambda: not ui.scanning)
        self.assertEqual(ui.value.get(), str(self.root / 'Halo MCC/halo2'))
        self.assertIn('Found MCC', ui.status_label.cget('text'))
        self.assertEqual(str(ui.prepare.cget('state')), 'normal')

    def test_missing_detected_installation_keeps_manual_folder_selection(self):
        def discover():
            raise ValueError('not installed')
        ui = self.make(discover_function=discover)
        self.wait_until(lambda: not ui.scanning)
        self.assertIn('not found', ui.status_label.cget('text'))
        with patch.object(ui.filedialog, 'askdirectory', return_value=str(self.root / 'custom MCC')):
            ui.browse_button.invoke()
        self.assertEqual(ui.value.get(), str(self.root / 'custom MCC'))

    def test_prepare_passes_one_source_and_blocks_changes_until_done(self):
        ui = self.make(mcc=self.root / 'Halo MCC')
        ui.prepare.invoke()
        self.wait_until(lambda: bool(self.calls))
        self.assertEqual(self.calls, [(self.root, self.root / 'Halo MCC')])
        self.assertTrue(ui.working)
        for widget in ui.controls + [ui.prepare, ui.cancel]:
            self.assertEqual(str(widget.cget('state')), 'disabled')
        ui.close()
        ui.start()
        ui.scan()
        self.assertFalse(ui.closed)
        self.assertEqual(len(self.calls), 1)
        self.release.set()
        self.wait_until(lambda: ui.closed)
        self.assertEqual(ui.result, 0)

    def test_blank_source_uses_automatic_detection_in_importer(self):
        ui = self.make(auto_detect=False)
        ui.start()
        self.wait_until(lambda: bool(self.calls))
        self.assertEqual(self.calls, [(self.root, None)])
        self.release.set()
        self.wait_until(lambda: ui.closed)

    def test_cancel_during_scan_does_not_import(self):
        def discover():
            self.scan_release.wait(5)
            return self.root / 'MCC'
        ui = self.make(discover_function=discover)
        ui.cancel.invoke()
        self.assertTrue(ui.closed)
        self.assertEqual(ui.result, 1)
        self.assertEqual(self.calls, [])

    def test_error_dialog_is_modal_and_prevents_underlying_import(self):
        ui = self.make(auto_detect=False)
        self.failure = 'incomplete MCC Classic installation'
        self.release.set()
        observed = []
        def dismiss():
            dialog = self.window.grab_current()
            if dialog is None:
                self.window.after(10, dismiss)
                return
            observed.append((ui.modal, dialog.winfo_toplevel() != self.window))
            ui.start()
            ui.scan()
            ui.close()
            self.assertFalse(ui.closed)
            self.assertEqual(len(self.calls), 1)
            dialog.destroy()
        self.window.after(10, dismiss)
        ui.start()
        self.wait_until(lambda: ui.error and not ui.modal)
        self.assertEqual(observed, [(True, True)])
        self.assertIsNone(self.window.grab_current())
        self.assertIn('did not complete', ui.status_label.cget('text'))
        self.assertEqual(str(ui.prepare.cget('state')), 'normal')

    def test_controls_and_visible_text_do_not_overlap(self):
        ui = self.make(auto_detect=False)
        widgets = ui.controls + [ui.status_label, ui.bar, ui.cancel, ui.prepare]
        rectangles = []
        for w in widgets:
            x, y, width, height = w.winfo_x(), w.winfo_y(), w.winfo_width(), w.winfo_height()
            self.assertGreaterEqual(x, 0)
            self.assertGreaterEqual(y, 0)
            self.assertLessEqual(x + width, self.window.winfo_width())
            self.assertLessEqual(y + height, self.window.winfo_height())
            for ox, oy, ow, oh in rectangles:
                self.assertFalse(x < ox + ow and x + width > ox and y < oy + oh and y + height > oy)
            rectangles.append((x, y, width, height))
        for item in ui.canvas.find_all():
            if ui.canvas.type(item) != 'text':
                continue
            x, y, right, bottom = ui.canvas.bbox(item)
            self.assertGreaterEqual(x, 0)
            self.assertLessEqual(right, self.window.winfo_width())
            self.assertLessEqual(bottom, self.window.winfo_height())
            for ox, oy, ow, oh in rectangles:
                self.assertFalse(x < ox + ow and right > ox and y < oy + oh and bottom > oy,
                                 ui.canvas.itemcget(item, 'text'))


if __name__ == '__main__':
    unittest.main()
