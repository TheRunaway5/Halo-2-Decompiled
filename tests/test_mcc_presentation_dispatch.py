"""MCC-only delivered dispatch; explicit reference helpers stay separate."""
import json
import os
from pathlib import Path
import shutil
import sys
import tempfile
import unittest
from unittest.mock import patch
import zipfile

from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import mcc_onboarding
import mcc_presentation as presentation
import mcc_loading_presentation as loading
import mcc_movies
import mcc_prompts
import package_mcc_test_build as packaging
import prepare_mcc_gameplay as preparation
import test_frontend_capability as capabilities
from test_mcc_movies import bundle_fixture
from test_mcc_prompts import prompt_fixture
from test_mcc_ui_layout import layout_fixture
from native_variant_test_support import variant_fixture
from test_mcc_gameplay_preparation import source as mcc_source_fixture


class PresentationOwnerTests(unittest.TestCase):
    def setUp(self):
        self.fixture = capabilities.FrontendCapabilityTests()
        self.fixture.setUp()
        self.addCleanup(self.fixture.doCleanups)
        self.root = self.fixture.root
        self.selection = self.root / 'frontend-runtime-selection.json'

    def select(self):
        fixture = self.fixture
        self.selection.write_text(json.dumps(dict(schema='H2_LOCAL_FRONTEND_RUNTIME_SELECTION_V1',
            runtime=fixture.runtime.relative_to(self.root).as_posix(),
            runtime_sha256=fixture.digest(fixture.runtime),
            capability_sha256=fixture.digest(fixture.sidecar))))

    def authored(self, *, title_only=False):
        if 'mcc_prompt_font_profile' in self.fixture.report['validation']:
            self.fixture.authored_presentation_family()
        if title_only:
            for flag in ('loading_presentation', 'original_loading_ambience'):
                self.fixture.report['validation'].pop(flag)
            for name in list(self.fixture.report['resources']):
                if name.startswith('presentation/loading-v2/'):
                    self.fixture.report['resources'].pop(name)
            self.fixture.publish()
        self.select()

    def test_authenticated_legacy_owner_requires_update_before_cache_import(self):
        for title_only in (False, True):
            with self.subTest(title_only=title_only):
                # Use a fresh fixture for each distinct capability contract.
                self.authored(title_only=title_only)
                before = self.fixture.sidecar.read_bytes()
                held = self.fixture.cache.with_name('held-cache')
                self.fixture.cache.rename(held)
                try:
                    with self.assertRaisesRegex(ValueError, 'MCC-capable runtime update'):
                        presentation.requirements(self.root)
                    self.assertFalse(self.fixture.available())
                finally:
                    held.rename(self.fixture.cache)
                self.assertEqual(before, self.fixture.sidecar.read_bytes())
                self.assertNotIn('mcc_prompt_font_profile', self.fixture.report['validation'])

    def test_explicit_v3_uses_complete_profile_without_authored_sources(self):
        self.select()
        value = presentation.requirements(self.root)
        self.assertEqual(value['family'], presentation.V3)
        self.assertFalse((self.root / mcc_prompts.SOURCE).exists())
        with patch.object(loading, 'preflight_v3') as v3, \
             patch.object(mcc_movies, 'required_bytes', side_effect=AssertionError('movie source selected')):
            self.assertEqual(presentation.preflight(self.root, self.root / 'halo2', value), 0)
        v3.assert_called_once()

    def test_no_capability_retains_unqualified_v3_default_and_pins_absence(self):
        value = presentation.requirements(self.root)
        self.assertEqual(value['family'], presentation.V3)
        self.assertFalse(value['qualified_owner'])
        self.selection.write_text('{}')
        with self.assertRaisesRegex(ValueError, 'selection changed'):
            presentation.recheck(value)

    def test_partial_unknown_and_mixed_profiles_never_fall_back(self):
        validation = self.fixture.report['validation']
        original = dict(validation)
        cases = [dict(mcc_prompt_font_profile=None), dict(mcc_loading_presentation_profile='unknown'),
                 dict(original_loading_ambience=True)]
        for change in cases:
            with self.subTest(change=change):
                validation.clear(); validation.update(original); validation.update(change)
                self.fixture.publish(); self.select()
                with self.assertRaises(ValueError):
                    presentation.requirements(self.root)
        validation.clear(); validation.update(original)
        validation.pop('mcc_prompt_font_profile')
        self.fixture.publish(); self.select()
        with self.assertRaises(ValueError):
            presentation.requirements(self.root)

    def test_changed_owner_and_parent_alias_reject_before_producers(self):
        self.select()
        before = self.fixture.sidecar.read_bytes()
        value = presentation.requirements(self.root)
        self.fixture.sidecar.write_bytes(before + b' ')
        with self.assertRaises(ValueError):
            presentation.recheck(value)
        with self.assertRaises(ValueError):
            presentation.requirements(self.root)
        self.fixture.sidecar.write_bytes(before)
        alias = self.root / 'alias'
        alias.symlink_to(self.fixture.runtime.parent, target_is_directory=True)
        record = json.loads(self.selection.read_text())
        record['runtime'] = 'alias/halo2_native'
        self.selection.write_text(json.dumps(record))
        with self.assertRaisesRegex(ValueError, 'symlink component'):
            presentation.requirements(self.root)

    def test_v1_automatic_loading_is_unsupported_without_relabeling(self):
        self.authored()
        self.fixture.report['validation']['original_loading_ambience'] = False
        self.fixture.publish(); self.select()
        before = self.fixture.sidecar.read_bytes()
        with self.assertRaisesRegex(ValueError, 'MCC-capable runtime update'):
            presentation.requirements(self.root)
        self.assertEqual(before, self.fixture.sidecar.read_bytes())

    def test_explicit_package_candidate_ignores_other_local_frontend_selection(self):
        self.select()
        self.selection.write_text('{invalid selection for another runtime')
        value = presentation.requirements(self.root, runtime=self.fixture.runtime)
        self.assertEqual(value['family'], presentation.V3)
        presentation.recheck(value)

    def test_legacy_package_requires_runtime_update_before_assembly(self):
        self.authored()
        output = self.root / 'unqualified-authored.zip'
        with patch.object(packaging, '_assemble', side_effect=AssertionError('archive stage reached')):
            with self.assertRaisesRegex(ValueError, 'MCC-capable runtime update'):
                packaging.package(self.root, self.fixture.runtime, self.root, output)
        self.assertFalse(output.exists())

    def test_invalid_explicit_release_does_not_choose_default(self):
        (self.root / 'runtime-selection.json').write_text('{}')
        with patch('update_runtime.selected_runtime', return_value=None):
            with self.assertRaisesRegex(ValueError, 'Selected release runtime is invalid'):
                presentation.requirements(self.root)

    def test_changed_shader_is_not_a_metadata_only_owner(self):
        self.select()
        value = presentation.requirements(self.root)
        (self.fixture.shaders / 'hud.frag.spv').write_bytes(b'changed')
        with self.assertRaisesRegex(ValueError, 'shader bytes changed'):
            presentation.recheck(value)

    def test_public_dispatch_rejects_legacy_and_stale_owner_values_before_donor_work(self):
        self.select()
        valid = presentation.requirements(self.root)
        # The ordinary capability reader authenticates these actual fixture
        # executable/shader/sidecar owners; only its legacy profile is refused.
        old = dict(valid, family=presentation.AUTHORED)
        stale = dict(valid, owner=dict(valid['owner'], report=dict(valid['owner']['report'])))
        stale['owner']['report']['validation'] = dict(stale['owner']['report']['validation'])
        stale['owner']['report']['validation'].pop('mcc_loading_presentation_profile')
        stale['owner']['report']['validation'].pop('mcc_prompt_font_profile')
        source = self.root / 'missing-MCC/halo2'
        cache = self.root / 'new-private-cache'
        calls = (
            lambda value: presentation.required_bytes(self.root, value),
            lambda value: presentation.preflight(self.root, source, value),
            lambda value: presentation.prepare(self.root, cache, source, value, progress=lambda _: None),
            lambda value: presentation.compatible({}, value),
            lambda value: presentation.repair(self.root, cache, source, value, progress=lambda _: None),
            presentation.recheck,
        )
        for value in (old, stale, dict(family=presentation.V3)):
            for index, call in enumerate(calls):
                with self.subTest(value=value['family'], entry=index):
                    with self.assertRaisesRegex(ValueError, 'MCC-capable runtime update'):
                        call(value)
        self.assertFalse(cache.exists())
        presentation.recheck(valid)

    def test_v3_prepare_and_repair_dispatch_only_to_the_mcc_producers(self):
        self.select()
        value = presentation.requirements(self.root)
        source, cache = self.root / 'halo2', self.root / 'private-cache'
        prepared, repaired = object(), object()
        with patch.object(loading, 'prepare_v3', return_value=prepared) as prepare, \
             patch.object(loading, 'repair_v3', return_value=repaired) as repair, \
             patch.object(loading, 'prepare', side_effect=AssertionError('V2 selected')), \
             patch.object(loading, 'repair', side_effect=AssertionError('V2 repair selected')), \
             patch.object(mcc_movies, 'prepare', side_effect=AssertionError('movie donor selected')), \
             patch.object(mcc_prompts, 'prepare', side_effect=AssertionError('prompt donor selected')):
            self.assertEqual(presentation.required_bytes(self.root, value), 0)
            self.assertEqual(presentation.prepare(self.root, cache, source, value),
                             dict(native_loading_presentation=prepared))
            self.assertIs(presentation.repair(self.root, cache, source, value), repaired)
        prepare.assert_called_once_with(self.root, cache, source, progress=print)
        repair.assert_called_once_with(self.root, cache, source, progress=print)
        presentation.recheck(value)


class PresentationFilesystemTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix='halo2-presentation-files-')
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name) / 'port'
        bundle_fixture(self.root)
        prompt_fixture(self.root)
        source = Path(__file__).resolve().parents[1]
        target = self.root / loading.AMBIENCE
        target.mkdir(parents=True)
        for name in ('manifest.json', loading.TONE):
            shutil.copyfile(source / loading.AMBIENCE / name, target / name)
        self.cache = self.root / 'assets/imported/mcc-copy'
        self.cache.mkdir(parents=True)
        (self.cache / 'bitmap-resources.json').write_text('{}')
        self.report = dict(schema='H2_MCC_TU15_PREPARATION_V1',
            external_installation_requirement='Halo MCC only', source=str(self.root / 'halo2'),
            runtime_active=False, full_tu15_parity=False, custom='preserve')
        (self.cache / 'provenance.json').write_text(json.dumps(self.report))
        self.value = dict(family=presentation.AUTHORED)
        self.bitmap = patch.object(loading, '_bitmap', side_effect=lambda cache, inventory, name:
            (Image.new('RGBA', (64, 64), (90, 90, 90, 255)), dict(tag=name)))
        self.bitmap.start(); self.addCleanup(self.bitmap.stop)

    def qualified_project(self):
        fixture = capabilities.FrontendCapabilityTests()
        fixture.setUp(); self.addCleanup(fixture.doCleanups)
        fixture.authored_presentation_family()
        shutil.copytree(self.root / 'resources', fixture.root / 'resources')
        layout_fixture(fixture.root)
        variant_fixture(fixture.root)
        from test_controller_prompt_assets import controller_prompt_fixture
        controller_prompt_fixture(fixture.root)
        (fixture.root / 'frontend-runtime-selection.json').write_text(json.dumps(dict(
            schema='H2_LOCAL_FRONTEND_RUNTIME_SELECTION_V1',
            runtime=fixture.runtime.relative_to(fixture.root).as_posix(),
            runtime_sha256=fixture.digest(fixture.runtime),
            capability_sha256=fixture.digest(fixture.sidecar))))
        source = mcc_source_fixture(Path(self.temporary.name) / 'MCC')
        for name in ('single_player_shared.map', 'lockout.map', 'sounds_neutral.dat'):
            (source / 'h2_maps_win64_dx11' / name).write_bytes(b'copied fixture ' + name.encode())
        return fixture, source

    def import_fixture(self, fixture, source):
        def maps(directory, stage, progress):
            rows = [dict(map=p.name, source_sha256=preparation.file_sha(p),
                         output_sha256=preparation.file_sha(p)) for p in sorted(directory.glob('*.map'))]
            preparation.write_json(stage / 'tag-conversion.json', dict(profile=preparation.PROFILE, maps=rows))
            preparation.write_json(stage / 'bitmap-resources.json', {})
            return dict(maps=len(rows))

        def loading_assets(root, stage):
            (stage / 'loading').mkdir()
            (stage / 'loading/mcc-loading-assets.json').write_text('{}')
            return dict(complete=True, vista_executable_bank_imported=False)

        # Only unrelated map/menu producers are fixture boundaries. The actual
        # fresh transaction, source copies and presentation producers execute.
        with patch.object(preparation, 'prepare_maps', side_effect=maps), \
             patch('mcc_menu_inventory.verify', return_value=dict(complete=True, banks=0,
                 frame_owners=0, unique_dds_pages=0, fonts={})), \
             patch('mcc_menu_assets.prepare', return_value=dict(complete=True,
                 frame_owners=0, unique_dds_pages=0, fonts={})), \
             patch('mcc_loading_assets.prepare', side_effect=loading_assets), \
             patch.object(preparation, 'inventory_player_screen_flashes',
                 return_value=dict(complete=False, summary={})), \
             patch.object(loading, 'preflight_v3', side_effect=AssertionError('V3 selected')):
            return preparation.prepare(fixture.root, source, progress=lambda _: None)

    def test_fresh_and_reuse_paths_refuse_legacy_owner_without_copying_or_repairing(self):
        fixture, source = self.qualified_project()
        cap = fixture.sidecar.read_bytes()
        selection = (fixture.root / 'frontend-runtime-selection.json').read_bytes()
        protected = {p: (p.read_bytes(), p.stat().st_ino, p.stat().st_mtime_ns)
                     for p in fixture.root.rglob('*') if p.is_file()}
        with patch.object(preparation, 'copy_verified', side_effect=AssertionError('MCC source copy selected')), \
             patch.object(loading, 'repair', side_effect=AssertionError('V2 repair selected')):
            with self.assertRaisesRegex(ValueError, 'MCC-capable runtime update'):
                self.import_fixture(fixture, source)
            with self.assertRaisesRegex(ValueError, 'MCC-capable runtime update'):
                mcc_onboarding.setup(fixture.root, source, progress=lambda _: None)
        self.assertEqual(protected, {p: (p.read_bytes(), p.stat().st_ino, p.stat().st_mtime_ns) for p in protected})
        self.assertEqual(cap, fixture.sidecar.read_bytes())
        self.assertEqual(selection, (fixture.root / 'frontend-runtime-selection.json').read_bytes())
        self.assertFalse(list((fixture.root / 'assets/imported').glob('.staging-*')))

    def test_legacy_owner_rejects_before_space_or_source_work(self):
        fixture, source = self.qualified_project()
        with patch.object(preparation.shutil, 'disk_usage', side_effect=AssertionError('space probe selected')), \
             patch.object(preparation, 'copy_verified', side_effect=AssertionError('source copy selected')):
            with self.assertRaisesRegex(ValueError, 'MCC-capable runtime update'):
                preparation.prepare(fixture.root, source, progress=lambda _: None)
        self.assertFalse(list((fixture.root / 'assets/imported').glob('.staging-*')))

    def test_supplied_sources_do_not_authorize_legacy_dispatch(self):
        before = {p: (p.read_bytes(), p.stat().st_mtime_ns) for p in self.root.rglob('*') if p.is_file()}
        for call in (lambda: presentation.preflight(self.root, self.root / 'halo2', self.value),
                     lambda: presentation.prepare(self.root, self.cache, self.root / 'halo2', self.value)):
            with self.assertRaisesRegex(ValueError, 'MCC-capable runtime update'):
                call()
        self.assertEqual(before, {p: (p.read_bytes(), p.stat().st_mtime_ns) for p in self.root.rglob('*') if p.is_file()})

    def test_custom_bank_failure_does_not_overwrite_it_or_provenance(self):
        path = self.cache / mcc_movies.BANK / 'intro_60.mp4'
        path.parent.mkdir(parents=True)
        path.write_bytes(b'custom user movie')
        before = (self.cache / 'provenance.json').read_bytes()
        with self.assertRaisesRegex(ValueError, 'MCC-capable runtime update'):
            presentation.repair(self.root, self.cache, self.root / 'halo2', self.value, progress=lambda _: None)
        self.assertEqual(path.read_bytes(), b'custom user movie')
        self.assertEqual(before, (self.cache / 'provenance.json').read_bytes())

    def test_different_active_profile_requires_separate_cache_without_rewrite(self):
        value = presentation.requirements(self.root)
        report = dict(native_loading_presentation=dict(profile=loading.PROFILE))
        before = json.dumps(report)
        self.assertFalse(presentation.compatible(report, value))
        self.assertEqual(before, json.dumps(report))
        self.assertTrue(presentation.compatible({}, value))
        self.assertTrue(presentation.compatible(dict(native_loading_presentation=dict(profile=loading.V3_PROFILE)), value))
        with self.assertRaises(ValueError):
            presentation.compatible(dict(native_loading_presentation=dict(profile='custom')), value)

    def test_explicit_reference_helpers_copy_every_owner_without_transcoding(self):
        with patch.object(packaging, 'converter_dependencies', side_effect=AssertionError('decoder required')):
            sources = packaging.presentation_sources(self.root, self.value)
            stage = self.root / 'portable'
            copied = packaging.copy_presentation_sources(self.root, stage, sources)
        self.assertEqual(copied, sources)
        self.assertEqual(len(sources['authored_prompts']['files']), 91)
        archive = self.root / 'tiny.zip'
        packaging.write_archive(stage, archive, sources['bundled_movies'])
        with zipfile.ZipFile(archive) as value:
            for name, row in sources['bundled_movies']['files'].items():
                member = value.getinfo(stage.name + '/' + name)
                self.assertEqual(value.read(member), (self.root / name).read_bytes())
                if name in sources['bundled_movies']['roles'].values():
                    self.assertEqual(member.compress_type, zipfile.ZIP_STORED)
            self.assertFalse(any('/assets/imported/' in n for n in value.namelist()))

    def test_package_source_tamper_cannot_publish_copied_bank(self):
        sources = packaging.presentation_sources(self.root, self.value)
        path = self.root / mcc_prompts.SOURCE / 'atlases/0.dds'
        path.write_bytes(b'changed')
        with self.assertRaises(ValueError):
            packaging.copy_presentation_sources(self.root, self.root / 'portable', sources)
        self.assertFalse((self.root / 'portable' / mcc_prompts.SOURCE).exists())

    def test_v3_package_branch_does_not_read_supplied_sources(self):
        with patch.object(packaging, 'converter_dependencies', return_value={'qualified': 'decoder'}) as decoder, \
             patch.object(packaging, 'verify_bundled_movies', side_effect=AssertionError('movies selected')), \
             patch.object(packaging, 'verify_bundled_prompts', side_effect=AssertionError('prompts selected')), \
             patch.object(packaging, 'bundled_ambience', side_effect=AssertionError('tone selected')):
            self.assertEqual(packaging.presentation_sources(self.root, dict(family=presentation.V3)),
                             dict(mcc_loading_converter={'qualified': 'decoder'}))
        decoder.assert_called_once_with(self.root)

    def test_bound_resource_root_is_used_by_explicit_reference_copy_helpers(self):
        data = Path(self.temporary.name) / 'writable-data'
        data.mkdir()
        binding = dict(schema='H2_INSTALLATION_LAYOUT_V1', data=str(data), resources=str(self.root))
        with patch.dict(os.environ, H2_INSTALLATION_LAYOUT=json.dumps(binding)):
            sources = packaging.presentation_sources(data, self.value)
            copied = packaging.copy_presentation_sources(data, data / 'portable', sources)
        self.assertEqual(copied, sources)


class PresentationPublicationTests(unittest.TestCase):
    def test_invalid_explicit_selection_stops_before_any_source_import(self):
        with tempfile.TemporaryDirectory(prefix='halo2-presentation-') as temporary:
            root = Path(temporary)
            (root / 'frontend-runtime-selection.json').write_text('{')
            source = root / 'MCC/halo2'
            source.mkdir(parents=True)
            with patch.object(mcc_onboarding, 'directory', return_value=source), \
                 patch.object(mcc_onboarding, 'prepare', side_effect=AssertionError('source import reached')):
                with self.assertRaises(ValueError):
                    mcc_onboarding.setup(root, source, progress=lambda _: None)


if __name__ == '__main__':
    unittest.main()
