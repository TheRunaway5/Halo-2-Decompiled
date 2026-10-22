#!/usr/bin/env python3
"""Exercise real Mach-O dependency relocation and rejection of thin libraries."""
from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path
import plistlib
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


class BundleTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(dir=os.environ['HALO2_TEST_SCRATCH'])
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.sdk = self.root / 'SDK with spaces'
        self.sdk.mkdir()
        self.app = self.root / 'Build.app'
        self.contents = self.app / 'Contents'
        (self.contents / 'MacOS').mkdir(parents=True)
        (self.contents / 'Info.plist').write_bytes(plistlib.dumps({
            'CFBundleExecutable': 'check', 'CFBundleIdentifier': 'net.halo2forever.bundle-test',
            'CFBundlePackageType': 'APPL'}))
        self.source = self.root / 'source.cpp'
        self.license = self.root / 'license.txt'
        self.license.write_text('Synthetic bundle test input.\n')
        self.arches = ['-arch', 'arm64', '-arch', 'x86_64']

    def compile(self, source, output, *options, arches=None):
        self.source.write_text(source)
        subprocess.run(['xcrun', 'clang++', *(self.arches if arches is None else arches),
                        '-mmacosx-version-min=13.0', '-Wl,-headerpad_max_install_names',
                        str(self.source), '-o', str(output), *map(str, options)], check=True)

    def invoke(self, *libraries):
        return subprocess.run([sys.executable, '-B', str(ROOT / 'tools/macos/bundle.py'),
                               '--app', str(self.app), '--license', 'Synthetic=' + str(self.license),
                               *[value for library in libraries for value in ('--library', str(library))]],
                              text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

    def test_transitive_relocation_signing_and_final_hashes(self):
        bottom, middle = self.sdk / 'libbottom.dylib', self.sdk / 'libmiddle.dylib'
        self.compile('extern "C" int bottom(){return 41;}\n', bottom,
                     '-dynamiclib', '-Wl,-install_name,@rpath/libbottom.dylib')
        self.compile('extern "C" int bottom();extern "C" int middle(){return bottom()+1;}\n', middle,
                     '-dynamiclib', bottom, '-Wl,-install_name,@rpath/libmiddle.dylib',
                     '-Wl,-rpath,' + str(self.sdk))
        self.compile('extern "C" int middle();int main(){return middle()==42?0:1;}\n',
                     self.contents / 'MacOS/check', middle, '-Wl,-rpath,' + str(self.sdk))
        source_hash = hashlib.sha256(middle.read_bytes()).hexdigest()
        result = self.invoke(middle)
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertEqual(hashlib.sha256(middle.read_bytes()).hexdigest(), source_hash)
        # Re-running fixup must preserve an already bundled app and its SDK inputs.
        result = self.invoke(middle)
        self.assertEqual(result.returncode, 0, result.stdout)
        manifest = json.loads(self.app.with_suffix('.bundle.json').read_text())
        for path, row in manifest['files'].items():
            self.assertEqual(hashlib.sha256((self.contents / path).read_bytes()).hexdigest(), row['sha256'])
        shutil.rmtree(self.sdk)
        moved = self.root / 'Relocated app with spaces.app'
        self.app.rename(moved)
        for arch in ('arm64', 'x86_64'):
            subprocess.run(['/usr/bin/arch', '-' + arch, str(moved / 'Contents/MacOS/check')], check=True)
        subprocess.run(['codesign', '--verify', '--deep', '--strict', str(moved)], check=True)

    def test_thin_dependency_is_rejected(self):
        self.compile('int main(){return 0;}\n', self.contents / 'MacOS/check')
        thin = self.sdk / 'libthin.dylib'
        self.compile('int thin(){return 0;}\n', thin, '-dynamiclib', arches=['-arch', 'arm64'])
        before = hashlib.sha256(thin.read_bytes()).hexdigest()
        result = self.invoke(thin)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("required ['arm64', 'x86_64']", result.stdout)
        self.assertEqual(hashlib.sha256(thin.read_bytes()).hexdigest(), before)

    def test_nested_executable_uses_its_own_loader_relative_closure(self):
        library = self.sdk / 'libnested.dylib'
        self.compile('extern "C" int nested(){return 42;}\n', library,
                     '-dynamiclib', '-Wl,-install_name,@rpath/libnested.dylib')
        self.compile('int main(){return 0;}\n', self.contents / 'MacOS/check')
        helper = self.contents / 'Resources/python/bin/helper'
        helper.parent.mkdir(parents=True)
        self.compile('extern "C" int nested();int main(){return nested()==42?0:1;}\n',
                     helper, library, '-Wl,-rpath,' + str(self.sdk))
        result = self.invoke()
        self.assertEqual(result.returncode, 0, result.stdout)
        shutil.rmtree(self.sdk)
        moved = self.root / 'Nested relocated Ω.app'
        self.app.rename(moved)
        for arch in ('arm64', 'x86_64'):
            subprocess.run(['/usr/bin/arch', '-' + arch,
                str(moved / 'Contents/Resources/python/bin/helper')], check=True)
        subprocess.run(['codesign', '--verify', '--deep', '--strict', str(moved)], check=True)

    def test_thin_embedded_extension_is_rejected(self):
        self.compile('int main(){return 0;}\n', self.contents / 'MacOS/check')
        extension = self.contents / 'Resources/python/extension.so'
        extension.parent.mkdir(parents=True)
        self.compile('int extension(){return 0;}\n', extension, '-dynamiclib',
                     arches=['-arch', 'arm64'])
        result = self.invoke()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("required ['arm64', 'x86_64']", result.stdout)

    def test_engine_resource_staging_and_signing(self):
        # Exercise the same staging and engine bundle functions as halo2_native.
        # No game assets or GPU implementation are substituted by this fixture.
        library = self.sdk / 'libMoltenVK.dylib'
        self.compile('int resource_fixture_library(){return 0;}\n', library,
                     '-dynamiclib', '-Wl,-install_name,@rpath/libMoltenVK.dylib')
        fixture = self.root / 'source with spaces'
        fixture.mkdir()
        for name in ('cmake', 'tools'):
            (fixture / name).symlink_to(ROOT / name, target_is_directory=True)
        tags = fixture / 'data/tags/variants'
        tags.mkdir(parents=True)
        (tags / 'resource-fixture.json').write_text('{"source_tag_probe":17}\n')
        (fixture / 'shaders').mkdir()
        (fixture / 'shaders/staged-marker.txt').write_text('Resource copy control.\n')
        (fixture / 'main.cpp').write_text('''#include "halo2/macos/executable.hpp"
#include <fstream>
#include <string>
int main(){
    const auto path=halo2::macos_bundle_contents()/"Resources/data/tags/variants/resource-fixture.json";
    std::ifstream input(path);std::string value;std::getline(input,value);
    return input && value=="{\\"source_tag_probe\\":17}"?0:1;
}
''')
        (fixture / 'CMakeLists.txt').write_text(f'''cmake_minimum_required(VERSION 3.24)
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")
set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0" CACHE STRING "")
project(EngineResourceFixture VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
include("{ROOT}/cmake/TestStorage.cmake")
include(CTest)
include("{ROOT}/cmake/RuntimeTagData.cmake")
include("{ROOT}/cmake/macos/Bundle.cmake")
file(COPY "${{CMAKE_CURRENT_SOURCE_DIR}}/shaders" DESTINATION "${{CMAKE_CURRENT_BINARY_DIR}}")
add_executable(resource_fixture main.cpp)
target_include_directories(resource_fixture PRIVATE "{ROOT}/include")
halo2_stage_tag_data(resource_fixture "${{CMAKE_CURRENT_SOURCE_DIR}}/data/tags")
halo2_macos_engine_bundle(resource_fixture)
add_test(NAME engine_resource_paths COMMAND ${{Python3_EXECUTABLE}} -B
    "{ROOT}/tests/macos/engine_bundle_resources_test.py" $<TARGET_BUNDLE_DIR:resource_fixture>)
''')
        output = self.root / 'result'
        result = subprocess.run([sys.executable, '-B', str(ROOT / 'tools/run_tests.py'),
            '--source', str(fixture), '--target', 'resource_fixture', '--filter', '^engine_resource_paths$',
            '--output', str(output), '--retain-report', 'engine-resources.json',
            '--cmake-arg=-DHALO2_MOLTENVK_LIBRARY=' + str(library),
            '--cmake-arg=-DHALO2_MOLTENVK_LICENSE=' + str(self.license),
            '--cmake-arg=-DHALO2_SDL_LICENSE=' + str(self.license)],
            text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        self.assertEqual(result.returncode, 0, result.stdout[-16000:])
        report = json.loads((output / 'evidence/engine-resources.json').read_text())
        self.assertEqual(report['executed_architectures'], ['arm64', 'x86_64'])
        self.assertTrue(report['strict_signature_verified'])
        build = Path(json.loads((output / 'report.json').read_text())['build'])
        self.assertFalse(build.exists(), 'Managed resource fixture build was retained')


if __name__ == '__main__':
    unittest.main()
