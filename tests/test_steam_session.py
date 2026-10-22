import json
import os
from pathlib import Path
import queue
import signal
import sys
import tempfile
import time
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from steam_session import SteamDownloadProcess

PLAN = dict(event='plan', source_id='a' * 24, token='b' * 32, files=7, bytes=100,
            download_bytes=50, required_free_bytes=1000, available_free_bytes=10000, inventory_sha256='c' * 64,
            entitlement_verified=True)
HELPER = '''import json,os,signal,sys,time
from pathlib import Path
mode=sys.argv[1]
def emit(value):print(json.dumps(value),flush=True)
emit(dict(event='ready',schema='H2_STEAM_CONTENT_SESSION_V1'))
start=json.loads(sys.stdin.readline());root=Path(start['data_root'])
emit(dict(event='authenticating'))
if mode=='hang':
 signal.signal(signal.SIGTERM,signal.SIG_IGN)
 while True:time.sleep(.1)
if mode=='cancel':
 emit(dict(event='qr',png='memory-only-qr'));sys.stdin.read();sys.exit(2)
if mode=='oversized':print('x'*1100000,flush=True);sys.stdin.read();sys.exit(2)
if mode=='truncated':sys.stdout.write('{');sys.stdout.flush();sys.exit(1)
if mode=='tainted':emit(dict(event='error',code='TAINTED-ACCOUNT-OR-TOKEN'));sys.exit(1)
if mode=='error':emit(dict(event='error',code='ownership_or_permission'));sys.exit(1)
if mode=='empty':sys.exit(0)
emit(dict(event='ownership_verified',app_id=976730,dlc_id=1064270,multiplayer_dlc_id=1097223))
plan=json.loads(sys.argv[2]);emit(plan)
command=json.loads(sys.stdin.readline());assert command==dict(command='download',token=plan['token'])
emit(dict(event='resume',bytes=10,files=1));emit(dict(event='progress',bytes=50,total=100));emit(dict(event='verifying'))
source=root/'downloads/steam-mcc'/plan['source_id']/'content/halo2';source.mkdir(parents=True)
if mode=='escape':source=root
emit(dict(event='complete',source=str(source),source_id=plan['source_id'],inventory_sha256=plan['inventory_sha256'],validated_compressed_bytes=40,reused_bytes=10))
'''


class SessionTests(unittest.TestCase):
    def setUp(self):
        self.owner = tempfile.TemporaryDirectory(prefix='steam-session-')
        self.addCleanup(self.owner.cleanup)
        self.root = Path(self.owner.name).resolve()
        self.helper = self.root / 'helper.py'
        self.helper.write_text(HELPER)
        self.events = queue.Queue()

    def start(self, mode):
        session = SteamDownloadProcess(self.root, self.helper, self.events, cleanup_timeout=.1,
            command=[sys.executable, '-B', str(self.helper), mode, json.dumps(PLAN)])
        self.addCleanup(session.shutdown)
        session.start()
        return session

    def until(self, session, predicate):
        deadline = time.monotonic() + 6
        while not predicate():
            session.poll()
            if time.monotonic() >= deadline: self.fail('Steam process did not reach its expected state')
            time.sleep(.01)

    def test_complete_requires_explicit_confirmation_and_reaping(self):
        preserved = self.root / 'unrelated'; preserved.write_text('preserve')
        session = self.start('success')
        self.until(session, lambda: session.state == 'planned')
        self.assertIsNone(session.complete)
        self.assertIsNotNone(session.process)
        session.download()
        with self.assertRaises(RuntimeError): session.download()
        self.until(session, lambda: session.process is None)
        events = list(self.events.queue)
        self.assertEqual(events[-1][0], 'steam-finished')
        self.assertEqual(events[-1][1]['source_id'], PLAN['source_id'])
        self.assertIsNone(session.reader)
        self.assertEqual(preserved.read_text(), 'preserve')

    def test_cannot_confirm_before_a_plan(self):
        session = self.start('cancel')
        with self.assertRaises(RuntimeError): session.download()

    def test_cannot_confirm_without_enough_free_space(self):
        session = SteamDownloadProcess(self.root, self.helper, self.events)
        session.state = 'planning'
        session._event(dict(PLAN, available_free_bytes=10))
        with self.assertRaisesRegex(RuntimeError, 'free space'): session.download()
        self.assertEqual(session.state, 'planned')

    def test_eof_cancels_and_reaps_the_helper(self):
        session = self.start('cancel')
        self.until(session, lambda: any(e[0] == 'steam' and e[1]['event'] == 'qr' for e in self.events.queue))
        child = session.process
        session.shutdown()
        self.assertEqual(child.returncode, 2)
        self.assertIsNone(session.process)
        self.assertIsNone(session.reader)

    @unittest.skipUnless(os.name == 'posix', 'POSIX process group ownership')
    def test_unresponsive_helper_is_killed_and_reaped(self):
        session = self.start('hang')
        self.until(session, lambda: session.state == 'auth')
        child = session.process
        start = time.monotonic(); session.shutdown()
        self.assertLess(time.monotonic() - start, 4)
        self.assertIn(child.returncode, (-signal.SIGTERM, -signal.SIGKILL))
        self.assertIsNone(session.reader)

    def test_failure_and_malformed_streams_never_admit_a_source(self):
        for mode in ('oversized', 'truncated', 'tainted', 'error', 'empty', 'escape'):
            with self.subTest(mode=mode):
                self.events = queue.Queue()
                session = self.start(mode)
                deadline = time.monotonic() + 6
                while session.process is not None:
                    if session.state == 'planned': session.download()
                    session.poll()
                    if time.monotonic() > deadline: self.fail('Malformed helper did not terminate')
                    time.sleep(.01)
                events = list(self.events.queue)
                self.assertEqual(events[-1][0], 'steam-error')
                self.assertFalse(any(e[0] == 'steam-finished' for e in events))
                self.assertNotIn('TAINTED', str(events))
                self.assertIsNone(session.reader)

    def test_plan_requires_owned_content_and_finite_sizes(self):
        for key, value in (('entitlement_verified', False), ('files', 6), ('bytes', -1),
                           ('bytes', True), ('bytes', 2**64), ('token', 'wrong'),
                           ('source_id', '../escape'), ('inventory_sha256', 'invalid'),
                           ('required_free_bytes', 0)):
            with self.subTest(key=key, value=value):
                session = SteamDownloadProcess(self.root, self.helper, self.events)
                session.state = 'planning'
                with self.assertRaises(ValueError): session._event(dict(PLAN, **{key: value}))
                self.assertIsNone(session.plan)


if __name__ == '__main__':
    unittest.main()
