"""Own the private Steam helper without logging account/session or QR material.

The setup UI consumes transient events. A completed source is handed to the
existing importer only after this process and its pipe reader have been reaped.
"""
import contextlib
import json
import os
from pathlib import Path
import re
import select
import signal
import subprocess
import threading

ERRORS = {
    'cancelled': 'Steam setup was cancelled. Partial downloads can be resumed after signing in again.',
    'timeout': 'Steam setup timed out. Try signing in again.',
    'ownership_or_permission': 'Steam could not confirm access to MCC and its Halo 2 content, or the download folder is unavailable.',
    'invalid_content_or_command': 'The Steam content plan could not be validated. No source was admitted for import.',
    'storage_or_connection': 'Check your free disk space and Steam connection, then try again.',
    'steam_session_failed': 'The Steam content session could not finish. Try signing in again.',
    'invalid_protocol': 'The Steam helper returned an invalid response.',
}
POLICY_REASONS = {'UnsafePath', 'EmptyManifest', 'EncryptedPaths', 'AmbiguousFile', 'InvalidChunks',
                  'IncompleteChunks', 'MissingRequiredFile', 'OverlappingFiles', 'OverlappingDirectory', 'UnexpectedManifest'}


def _hex(value, length):
    return isinstance(value, str) and re.fullmatch('[0-9a-fA-F]{' + str(length) + '}', value) is not None


class SteamDownloadProcess:
    def __init__(self, root, helper, events, *, command=None, cleanup_timeout=2):
        self.root, self.helper, self.events = Path(root).absolute(), Path(helper), events
        self.command = list(command) if command else [str(self.helper), 'session']
        self.cleanup_timeout = cleanup_timeout
        self.process = self.reader = None
        self.reader_stop, self.reader_done = threading.Event(), threading.Event()
        self.lock = threading.Lock()
        self.state, self.plan, self.complete, self.failure = 'new', None, None, None
        self.failure_reason, self.policy_details = None, None
        self.reused_bytes, self.reused_files = 0, 0

    def _send(self, value):
        if self.process is None or self.process.stdin is None:
            raise RuntimeError('The Steam helper is not running')
        self.process.stdin.write((json.dumps(value) + '\n').encode('utf-8'))
        self.process.stdin.flush()

    def start(self):
        if self.process is not None or self.state != 'new':
            raise RuntimeError('A Steam session is already owned')
        self.root.mkdir(parents=True, exist_ok=True)
        environment = os.environ.copy()
        environment['DOTNET_EnableDiagnostics'] = '0'
        self.process = subprocess.Popen(self.command, cwd=self.root, env=environment,
            stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL,
            start_new_session=os.name == 'posix')
        try:
            self.state = 'starting'
            self.reader = threading.Thread(target=self._read, daemon=True)
            self.reader.start()
            self._send(dict(command='start', data_root=str(self.root)))
        except BaseException:
            self.shutdown()
            raise

    def _event(self, value):
        if not isinstance(value, dict): raise ValueError('Invalid event')
        kind = value.get('event')
        with self.lock:
            if kind == 'ready' and self.state == 'starting':
                if value != dict(event='ready', schema='H2_STEAM_CONTENT_SESSION_V1'): raise ValueError('Invalid schema')
                self.state = 'auth'
            elif kind == 'authenticating' and self.state == 'auth':
                if set(value) != {'event'}: raise ValueError('Invalid authentication event')
            elif kind == 'qr' and self.state == 'auth':
                if set(value) != {'event', 'png'} or not isinstance(value['png'], str) or len(value['png']) > 512 * 1024:
                    raise ValueError('Invalid QR event')
            elif kind == 'ownership_verified' and self.state == 'auth':
                if value != dict(event=kind, app_id=976730, dlc_id=1064270, multiplayer_dlc_id=1097223):
                    raise ValueError('Invalid ownership event')
                self.state = 'planning'
            elif kind == 'plan' and self.state == 'planning':
                expected = {'event', 'source_id', 'token', 'files', 'bytes', 'download_bytes',
                            'required_free_bytes', 'available_free_bytes', 'inventory_sha256', 'entitlement_verified'}
                if (set(value) != expected or value['entitlement_verified'] is not True or
                    not _hex(value['source_id'], 24) or not _hex(value['token'], 32) or
                    not _hex(value['inventory_sha256'], 64) or
                    any(type(value[k]) is not int or not 0 <= value[k] <= 2**63-1
                        for k in ('files', 'bytes', 'download_bytes', 'required_free_bytes', 'available_free_bytes')) or
                    value['files'] < 7 or value['required_free_bytes'] < value['bytes']):
                    raise ValueError('Invalid content plan')
                self.plan = dict(value)
                self.state = 'planned'
            elif kind == 'progress' and self.state == 'downloading':
                if (set(value) != {'event', 'bytes', 'total'} or
                    any(type(value[k]) is not int or not 0 <= value[k] <= 2**63-1 for k in ('bytes', 'total'))):
                    raise ValueError('Invalid download progress')
            elif kind == 'resume' and self.state == 'downloading':
                if (set(value) != {'event', 'bytes', 'files'} or
                    type(value['bytes']) is not int or not self.reused_bytes <= value['bytes'] <= self.plan['bytes'] or
                    type(value['files']) is not int or not self.reused_files <= value['files'] <= self.plan['files']):
                    raise ValueError('Invalid verified reuse counters')
                self.reused_bytes, self.reused_files = value['bytes'], value['files']
            elif kind == 'verifying' and self.state == 'downloading':
                if set(value) != {'event'}: raise ValueError('Invalid verification event')
                self.state = 'verifying'
            elif kind == 'complete' and self.state == 'verifying':
                expected = self.root / 'downloads/steam-mcc' / self.plan['source_id'] / 'content/halo2'
                if (set(value) != {'event', 'source', 'source_id', 'inventory_sha256', 'validated_compressed_bytes', 'reused_bytes'} or
                    value['source_id'] != self.plan['source_id'] or
                    value['inventory_sha256'] != self.plan['inventory_sha256'] or
                    value['source'] != str(expected) or not expected.is_dir() or
                    expected.resolve() != expected.absolute() or
                    type(value['validated_compressed_bytes']) is not int or not 0 <= value['validated_compressed_bytes'] <= self.plan['download_bytes'] or
                    type(value['reused_bytes']) is not int or value['reused_bytes'] != self.reused_bytes):
                    raise ValueError('Invalid completed source')
                self.complete = dict(value)
                self.state = 'complete'
            elif (kind == 'error' and set(value) in ({'event', 'code'}, {'event', 'code', 'reason', 'details'})
                  and value['code'] in ERRORS):
                if 'reason' in value:
                    if value['reason'] not in POLICY_REASONS or len(json.dumps(value['details'])) > 2048:
                        raise ValueError('Invalid policy diagnostic')
                    self.failure_reason, self.policy_details = value['reason'], value['details']
                self.failure = value['code']
                return
            else:
                raise ValueError('Unexpected Steam event')
        # QR PNG bytes exist only in this transient queue and the UI image.
        self.events.put(('steam', value))

    def _read(self):
        pending = bytearray()
        try:
            fd = self.process.stdout.fileno()
            while not self.reader_stop.is_set():
                if not select.select([fd], [], [], .05)[0]: continue
                data = os.read(fd, 8192)
                if not data:
                    if pending: raise ValueError('Incomplete event')
                    break
                pending.extend(data)
                if len(pending) > 1024 * 1024: raise ValueError('Oversized event')
                while b'\n' in pending:
                    line, _, pending = pending.partition(b'\n')
                    self._event(json.loads(line))
        except Exception:
            self.failure = 'invalid_protocol'
        finally:
            self.reader_done.set()

    def download(self):
        with self.lock:
            if self.state != 'planned' or self.failure: raise RuntimeError('No validated Steam download plan')
            if self.plan['available_free_bytes'] < self.plan['required_free_bytes']:
                raise RuntimeError('There is not enough free space for this Steam download and preparation.')
            self.state = 'downloading'
            self._send(dict(command='download', token=self.plan['token']))

    def poll(self):
        if self.process is None: return
        if self.failure == 'invalid_protocol':
            self.shutdown()
            self.events.put(('steam-error', ERRORS[self.failure]))
            return
        code = self.process.poll()
        if code is None: return
        self.reader_done.wait(.25)
        success = code == 0 and self.complete is not None and self.failure is None and self.reader_done.is_set()
        complete = self.complete
        self.shutdown()
        if success:
            self.events.put(('steam-finished', complete))
        else:
            self.events.put(('steam-error', ERRORS.get(self.failure, ERRORS['steam_session_failed'])))

    def shutdown(self):
        process = self.process
        try:
            if process is not None:
                if process.stdin is not None:
                    with contextlib.suppress(OSError): process.stdin.close()
                try:
                    process.wait(timeout=self.cleanup_timeout)
                except subprocess.TimeoutExpired:
                    if os.name == 'posix':
                        with contextlib.suppress(ProcessLookupError): os.killpg(process.pid, signal.SIGTERM)
                    else: process.terminate()
                    try: process.wait(timeout=1)
                    except subprocess.TimeoutExpired:
                        if os.name == 'posix':
                            with contextlib.suppress(ProcessLookupError): os.killpg(process.pid, signal.SIGKILL)
                        else: process.kill()
                        process.wait()
                if os.name == 'posix':
                    with contextlib.suppress(ProcessLookupError): os.killpg(process.pid, signal.SIGKILL)
        finally:
            self.reader_stop.set()
            if self.reader is not None: self.reader.join(timeout=2)
            if process is not None:
                for stream in (process.stdin, process.stdout):
                    if stream is not None:
                        with contextlib.suppress(OSError): stream.close()
            self.process = None
            if self.reader is not None and self.reader.is_alive():
                raise RuntimeError('Steam helper reader cleanup could not be verified')
            self.reader = None
