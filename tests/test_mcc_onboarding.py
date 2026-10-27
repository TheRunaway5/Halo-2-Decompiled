"""Verify MCC cache selection, reuse and failure publication boundaries."""
import hashlib
import json
from pathlib import Path
import shutil
import sys
import tempfile
import unittest
from unittest.mock import ANY, patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import mcc_onboarding as onboarding
from audit_rendering import FormatError, file_sha
from prepare_mcc_gameplay import PROFILE, source_directories, source_files
from test_mcc_movies import bundle_fixture as movie_fixture
from test_mcc_prompts import prompt_fixture
from test_mcc_ui_layout import layout_fixture
from test_mcc_loading_presentation import v3_summary_fixture
from native_variant_test_support import variant_fixture
from native_variant_assets import LAYOUTS, MANIFEST
from custom_ui_test_support import custom_ui_fixture, custom_ui_menu_fixture, empty_custom_map_fixture


def bundle_fixture(root):
    from test_controller_prompt_assets import controller_prompt_fixture
    layout_fixture(root)
    variant_fixture(root)
    controller_prompt_fixture(root)
    custom_ui_fixture(root)
    empty_custom_map_fixture(root)


class MccOnboardingTests(unittest.TestCase):
    def setUp(self):
        # This suite isolates cache publication and reuse. Exact all-eleven
        # loading page ownership has its own real-source and fixture checks.
        loading_sources = patch('mcc_loading_assets.compatible', return_value=True)
        loading_sources.start()
        self.addCleanup(loading_sources.stop)
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
                p.write_bytes((b'daeh\x0d\0\0\0' if name.endswith('mainmenu.map') else b'') + b'MCC donor ' + name.encode())
        (self.source / 'tags/My Mod/Empty').mkdir(parents=True)
        p = self.source / 'data/My Mod/Texture.bin'
        p.parent.mkdir(parents=True)
        p.write_bytes(b'authored mod data')
        (self.source / 'game.cfg').write_bytes(b'settings')
        self.cache.mkdir(parents=True)
        # Broad cache transactions use a stub producer; strict PCM/provenance
        # and media failures are exercised by loading-presentation controls.
        producer = patch('mcc_loading_presentation._prepare_v3',
            side_effect=lambda root, cache, source, progress: v3_summary_fixture(cache))
        self.loading_producer = producer.start()
        self.addCleanup(producer.stop)
        shutil.copytree(self.source, self.cache / 'halo2')
        for name in onboarding.REQUIRED:
            path = self.cache / name
            if not path.exists():
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text('{}')
        custom_ui_menu_fixture(self.cache, self.root)
        files, omitted = source_files(self.source)
        self.report = dict(schema=PROFILE, source=str(self.source),
                           external_installation_requirement='Halo MCC only', runtime_active=False,
                           full_tu15_parity=False, excluded_files=omitted,
                           source_directories=[p.as_posix() for p in source_directories(self.source)],
                           source_files={p.as_posix(): dict(bytes=s.st_size, sha256=file_sha(self.source / p)) for p, s in files})
        self.write_report()
        self.write_conversion()
        self.messages = []

    def write_report(self):
        (self.cache / 'provenance.json').write_text(json.dumps(self.report))

    def write_conversion(self):
        rows = [dict(map=Path(name).name, source_sha256=record['sha256'],
                     output_sha256=file_sha(self.cache / 'halo2' / name))
                for name, record in self.report['source_files'].items()
                if Path(name).parent.as_posix() == 'h2_maps_win64_dx11' and name.endswith('.map')]
        conversion = dict(profile=PROFILE, maps=rows)
        (self.cache / 'tag-conversion.json').write_text(json.dumps(conversion))
        return conversion

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
        for name in ('mainmenu', 'shared', 'single_player_shared'):
            self.assertNotIn(f'h2_maps_win64_dx11/{name}.map', maps)
        self.assertNotIn('mods/My Mod/outside.map', maps)
        self.assertEqual(onboarding.runtime_status(self.root), onboarding.NOT_PLAYABLE)

    def test_preview_keeps_distinct_mod_owners_with_stock_resource_basenames(self):
        self.select()
        names = [f'mods/{mod}/Maps/{name}'
                 for mod in ('First Mod', 'Second Mod')
                 for name in ('mainmenu.map', 'Shared.MAP', 'single_player_shared.map')]
        names.append('mods/Third Mod/h2_maps_win64_dx11/mainmenu.map')
        expected = {}
        for index, name in enumerate(names):
            path = self.cache / 'halo2' / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(f'distinct mod owner {index}'.encode())
            expected[name] = path
        outside = self.root / 'outside-resources.map'
        outside.write_bytes(b'not a private cache owner')
        alias = self.cache / 'halo2/mods/Linked Mod/Maps/shared.map'
        alias.parent.mkdir(parents=True)
        alias.symlink_to(outside)
        protected = [self.root / onboarding.REGISTRY, self.cache / 'provenance.json',
                     self.cache / 'tag-conversion.json', *expected.values()]
        before = {path: path.read_bytes() for path in protected}
        maps = onboarding.preview_maps(self.root)
        self.assertEqual(set(maps), set(expected) | {'h2_maps_win64_dx11/lockout.map'})
        for name, path in expected.items():
            self.assertEqual(maps[name], path)
        self.assertEqual(before, {path: path.read_bytes() for path in protected})
        report = onboarding.selected(self.root)[1]
        self.assertFalse(report['runtime_active'])
        self.assertFalse(report['full_tu15_parity'])
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
        # This pre-existing lightweight fixture verifies import reuse only; its
        # synthetic manifest is not native owner authority. Dedicated combined
        # repair tests exercise the real planner with authenticated metadata.
        class ImportReusePlan:
            def __init__(self, report):
                self.updated_provenance = report
                self.index_bytes, self.previous_index = b'{}', b'{}'
                self.provenance_bytes = (json.dumps(report) + '\n').encode()
            def check_beforeimage(self):
                pass
            def check_inputs(self):
                pass
        with patch('native_shader_bundle.TABLE_SHA', hashlib.sha256(tables).hexdigest()), \
             patch('mcc_native_map_owner_publication.prepare_owner_index_refresh',
                   side_effect=lambda cache, old, candidate, **kwargs: ImportReusePlan(candidate)), \
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

    def test_reused_cache_rejects_declared_owner_pointer_without_rewriting_it(self):
        self.select()
        self.report['native_shader_library'] = dict(maps={}, owner_bindings=dict(
            schema='H2_MCC_NATIVE_MAP_OWNERS_V1', path='../map-owners.json', bytes=2, sha256='0' * 64))
        self.write_report()
        protected = [p for base in (self.source, self.cache) for p in base.rglob('*') if p.is_file()]
        protected.append(self.root / onboarding.REGISTRY)
        before = {p: (p.read_bytes(), p.stat().st_ino, p.stat().st_mtime_ns) for p in protected}
        # Use the actual read-only owner verifier after the original cache and
        # source guards; setup must not repair or erase a declared bad pointer.
        with self.assertRaisesRegex(FormatError, 'corrupt declared owner pointer'):
            self.verify(self.source)
        self.assertEqual(before, {p: (p.read_bytes(), p.stat().st_ino, p.stat().st_mtime_ns) for p in protected})
        self.assertFalse((self.cache / 'native-shaders').exists())
        self.assertEqual(list((self.root / onboarding.REGISTRY).parent.glob('mcc-source.tsv.*.tmp')), [])

    def test_legacy_owner_index_absence_keeps_original_cache_hash_guards(self):
        cache, report = self.verify(self.source)
        self.assertEqual(cache, self.cache)
        self.assertNotIn('owner_bindings', report.get('native_shader_library', {}))
        self.assertFalse((self.cache / 'native-shaders/map-owners.json').exists())
        provenance = (self.cache / 'provenance.json').read_bytes()
        path = self.cache / 'halo2/h2_maps_win64_dx11/lockout.map'
        path.write_bytes(b'changed legacy cached map')
        with self.assertRaisesRegex(FormatError, 'cached file changed'):
            self.verify(self.source)
        self.assertEqual((self.cache / 'provenance.json').read_bytes(), provenance)
        self.assertFalse((self.cache / 'native-shaders').exists())

    def test_derived_map_uses_verified_output_hash(self):
        path = self.cache / 'halo2/h2_maps_win64_dx11/lockout.map'
        path.write_bytes(b'converted authored MCC map')
        self.write_conversion()
        self.verify(self.source)
        path.write_bytes(b'changed converted map')
        with self.assertRaisesRegex(FormatError, 'cached file changed'):
            self.verify(self.source)

    def test_unchanged_map_missing_conversion_result_is_rejected(self):
        conversion = self.write_conversion()
        conversion['maps'] = [row for row in conversion['maps'] if row['map'] != 'lockout.map']
        (self.cache / 'tag-conversion.json').write_text(json.dumps(conversion))
        before = (self.cache / 'halo2/h2_maps_win64_dx11/lockout.map').read_bytes()
        with self.assertRaisesRegex(FormatError, 'missing converted map owners.*lockout.map'):
            self.verify()
        self.assertEqual(before, (self.cache / 'halo2/h2_maps_win64_dx11/lockout.map').read_bytes())

    def test_duplicate_or_foreign_conversion_owners_cannot_be_laundered(self):
        for name, error in (('lockout.map', 'duplicate converted map'),
                            ('foreign.map', 'no copied stock owner'),
                            ('mods/My Mod/lockout.map', 'retain its stock relative owner'),
                            ('../lockout.map', 'invalid source-relative owner'),
                            ('/lockout.map', 'invalid source-relative owner'),
                            ('mods\\lockout.map', 'invalid source-relative owner')):
            with self.subTest(name=name):
                conversion = self.write_conversion()
                conversion['maps'].append(dict(conversion['maps'][0], map=name))
                (self.cache / 'tag-conversion.json').write_text(json.dumps(conversion))
                with self.assertRaisesRegex(FormatError, error):
                    self.verify()

    def test_conversion_source_and_output_digest_owners_are_checked(self):
        for field, value, error in (('source_sha256', '0' * 64, 'source differs'),
                                     ('source_sha256', 'not a digest', 'source differs'),
                                     ('output_sha256', 'not a digest', 'invalid converted map output')):
            with self.subTest(field=field, value=value):
                conversion = self.write_conversion()
                conversion['maps'][0][field] = value
                (self.cache / 'tag-conversion.json').write_text(json.dumps(conversion))
                with self.assertRaisesRegex(FormatError, error):
                    self.verify()

    def test_preserved_mod_owners_remain_distinct_from_stock_conversion(self):
        from mcc_conversion_coverage import validate
        additions = ('mods/First/maps/lockout.map', 'mods/Second/maps/lockout.map',
                     'mods/First/tags/objects/custom.weapon',
                     'mods/First/data/objects/custom.tif')
        for name in additions:
            self.report['source_files'][name] = dict(bytes=1, sha256='a' * 64)
        outputs, coverage = validate(self.report['source_files'], self.write_conversion())
        self.assertEqual(coverage['preserved_additional_map_owners'], list(additions[:2]))
        self.assertEqual(coverage['preserved_loose_tag_owners'], [additions[2]])
        self.assertEqual(coverage['preserved_loose_data_owners'],
                         [self.source.joinpath('data/My Mod/Texture.bin').relative_to(self.source).as_posix(), additions[3]])
        self.assertNotIn(additions[0], outputs)
        self.assertNotIn(additions[1], outputs)
        self.assertFalse(coverage['additional_map_or_loose_tag_conversion_qualified'])
        self.assertFalse(coverage['full_tu15_semantic_conversion'])

    def test_fresh_coverage_receipt_cannot_hide_missing_or_changed_owners(self):
        from mcc_conversion_coverage import validate
        _, coverage = validate(self.report['source_files'], self.write_conversion())
        self.report['conversion_coverage'] = coverage
        self.write_report()
        self.verify()
        self.report['conversion_coverage']['stock_map_owners'].pop()
        self.write_report()
        with self.assertRaisesRegex(FormatError, 'coverage differs from its copied-source owners'):
            self.verify()

    def test_malformed_conversion_reports_preserve_the_existing_selection(self):
        self.select()
        before = (self.root / onboarding.REGISTRY).read_bytes()
        for raw in ('{', '[]', '{}', json.dumps(dict(profile=PROFILE, maps=None)),
                    json.dumps(dict(profile=PROFILE, maps=[None])),
                    json.dumps(dict(profile=PROFILE, maps=[dict(map=None)]))):
            with self.subTest(raw=raw):
                (self.cache / 'tag-conversion.json').write_text(raw)
                with self.assertRaises(ValueError):
                    self.select()
                self.assertEqual((self.root / onboarding.REGISTRY).read_bytes(), before)
                self.assertEqual(list((self.root / onboarding.REGISTRY).parent.glob('mcc-source.tsv.*.tmp')), [])

    def test_cased_mod_paths_keep_their_authored_identity_without_conversion_admission(self):
        from mcc_conversion_coverage import validate
        additions = ('mods/My Mod/Tags/Objects/Custom.weapon',
                     'mods/My Mod/Data/Objects/Custom.tif', 'mods/My Mod/Maps/Lockout.MAP')
        for name in additions:
            self.report['source_files'][name] = dict(bytes=1, sha256='a' * 64)
        _, coverage = validate(self.report['source_files'], self.write_conversion())
        self.assertEqual(coverage['preserved_additional_map_owners'], [additions[2]])
        self.assertEqual(coverage['preserved_loose_tag_owners'], [additions[0]])
        self.assertEqual(coverage['preserved_loose_data_owners'],
                         ['data/My Mod/Texture.bin', additions[1]])
        self.assertFalse(coverage['additional_map_or_loose_tag_conversion_qualified'])

    def test_unchanged_installation_reuses_private_cache_without_reimport(self):
        with patch.object(onboarding, 'discover', return_value=self.source) as discovery, patch.object(onboarding, 'prepare') as prepare:
            cache, report = onboarding.setup(self.root, progress=self.messages.append)
        discovery.assert_called_once_with()
        prepare.assert_not_called()
        self.assertEqual(cache, self.cache)
        self.assertTrue(report['controller_prompts']['complete'])
        self.assertEqual(len(report['controller_prompts']['files']),18)
        self.assertTrue((cache/'presentation/prompts-playstation/atlases/1.dds').is_file())
        self.assertTrue((cache/'presentation/prompts-xbox/atlases/1.dds').is_file())
        self.assertEqual((cache / 'data/tags/variants/bomb-defusal-v1.json').read_bytes(),
                         (self.root / 'data/tags/variants/bomb-defusal-v1.json').read_bytes())
        self.assertEqual((cache / LAYOUTS).read_bytes(), (self.root / LAYOUTS).read_bytes())
        receipt = json.loads((cache / MANIFEST).read_bytes())
        self.assertEqual(receipt['files'][LAYOUTS]['sha256'], file_sha(self.root / LAYOUTS))
        self.assertEqual(onboarding.selected(self.root)[0], self.cache)
        self.assertFalse((self.root / 'resources/movies').exists())
        self.assertFalse((self.cache / 'presentation/movies').exists())
        self.assertNotIn('port_bundled_movies', onboarding.selected(self.root)[1])
        self.assertNotIn('port_authored_prompts', onboarding.selected(self.root)[1])
        self.assertFalse((self.root / 'resources/presentation/prompts').exists())
        self.assertFalse((self.cache / 'presentation/prompts').exists())
        self.assertEqual(onboarding.selected(self.root)[1]['native_loading_presentation']['profile'],
                         'mcc-logo-glow-mcc-track-oneshot-v1')
        self.assertNotIn('port_bundled_loading_ambience', onboarding.selected(self.root)[1])
        self.loading_producer.assert_called_once()

    def test_legacy_loading_inventory_requests_fresh_import_and_preserves_selection(self):
        self.select()
        registry = self.root / onboarding.REGISTRY
        before = registry.read_bytes()
        old_loading = (self.cache / 'loading/mcc-loading-assets.json').read_bytes()
        with patch('mcc_loading_assets.compatible', return_value=False), \
             patch.object(onboarding, 'prepare', side_effect=FormatError('fresh loading stage interrupted')) as prepare:
            with self.assertRaisesRegex(FormatError, 'fresh loading stage interrupted'):
                onboarding.setup(self.root, self.source, progress=self.messages.append)
        prepare.assert_called_once()
        self.assertEqual(registry.read_bytes(), before)
        self.assertEqual((self.cache / 'loading/mcc-loading-assets.json').read_bytes(), old_loading)

    def test_reuse_repairs_missing_defusal_without_reimporting_or_rewriting_maps(self):
        self.select()
        bomb = self.cache / 'data/tags/variants/bomb-defusal-v1.json'
        bomb.unlink()
        layouts = self.cache / 'data/tags/bomb-defusal-sites-v1.json'
        layouts.unlink()
        before = {p: (p.read_bytes(), p.stat().st_ino, p.stat().st_mtime_ns)
                  for p in (self.cache / 'halo2').rglob('*') if p.is_file()}
        with patch.object(onboarding, 'prepare', side_effect=AssertionError('MCC reimport')):
            onboarding.setup(self.root, self.source, progress=self.messages.append)
        self.assertEqual(bomb.read_bytes(), (self.root / 'data/tags/variants/bomb-defusal-v1.json').read_bytes())
        self.assertEqual(layouts.read_bytes(), (self.root / 'data/tags/bomb-defusal-sites-v1.json').read_bytes())
        self.assertEqual(before, {p: (p.read_bytes(), p.stat().st_ino, p.stat().st_mtime_ns) for p in before})

    def test_missing_bundled_defusal_does_not_publish_cache_selection(self):
        registry = self.root / onboarding.REGISTRY
        self.assertFalse(registry.exists())
        for name in ('data/tags/variants/bomb-defusal-v1.json', LAYOUTS):
            with self.subTest(missing=name):
                source = self.root / name
                raw = source.read_bytes()
                source.unlink()
                try:
                    with self.assertRaisesRegex(FormatError, 'Missing or oversized'):
                        self.select()
                    self.assertFalse(registry.exists())
                finally:
                    source.write_bytes(raw)

    def test_explicit_reference_movie_repair_does_not_reverify_or_reimport_mcc(self):
        movie_fixture(self.root)
        self.select()
        before = {p.relative_to(self.cache): p.read_bytes() for p in (self.cache / 'halo2').rglob('*') if p.is_file()}
        with patch.object(onboarding, 'prepare', side_effect=AssertionError('MCC reimport')), \
             patch.object(onboarding, 'source_files', side_effect=AssertionError('MCC rehash')):
            cache, report = onboarding.repair_movies(self.root, progress=self.messages.append)
        self.assertEqual(cache, self.cache)
        self.assertTrue(report['port_bundled_movies']['complete'])
        self.assertEqual(before, {p.relative_to(self.cache): p.read_bytes() for p in (self.cache / 'halo2').rglob('*') if p.is_file()})

    def test_normal_reuse_preserves_old_reference_bank_without_its_source(self):
        # An explicit historical/reference owner remains historical. Normal
        # setup must neither repair it nor rewrite its provenance as MCC art.
        prompt_fixture(self.root)
        self.select()
        _, legacy = onboarding.repair_prompts(self.root, progress=lambda _: None)
        entry = legacy['port_authored_prompts']
        bank = self.cache / 'presentation/prompts'
        before = {p.relative_to(bank): (p.read_bytes(), p.stat().st_ino, p.stat().st_mtime_ns)
                  for p in bank.rglob('*') if p.is_file()}
        shutil.rmtree(self.root / 'resources/presentation/prompts')
        with patch.object(onboarding, 'prepare', side_effect=AssertionError('MCC reimport')), \
             patch.object(onboarding, 'repair_prompts', side_effect=AssertionError('Reference prompt repair selected')), \
             patch('mcc_prompts.repair', side_effect=AssertionError('Reference prompt owner selected')):
            cache, report = onboarding.setup(self.root, self.source, progress=self.messages.append)
        self.assertEqual(cache, self.cache)
        self.assertEqual(report['port_authored_prompts'], entry)
        self.assertEqual(before, {p.relative_to(bank): (p.read_bytes(), p.stat().st_ino, p.stat().st_mtime_ns)
                                 for p in bank.rglob('*') if p.is_file()})

    def test_normal_reuse_ignores_preserved_movie_bank_and_missing_or_invalid_source(self):
        movie_fixture(self.root)
        self.select()
        _, historical = onboarding.repair_movies(self.root, progress=lambda _: None)
        entry = historical['port_bundled_movies']
        bank = self.cache / 'presentation/movies'
        shutil.rmtree(self.root / 'resources/movies')
        (self.root / 'resources/movie-manifest.json').unlink()
        for invalid_source in (False, True):
            with self.subTest(invalid_source=invalid_source):
                if invalid_source:
                    source = self.root / 'resources/movies'
                    source.mkdir()
                    (source / 'manifest.json').write_bytes(b'invalid preserved source metadata')
                    (source / 'intro_60.mp4').write_bytes(b'invalid preserved source payload')
                    (bank / 'manifest.json').write_bytes(b'invalid historical bank; not an MCC role')
                paths = [p for base in (bank, self.root / 'resources/movies')
                         for p in base.rglob('*') if p.is_file()]
                before = {p: (p.read_bytes(), p.stat().st_ino, p.stat().st_mtime_ns) for p in paths}
                with patch.object(onboarding, 'prepare', side_effect=AssertionError('MCC reimport')), \
                     patch.object(onboarding, 'repair_movies', side_effect=AssertionError('Reference movie repair selected')), \
                     patch('mcc_movies.repair', side_effect=AssertionError('Reference movie owner selected')):
                    cache, report = onboarding.setup(self.root, self.source, progress=self.messages.append)
                self.assertEqual(cache, self.cache)
                self.assertEqual(report['port_bundled_movies'], entry)
                self.assertEqual(before, {p: (p.read_bytes(), p.stat().st_ino, p.stat().st_mtime_ns) for p in paths})

    def test_explicit_reference_prompt_repair_does_not_reverify_or_reimport_mcc(self):
        prompt_fixture(self.root)
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
        # The fresh producer returns a new owner. Returning the old cache here
        # would incorrectly ask selection to install defaults into that owner.
        fresh = self.cache.with_name('mcc-fresh')
        shutil.copytree(self.cache, fresh)
        with patch.object(onboarding, 'prepare', return_value=(fresh, self.report)) as prepare:
            onboarding.setup(self.root, self.source, progress=self.messages.append)
        prepare.assert_called_once_with(self.root, self.source, progress=self.messages.append, presentation=ANY)
        self.assertFalse(prepare.call_args.kwargs['presentation']['qualified_owner'])
        self.assertEqual((self.cache / 'provenance.json').read_bytes(), before)
        self.assertEqual(onboarding.selected(self.root)[0], fresh)
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
        prepare.assert_called_once_with(self.root, self.source, progress=self.messages.append, presentation=ANY)
        self.assertFalse(prepare.call_args.kwargs['presentation']['qualified_owner'])
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
