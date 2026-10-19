"""Verify MCC cache selection, reuse and failure publication boundaries."""
import hashlib
import json
from pathlib import Path
import shutil
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import mcc_onboarding as onboarding
from audit_rendering import FormatError, file_sha
from prepare_mcc_gameplay import PROFILE, source_directories, source_files
from test_mcc_movies import bundle_fixture as movie_fixture
from test_mcc_prompts import prompt_fixture


def bundle_fixture(root):
    movie_fixture(root)
    prompt_fixture(root)


class MccOnboardingTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix='halo2-mcc-onboarding-')
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name) / 'port'
        self.source = Path(self.temporary.name) / 'Halo MCC/halo2'
        self.cache = self.root / 'assets/imported/mcc-test'
        bundle_fixture(self.root)
        for name in onboarding.REQUIRED:
            if name.startswith('halo2/'):
                p = self.source / name[6:]
                p.parent.mkdir(parents=True, exist_ok=True)
                p.write_bytes(b'MCC donor ' + name.encode())
        (self.source / 'tags/My Mod/Empty').mkdir(parents=True)
        p = self.source / 'data/My Mod/Texture.bin'
        p.parent.mkdir(parents=True)
        p.write_bytes(b'authored mod data')
        (self.source / 'game.cfg').write_bytes(b'settings')
        self.cache.mkdir(parents=True)
        shutil.copytree(self.source, self.cache / 'halo2')
        for name in onboarding.REQUIRED:
            path = self.cache / name
            if not path.exists():
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text('{}')
        files, omitted = source_files(self.source)
        self.report = dict(schema=PROFILE, source=str(self.source),
                           external_installation_requirement='Halo MCC only', runtime_active=False,
                           full_tu15_parity=False, excluded_files=omitted,
                           source_directories=[p.as_posix() for p in source_directories(self.source)],
                           source_files={p.as_posix(): dict(bytes=s.st_size, sha256=file_sha(self.source / p)) for p, s in files})
        self.write_report()
        (self.cache / 'tag-conversion.json').write_text(json.dumps(dict(maps=[])))
        self.messages = []

    def write_report(self):
        (self.cache / 'provenance.json').write_text(json.dumps(self.report))

    def verify(self, source=None):
        return onboarding.private_cache(self.root, self.cache, verify=True, source=source, progress=self.messages.append)

    def select(self):
        return onboarding.select(self.root, self.cache, progress=self.messages.append)

    def test_select_preserves_mcc_directory_case_and_incomplete_readiness(self):
        self.select()
        cache, report = onboarding.selected(self.root)
        self.assertEqual(cache, self.cache)
        self.assertFalse(report['runtime_active'])
        self.assertFalse(report['full_tu15_parity'])
        self.assertEqual(onboarding.runtime_status(self.root), onboarding.NOT_PLAYABLE)
        self.assertEqual((cache / 'halo2/data/My Mod/Texture.bin').read_bytes(), b'authored mod data')
        self.assertTrue((cache / 'halo2/tags/My Mod/Empty').is_dir())

    def test_preview_lists_only_owned_mcc_map_paths_and_preserves_mod_case(self):
        self.select()
        mod = self.cache/'halo2/mods/My Mod/Custom.MAP'
        mod.parent.mkdir(parents=True)
        mod.write_bytes(b'owned mod map')
        outside = self.root/'outside.map'
        outside.write_bytes(b'outside')
        (mod.parent/'outside.map').symlink_to(outside)
        maps = onboarding.preview_maps(self.root)
        self.assertEqual(maps['mods/My Mod/Custom.MAP'], mod)
        self.assertIn('h2_maps_win64_dx11/lockout.map', maps)
        self.assertFalse(any(Path(name).stem in ('mainmenu', 'shared', 'single_player_shared', 'outside') for name in maps))
        self.assertEqual(onboarding.runtime_status(self.root), onboarding.NOT_PLAYABLE)

    def test_shader_repair_reuses_import_and_preserves_gameplay_gate(self):
        self.select()
        before = {p.relative_to(self.cache): p.read_bytes() for p in (self.cache / 'halo2').rglob('*') if p.is_file()}
        tables = b'T' * (28 * 1024)
        def install(root, output):
            directory = output / 'native-shaders'
            directory.mkdir()
            (directory / 'catalog.json').write_text(json.dumps(dict(
                schema='H2_PRESERVED_NATIVE_SHADER_CATALOG_V1', maps=[dict(map='lockout', complete=True)])))
            (directory / 'lockout').mkdir()
            (directory / 'lockout/library.json').write_text(json.dumps(dict(map='lockout', custom=True)))
            (directory / 'function-tables.bin').write_bytes(tables)
            return dict(complete=True, source='bundled native material library', maps=dict(lockout='verified'))
        with patch('native_shader_bundle.TABLE_SHA', hashlib.sha256(tables).hexdigest()), \
             patch('preserve_native_shaders.copy_selected', side_effect=install) as copy:
            cache, report = onboarding.repair_shaders(self.root, progress=self.messages.append)
            onboarding.repair_shaders(self.root, progress=self.messages.append)
        copy.assert_called_once()
        self.assertEqual(cache, self.cache)
        self.assertTrue(report['native_shader_library']['complete'])
        self.assertFalse(report['runtime_active'])
        self.assertFalse(report['full_tu15_parity'])
        self.assertEqual(onboarding.runtime_status(self.root), onboarding.NOT_PLAYABLE)
        self.assertEqual(before, {p.relative_to(self.cache): p.read_bytes() for p in (self.cache / 'halo2').rglob('*') if p.is_file()})

    def test_failed_shader_repair_publishes_no_partial_library(self):
        self.select()
        before = (self.cache / 'provenance.json').read_bytes()
        def broken(root, output):
            (output / 'native-shaders').mkdir()
            raise FormatError('shader fingerprint differs')
        with patch('preserve_native_shaders.copy_selected', side_effect=broken):
            with self.assertRaisesRegex(FormatError, 'fingerprint differs'):
                onboarding.repair_shaders(self.root)
        self.assertEqual((self.cache / 'provenance.json').read_bytes(), before)
        self.assertFalse((self.cache / 'native-shaders').exists())
        self.assertFalse(list(self.cache.glob('.native-shaders-*')))

    def test_malformed_legacy_registries_have_no_effect(self):
        for name in ('asset-sources', 'gameplay-source', 'loading-source', 'hud-source', 'menu-source'):
            (self.root / ('assets/' + name + '.tsv')).write_text('invalid old data')
        self.select()
        self.assertEqual(onboarding.selected(self.root)[0], self.cache)

    def test_missing_selection_asks_for_mcc_preparation(self):
        self.assertIsNone(onboarding.selected(self.root))
        self.assertIn('MCC has not been imported', onboarding.runtime_status(self.root))

    def test_failed_verification_preserves_existing_selection(self):
        self.select()
        registry = self.root / onboarding.REGISTRY
        before = registry.read_bytes()
        (self.cache / 'halo2/game.cfg').write_bytes(b'changed')
        with self.assertRaisesRegex(FormatError, 'changed'):
            self.select()
        self.assertEqual(registry.read_bytes(), before)
        self.assertEqual(list(registry.parent.glob('mcc-source.tsv.*.tmp')), [])

    def test_derived_map_uses_verified_output_hash(self):
        path = self.cache / 'halo2/h2_maps_win64_dx11/lockout.map'
        path.write_bytes(b'converted authored MCC map')
        (self.cache / 'tag-conversion.json').write_text(json.dumps(dict(maps=[dict(
            map='lockout.map', output_sha256=file_sha(path))])))
        self.verify(self.source)
        path.write_bytes(b'changed converted map')
        with self.assertRaisesRegex(FormatError, 'cached file changed'):
            self.verify(self.source)

    def test_unchanged_installation_reuses_private_cache_without_reimport(self):
        with patch.object(onboarding, 'discover', return_value=self.source) as discovery, patch.object(onboarding, 'prepare') as prepare:
            cache, _ = onboarding.setup(self.root, progress=self.messages.append)
        discovery.assert_called_once_with()
        prepare.assert_not_called()
        self.assertEqual(cache, self.cache)
        self.assertEqual(onboarding.selected(self.root)[0], self.cache)
        self.assertTrue((self.cache / 'presentation/movies/intro_60.mp4').is_file())
        self.assertIn('port_bundled_movies', onboarding.selected(self.root)[1])
        self.assertIn('port_authored_prompts', onboarding.selected(self.root)[1])

    def test_movie_only_repair_does_not_reverify_or_reimport_mcc(self):
        self.select()
        before = {p.relative_to(self.cache): p.read_bytes() for p in (self.cache / 'halo2').rglob('*') if p.is_file()}
        with patch.object(onboarding, 'prepare', side_effect=AssertionError('MCC reimport')), \
             patch.object(onboarding, 'source_files', side_effect=AssertionError('MCC rehash')):
            cache, report = onboarding.repair_movies(self.root, progress=self.messages.append)
        self.assertEqual(cache, self.cache)
        self.assertTrue(report['port_bundled_movies']['complete'])
        self.assertEqual(before, {p.relative_to(self.cache): p.read_bytes() for p in (self.cache / 'halo2').rglob('*') if p.is_file()})

    def test_prompt_only_repair_does_not_reverify_or_reimport_mcc(self):
        self.select()
        before = {p.relative_to(self.cache): p.read_bytes() for p in (self.cache / 'halo2').rglob('*') if p.is_file()}
        with patch.object(onboarding, 'prepare', side_effect=AssertionError('MCC reimport')), \
             patch.object(onboarding, 'source_files', side_effect=AssertionError('MCC rehash')):
            cache, report = onboarding.repair_prompts(self.root, progress=self.messages.append)
        self.assertEqual(cache, self.cache)
        self.assertTrue(report['port_authored_prompts']['complete'])
        self.assertEqual(before, {p.relative_to(self.cache): p.read_bytes() for p in (self.cache / 'halo2').rglob('*') if p.is_file()})

    def enable_current_graphics(self):
        from mcc_bitmap_correction_bundle import BUNDLE_V2, PROFILES_V2
        from mcc_bitmap_correction_bundle_v3 import BUNDLE_V3, MAP_NAME, PROFILES_V3
        for path in (BUNDLE_V2, BUNDLE_V3):
            target = self.root / path
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(b"installed bundle sentinel")
        return dict(schema="H2_MCC_APPLIED_GRAPHICS_CORRECTIONS_V3", complete=True,
                    restored_mips=40, changes=[],
                    profiles=[dict(map=name, tag=p["tag"], restored_mips=p["mip_count"],
                                   format=p["format"], target_sha256=p["target_sha256"])
                              for name, profiles in (("zanzibar.map", PROFILES_V2), (MAP_NAME, PROFILES_V3))
                              for p in profiles])

    def test_older_graphics_cache_requires_fresh_import_without_editing_it(self):
        self.enable_current_graphics()
        before = (self.cache / 'provenance.json').read_bytes()
        with patch.object(onboarding, 'prepare', return_value=(self.cache, self.report)) as prepare:
            onboarding.setup(self.root, self.source, progress=self.messages.append)
        prepare.assert_called_once_with(self.root, self.source, progress=self.messages.append)
        self.assertEqual((self.cache / 'provenance.json').read_bytes(), before)
        self.assertTrue(any('current graphics corrections' in message for message in self.messages))

    def test_complete_current_graphics_cache_reuses_verified_copy(self):
        self.report['graphics_corrections'] = self.enable_current_graphics()
        self.write_report()
        with patch.object(onboarding, 'prepare') as prepare:
            cache, _ = onboarding.setup(self.root, self.source, progress=self.messages.append)
        prepare.assert_not_called()
        self.assertEqual(cache, self.cache)

    def test_available_v4_requires_complete_six_image_profile_before_reuse(self):
        from mcc_bitmap_correction_bundle_v4 import BUNDLE_V4, MAP_NAME, PROFILES_V4
        current = self.enable_current_graphics()
        path = self.root / BUNDLE_V4
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(b"installed v4 bundle sentinel")
        self.assertTrue(onboarding._needs_graphics_reimport(self.root, {'graphics_corrections': current}))
        upgraded = dict(current, schema="H2_MCC_APPLIED_GRAPHICS_CORRECTIONS_V4",
                        restored_mips=48, profiles=current['profiles'] + [
                            dict(map=MAP_NAME, tag=p['tag'], restored_mips=p['mip_count'],
                                 format=p['format'], target_sha256=p['target_sha256'])
                            for p in PROFILES_V4])
        self.assertFalse(onboarding._needs_graphics_reimport(self.root, {'graphics_corrections': upgraded}))
        path.unlink()
        self.assertFalse(onboarding._needs_graphics_reimport(self.root, {'graphics_corrections': upgraded}))
        upgraded['profiles'] = upgraded['profiles'][:-1]
        self.assertTrue(onboarding._needs_graphics_reimport(self.root, {'graphics_corrections': upgraded}))

    def test_incomplete_current_graphics_profile_cannot_reuse_cache(self):
        self.report['graphics_corrections'] = self.enable_current_graphics()
        self.report['graphics_corrections']['profiles'].pop()
        self.write_report()
        with patch.object(onboarding, 'prepare', return_value=(self.cache, self.report)) as prepare:
            onboarding.setup(self.root, self.source, progress=self.messages.append)
        prepare.assert_called_once()

    def test_failed_graphics_upgrade_preserves_previous_selection_and_cache(self):
        self.select()
        self.enable_current_graphics()
        registry = self.root / onboarding.REGISTRY
        before_registry, before_cache = registry.read_bytes(), (self.cache / 'provenance.json').read_bytes()
        with patch.object(onboarding, 'prepare', side_effect=FormatError('interrupted graphics import')):
            with self.assertRaisesRegex(FormatError, 'interrupted graphics import'):
                onboarding.setup(self.root, self.source, progress=self.messages.append)
        self.assertEqual(registry.read_bytes(), before_registry)
        self.assertEqual((self.cache / 'provenance.json').read_bytes(), before_cache)

    def test_graphics_correction_hashes_are_checked_after_tag_conversion(self):
        path = self.cache / 'halo2/h2_maps_win64_dx11/textures.dat'
        name = path.relative_to(self.cache / 'halo2').as_posix()
        donor = self.report['source_files'][name]['sha256']
        path.write_bytes(b'corrected texture resource')
        change = dict(path='halo2/' + name, donor_sha256=donor, converted_sha256=file_sha(path))
        for key in ('output_changes', 'graphics_corrections'):
            with self.subTest(key=key):
                self.report.pop('output_changes', None)
                self.report.pop('graphics_corrections', None)
                self.report[key] = [change] if key == 'output_changes' else dict(changes=[change])
                self.write_report()
                self.verify(self.source)
        change['donor_sha256'] = 'unrelated donor'
        self.write_report()
        with self.assertRaisesRegex(FormatError, 'does not match the copied donor'):
            self.verify(self.source)

    def test_explicit_mcc_directory_takes_precedence_over_scan(self):
        with patch.object(onboarding, 'discover') as discovery, patch.object(onboarding, 'prepare') as prepare:
            onboarding.setup(self.root, self.source.parent, progress=self.messages.append)
        discovery.assert_not_called()
        prepare.assert_not_called()

    def test_source_update_requires_new_import(self):
        self.select()
        (self.source / 'data/My Mod/new tag.bin').write_bytes(b'new tag')
        with patch.object(onboarding, 'prepare', return_value=(self.cache, self.report)) as prepare:
            onboarding.setup(self.root, self.source, progress=self.messages.append)
        prepare.assert_called_once_with(self.root, self.source, progress=self.messages.append)
        self.assertTrue(any('asset set has changed' in m for m in self.messages))

    def test_source_hash_change_rejects_cache_reuse(self):
        (self.source / 'game.cfg').write_bytes(b'new installed settings')
        with self.assertRaisesRegex(FormatError, 'installed MCC file has changed'):
            self.verify(self.source)

    def test_update_during_verification_is_detected(self):
        changed = False
        def hashing(path):
            nonlocal changed
            digest = file_sha(path)
            if path.name == 'textures.dat' and path.is_relative_to(self.source) and not changed:
                changed = True
                (self.source / 'game.cfg').write_bytes(b'changed after it was hashed')
            return digest
        with patch.object(onboarding, 'file_sha', side_effect=hashing):
            with self.assertRaisesRegex(FormatError, 'changed during verification'):
                self.verify(self.source)
        self.assertTrue(changed)

    def test_private_cache_file_and_empty_directory_changes_are_detected(self):
        directory = self.cache / 'halo2/tags/My Mod/Empty'
        directory.rmdir()
        with self.assertRaisesRegex(FormatError, 'directory tree has changed'):
            self.verify()
        directory.mkdir()
        (self.cache / 'halo2/extra.bin').touch()
        with self.assertRaisesRegex(FormatError, 'file inventory has changed'):
            self.verify()

    def test_foreign_and_symlinked_caches_are_rejected(self):
        outside = self.root / 'foreign'
        outside.mkdir()
        with self.assertRaisesRegex(FormatError, 'private MCC cache'):
            onboarding.private_cache(self.root, outside)
        link = self.cache.parent / 'mcc-link'
        link.symlink_to(self.cache, target_is_directory=True)
        with self.assertRaisesRegex(FormatError, 'not a symlink'):
            onboarding.private_cache(self.root, link)

    def test_missing_required_bank_and_symlinked_manifest_are_rejected(self):
        p = self.cache / 'loading/mcc-loading-assets.json'
        p.unlink()
        with self.assertRaisesRegex(FormatError, 'incomplete'):
            self.select()
        p.symlink_to(self.cache / 'tag-conversion.json')
        with self.assertRaisesRegex(FormatError, 'incomplete'):
            self.select()

    def test_failed_new_import_does_not_replace_prior_selection(self):
        self.select()
        previous = (self.root / onboarding.REGISTRY).read_bytes()
        (self.source / 'game.cfg').write_bytes(b'update')
        with patch.object(onboarding, 'prepare', side_effect=FormatError('interrupted')):
            with self.assertRaisesRegex(FormatError, 'interrupted'):
                onboarding.setup(self.root, self.source, progress=self.messages.append)
        self.assertEqual((self.root / onboarding.REGISTRY).read_bytes(), previous)


if __name__ == '__main__':
    unittest.main()
