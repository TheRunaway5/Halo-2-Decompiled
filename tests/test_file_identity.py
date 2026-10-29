"""Real read/replace races at the native launcher and MCC publication guards."""
import contextlib
import hashlib
import io
import mmap
import os
from pathlib import Path
import sys
import tempfile
import time
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import file_identity
from file_identity import descriptor_identity, path_identity
import native_gameplay_test as admission
import native_renderer_shaders as renderer
import mcc_native_map_owner_publication as publication


def shared_reader(path):
    """Allow actual replacement on Windows so the identity guard is exercised.

    Ordinary CRT readers may deny delete-sharing. This test reader deliberately
    grants all sharing, ensuring a replacement cannot pass only because the OS
    happened to prohibit the attempted race.
    """
    if os.name != 'nt':
        return io.open(path, 'rb')
    import ctypes
    import msvcrt
    handle = file_identity._open(str(path), 0x80000000, 7, None, 3, 0x80, None)
    if handle == ctypes.c_void_p(-1).value:
        raise ctypes.WinError(ctypes.get_last_error())
    try:
        fd = msvcrt.open_osfhandle(handle, os.O_RDONLY | os.O_BINARY)
    except BaseException:
        file_identity._close(handle)
        raise
    return os.fdopen(fd, 'rb')


class MutatingReader:
    def __init__(self, stream, mutation):
        self.stream, self.mutation, self.changed = stream, mutation, False

    def __getattr__(self, name):
        return getattr(self.stream, name)

    def read(self, *args):
        raw = self.stream.read(*args)
        if not self.changed:
            self.changed = True
            self.mutation()
        return raw

    def __enter__(self):
        return self

    def __exit__(self, *args):
        return self.stream.__exit__(*args)


class FileIdentityTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix='halo2-file-identity-',
                                                     dir=os.environ.get('HALO2_TEST_SCRATCH'))
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name).resolve()
        self.path = self.root / 'owner.bin'
        self.path.write_bytes(b'a' * 128)
        self.initial = self.path.stat()

    def mutate_restoring_mtime(self):
        time.sleep(.01)
        self.path.write_bytes(b'b' * self.initial.st_size)
        os.utime(self.path, ns=(self.initial.st_atime_ns, self.initial.st_mtime_ns))
        self.assertEqual(self.path.stat().st_size, self.initial.st_size)
        self.assertEqual(self.path.stat().st_mtime_ns, self.initial.st_mtime_ns)

    def replace_same_bytes_and_mtime(self):
        replacement = self.root / 'replacement.bin'
        replacement.write_bytes(b'a' * 128)
        os.utime(replacement, ns=(self.initial.st_atime_ns, self.initial.st_mtime_ns))
        os.replace(replacement, self.path)

    def test_unchanged_open_descriptor_and_named_owner_match(self):
        with shared_reader(self.path) as stream:
            self.assertEqual(descriptor_identity(stream.fileno()), path_identity(self.path))
        self.assertEqual(admission._file_digest(self.path), hashlib.sha256(b'a' * 128).hexdigest())
        self.assertEqual(renderer._direct_bytes(self.path, 1024), b'a' * 128)
        raw, pin = publication._read(self.root, self.path.name, [])
        self.assertEqual((raw, pin['bytes']), (b'a' * 128, 128))

    def test_same_bytes_and_mtime_replacement_retains_distinct_kernel_owner(self):
        denied = False
        with shared_reader(self.path) as stream:
            before = descriptor_identity(stream.fileno())
            try:
                self.replace_same_bytes_and_mtime()
            except PermissionError:
                # Some Windows filesystems/CRT implementations reject a held
                # destination even with delete sharing. That attack was blocked
                # by the OS; also exercise the ID guard after releasing it.
                self.assertEqual(os.name, 'nt')
                self.assertEqual(path_identity(self.path), descriptor_identity(stream.fileno()))
                denied = True
            else:
                self.assertNotEqual(path_identity(self.path)[:2], descriptor_identity(stream.fileno())[:2])
        if denied:
            self.replace_same_bytes_and_mtime()
        self.assertEqual(path_identity(self.path)[2:4], before[2:4])
        self.assertNotEqual(path_identity(self.path)[:2], before[:2])

    def test_digest_refuses_same_bytes_replacement_after_hashing(self):
        original_open, original_hash = Path.open, hashlib.file_digest
        attempts = []

        def opened(path, *args, **kwargs):
            return shared_reader(path) if path == self.path else original_open(path, *args, **kwargs)

        def hashed(stream, *args, **kwargs):
            result = original_hash(stream, *args, **kwargs)
            attempts.append(True)
            self.replace_same_bytes_and_mtime()
            return result

        with patch.object(Path, 'open', opened), patch.object(admission.hashlib, 'file_digest', hashed):
            try:
                with self.assertRaises(admission.NativeTestOwnerChanged):
                    admission._file_digest(self.path)
            except PermissionError:
                self.assertEqual(os.name, 'nt')
                self.assertEqual(self.path.read_bytes(), b'a' * 128)
        self.assertEqual(attempts, [True])

    def test_digest_refuses_mid_read_mutation_even_when_size_and_mtime_are_restored(self):
        original_hash = hashlib.file_digest

        def hashed(stream, *args, **kwargs):
            result = original_hash(stream, *args, **kwargs)
            self.mutate_restoring_mtime()
            return result

        with patch.object(admission.hashlib, 'file_digest', hashed):
            with self.assertRaises(admission.NativeTestOwnerChanged):
                admission._file_digest(self.path)

    def test_shader_direct_read_retains_change_time_guard(self):
        original_open = Path.open

        def opened(path, *args, **kwargs):
            stream = original_open(path, *args, **kwargs)
            return MutatingReader(stream, self.mutate_restoring_mtime) if path == self.path else stream

        with patch.object(Path, 'open', opened):
            with self.assertRaisesRegex(ValueError, 'changed during validation'):
                renderer._direct_bytes(self.path, 1024)

    def test_executable_catalog_refuses_change_after_mapping(self):
        self.path.write_bytes(b'native_world/v-' + b'a' * 64 + b'.vert.spv\0payload')
        self.initial = self.path.stat()
        original_map = mmap.mmap

        @contextlib.contextmanager
        def mapped(*args, **kwargs):
            with original_map(*args, **kwargs) as data:
                yield data
            self.mutate_restoring_mtime()

        with patch.object(renderer.mmap, 'mmap', mapped):
            with self.assertRaisesRegex(ValueError, 'executable changed during validation'):
                renderer._bound_shader_requirements(self.path)

    def test_mcc_publication_rejects_mid_read_mutation_without_repinning(self):
        original_fdopen = os.fdopen

        def opened(fd, *args, **kwargs):
            return MutatingReader(original_fdopen(fd, *args, **kwargs), self.mutate_restoring_mtime)

        pins = []
        with patch.object(publication.os, 'fdopen', opened):
            with self.assertRaisesRegex(ValueError, 'owner changed while reading'):
                publication._read(self.root, self.path.name, pins)
        self.assertEqual(pins, [])


if __name__ == '__main__':
    unittest.main()
