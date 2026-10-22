#!/usr/bin/env python3
"""Portable admission controls; native execution has a separate Mac CTest."""
import copy
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
import mcc_loading_presentation as loading
from audit_rendering import FormatError
from macos import loading_converter as native


def thin(arch, kind):
    return struct.pack('<8I', 0xfeedfacf, *native.ARCHITECTURES[arch], kind, 0, 0, 0, 0)


def fat(kind):
    result = bytearray(8192 + 32)
    struct.pack_into('>II', result, 0, 0xcafebabe, 2)
    for i, arch in enumerate(('arm64', 'x86_64')):
        offset = 4096 * (i + 1)
        struct.pack_into('>5I', result, 8 + 20*i, *native.ARCHITECTURES[arch], offset, 32, 12)
        result[offset:offset+32] = thin(arch, kind)
    return result


class Admission(unittest.TestCase):
    def setUp(self):
        self.owner = tempfile.TemporaryDirectory(dir=os.environ.get('HALO2_TEST_SCRATCH'))
        self.addCleanup(self.owner.cleanup)
        self.root = Path(self.owner.name)
        for name in (native.EXECUTABLE, *native.LIBRARIES, *native.LICENSES, loading.CODEBOOK):
            path = self.root / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(fat(2 if name == native.EXECUTABLE else 6) if name in
                             (native.EXECUTABLE, *native.LIBRARIES) else b'fixture notice\n')
        shutil.copyfile(ROOT / loading.CODEBOOK, self.root / loading.CODEBOOK)
        (self.root / native.EXECUTABLE).chmod(0o755)
        self.value = dict(schema=native.SCHEMA, architectures=['arm64', 'x86_64'],
            files={name: dict(bytes=(self.root / name).stat().st_size,
                sha256=hashlib.sha256((self.root / name).read_bytes()).hexdigest()) for name in
                (native.EXECUTABLE, *native.LIBRARIES, *native.LICENSES, loading.CODEBOOK)})
        self.manifest = self.root / loading.CONVERTER_MANIFEST
        self.manifest.parent.mkdir(parents=True, exist_ok=True)
        self.write_manifest()

    def write_manifest(self):
        self.manifest.write_text(json.dumps(self.value))

    def test_shared_discovery_and_offline_provenance(self):
        actual = loading.converter_dependencies(self.root)
        self.assertEqual(actual['converter']['path'], native.EXECUTABLE)
        loading._converter_runtime_metadata(actual['converter'], actual['codebook'])
        with self.assertRaisesRegex(FormatError, 'PCM differs'):
            # An intact Mac runtime cannot admit a noncanonical audio payload.
            size = loading.V3_FRAMES*8
            header = (b'RIFF' + struct.pack('<I', size+36) + b'WAVEfmt ' +
                      struct.pack('<IHHIIHH', 16, 3, 2, 44100, 44100*8, 8, 32) +
                      b'data' + struct.pack('<I', size))
            loading._v3_pcm(header + b'\0' * size)

    def test_cpu_owner_and_fat_table_controls(self):
        path = self.root / native.EXECUTABLE
        original = path.read_bytes()
        cases = []
        for offset, fmt, value in ((4, '>I', 3), (16, '>I', 16), (24, '>I', 31),
                                   (28, '>I', 0x100000c), (32, '>I', 0)):
            data = bytearray(original)
            struct.pack_into(fmt, data, offset, value)
            cases.append(data)
        cases += [original[:20], thin('arm64', 6), b'not a Mach-O image']
        for data in cases:
            with self.subTest(header=bytes(data[:28]).hex()):
                path.write_bytes(data)
                with self.assertRaises(FormatError):
                    native.image_architectures(path, 2)
        path.write_bytes(thin('arm64', 2))
        self.assertEqual(native.image_architectures(path, 2), {'arm64'})

    def test_mismatched_declared_architecture(self):
        for arches in ([], ['arm64', 'arm64'], ['arm64', 'ppc'], ['arm64'], ['arm64', True]):
            with self.subTest(arches=arches):
                self.value['architectures'] = arches
                self.write_manifest()
                with self.assertRaises(FormatError):
                    loading.converter_dependencies(self.root)

    def test_changed_missing_and_extra_private_files(self):
        for name in (*native.LIBRARIES, *native.LICENSES, loading.CODEBOOK):
            path = self.root / name
            original = path.read_bytes()
            path.write_bytes(original + b'changed')
            with self.subTest(changed=name), self.assertRaises(FormatError):
                loading.converter_dependencies(self.root)
            path.unlink()
            with self.subTest(missing=name), self.assertRaises(FormatError):
                loading.converter_dependencies(self.root)
            path.write_bytes(original)
        extra = self.root / native.BASE / 'lib/foreign.dylib'
        extra.write_bytes(b'foreign image')
        with self.assertRaisesRegex(FormatError, 'inventory'):
            loading.converter_dependencies(self.root)

    def test_linked_private_owner_and_execution_permission(self):
        path = self.root / native.BASE
        moved = self.root / 'moved'
        path.rename(moved)
        path.symlink_to(moved, target_is_directory=True)
        with self.assertRaises(FormatError):
            loading.converter_dependencies(self.root)
        path.unlink(); moved.rename(path)
        executable = self.root / native.EXECUTABLE
        executable.chmod(0o644)
        with self.assertRaisesRegex(FormatError, 'execution permission'):
            loading.converter_dependencies(self.root)

    def test_metadata_rejects_extra_owners_and_bad_provenance(self):
        actual = loading.converter_dependencies(self.root)
        for change in ('extra', 'hash', 'path', 'manifest'):
            decoder = copy.deepcopy(actual['converter'])
            if change == 'extra':
                decoder['runtime']['files']['lib/foreign.so'] = dict(bytes=1, sha256='0'*64)
            elif change == 'hash':
                decoder['runtime']['files'][native.EXECUTABLE]['sha256'] = '0'*64
            elif change == 'path':
                decoder['path'] = loading.CONVERTER_WRAPPER
            else:
                decoder['runtime']['manifest']['path'] = '../manifest.json'
            with self.subTest(change=change), self.assertRaises(FormatError):
                loading._converter_runtime_metadata(decoder, actual['codebook'])


if __name__ == '__main__':
    unittest.main()
