"""Exercise actual MCC setup controls under a desktop or xvfb-run."""
import os
import contextlib
import json
from pathlib import Path
import queue
import shlex
import signal
import subprocess
import sys
import tempfile
import threading
import time
from types import SimpleNamespace
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from setup_assets_ui import SetupProcess, SourceSelector
from setup_pipe import read_available
from file_lock import file_lock


TOOLS = Path(__file__).resolve().parents[1] / 'tools'


class SetupPipeTests(unittest.TestCase):
    def test_idle_data_and_eof_are_distinct_and_bounded(self):
        read, write = os.pipe()
        try:
            started = time.monotonic()
            self.assertIsNone(read_available(read, 4096, .025))
            self.assertLess(time.monotonic() - started, 1)
            os.write(write, b'progress\n')
            self.assertEqual(read_available(read, 4, .1), b'prog')
            self.assertEqual(read_available(read, 4096, .1), b'ress\n')
            os.close(write); write = None
            self.assertEqual(read_available(read, 4096, .1), b'')
        finally:
            os.close(read)
            if write is not None:
                os.close(write)

    def test_invalid_descriptor_is_an_error_not_eof(self):
        with self.assertRaises((OSError, ValueError)):
            read_available(-1, 4096, 0)

    def test_exited_child_with_inherited_open_stdout_cannot_report_success_or_hang(self):
        read, write = os.pipe()
        worker = SetupProcess(Path.cwd(), queue.Queue())
        worker.process = SimpleNamespace(stdout=os.fdopen(read, 'rb'), stdin=None,
                                         poll=lambda: 0, wait=lambda: 0)
        worker.reader = threading.Thread(target=worker._read, args=(worker.process,))
        reader = worker.reader
        reader.start()
        try:
            started = time.monotonic()
            worker.poll()
            self.assertLess(time.monotonic() - started, 1)
            self.assertFalse(reader.is_alive())
            self.assertIsNone(worker.process)
            self.assertIsNone(worker.reader)
            result = worker.events.get_nowait()
            self.assertEqual(result[0], 'error')
            self.assertIn('output did not close', result[1])
            self.assertTrue(worker.events.empty())
        finally:
            os.close(write)
            worker.shutdown()


class SetupFileLockTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix='halo2-setup-lock-',
                                                     dir=os.environ.get('HALO2_TEST_SCRATCH'))
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.path = self.root / 'owned.lock'
        self.fd = os.open(self.path, os.O_RDWR | os.O_CREAT, 0o600)
        self.addCleanup(os.close, self.fd)

    def child(self, mode):
        script = '''import os,sys
from pathlib import Path
sys.path.insert(0,TOOLS)
from file_lock import file_lock
path=Path(sys.argv[1]);mode=sys.argv[2]
fd=os.open(path,os.O_RDWR)
try:
    if mode=='wait': path.with_suffix('.waiting').touch()
    with file_lock(fd,shared=mode=='shared',blocking=mode=='wait'):
        path.with_suffix('.acquired').touch()
except BlockingIOError: raise SystemExit(23)
finally: os.close(fd)
'''.replace('TOOLS', repr(str(TOOLS)))
        return subprocess.Popen([sys.executable, '-B', '-c', script, str(self.path), mode],
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    def attempt(self, mode):
        with self.child(mode) as process:
            stdout, stderr = process.communicate(timeout=20)
            self.assertEqual(stdout, b'')
            self.assertEqual(stderr, b'')
            return process.returncode

    def test_exclusive_lock_excludes_another_process_and_release_allows_retry(self):
        os.lseek(self.fd, 7, os.SEEK_SET)
        with file_lock(self.fd):
            self.assertEqual(self.attempt('exclusive'), 23)
            self.assertEqual(self.attempt('shared'), 23)
            self.assertEqual(os.lseek(self.fd, 0, os.SEEK_CUR), 7)
        self.assertEqual(self.attempt('exclusive'), 0)

    def test_shared_lock_allows_other_readers_and_excludes_a_writer(self):
        with file_lock(self.fd, shared=True):
            self.assertEqual(self.attempt('shared'), 0)
            self.assertEqual(self.attempt('exclusive'), 23)

    def test_actual_mcc_bank_lock_excludes_another_importer(self):
        from mcc_movies import _lock
        self.path = self.root / '.bundled-movies.lock'
        with _lock(self.root):
            self.assertEqual(self.attempt('exclusive'), 23)
        self.assertEqual(self.attempt('exclusive'), 0)

    def test_blocking_acquisition_waits_for_the_kernel_lock_then_proceeds(self):
        process = None
        try:
            with file_lock(self.fd):
                process = self.child('wait')
                deadline = time.monotonic() + 20
                while not self.path.with_suffix('.waiting').exists() and time.monotonic() < deadline:
                    time.sleep(.01)
                self.assertTrue(self.path.with_suffix('.waiting').exists())
                self.assertFalse(self.path.with_suffix('.acquired').exists())
                self.assertIsNone(process.poll())
            stdout, stderr = process.communicate(timeout=20)
            self.assertEqual((process.returncode, stdout, stderr), (0, b'', b''))
            self.assertTrue(self.path.with_suffix('.acquired').exists())
        finally:
            if process is not None:
                if process.poll() is None:
                    process.kill(); process.wait(timeout=5)
                process.stdout.close(); process.stderr.close()

    def test_reused_publication_helper_does_not_replace_an_existing_owner(self):
        from mcc_prepared_cache_delta import _publish
        source, target = self.root / 'source', self.root / 'target'
        source.mkdir(); target.mkdir()
        with self.assertRaises(FileExistsError):
            _publish(source, target)
        self.assertTrue(source.is_dir())
        self.assertTrue(target.is_dir())
        target.rmdir()
        _publish(source, target)
        self.assertFalse(source.exists())
        self.assertTrue(target.is_dir())

    @unittest.skipUnless(os.name == 'nt', 'Windows import and Linux-only research boundary')
    def test_windows_imports_research_helpers_without_claiming_linux_reflinks(self):
        from mcc_prepared_cache_delta import _transaction
        from prepare_mcc_elongation_glass import allocated_unshared
        with self.assertRaisesRegex(ValueError, 'requires Linux FICLONE'):
            _transaction(self.root / 'absent', self.root / 'new', lambda *args: None)
        self.assertIsNone(allocated_unshared(self.path))
        self.assertFalse((self.root / 'new').exists())


class SetupFixture:
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix='halo2-owned-setup-',
                                                     dir=os.environ.get('HALO2_TEST_SCRATCH'))
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name) / 'portable setup with spaces'
        (self.root / 'tools').mkdir(parents=True)
        (self.root / 'mode').write_text('wait')
        (self.root / 'prior-cache').write_bytes(b'valid existing cache')
        script = '''import json,os,signal,sys,threading,time
from pathlib import Path
sys.path.insert(0,TOOLS)
import mcc_onboarding
from setup_assets import main
def operation(root, source=None, **kwargs):
    root=Path(root); mode=(root/'mode').read_text()
    (root/'child-info.json').write_text(json.dumps(dict(pid=os.getpid(), main_thread=threading.current_thread() is threading.main_thread(), source=str(source), argv=sys.argv)))
    if mode=='fail':
        print('p'*20000)
        raise ValueError('tiny fixture import failed')
    if mode=='quick':
        print('tiny setup complete')
        return
    if mode=='lock-wait':
        from file_lock import file_lock
        fd=os.open(root/'setup.lock',os.O_RDWR|os.O_CREAT,0o600)
        try:
            (root/'ready').touch()
            with file_lock(fd): (root/'unexpected-acquire').touch()
        finally: os.close(fd)
        return
    pending=root/'.owned-partial'; pending.write_bytes(b'partial')
    if mode=='ignore': signal.signal(signal.SIGTERM,signal.SIG_IGN)
    (root/'ready').touch()
    print('tiny fixture copying', flush=True)
    try:
        while not (root/'release').exists(): time.sleep(.01)
    finally:
        pending.unlink(missing_ok=True)
        (root/'cleaned').touch()
mcc_onboarding.setup=operation
mcc_onboarding.select=operation
raise SystemExit(main())
'''.replace('TOOLS', repr(str(TOOLS)))
        (self.root / 'tools/setup_assets.py').write_text(script)
        self.worker = SetupProcess(self.root, queue.Queue(), cleanup_timeout=2)
        self.addCleanup(self.stop_worker)

    def stop_worker(self):
        with contextlib.suppress(RuntimeError):
            self.worker.shutdown()

    def wait_until(self, condition):
        end = time.monotonic() + 10
        while not condition() and time.monotonic() < end:
            time.sleep(.01)
        self.assertTrue(condition(), 'Owned setup did not reach its expected state')

    def start_worker(self, mode='wait'):
        (self.root / 'mode').write_text(mode)
        self.worker.start(self.root / 'chosen MCC')
        return self.worker.process

    def terminal_event(self):
        result = []
        def finished():
            self.worker.poll()
            while not self.worker.events.empty():
                row = self.worker.events.get_nowait()
                self.assertLessEqual(len(row[1] or ''), 8300)
                if row[0] in ('done', 'error'):
                    result.append(row)
            return bool(result)
        self.wait_until(finished)
        return result[-1]

    def preserved(self):
        self.assertEqual((self.root / 'prior-cache').read_bytes(), b'valid existing cache')
        self.assertFalse((self.root / '.owned-partial').exists())
        self.assertIsNone(self.worker.process)
        self.assertIsNone(self.worker.reader)
        self.assertEqual(self.worker.previous, {})


class SetupProcessTests(SetupFixture, unittest.TestCase):
    def test_owner_eof_cancels_a_writer_waiting_for_another_importer_lock(self):
        fd = os.open(self.root / 'setup.lock', os.O_RDWR | os.O_CREAT, 0o600)
        try:
            with file_lock(fd):
                process = self.start_worker('lock-wait')
                self.wait_until(lambda: (self.root / 'ready').exists())
                self.worker.shutdown()
            self.assertEqual(process.returncode, 143)
            self.assertFalse((self.root / 'unexpected-acquire').exists())
            self.preserved()
        finally:
            os.close(fd)

    def test_separate_resources_keep_cli_and_writes_with_their_owners(self):
        from installation_paths import ENVIRONMENT, SCHEMA
        resources = self.root.parent / 'signed resources'
        resources.mkdir()
        (self.root / 'tools').rename(resources / 'tools')
        script = resources / 'tools/setup_assets.py'
        before = script.read_bytes()
        binding = json.dumps(dict(schema=SCHEMA, data=str(self.root), resources=str(resources)))
        with patch.dict(os.environ, {ENVIRONMENT: binding}):
            self.start_worker('quick')
            self.assertEqual(self.terminal_event()[0], 'done')
        info = json.loads((self.root / 'child-info.json').read_text())
        self.assertEqual(info['argv'][0], str(script))
        self.assertEqual(script.read_bytes(), before)
        self.assertEqual(list(resources.rglob('*')), [resources / 'tools', script])
        self.preserved()

    def test_child_runs_cli_main_thread_and_eof_cleans_then_reaps(self):
        process = self.start_worker()
        self.wait_until(lambda: (self.root / 'ready').exists())
        info = json.loads((self.root / 'child-info.json').read_text())
        self.assertTrue(info['main_thread'])
        self.assertEqual(info['source'], str(self.root / 'chosen MCC'))
        self.assertIn('--gui-owner', info['argv'])
        self.assertNotIn('--interactive', info['argv'])
        reader = self.worker.reader
        self.worker.shutdown()
        self.assertEqual(process.returncode, 143)
        self.assertFalse(reader.is_alive())
        self.assertTrue((self.root / 'cleaned').exists())
        self.preserved()
        if os.name == 'posix':
            with self.assertRaises(ProcessLookupError):
                os.kill(process.pid, 0)
        else:
            self.assertIsNotNone(process.poll())

    def test_failure_stream_is_bounded_and_retry_restores_custom_handlers(self):
        old = {n: signal.getsignal(n) for n in (signal.SIGINT, signal.SIGTERM)}
        handlers = {n: (lambda number, frame: None) for n in old}
        try:
            for n, handler in handlers.items(): signal.signal(n, handler)
            self.start_worker('fail')
            kind, detail = self.terminal_event()
            self.assertEqual(kind, 'error')
            self.assertIn('tiny fixture import failed', detail)
            self.assertIn('exit 1', detail)
            self.assertLessEqual(len(self.worker.tail), 8192)
            self.assertTrue(all(signal.getsignal(n) is h for n, h in handlers.items()))
            self.start_worker('quick')
            self.assertEqual(self.terminal_event()[0], 'done')
            self.assertTrue(all(signal.getsignal(n) is h for n, h in handlers.items()))
            self.preserved()
        finally:
            self.stop_worker()
            for n, handler in old.items(): signal.signal(n, handler)

    @unittest.skipUnless(os.name == 'posix', 'POSIX delivery race; Windows cancellation uses owner-pipe EOF')
    def test_signal_between_creation_and_publication_cannot_orphan_child(self):
        children = []
        before = signal.getsignal(signal.SIGTERM)
        def spawn(*args, **kwargs):
            process = subprocess.Popen(*args, **kwargs)
            children.append(process)
            os.kill(os.getpid(), signal.SIGTERM)
            return process
        self.worker.popen = spawn
        self.start_worker()
        self.assertEqual(self.worker.stop_signal, signal.SIGTERM)
        self.assertEqual(children[0].returncode, 143)
        self.assertIs(signal.getsignal(signal.SIGTERM), before)
        self.preserved()

    def test_spawn_exception_restores_handlers_and_allows_retry(self):
        before = {n: signal.getsignal(n) for n in (signal.SIGINT, signal.SIGTERM)}
        def fail(*args, **kwargs): raise OSError('tiny spawn failure')
        self.worker.popen = fail
        with self.assertRaisesRegex(OSError, 'tiny spawn failure'):
            self.start_worker()
        self.assertEqual({n: signal.getsignal(n) for n in before}, before)
        self.worker.popen = subprocess.Popen
        self.start_worker('quick')
        self.assertEqual(self.terminal_event()[0], 'done')
        self.preserved()

    @unittest.skipUnless(os.name == 'posix', 'POSIX interpreter wrapper')
    def test_bundled_wrapper_and_relative_source_are_preserved(self):
        wrapper = self.root / 'python/bin/python3'
        wrapper.parent.mkdir(parents=True)
        wrapper.write_text('#!/bin/sh\n: > ' + shlex.quote(str(self.root / 'wrapper-used')) +
                           '\nexec ' + shlex.quote(sys.executable) + ' "$@"\n')
        wrapper.chmod(0o700)
        (self.root / 'mode').write_text('quick')
        with contextlib.chdir(self.root):
            self.worker.start(Path('relative MCC folder'))
        self.assertEqual(self.terminal_event()[0], 'done')
        self.assertTrue((self.root / 'wrapper-used').exists())
        info = json.loads((self.root / 'child-info.json').read_text())
        self.assertEqual(info['source'], str(self.root / 'relative MCC folder'))
        self.preserved()

    def test_forced_cleanup_is_explicit_and_does_not_delete_valid_cache(self):
        self.worker.cleanup_timeout = .05
        process = self.start_worker('ignore')
        self.wait_until(lambda: (self.root / 'ready').exists())
        self.wait_until(lambda: 'tiny fixture copying' in self.worker.tail)
        with self.assertRaisesRegex(RuntimeError, 'cleanup could not be verified'):
            self.worker.shutdown()
        self.assertEqual(process.returncode, -signal.SIGKILL if os.name == 'posix' else 1)
        self.assertIsNone(self.worker.process)
        self.assertEqual((self.root / 'prior-cache').read_bytes(), b'valid existing cache')
        self.assertTrue((self.root / '.owned-partial').exists())

    def test_owner_eof_before_cli_work_creates_no_partial_cache(self):
        command = [sys.executable, '-B', str(self.root / 'tools/setup_assets.py'),
                   '--root', str(self.root), '--gui-owner']
        with subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                              stderr=subprocess.STDOUT) as process:
            process.stdin.close()
            self.assertEqual(process.wait(timeout=10), 143)
        self.assertFalse((self.root / 'child-info.json').exists())
        self.preserved()


@unittest.skipUnless(os.environ.get('DISPLAY'), 'Tk display required (use xvfb-run)')
@unittest.skipUnless(os.name == 'posix', 'POSIX signal/process-group lifetime contract')
class SetupGuiLifetimeTests(SetupFixture, unittest.TestCase):
    def gui(self, action):
        driver = self.root / 'gui-driver.py'
        driver.write_text('''import json,os,signal,sys
from pathlib import Path
sys.path.insert(0,TOOLS)
import setup_assets_ui as ui
root=Path(sys.argv[1]); action=sys.argv[2]
previous={n:signal.getsignal(n) for n in (signal.SIGINT,signal.SIGTERM)}
if action=='spawn-death':
    popen=ui.subprocess.Popen
    def spawn(*args,**kwargs):
        process=popen(*args,**kwargs)
        (root/'spawned-pid').write_text(str(process.pid))
        os.kill(os.getpid(),signal.SIGKILL)
    ui.subprocess.Popen=spawn
original=ui.SourceSelector.__init__
def init(self,*args,**kwargs):
    kwargs['auto_detect']=False
    original(self,*args,**kwargs)
    self.window.after(1,self.start)
    if action in ('exception','destroy'):
        def tick():
            if not (root/'ready').exists(): self.window.after(10,tick); return
            if action=='exception': raise RuntimeError('tiny Tk callback failure')
            self.window.destroy()
        self.window.after(10,tick)
ui.SourceSelector.__init__=init
try:
    code=ui.run(root,root/'chosen MCC')
except RuntimeError as error:
    (root/'gui-error').write_text(str(error));code=1
finally:
    (root/'handlers-restored.json').write_text(json.dumps(all(signal.getsignal(n) is h for n,h in previous.items())))
raise SystemExit(code)
'''.replace('TOOLS', repr(str(TOOLS))))
        process = subprocess.Popen([sys.executable, '-B', str(driver), str(self.root), action],
                                   stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
        self.addCleanup(self.stop_gui, process)
        return process

    def stop_gui(self, process):
        if process.poll() is None:
            process.terminate()
            try: process.wait(timeout=10)
            except subprocess.TimeoutExpired: process.kill(); process.wait()
        if process.stderr: process.stderr.close()

    def assert_child_stopped(self):
        child = json.loads((self.root / 'child-info.json').read_text())['pid']
        def stopped():
            try:
                # A dead GUI cannot reap its orphan; the OS reaper owns that.
                return Path(f'/proc/{child}/stat').read_text().split()[2] == 'Z'
            except FileNotFoundError: return True
        self.wait_until(stopped)
        self.assertTrue((self.root / 'cleaned').exists())
        self.assertFalse((self.root / '.owned-partial').exists())
        self.assertEqual((self.root / 'prior-cache').read_bytes(), b'valid existing cache')

    def test_actual_gui_sigterm_waits_for_child_cleanup_and_restores_handlers(self):
        process = self.gui('signal')
        self.wait_until(lambda: (self.root / 'ready').exists())
        process.send_signal(signal.SIGTERM)
        self.assertEqual(process.wait(timeout=10), 143)
        self.assertTrue(json.loads((self.root / 'handlers-restored.json').read_text()))
        self.assert_child_stopped()

    def test_killed_gui_owner_eof_stops_actual_cli_writer(self):
        process = self.gui('signal')
        self.wait_until(lambda: (self.root / 'ready').exists())
        process.kill();process.wait(timeout=10)
        self.assert_child_stopped()

    def test_killed_gui_during_spawn_does_not_strand_unpublished_child(self):
        process = self.gui('spawn-death')
        self.assertEqual(process.wait(timeout=10), -signal.SIGKILL)
        child = int((self.root / 'spawned-pid').read_text())
        def stopped():
            try: return Path(f'/proc/{child}/stat').read_text().split()[2] == 'Z'
            except FileNotFoundError: return True
        self.wait_until(stopped)
        if (self.root / 'child-info.json').exists():
            self.assertTrue((self.root / 'cleaned').exists())
        self.assertFalse((self.root / '.owned-partial').exists())
        self.assertEqual((self.root / 'prior-cache').read_bytes(), b'valid existing cache')

    def test_tk_callback_exception_cleans_child_and_restores_handlers(self):
        process = self.gui('exception')
        self.assertEqual(process.wait(timeout=10), 1)
        self.assertIn('tiny Tk callback failure', (self.root / 'gui-error').read_text())
        self.assertTrue(json.loads((self.root / 'handlers-restored.json').read_text()))
        self.assert_child_stopped()

    def test_external_window_destroy_still_cleans_owned_child(self):
        process = self.gui('destroy')
        self.assertEqual(process.wait(timeout=10), 1)
        self.assertTrue(json.loads((self.root / 'handlers-restored.json').read_text()))
        self.assert_child_stopped()


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
