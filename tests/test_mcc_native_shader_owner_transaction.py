"""Combined native-repair metadata rollback, with synthetic owned map bytes.

Only the authentic shipped scalar constants are decoded. All map/program bytes
below are constructed fixtures; no MCC installation or reference bank is read.
"""
import copy
import errno
import json
import os
from pathlib import Path
import shutil
import signal
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import mcc_onboarding as onboarding
import mcc_native_map_owner_publication as publication
import mcc_native_map_owners as owners
import native_shader_bundle as bundle
import preserve_native_shaders as preserved
from audit_rendering import FormatError
from mcc_native_scalar_fixture import shipped_native_scalar_tables
from test_mcc_native_map_owners import Fixture, encoded, sha

CAIRO = '01b_spacestation'


def snapshot(root):
    return ({p.relative_to(root).as_posix(): p.read_bytes() for p in root.rglob('*') if p.is_file()},
            sorted(p.relative_to(root).as_posix() for p in root.rglob('*') if p.is_dir()))


class NativeShaderOwnerTransactionTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tables = shipped_native_scalar_tables()

    def setUp(self):
        scratch = os.environ.get('HALO2_TEST_SCRATCH')
        self.assertTrue(scratch and Path(scratch).is_dir(), 'Use the owned run_test lifecycle')
        temporary = tempfile.TemporaryDirectory(prefix='shader-owner-transaction-', dir=scratch)
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.cache, self.source, self.policies = (self.root / name for name in ('cache', 'retained', 'policies'))
        self.cache.mkdir(); self.source.mkdir(); self.policies.mkdir()
        f = self.fixture = Fixture()
        self.stock = f.add_map('h2_maps_win64_dx11/lockout.map', b'original copied source')
        f.add_map('h2_maps_win64_dx11/shared.map', b'copied shared resource', role='resources')
        self.custom = [f.add_map('Mods/Pack \u03a9/' + name + '/Maps/lockout.map', name.encode()) for name in ('A', 'B')]
        converted, final = b'converted map', b'final encoded map after graphics, with another extent'
        f.conversion['maps'][0]['output_sha256'] = sha(converted)
        f.provenance['graphics_corrections'] = dict(changes=[dict(path=self.stock,
            donor_sha256=sha(converted), converted_sha256=sha(final))])
        f.snapshots[self.stock] = final
        f.verified[self.stock] = dict(bytes=len(final), sha256=sha(final))
        for name, raw in f.snapshots.items():
            self.write(self.cache / name, raw)
        (self.cache / 'halo2/Mods/Shared Assets').mkdir()
        self.libraries = {}
        for name in ('lockout', CAIRO):
            self.libraries[name] = self.library(self.source / name, name, name)
        self.catalog = dict(schema=owners.CATALOG_SCHEMA, authored_catalog='retain exact metadata',
            maps=[dict(map=name, complete=True, tags=1, authored_row=name) for name in ('lockout', CAIRO)])
        self.write(self.source / 'catalog.json', encoded(self.catalog))
        registry = self.root / 'assets/native-shaders-source.tsv'
        self.write(registry, ('H2_NATIVE_SHADER_SOURCE_V1\nnative\t' + str(self.source) + '\n').encode())
        shutil.copytree(self.source / 'lockout', self.cache / 'native-shaders/lockout')
        self.write(self.cache / 'native-shaders/catalog.json', encoded({**self.catalog, 'maps': self.catalog['maps'][:1]}))
        self.custom_banks = []
        for owner, marker in zip(self.custom, ('A', 'B')):
            directory = 'native-shaders/maps/halo2/Mods/Pack \u03a9/' + marker + '/Maps/lockout'
            raw = self.library(self.cache / directory, 'lockout', 'custom ' + marker)
            self.custom_banks.append(owners.bank_identity(directory, encoded(raw)))
        self.policy = dict(schema='H2_MCC_RETAINED_NATIVE_LIBRARY_CLOSURE_V1', source_map=CAIRO,
            source_canonical_sha256=bundle.canonical_digest(self.libraries[CAIRO]), libraries=[
                dict(map='lockout', canonical_sha256=bundle.canonical_digest(self.libraries['lockout']),
                     missing=[['stem', 'fixture-template']]),
                dict(map=CAIRO, canonical_sha256=bundle.canonical_digest(self.libraries[CAIRO]), missing=[])])
        self.write(self.policies / 'mcc-native-library-closure.json', encoded(self.policy))
        self.write(self.policies / 'mcc-native-hud-bootstrap.json', encoded(dict(schema=owners.HUD_SCHEMA,
            source_map=CAIRO, source_canonical_sha256=self.policy['source_canonical_sha256'], maps=[])))
        self.report = copy.deepcopy(f.provenance)
        self.report['external_installation_requirement'] = 'Halo MCC only'
        self.report['unrelated_presentation'] = dict(family='retain', receipt='unchanged')
        self.report['native_shader_library']['maps'] = dict(lockout=sha(encoded(self.libraries['lockout'])))
        self.write(self.cache / 'tag-conversion.json', encoded(f.conversion))
        self.write_report()
        policy = patch.object(bundle, 'closure_policy', return_value=self.policy)
        policy.start(); self.addCleanup(policy.stop)
        # Supply explicit synthetic policy authority to the REAL planner and
        # verifier. No TABLE_SHA or ownership implementation is replaced.
        verify, prepare = publication.verify_owner_index, publication.prepare_owner_index_refresh
        for name, original in (('verify_owner_index', verify), ('prepare_owner_index_refresh', prepare)):
            def with_policy(*args, _original=original, **kwargs):
                kwargs.setdefault('policy_root', self.policies)
                return _original(*args, **kwargs)
            patched = patch.object(publication, name, side_effect=with_policy)
            patched.start(); self.addCleanup(patched.stop)

    def write(self, path, raw):
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(raw)

    def write_report(self):
        self.write(self.cache / 'provenance.json', encoded(self.report))

    def library(self, directory, name, marker):
        records = []
        for group, raw in (('shad', ('synthetic native program ' + marker).encode()), ('matg', bytes(464))):
            filename = sha(raw) + '.bin'
            self.write(directory / 'payloads' / filename, raw)
            records.append(dict(group=group, name='shaders\\' + marker, payload=filename,
                bytes=len(raw), fnv64=preserved.fnv(raw), pointers=[], references=[], strings=[]))
        library = dict(schema=owners.LIBRARY_SCHEMA, map=name, source_sha256='a' * 64,
            shader_tags=records[:1], rasterizer=records[1])
        self.write(directory / 'library.json', encoded(library))
        self.write(directory / 'function-tables.bin', self.tables)
        return library

    def repair(self):
        return onboarding.repair_cache_shaders(self.root, self.cache, self.report, progress=lambda _: None)

    def no_temporary(self):
        self.assertFalse(list(self.cache.glob('.native-shaders-*')))
        self.assertFalse(list(self.cache.glob('.shader-provenance-*')))

    def indexed_custom(self):
        # An older index already authenticates its separate common dependency,
        # while Cairo is not yet listed as a whole stock bank in the catalog.
        shutil.copytree(self.source / CAIRO, self.cache / 'native-shaders' / CAIRO)
        self.write(self.cache / 'native-shaders/function-tables.bin', self.tables)
        receipts = []
        for owner, bank in zip(self.custom, self.custom_banks):
            receipts.append(encoded(dict(map=owner,
                input_bytes=self.fixture.verified[owner]['bytes'], input_sha256=self.fixture.verified[owner]['sha256'],
                source_sha256=self.fixture.provenance['source_files'][owner.removeprefix('halo2/')]['sha256'],
                cache_role='scenario', shared_directory='halo2/Mods/Shared Assets', native_mode='owned-bank',
                **{key: bank[key] for key in owners.BANK_FIELDS}, bank_owner_map=owner,
                producer='explicit fixture author', profile='fixture-custom-owner-v1')))
        _, self.report = publication.repair_owner_index(self.cache, self.report,
            policy_root=self.policies, custom_receipts=receipts)

    def test_legacy_repair_commits_real_final_owner_index_and_preserves_programs(self):
        before = snapshot(self.cache)[0]
        with patch.object(publication.os, 'link', side_effect=OSError(errno.EOPNOTSUPP, 'no hardlinks')) as link:
            _, updated = self.repair()
        link.assert_not_called()
        self.assertIsNotNone(publication.verify_owner_index(self.cache, updated))
        index = owners.decode_index((self.cache / publication.INDEX).read_bytes())
        rows = {row['map']: row for row in index['maps']}
        self.assertEqual(rows[self.stock]['input_sha256'], sha(self.fixture.snapshots[self.stock]))
        self.assertNotEqual(rows[self.stock]['source_sha256'], rows[self.stock]['input_sha256'])
        self.assertEqual({rows[owner]['native_mode'] for owner in self.custom}, {'none'})
        self.assertNotIn('halo2/h2_maps_win64_dx11/' + CAIRO + '.map', rows)
        self.assertTrue(any(row['kind'] == 'authenticated-common-templates' for row in index['dependencies']))
        for name, raw in before.items():
            if name not in ('native-shaders/catalog.json', 'provenance.json'):
                self.assertEqual((self.cache / name).read_bytes(), raw)
        self.assertFalse(updated['runtime_active']); self.assertFalse(updated['full_tu15_parity'])
        self.assertEqual(updated['unrelated_presentation'], self.report['unrelated_presentation'])
        self.no_temporary()

    def test_each_publication_failure_restores_the_complete_legacy_preimage(self):
        before = snapshot(self.cache)
        absent, replace = publication._publish_absent, onboarding.os.replace
        for relative in ('native-shaders/' + CAIRO, 'native-shaders/function-tables.bin',
                         'native-shaders/catalog.json', publication.INDEX, 'provenance.json'):
            with self.subTest(relative=relative):
                failed = False
                def publish(source, target, *, operation):
                    nonlocal failed
                    if Path(target) == self.cache / relative and not failed:
                        failed = True
                        raise OSError('injected transaction boundary failure')
                    return operation(source, target)
                with patch.object(publication, '_publish_absent', side_effect=lambda s, t: publish(s, t, operation=absent)), \
                     patch.object(onboarding.os, 'replace', side_effect=lambda s, t: publish(s, t, operation=replace)):
                    with self.assertRaisesRegex(OSError, 'injected transaction boundary failure'):
                        self.repair()
                self.assertTrue(failed)
                self.assertEqual(snapshot(self.cache), before)
                self.no_temporary()

    def test_absent_native_install_is_rolled_back_with_failed_owner_metadata(self):
        shutil.rmtree(self.cache / 'native-shaders')
        self.report['native_shader_library']['maps'] = {}
        self.write_report()
        before = snapshot(self.cache)
        replace = onboarding.os.replace
        def fail(source, target):
            if Path(target) == self.cache / 'provenance.json':
                raise OSError('failed final provenance')
            return replace(source, target)
        with patch.object(onboarding.os, 'replace', side_effect=fail):
            with self.assertRaisesRegex(OSError, 'failed final provenance'):
                self.repair()
        self.assertEqual(snapshot(self.cache), before)
        _, updated = self.repair()
        self.assertIsNotNone(publication.verify_owner_index(self.cache, updated))
        self.no_temporary()

    def test_authenticated_custom_bindings_survive_catalog_and_stock_proof_refresh(self):
        self.indexed_custom()
        old_index = owners.decode_index((self.cache / publication.INDEX).read_bytes())
        before = snapshot(self.cache)[0]
        _, updated = self.repair()
        self.assertIsNotNone(publication.verify_owner_index(self.cache, updated))
        new_index = owners.decode_index((self.cache / publication.INDEX).read_bytes())
        for owner in self.custom:
            self.assertEqual(next(row for row in old_index['maps'] if row['map'] == owner),
                             next(row for row in new_index['maps'] if row['map'] == owner))
        for name, raw in before.items():
            if name not in ('native-shaders/catalog.json', publication.INDEX, 'provenance.json'):
                self.assertEqual((self.cache / name).read_bytes(), raw)
        rows = json.loads((self.cache / 'native-shaders/catalog.json').read_bytes())['maps']
        self.assertEqual(rows[0], self.catalog['maps'][0])
        self.assertEqual(sum(row['map'] == CAIRO for row in rows), 1)
        self.no_temporary()

    def test_corrupt_declared_pointer_fails_before_any_closure_copy(self):
        self.report['native_shader_library']['owner_bindings'] = dict(path='../outside', schema=owners.SCHEMA,
            bytes=0, sha256='0' * 64)
        self.write_report()
        before = snapshot(self.cache)
        with patch.object(preserved, 'copy_selected', side_effect=AssertionError('copy before index authentication')):
            with self.assertRaisesRegex(FormatError, 'corrupt declared owner pointer'):
                self.repair()
        self.assertEqual(snapshot(self.cache), before)

    def test_existing_index_and_custom_bindings_are_restored_on_metadata_failure(self):
        self.indexed_custom()
        before = snapshot(self.cache)
        replace = onboarding.os.replace
        for relative in (publication.INDEX, 'provenance.json'):
            with self.subTest(relative=relative):
                failed = False
                def publish(source, target):
                    nonlocal failed
                    if Path(target) == self.cache / relative and not failed:
                        failed = True
                        raise OSError('indexed metadata commit failed')
                    return replace(source, target)
                with patch.object(onboarding.os, 'replace', side_effect=publish):
                    with self.assertRaisesRegex(OSError, 'indexed metadata commit failed'):
                        self.repair()
                self.assertTrue(failed)
                self.assertEqual(snapshot(self.cache), before)
                self.assertIsNotNone(publication.verify_owner_index(self.cache, self.report))
                self.no_temporary()

    def test_no_stock_bank_proofs_leave_legacy_index_absent(self):
        self.report['native_shader_library']['maps'] = {}
        self.write_report()
        self.write(self.cache / 'native-shaders/function-tables.bin', self.tables)
        self.write(self.cache / 'native-shaders/catalog.json', encoded(dict(schema=owners.CATALOG_SCHEMA, maps=[])))
        before = snapshot(self.cache)
        self.assertEqual(self.repair()[1], self.report)
        self.assertEqual(snapshot(self.cache), before)
        self.assertFalse((self.cache / publication.INDEX).exists())

    @unittest.skipUnless(os.name == 'posix', 'POSIX setup SIGTERM')
    def test_sigterm_after_final_replace_rolls_back_every_owned_change(self):
        before = snapshot(self.cache)
        replace = onboarding.os.replace
        stopped = False
        def publish(source, target):
            nonlocal stopped
            result = replace(source, target)
            if Path(target) == self.cache / 'provenance.json' and not stopped:
                stopped = True
                signal.raise_signal(signal.SIGTERM)
            return result
        with patch.object(onboarding.os, 'replace', side_effect=publish):
            with self.assertRaises(SystemExit) as error:
                self.repair()
        self.assertEqual(error.exception.code, 128 + signal.SIGTERM)
        self.assertEqual(snapshot(self.cache), before)
        self.no_temporary()

    def test_progress_failure_after_commit_retains_a_consistent_pair(self):
        def progress(message):
            if message.startswith('Retained native materials are ready.'):
                raise RuntimeError('post-commit callback failed')
        with self.assertRaisesRegex(RuntimeError, 'post-commit callback failed'):
            onboarding.repair_cache_shaders(self.root, self.cache, self.report, progress=progress)
        updated = publication._json((self.cache / 'provenance.json').read_bytes())
        self.assertIsNotNone(publication.verify_owner_index(self.cache, updated))
        self.no_temporary()

    def test_changed_final_map_is_rejected_and_external_bytes_are_retained(self):
        before = snapshot(self.cache)[0]
        prepare = publication.prepare_owner_index_refresh
        def changed(*args, **kwargs):
            (self.cache / self.stock).write_bytes(b'changed externally after closure staging')
            return prepare(*args, **kwargs)
        with patch.object(publication, 'prepare_owner_index_refresh', side_effect=changed):
            with self.assertRaisesRegex(FormatError, 'wrong final encoded'):
                self.repair()
        after = snapshot(self.cache)[0]
        self.assertEqual(set(after), set(before))
        for name, raw in before.items():
            if name != self.stock:
                self.assertEqual(after[name], raw)
        self.assertEqual(after[self.stock], b'changed externally after closure staging')
        self.no_temporary()

    def test_failed_rollback_keeps_the_only_metadata_recovery_copy(self):
        original = (self.cache / 'native-shaders/catalog.json').read_bytes()
        absent, replace = publication._publish_absent, onboarding.os.replace
        def fail_index(source, target):
            if Path(target) == self.cache / publication.INDEX:
                raise OSError('index commit failed')
            return absent(source, target)
        def fail_restore(source, target):
            if Path(source).suffix == '.previous' and Path(target) == self.cache / 'native-shaders/catalog.json':
                raise OSError('catalog restoration failed')
            return replace(source, target)
        with patch.object(publication, '_publish_absent', side_effect=fail_index), \
             patch.object(onboarding.os, 'replace', side_effect=fail_restore):
            with self.assertRaisesRegex(FormatError, 'rollback incomplete; recovery files retained'):
                self.repair()
        recovery = list(self.cache.glob('.native-shaders-*'))
        self.assertEqual(len(recovery), 1)
        self.assertTrue(any(path.read_bytes() == original for path in recovery[0].glob('*.previous')))
        # The owned TemporaryDirectory still cleans this deliberately retained
        # recovery fixture; a real failed rollback reports its retained path.

    def test_final_map_change_after_derivation_rolls_back_metadata_pair(self):
        before = snapshot(self.cache)[0]
        absent = publication._publish_absent
        def change_before_index(source, target):
            if Path(target) == self.cache / publication.INDEX:
                (self.cache / self.stock).write_bytes(b'changed external final input during commit')
            return absent(source, target)
        with patch.object(publication, '_publish_absent', side_effect=change_before_index):
            with self.assertRaisesRegex(FormatError, 'input changed during metadata publication'):
                self.repair()
        after = snapshot(self.cache)[0]
        self.assertEqual(set(after), set(before))
        for name, raw in before.items():
            if name != self.stock:
                self.assertEqual(after[name], raw)
        self.assertEqual(after[self.stock], b'changed external final input during commit')
        self.no_temporary()


if __name__ == '__main__':
    unittest.main()
