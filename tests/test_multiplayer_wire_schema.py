#!/usr/bin/env python3
"""Exercise the production wire-schema CMake module in private native projects."""
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
MODULE = ROOT / 'cmake/MultiplayerWireSchema.cmake'
RESULTS = []
# These types/serializers can change bytes while the C++ wrapper stays equal.
# Keep this witness independent of the module's declared input inventory.
TRANSITIVE_WIRE_OWNERS = (
    'include/halo2/source_first_person_context_tu15.hpp',
    'include/halo2/source_runtime_resources.hpp',
    'include/halo2/transform.hpp',
    'rust/online_protocol/src/native_requests.rs',
    'rust/online_protocol/src/netcode/pose.rs',
)
MAIN = r'''#include <cstdint>
#include <cstdlib>
#include <iostream>
namespace halo2 {std::uint64_t native_multiplayer_wire_schema();}
int main(int argc,char **argv) {
    if(argc!=2)return 2;
    const auto actual=halo2::native_multiplayer_wire_schema();
    std::cout<<std::hex<<actual<<'\n';
    return actual==std::strtoull(argv[1],nullptr,16)?0:1;
}
'''
CMAKE = '''cmake_minimum_required(VERSION 3.20)
project(NativeWireSchemaFixture LANGUAGES CXX)
enable_testing()
add_library(halo2_systems STATIC seed.cpp)
include(cmake/MultiplayerWireSchema.cmake)
add_executable(wire_schema_export main.cpp)
target_link_libraries(wire_schema_export PRIVATE halo2_systems)
add_test(NAME wire_schema_export COMMAND wire_schema_export "${multiplayer_wire_id}")
'''


class WireSchema(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.directory = tempfile.TemporaryDirectory(
            prefix='wire-schema-', dir=os.environ.get('HALO2_TEST_SCRATCH'))
        cls.addClassCleanup(cls.directory.cleanup)
        cls.root = Path(cls.directory.name)
        cls.module = MODULE.read_bytes()
        text = cls.module.decode()
        declared = re.search(r'set\(multiplayer_wire_inputs\s+(.*?)\)', text, re.S)
        if not declared:
            raise AssertionError('Production module has no explicit wire-input inventory')
        cls.inputs = tuple(declared.group(1).split())
        if not cls.inputs or len(set(cls.inputs)) != len(cls.inputs):
            raise AssertionError('Wire inventory is empty or repeats an owner')
        cls.data = {}
        for name in dict.fromkeys((*cls.inputs, *TRANSITIVE_WIRE_OWNERS)):
            path = Path(name)
            if path.is_absolute() or '..' in path.parts:
                raise AssertionError('Wire inventory escaped its source tree')
            cls.data[name] = ((ROOT / path).read_bytes().replace(b'\r\n', b'\n')
                              if (ROOT / path).is_file() else None)
        cls.baseline = cls.fixture('baseline')
        cls.expected = cls.validate(cls.baseline, 'baseline')

    @classmethod
    def fixture(cls, name, crlf=False):
        source = cls.root / name / 'source'
        source.mkdir(parents=True)
        (source / 'cmake').mkdir()
        (source / 'cmake/MultiplayerWireSchema.cmake').write_bytes(cls.module)
        (source / 'CMakeLists.txt').write_text(CMAKE)
        (source / 'main.cpp').write_text(MAIN)
        (source / 'seed.cpp').write_text('int native_wire_fixture_seed(){return 15;}\n')
        for relative, contents in cls.data.items():
            if contents is None:
                continue
            path = source / relative
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(contents.replace(b'\n', b'\r\n') if crlf else contents)
        return source

    @classmethod
    def validate(cls, source, name):
        owner = source.parent / 'owner'
        owner.mkdir(exist_ok=True)
        output = source.parent / 'logs'
        env = dict(os.environ, HALO2_TEST_SCRATCH=str(owner), TMPDIR=str(owner),
                   TMP=str(owner), TEMP=str(owner))
        command = [sys.executable, '-B', str(ROOT / 'tools/run_tests.py'), '--build-worker',
                   '--source', str(source), '--output', str(output), '--target', 'wire_schema_export',
                   '--filter', '^wire_schema_export$', '--jobs', '2', '--test-jobs', '1']
        result = subprocess.run(command, env=env, text=True, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT)
        if result.returncode:
            raise AssertionError(f'{name}: official CMake worker failed\n{result.stdout[-12000:]}')
        manifest = json.loads((owner / 'build/multiplayer-wire-schema.json').read_text())
        identity = b'Halo2NativeWireSource-v1\n'
        for relative in cls.inputs:
            path = source / relative
            digest = (hashlib.sha256(path.read_bytes().replace(b'\r\n', b'\n')).hexdigest()
                      if path.is_file() else 'absent')
            identity += f'{relative}:{digest}\n'.encode()
        expected = hashlib.sha256(identity).hexdigest()
        if manifest != {'schema': 'Halo2NativeWireSource-v1', 'id': expected[:16], 'sha256': expected}:
            raise AssertionError(f'{name}: generated identity differs from independent normalized-byte digest')
        RESULTS.append({'case': name, 'id': manifest['id'], 'sha256': manifest['sha256'],
                        'cmake_worker_exit': result.returncode, 'native_export_test': True})
        return manifest['id']

    def test_crlf_and_lf_have_identical_identity(self):
        self.assertEqual(self.validate(self.fixture('crlf', crlf=True), 'crlf'), self.expected)

    def test_identical_files_in_another_path_have_identical_identity(self):
        self.assertEqual(self.validate(self.fixture('another path/deep/source owner'), 'relocated'), self.expected)

    def test_each_declared_wire_owner_changes_identity(self):
        for relative in self.inputs:
            with self.subTest(owner=relative):
                path = self.baseline / relative
                before = path.read_bytes() if path.exists() else None
                try:
                    path.parent.mkdir(parents=True, exist_ok=True)
                    path.write_bytes((before or b'') + b'\n// Native wire owner mutation witness.\n')
                    self.assertNotEqual(self.validate(self.baseline, f'owner:{relative}'), self.expected)
                finally:
                    if before is None:
                        path.unlink()
                    else:
                        path.write_bytes(before)

    def test_transitive_wire_type_and_rust_owner_changes_identity(self):
        for relative in TRANSITIVE_WIRE_OWNERS:
            with self.subTest(owner=relative):
                path = self.baseline / relative
                before = path.read_bytes() if path.exists() else None
                try:
                    # control's raw serialized width is an actual wire-layout
                    # change even if packet.cpp and packet.hpp stay identical.
                    changed = (before or b'').replace(b'0x300> control', b'0x301> control')
                    if changed == (before or b''):
                        changed += b'\n// Transitive native wire owner mutation witness.\n'
                    path.parent.mkdir(parents=True, exist_ok=True)
                    path.write_bytes(changed)
                    self.assertNotEqual(self.validate(self.baseline, f'transitive:{relative}'), self.expected)
                finally:
                    if before is None:
                        path.unlink()
                    else:
                        path.write_bytes(before)

    def test_absent_owner_is_distinct_and_reappearance_restores_identity(self):
        relative = next(name for name in self.inputs if (self.baseline / name).is_file())
        path = self.baseline / relative
        before = path.read_bytes()
        try:
            path.unlink()
            self.assertNotEqual(self.validate(self.baseline, 'absent-owner'), self.expected)
        finally:
            path.write_bytes(before)
        self.assertEqual(self.validate(self.baseline, 'restored-owner'), self.expected)

    def test_unrelated_file_change_preserves_identity(self):
        (self.baseline / 'unrelated-gameplay.cpp').write_text('int unrelated_gameplay(){return 42;}\n')
        self.assertEqual(self.validate(self.baseline, 'unrelated'), self.expected)


if __name__ == '__main__':
    result = unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(WireSchema))
    report = os.environ.get('HALO2_WIRE_SCHEMA_TEST_REPORT')
    if report:
        Path(report).write_text(json.dumps({'passed': result.wasSuccessful(),
            'module_sha256': hashlib.sha256(WireSchema.module).hexdigest(), 'cases': RESULTS,
            'owned_fixture_removed': not WireSchema.root.exists(),
            'large_artifacts_retained': []}, indent=2) + '\n')
    raise SystemExit(not result.wasSuccessful())
