"""Retained native dependencies never require the corresponding gameplay map."""
import copy
import json
from pathlib import Path
import shutil
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import mcc_onboarding as onboarding
import native_shader_bundle as bundle
import preserve_native_shaders as preserved
from audit_rendering import FormatError

CAIRO = '01b_spacestation'


def snapshot(root):
    return {p.relative_to(root).as_posix(): p.read_bytes() for p in root.rglob('*') if p.is_file()}


class NativeShaderClosureOnboardingTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix='h2-native-closure-')
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.source = self.root / 'retained'
        self.cache = self.root / 'cache'
        self.cache.mkdir()
        self.tables = b'T' * (28 * 1024)
        tables = patch.object(bundle, 'TABLE_SHA', bundle.digest(self.tables))
        tables.start();self.addCleanup(tables.stop)
        rows = []
        self.libraries = {}
        for name in ('lockout', CAIRO, 'midship'):
            directory = self.source / name
            (directory / 'payloads').mkdir(parents=True)
            records = []
            for group, payload in [('shad', ('native program ' + name).encode()), ('matg', bytes(464))]:
                filename = bundle.digest(payload) + '.bin'
                (directory / 'payloads' / filename).write_bytes(payload)
                records.append(dict(group=group, name='shaders\\' + name, payload=filename, bytes=len(payload),
                    fnv64=preserved.fnv(payload), pointers=[], references=[], strings=[]))
            library = dict(schema='H2_PRESERVED_NATIVE_SHADERS_V1', map=name,
                shader_tags=records[:1], rasterizer=records[1])
            (directory / 'library.json').write_bytes(bundle.encoded(library))
            (directory / 'function-tables.bin').write_bytes(self.tables)
            self.libraries[name] = library
            rows.append(dict(map=name, complete=True, tags=1, authored_note='retain catalog row'))
        self.catalog = dict(schema=bundle.CATALOG_SCHEMA, maps=rows, authored_catalog='retain metadata')
        (self.source / 'catalog.json').write_bytes(bundle.encoded(self.catalog))
        self.policy = dict(schema='H2_MCC_RETAINED_NATIVE_LIBRARY_CLOSURE_V1', source_map=CAIRO,
            source_canonical_sha256=bundle.canonical_digest(self.libraries[CAIRO]),
            libraries=[dict(map=name, canonical_sha256=bundle.canonical_digest(library),
                missing=[['stem', 'shaders\\shader_templates\\opaque\\tex_bump_illum_bloom']] if name == 'lockout' else [])
                for name, library in self.libraries.items()])
        policy = patch.object(bundle, 'closure_policy', return_value=self.policy)
        policy.start();self.addCleanup(policy.stop)
        bundle.create(self.source, self.root / bundle.BUNDLE)
        self.report = dict(runtime_active=False, full_tu15_parity=False,
            native_shader_library=dict(complete=True, source='existing retained selection', maps=dict(lockout='old')))
        (self.cache / 'provenance.json').write_bytes(bundle.encoded(self.report))
        (self.cache / 'halo2').mkdir()
        (self.cache / 'halo2/mod-data.bin').write_bytes(b'owned MCC content retained')

    def output(self, name='prepared'):
        path = self.root / name
        path.mkdir()
        return path

    def registry(self, *, prune=False):
        registry = self.root / 'assets/native-shaders-source.tsv'
        registry.parent.mkdir(exist_ok=True)
        registry.write_text('H2_NATIVE_SHADER_SOURCE_V1\nnative\t' + str(self.source) + '\n')
        if prune:
            (self.source / 'catalog.json').write_bytes(bundle.encoded({**self.catalog, 'maps': self.catalog['maps'][:1]}))
            shutil.rmtree(self.source / CAIRO)

    def legacy(self):
        destination = self.cache / 'native-shaders'
        destination.mkdir()
        shutil.copytree(self.source / 'lockout', destination / 'lockout')
        (destination / 'function-tables.bin').write_bytes(self.tables)
        rows = [self.catalog['maps'][0], dict(map='my_mod', complete=True, custom='retained')]
        (destination / 'catalog.json').write_bytes(bundle.encoded({**self.catalog, 'maps': rows}))
        (destination / 'my_mod').mkdir()
        (destination / 'my_mod/custom-program.bin').write_bytes(b'custom native program')
        return destination

    def repair(self):
        return onboarding.repair_cache_shaders(self.root, self.cache, self.report, progress=lambda _: None)

    def assert_native(self, output, names):
        native = output / 'native-shaders'
        catalog = json.loads((native / 'catalog.json').read_bytes())
        self.assertEqual(set(names), {row['map'] for row in catalog['maps']})
        for name in names:
            self.assertEqual(json.loads((native / name / 'library.json').read_bytes()), self.libraries[name])
            self.assertEqual(snapshot(native / name), snapshot(self.source / name) if (self.source / name).exists() else self.cairo_before)
        self.assertFalse(list(output.rglob('*.map')))
        self.assertFalse(list(output.rglob('*.xbe')))

    def test_missing_common_scalar_table_reuses_exact_existing_bytes(self):
        destination = self.legacy()
        _, report = self.repair()
        common = destination / 'function-tables.bin'
        common.unlink()
        before = snapshot(self.cache)
        with patch.object(preserved, 'copy_selected', side_effect=AssertionError('existing banks suffice')):
            _, updated = onboarding.repair_cache_shaders(self.root, self.cache, report, progress=lambda _: None)
        after = snapshot(self.cache)
        self.assertEqual(set(after) - set(before), {'native-shaders/function-tables.bin'})
        self.assertEqual(common.read_bytes(), self.tables)
        for name, data in before.items():
            self.assertEqual(after[name], data)
        self.assertEqual(updated, report)
        onboarding.repair_cache_shaders(self.root, self.cache, updated, progress=lambda _: None)
        self.assertEqual(snapshot(self.cache), after)

    def test_changed_common_scalar_table_rejects_before_closure_repair(self):
        destination = self.legacy()
        (destination / 'function-tables.bin').write_bytes(b'X' * len(self.tables))
        before = snapshot(self.cache)
        with self.assertRaisesRegex(FormatError, 'function tables have changed'):
            self.repair()
        self.assertEqual(snapshot(self.cache), before)

    def test_common_scalar_symlink_is_never_replaced(self):
        destination = self.legacy()
        (destination / 'function-tables.bin').unlink()
        outside = self.root / 'external-tables.bin'
        outside.write_bytes(self.tables)
        (destination / 'function-tables.bin').symlink_to(outside)
        with self.assertRaisesRegex(FormatError, 'private file owner'):
            self.repair()
        self.assertTrue((destination / 'function-tables.bin').is_symlink())
        self.assertEqual(outside.read_bytes(), self.tables)

    def test_failed_closure_does_not_publish_missing_common_table(self):
        destination = self.legacy()
        (destination / 'function-tables.bin').unlink()
        shutil.copytree(self.source / CAIRO, destination / CAIRO)
        (destination / CAIRO / 'library.json').write_bytes(bundle.encoded(self.libraries[CAIRO] | {'custom': True}))
        before = snapshot(self.cache)
        with self.assertRaisesRegex(FormatError, 'customized or incomplete'):
            self.repair()
        self.assertEqual(snapshot(self.cache), before)

    def test_scalar_publication_failure_leaves_existing_banks_and_no_temporary(self):
        destination = self.legacy()
        _, report = self.repair()
        (destination / 'function-tables.bin').unlink()
        before = snapshot(self.cache)
        with patch.object(onboarding.os, 'link', side_effect=PermissionError('fixture publication failure')):
            with self.assertRaisesRegex(PermissionError, 'fixture publication failure'):
                onboarding.repair_cache_shaders(self.root, self.cache, report, progress=lambda _: None)
        self.assertEqual(snapshot(self.cache), before)

    def test_mp_only_bundle_selection_retains_cairo_without_gameplay_map(self):
        output = self.output()
        result = bundle.copy_bundled(self.root, output, maps={'lockout'})
        self.assertEqual(set(result['maps']), {'lockout', CAIRO})
        self.assert_native(output, {'lockout', CAIRO})

    def test_mp_only_source_selection_uses_retained_cairo_without_bundle(self):
        self.registry()
        output = self.output()
        with patch.object(bundle, 'load_bundled', side_effect=AssertionError('bundle was not needed')):
            result = preserved.copy_selected(self.root, output, maps={'lockout'})
        self.assertEqual(set(result['maps']), {'lockout', CAIRO})
        self.assert_native(output, {'lockout', CAIRO})

    def test_pruned_source_selection_adds_only_canonical_cairo_from_port_bundle(self):
        self.cairo_before = snapshot(self.source / CAIRO)
        self.registry(prune=True)
        output = self.output()
        result = preserved.copy_selected(self.root, output)
        self.assertEqual(set(result['maps']), {'lockout', CAIRO})
        self.assert_native(output, {'lockout', CAIRO})

    def test_default_full_copies_preserve_all_existing_native_programs(self):
        bundled = self.output('bundled')
        result = bundle.copy_bundled(self.root, bundled)
        self.assertEqual(set(result['maps']), set(self.libraries))
        self.assert_native(bundled, set(self.libraries))
        self.registry()
        selected = self.output('selected')
        before = snapshot(self.source)
        result = preserved.copy_selected(self.root, selected)
        self.assertEqual(set(result['maps']), set(self.libraries))
        for name, data in before.items():
            self.assertEqual((selected / 'native-shaders' / name).read_bytes(), data)

    def test_missing_cairo_in_bundle_rejects_before_output_publication(self):
        (self.source / 'catalog.json').write_bytes(bundle.encoded({**self.catalog, 'maps': self.catalog['maps'][:1]}))
        bundle.create(self.source, self.root / bundle.BUNDLE)
        output = self.output()
        with self.assertRaisesRegex(FormatError, 'Cairo native library is missing'):
            bundle.copy_bundled(self.root, output, maps={'lockout'})
        self.assertFalse((output / 'native-shaders').exists())

    def test_custom_bundle_recipient_does_not_inherit_canonical_dependency(self):
        custom = copy.deepcopy(self.libraries['lockout']);custom['custom'] = True
        (self.source / 'lockout/library.json').write_bytes(bundle.encoded(custom))
        bundle.create(self.source, self.root / bundle.BUNDLE)
        output = self.output()
        result = bundle.copy_bundled(self.root, output, maps={'lockout'})
        self.assertEqual(set(result['maps']), {'lockout'})
        self.assertFalse((output / 'native-shaders' / CAIRO).exists())
        self.assertEqual(json.loads((output / 'native-shaders/lockout/library.json').read_bytes()), custom)

    def test_old_pruned_cache_gets_append_only_atomic_dependency_repair(self):
        destination = self.legacy()
        before = snapshot(self.cache)
        catalog = json.loads((destination / 'catalog.json').read_bytes())
        _, report = self.repair()
        self.assertEqual(json.loads((destination / 'catalog.json').read_bytes())['maps'][:2], catalog['maps'])
        for name, data in before.items():
            if name not in ('native-shaders/catalog.json', 'provenance.json'):
                self.assertEqual((self.cache / name).read_bytes(), data)
        self.assertEqual(snapshot(destination / CAIRO), snapshot(self.source / CAIRO))
        self.assertEqual(report['native_shader_library']['closure_repair']['dependent_maps'], ['lockout'])
        self.assertFalse(report['native_shader_library']['closure_repair']['existing_programs_replaced'])
        self.assertFalse(report['runtime_active']);self.assertFalse(report['full_tu15_parity'])
        after = snapshot(self.cache)
        with patch.object(preserved, 'copy_selected', side_effect=AssertionError('already repaired')):
            onboarding.repair_cache_shaders(self.root, self.cache, report, progress=lambda _: None)
        self.assertEqual(snapshot(self.cache), after)

    def test_pruned_source_registry_can_repair_existing_mp_cache_from_bundle(self):
        self.cairo_before = snapshot(self.source / CAIRO)
        self.registry(prune=True)
        destination = self.legacy()
        self.repair()
        self.assertEqual(snapshot(destination / CAIRO), self.cairo_before)

    def test_existing_custom_cairo_bank_is_never_replaced(self):
        destination = self.legacy()
        shutil.copytree(self.source / CAIRO, destination / CAIRO)
        manifest = destination / CAIRO / 'library.json'
        custom = self.libraries[CAIRO] | {'custom': True}
        manifest.write_bytes(bundle.encoded(custom))
        before = snapshot(self.cache)
        with self.assertRaisesRegex(FormatError, 'customized or incomplete'):
            self.repair()
        self.assertEqual(snapshot(self.cache), before)

    def test_existing_canonical_cairo_without_catalog_row_is_reused(self):
        destination = self.legacy()
        shutil.copytree(self.source / CAIRO, destination / CAIRO)
        before = snapshot(destination / CAIRO)
        with patch.object(preserved, 'copy_selected', side_effect=AssertionError('Cairo already exists')):
            self.repair()
        self.assertEqual(snapshot(destination / CAIRO), before)
        rows = json.loads((destination / 'catalog.json').read_bytes())['maps']
        self.assertEqual(sum(row['map'] == CAIRO for row in rows), 1)

    def test_missing_cairo_directory_restores_only_that_bank_and_preserves_catalog_row(self):
        destination = self.legacy()
        path = destination / 'catalog.json'
        catalog = json.loads(path.read_bytes())
        catalog['maps'].append(self.catalog['maps'][1])
        path.write_bytes(bundle.encoded(catalog))
        self.repair()
        self.assertEqual(json.loads(path.read_bytes()), catalog)
        self.assertEqual(snapshot(destination / CAIRO), snapshot(self.source / CAIRO))

    def test_fresh_selection_preserves_custom_cairo_by_refusing_replacement(self):
        self.registry()
        manifest = self.source / CAIRO / 'library.json'
        manifest.write_bytes(bundle.encoded(self.libraries[CAIRO] | {'custom': True}))
        before = snapshot(self.source)
        output = self.output()
        with self.assertRaisesRegex(FormatError, 'unavailable or customized'):
            preserved.copy_selected(self.root, output, maps={'lockout'})
        self.assertEqual(snapshot(self.source), before)
        self.assertFalse((output / 'native-shaders').exists())

    def test_changed_recipient_payload_is_not_reported_repaired(self):
        destination = self.legacy()
        payload = destination / 'lockout/payloads' / self.libraries['lockout']['shader_tags'][0]['payload']
        payload.write_bytes(b'custom altered program')
        before = snapshot(self.cache)
        with self.assertRaisesRegex(FormatError, 'changed payloads'):
            self.repair()
        self.assertEqual(snapshot(self.cache), before)

    def test_custom_recipient_and_unknown_catalog_are_preserved(self):
        destination = self.legacy()
        manifest = destination / 'lockout/library.json'
        manifest.write_bytes(bundle.encoded(self.libraries['lockout'] | {'custom': True}))
        before = snapshot(self.cache)
        with patch.object(preserved, 'copy_selected', side_effect=AssertionError('custom bank source read')):
            self.assertEqual(self.repair()[1], self.report)
        self.assertEqual(snapshot(self.cache), before)
        catalog = destination / 'catalog.json'
        catalog.write_text('{"schema":"unknown"}')
        before = snapshot(self.cache)
        with self.assertRaisesRegex(FormatError, 'Invalid cached native shader catalog'):
            self.repair()
        self.assertEqual(snapshot(self.cache), before)

    def test_closure_repair_rolls_back_catalog_and_new_bank_if_provenance_fails(self):
        self.legacy()
        before = snapshot(self.cache)
        replace = onboarding.os.replace
        def publish(source, target):
            if Path(target) == self.cache / 'provenance.json':
                raise OSError('simulated provenance publication failure')
            return replace(source, target)
        with patch.object(onboarding.os, 'replace', side_effect=publish):
            with self.assertRaisesRegex(OSError, 'provenance publication failure'):
                self.repair()
        self.assertEqual(snapshot(self.cache), before)
        self.assertFalse(list(self.cache.glob('.native-shaders-*')))
        self.assertFalse(list(self.cache.glob('.shader-provenance-*')))

    def test_new_library_install_also_rolls_back_if_provenance_fails(self):
        before = snapshot(self.cache)
        with patch.object(onboarding.os, 'replace', side_effect=OSError('publication failed')):
            with self.assertRaisesRegex(OSError, 'publication failed'):
                self.repair()
        self.assertEqual(snapshot(self.cache), before)

    def test_missing_retained_source_preserves_old_cache(self):
        self.legacy()
        (self.root / bundle.BUNDLE).unlink()
        before = snapshot(self.cache)
        with self.assertRaisesRegex(FormatError, 'missing'):
            self.repair()
        self.assertEqual(snapshot(self.cache), before)

    def test_incomplete_or_custom_cairo_catalog_entry_is_preserved(self):
        destination = self.legacy()
        path = destination / 'catalog.json'
        catalog = json.loads(path.read_bytes())
        catalog['maps'].append(dict(map=CAIRO, complete=False, tags=17, custom=True))
        path.write_bytes(bundle.encoded(catalog))
        before = snapshot(self.cache)
        with self.assertRaisesRegex(FormatError, 'catalog entry is customized'):
            self.repair()
        self.assertEqual(snapshot(self.cache), before)

    def test_bundled_bank_installer_never_overwrites_an_existing_directory(self):
        package = bundle.load_bundled(self.root)
        destination = self.output()
        (destination / CAIRO).mkdir()
        (destination / CAIRO / 'library.json').write_bytes(b'custom existing bank')
        before = snapshot(destination)
        with self.assertRaises(FileExistsError):
            bundle.install_bundled_library(destination, CAIRO, package)
        self.assertEqual(snapshot(destination), before)


if __name__ == '__main__':
    unittest.main()
