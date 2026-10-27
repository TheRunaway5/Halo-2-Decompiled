"""Fresh, resumed and updated standard multiplayer asset installation."""
import json
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import native_variant_assets as assets
from audit_rendering import FormatError
from native_variant_test_support import variant_fixture


class NativeVariantAssetTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix='native-variant-assets-')
        self.addCleanup(temporary.cleanup)
        self.work = Path(temporary.name)
        self.source = self.work / 'Read-only resources'
        self.destination = self.work / 'User data'
        variant_fixture(self.source)

    def test_fresh_install_includes_defusal_and_is_idempotent(self):
        before = assets.source_bundle(self.source)
        entry = assets.install(self.source, self.destination)
        self.assertEqual(set(entry['files']), set(before))
        self.assertIn(assets.DIRECTORY + '/bomb-defusal-v1.json', entry['files'])
        self.assertIn(assets.LAYOUTS, entry['files'])
        for name, raw in before.items():
            self.assertEqual((self.destination / name).read_bytes(), raw)
        stamps = {p: (p.stat().st_ino, p.stat().st_mtime_ns)
                  for p in self.destination.rglob('*') if p.is_file()}
        self.assertEqual(assets.install(self.source, self.destination), entry)
        self.assertEqual(stamps, {p: (p.stat().st_ino, p.stat().st_mtime_ns) for p in stamps})
        self.assertEqual(before, assets.source_bundle(self.source))
        self.assertFalse((self.source / assets.MANIFEST).exists())

    def test_resumes_missing_definition_and_updates_only_managed_files(self):
        assets.install(self.source, self.destination)
        bomb = self.destination / assets.DIRECTORY / 'bomb-defusal-v1.json'
        bomb.unlink()
        unrelated = bomb.with_name('my-mode.json')
        unrelated.write_bytes(b'user-authored mode')
        assets.install(self.source, self.destination)
        self.assertTrue(bomb.is_file())
        source = self.source / assets.DIRECTORY / bomb.name
        row = json.loads(source.read_bytes())
        row['description'] += ' Updated standard definition.'
        source.write_text(json.dumps(row))
        assets.install(self.source, self.destination)
        self.assertEqual(bomb.read_bytes(), source.read_bytes())
        self.assertEqual(unrelated.read_bytes(), b'user-authored mode')

    def test_customized_definition_is_retained_before_any_updates(self):
        assets.install(self.source, self.destination)
        bomb = self.destination / assets.DIRECTORY / 'bomb-defusal-v1.json'
        bomb.write_bytes(b'custom settings')
        before = {p: p.read_bytes() for p in self.destination.rglob('*') if p.is_file()}
        with self.assertRaisesRegex(FormatError, 'Customized multiplayer definition retained'):
            assets.install(self.source, self.destination)
        self.assertEqual(before, {p: p.read_bytes() for p in before})

    def test_missing_packaged_definition_fails_without_publishing(self):
        (self.source / assets.DIRECTORY / 'bomb-defusal-v1.json').unlink()
        with self.assertRaisesRegex(FormatError, 'Missing or oversized'):
            assets.install(self.source, self.destination)
        self.assertFalse(self.destination.exists())

    def test_read_only_bound_resources_install_into_mutable_data(self):
        binding = json.dumps(dict(schema='H2_INSTALLATION_LAYOUT_V1',
            data=str(self.destination), resources=str(self.source)))
        with patch.dict(os.environ, H2_INSTALLATION_LAYOUT=binding):
            assets.install(self.destination, self.destination)
        self.assertEqual((self.destination / assets.DIRECTORY / 'bomb-defusal-v1.json').read_bytes(),
                         (self.source / assets.DIRECTORY / 'bomb-defusal-v1.json').read_bytes())
        self.assertFalse((self.source / assets.MANIFEST).exists())

    def test_symlink_destination_is_rejected_and_outside_files_preserved(self):
        outside = self.work / 'outside'
        outside.mkdir()
        self.destination.mkdir()
        (self.destination / 'data').symlink_to(outside, target_is_directory=True)
        with self.assertRaises(FormatError):
            assets.install(self.source, self.destination)
        self.assertEqual(list(outside.iterdir()), [])

    def test_macos_launcher_stages_defusal_in_native_resource_path(self):
        # Exercise the actual copy/manifest route; Mach-O tools and signing
        # remain mocked, so this is asset delivery evidence only.
        sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools/macos'))
        import package_launcher as packaging
        from test_controller_prompt_assets import controller_prompt_fixture
        from custom_ui_test_support import custom_ui_fixture
        from custom_loading_test_support import custom_loading_fixture, permitted_source_policy_fixture
        from test_custom_map_assets import custom_map_fixture
        controller_prompt_fixture(self.source)
        custom_ui_fixture(self.source)
        custom_map_fixture(self.source)
        custom_loading_fixture(self.source)
        permitted_source_policy_fixture(self.source)
        for name in ('tools/steam/content-profile.json', 'tools/macos/__init__.py',
                     'tools/macos/atomic_files.py', 'tools/macos/steam_setup.py',
                     'tools/macos/loading_converter.py', 'resources/launcher.svg',
                     'resources/frontend/mcc-native-layout.json'):
            path = self.source / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(b'fixture: ' + name.encode())
        for name in ('tools/data', 'tools/reference', 'resources/licenses'):
            (self.source / name).mkdir(parents=True, exist_ok=True)
        sdk = self.work / 'sdk'
        (sdk / 'python').mkdir(parents=True)
        (sdk / 'licenses').mkdir()
        (sdk / 'manifest.json').write_text(json.dumps(dict(schema='H2_MACOS_PYTHON_RUNTIME_V1',
            architectures=['arm64', 'x86_64'], files={}, python='fixture', pillow='fixture', certifi='fixture')))
        launcher = self.work / 'launcher'
        launcher.write_bytes(b'synthetic launcher; not executed')
        app = self.work / 'Halo 2.app'
        with patch.object(packaging, 'command'), patch.object(packaging, 'bundle'):
            packaging.package(self.source, launcher, sdk, app, ['arm64', 'x86_64'], [], [])
        resources = app / 'Contents/Resources'
        for name, raw in assets.source_bundle(self.source).items():
            self.assertEqual((resources / name).read_bytes(), raw)
        manifest = json.loads((resources / 'launcher-build.json').read_bytes())
        self.assertIn(assets.DIRECTORY + '/bomb-defusal-v1.json', manifest['bundled_native_variants']['files'])
        self.assertEqual(set(manifest['bundled_custom_assets']['groups']), {
            'native_variants', 'controller_prompts', 'custom_ui', 'custom_maps', 'custom_loading',
            'permitted_source_policies'})
        self.assertEqual(len(manifest['bundled_custom_assets']['groups']['custom_loading']['files']), 5)
        for name, row in manifest['bundled_custom_assets']['files'].items():
            self.assertEqual((resources / name).stat().st_size, row['bytes'])
            self.assertEqual((resources / name).read_bytes(), (self.source / name).read_bytes())


if __name__ == '__main__':
    unittest.main()
