"""Verify complete custom source delivery and refresh without engine relinking."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import textwrap
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import custom_assets as assets
from audit_rendering import FormatError
from custom_ui_test_support import custom_ui_fixture
from custom_loading_test_support import custom_loading_fixture, permitted_source_policy_fixture
from native_variant_test_support import variant_fixture
from test_controller_prompt_assets import controller_prompt_fixture
from test_custom_map_assets import custom_map_fixture


class CustomAssetSources(unittest.TestCase):
    def setUp(self):
        self.owner = tempfile.TemporaryDirectory(prefix='custom-assets-', dir=os.environ.get('HALO2_TEST_SCRATCH'))
        self.addCleanup(self.owner.cleanup)
        self.work = Path(self.owner.name)
        self.source = self.work / 'port'
        self.destination = self.work / 'build/custom-assets'
        variant_fixture(self.source)
        controller_prompt_fixture(self.source)
        custom_ui_fixture(self.source)
        custom_map_fixture(self.source, populated=True)
        # The tiny map fixture exercises source delivery/deduplication, not a
        # native material owner. Production map-source authentication remains
        # covered by the custom-map preparation tests with real owners.
        authenticator = patch('custom_map_assets._verify_source_index', return_value={})
        authenticator.start()
        self.addCleanup(authenticator.stop)
        custom_loading_fixture(self.source)
        permitted_source_policy_fixture(self.source)

    def test_complete_install_retains_sources_and_stable_receipt(self):
        pins = assets.inventory(self.source)
        receipt = assets.install(self.source, self.destination)
        self.assertEqual(receipt['files'], pins)
        self.assertEqual(set(receipt['groups']), {
            'native_variants', 'controller_prompts', 'custom_ui', 'custom_maps', 'custom_loading',
            'permitted_source_policies'})
        self.assertEqual(assets.inventory(self.destination), pins)
        self.assertEqual(assets.required_bytes(self.source), sum(row['bytes'] for row in pins.values()))
        self.assertFalse(any('authoring/' in name for name in pins))
        path = self.destination / assets.MANIFEST
        timestamp = path.stat().st_mtime_ns
        self.assertEqual(assets.install(self.source, self.destination), receipt)
        self.assertEqual(path.stat().st_mtime_ns, timestamp)
        self.assertFalse((self.source / assets.MANIFEST).exists())
        self.assertFalse(list(self.destination.rglob('.custom-asset-*')))
        self.assertEqual(len(receipt['groups']['custom_loading']['files']), 5)
        self.assertEqual(set(receipt['groups']['permitted_source_policies']['files']), {
            'tools/data/h2ek-original-loading-v2.json'})
        self.assertFalse(any(name.endswith(('.exe', '.xbox-adpcm')) for name in receipt['files']))

    def test_optional_loading_art_is_authenticated_when_present(self):
        from custom_loading_assets import inventory
        from mcc_loading_assets import CUSTOM
        unrelated = self.source / CUSTOM / 'untracked-reference.bin'
        unrelated.write_bytes(b'preserved reference')
        records = inventory(self.source)
        self.assertEqual(len(records), 5)
        self.assertNotIn(unrelated.relative_to(self.source).as_posix(), records)
        (self.source / CUSTOM / 'loading-03.dds').write_bytes(b'changed source')
        with self.assertRaisesRegex(FormatError, 'differs'):
            assets.install(self.source, self.destination)
        self.assertFalse(self.destination.exists())
        self.assertEqual(inventory(self.work / 'absent'), {})

    def test_aliases_share_only_destination_inodes(self):
        assets.install(self.source, self.destination)
        prefix = 'assets/custom-maps/fixture/content/'
        first = prefix + 'halo2/Mods/fixture/fixture.map'
        second = prefix + 'halo2/h2_maps_win64_dx11/textures.dat'
        self.assertEqual((self.destination / first).stat().st_ino, (self.destination / second).stat().st_ino)
        self.assertNotEqual((self.destination / first).stat().st_ino, (self.source / first).stat().st_ino)
        (self.destination / first).write_bytes(b'staged custom bytes')
        self.assertEqual((self.source / first).read_bytes(), b'compiled-map-fixture')

    def test_source_updates_refresh_but_customized_destinations_are_preserved(self):
        assets.install(self.source, self.destination)
        name = 'data/tags/variants/fiesta-slayer-v1.json'
        source = self.source / name
        row = json.loads(source.read_bytes())
        row['description'] += ' Updated.'
        source.write_text(json.dumps(row))
        assets.install(self.source, self.destination)
        self.assertEqual((self.destination / name).read_bytes(), source.read_bytes())
        (self.destination / name).write_bytes(b'personal game mode')
        manifest = (self.destination / assets.MANIFEST).read_bytes()
        with self.assertRaisesRegex(FormatError, 'Customized'):
            assets.install(self.source, self.destination)
        self.assertEqual((self.destination / name).read_bytes(), b'personal game mode')
        self.assertEqual((self.destination / assets.MANIFEST).read_bytes(), manifest)

    def test_missing_required_sources_fail_before_staging(self):
        (self.source / 'resources/custom-maps.json').unlink()
        with self.assertRaises(FormatError):
            assets.install(self.source, self.destination)
        self.assertFalse(self.destination.exists())

    def test_destination_change_during_verification_cannot_publish_receipt(self):
        assets.install(self.source, self.destination)
        before = (self.destination / assets.MANIFEST).read_bytes()
        name = 'data/tags/variants/fiesta-slayer-v1.json'
        original = assets._groups
        calls = 0
        def changed(root):
            nonlocal calls
            result = original(root)
            calls += 1
            if calls == 2:
                (self.destination / name).write_bytes(b'concurrent custom settings')
            return result
        with patch.object(assets, '_groups', side_effect=changed):
            with self.assertRaisesRegex(FormatError, 'changed before publication'):
                assets.install(self.source, self.destination)
        self.assertEqual((self.destination / assets.MANIFEST).read_bytes(), before)
        self.assertEqual((self.destination / name).read_bytes(), b'concurrent custom settings')

    def test_owned_resource_binding_and_destination_symlink_rejection(self):
        binding = json.dumps(dict(schema='H2_INSTALLATION_LAYOUT_V1',
                                  data=str(self.work / 'user'), resources=str(self.source)))
        with patch.dict(os.environ, H2_INSTALLATION_LAYOUT=binding):
            receipt = assets.install(self.work / 'user', self.destination)
        self.assertEqual(receipt['files'], assets.inventory(self.source))
        foreign = self.work / 'foreign'
        foreign.mkdir()
        alternate = self.work / 'alternate'
        alternate.mkdir()
        (alternate / 'resources').symlink_to(foreign, target_is_directory=True)
        with self.assertRaises(FormatError):
            assets.install(self.source, alternate)
        self.assertEqual(list(foreign.iterdir()), [])


@unittest.skipUnless(shutil.which('cmake') and shutil.which('ninja') and shutil.which('c++'),
                     'CMake, Ninja and C++ compiler required')
class CustomAssetBuildDependency(unittest.TestCase):
    def test_asset_only_build_refresh_runs_without_relink(self):
        # Validate the real CMake module in a tiny compiler fixture through the
        # official disposable runner. The fixture's copy helper substitutes
        # small source files for the production artwork/maps tested above.
        with tempfile.TemporaryDirectory(prefix='custom-build-', dir=os.environ.get('HALO2_TEST_SCRATCH')) as owner:
            work = Path(owner)
            source = work / 'source'
            (source / 'tools').mkdir(parents=True)
            (source / 'data/tags').mkdir(parents=True)
            (source / 'data/tags/mode.json').write_text('initial game mode')
            (source / 'art.txt').write_text('initial custom art')
            (source / 'main.cpp').write_text('int main() { return 0; }\n')
            (source / 'tools/custom_assets.py').write_text(textwrap.dedent('''\
                import argparse, json
                from pathlib import Path
                p=argparse.ArgumentParser();p.add_argument('--root',type=Path);p.add_argument('--destination',type=Path)
                a=p.parse_args();a.destination.mkdir(parents=True,exist_ok=True)
                data=(a.root/'art.txt').read_text()
                target=a.destination/'art.txt';target.write_text(data)
                receipt=a.destination/'custom-assets.json'
                raw=json.dumps({'art':data})
                if not receipt.exists() or receipt.read_text()!=raw:receipt.write_text(raw)
                '''))
            (source / 'refresh.py').write_text(textwrap.dedent('''\
                from pathlib import Path
                import subprocess, sys
                source,build=map(Path,sys.argv[1:])
                binary=build/'fixture_engine';before=binary.stat().st_mtime_ns
                assert (build/'custom-assets/art.txt').read_text()=='initial custom art'
                (source/'art.txt').write_text('updated custom art')
                (source/'data/tags/mode.json').write_text('updated game mode')
                subprocess.run(['cmake','--build',str(build),'--target','fixture_engine'],check=True)
                assert (build/'custom-assets/art.txt').read_text()=='updated custom art'
                assert (build/'data/tags/mode.json').read_text()=='updated game mode'
                assert binary.stat().st_mtime_ns==before,'Asset-only build unexpectedly relinked Linux executable'
                # Clear the explicit fixture override. The enclosing official
                # runner supplies HALO2_TEST_SCRATCH, so this now defaults OFF.
                subprocess.run(['cmake','-S',str(source),'-B',str(build),
                                '-UHALO2_STAGE_CUSTOM_ASSETS'],check=True)
                assert 'HALO2_STAGE_CUSTOM_ASSETS:BOOL=OFF' in (build/'CMakeCache.txt').read_text()
                (source/'tools/custom_assets.py').write_text('raise AssertionError("Disposable source copy ran")')
                (source/'art.txt').write_text('source art excluded from component build')
                (source/'data/tags/mode.json').write_text('component game mode')
                subprocess.run(['cmake','--build',str(build),'--target','fixture_engine'],check=True)
                assert (build/'custom-assets/art.txt').read_text()=='updated custom art'
                assert (build/'data/tags/mode.json').read_text()=='component game mode'
                assert binary.stat().st_mtime_ns==before
                '''))
            (source / 'CMakeLists.txt').write_text(textwrap.dedent(f'''\
                cmake_minimum_required(VERSION 3.24)
                project(CustomAssetFixture LANGUAGES CXX)
                find_package(Python3 REQUIRED COMPONENTS Interpreter)
                enable_testing()
                add_executable(fixture_engine main.cpp)
                include("{ROOT / 'cmake/RuntimeTagData.cmake'}")
                halo2_stage_tag_data(fixture_engine "${{CMAKE_CURRENT_SOURCE_DIR}}/data/tags")
                add_test(NAME asset_refresh COMMAND "${{Python3_EXECUTABLE}}" -B
                    "${{CMAKE_CURRENT_SOURCE_DIR}}/refresh.py" "${{CMAKE_CURRENT_SOURCE_DIR}}" "${{CMAKE_CURRENT_BINARY_DIR}}")
                '''))
            output = work / 'reports'
            subprocess.run([sys.executable, '-B', str(ROOT / 'tools/run_tests.py'),
                '--source', str(source), '--output', str(output), '--scratch-root', str(work / 'scratch'),
                '--target', 'fixture_engine', '--filter', '^asset_refresh$', '--cmake-arg=-GNinja',
                '--cmake-arg=-DHALO2_STAGE_CUSTOM_ASSETS=ON'], check=True)
            report = json.loads((output / 'report.json').read_bytes())
            self.assertTrue(report['passed'])
            self.assertFalse(Path(report['build']).exists())


if __name__ == '__main__':
    unittest.main()
