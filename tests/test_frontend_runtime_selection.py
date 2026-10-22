"""Independent local frontend owner admission; no game or renderer execution."""
import json
import os
from pathlib import Path
import sys
import unittest
from unittest.mock import patch

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import frontend_runtime_selection as selection
import native_gameplay_test as admission
import test_frontend_capability as fixtures


class FrontendRuntimeSelectionTests(unittest.TestCase):
    def setUp(self):
        fixture=fixtures.FrontendCapabilityTests()
        fixture.setUp();self.addCleanup(fixture.doCleanups)
        self.fixture=fixture
        self.root,self.cache,self.frontend=fixture.root,fixture.cache,fixture.runtime
        self.manifest=self.root/selection.SELECTION
        self.gameplay=self.root/'camera-v3/halo2_native'
        self.gameplay.parent.mkdir()
        self.gameplay.write_bytes(b'Independent gameplay owner\0'+os.fsencode(fixture.shaders)+b'\0')
        self.gameplay.chmod(0o700)
        native=dict(schema=admission.SCHEMA,capability=admission.CAPABILITY,
            runtime=dict(path=str(self.gameplay),sha256=fixture.digest(self.gameplay)),
            shaders=fixture.report['shaders'],map=dict(path=admission.MAP,
                sha256=fixture.digest(self.cache/admission.FRONTEND_MAP)),
            validation=dict(passed=True,scope=admission.SCOPE,report_sha256='2'*64))
        (self.cache/admission.MAP).write_bytes((self.cache/admission.FRONTEND_MAP).read_bytes())
        self.gameplay.with_name(self.gameplay.name+'.native-test.json').write_text(json.dumps(native))
        self.record=dict(schema=selection.SCHEMA,runtime=self.frontend.relative_to(self.root).as_posix(),
            runtime_sha256=fixture.digest(self.frontend),capability_sha256=fixture.digest(fixture.sidecar))

    def publish(self):self.manifest.write_text(json.dumps(self.record))
    def selected(self):return selection.selected_frontend_runtime(self.root,self.gameplay,self.cache)

    def test_absent_selection_preserves_single_runtime_frontend_admission(self):
        self.assertEqual(selection.selected_frontend_runtime(self.root,self.frontend,self.cache),self.frontend)
        self.assertIsNone(self.selected())
        self.assertTrue(admission.native_gameplay_test_available(self.root,self.gameplay,self.cache))

    def test_exact_separate_frontend_preserves_gameplay_owner_and_capability(self):
        self.publish()
        before={p:p.read_bytes() for p in self.root.rglob('*') if p.is_file()}
        self.assertEqual(self.selected(),self.frontend)
        self.assertTrue(admission.native_gameplay_test_available(self.root,self.gameplay,self.cache))
        self.assertFalse(admission.native_gameplay_test_available(self.root,self.frontend,self.cache))
        self.assertEqual(before,{p:p.read_bytes() for p in self.root.rglob('*') if p.is_file()})

    def test_missing_stale_runtime_or_sidecar_does_not_fall_back(self):
        for path in (self.frontend,self.fixture.sidecar):
            with self.subTest(path=path):
                self.publish();old=path.read_bytes()
                path.write_bytes(old+b'changed')
                self.assertIsNone(self.selected())
                self.assertTrue(admission.native_gameplay_test_available(self.root,self.gameplay,self.cache))
                path.write_bytes(old)
                path.rename(path.with_suffix('.held'))
                self.assertIsNone(self.selected())
                path.with_suffix('.held').rename(path)

    def test_registry_schema_hashes_and_unsafe_paths_are_rejected(self):
        cases=[dict(schema='unqualified'),dict(runtime_sha256='0'*64),dict(capability_sha256='0'*64)]
        cases.extend(dict(runtime=value) for value in ('',str(self.frontend),'../outside',
            'build/../build/halo2_native','build//halo2_native','build\\halo2_native','./build/halo2_native'))
        for changed in cases:
            with self.subTest(changed=changed):
                self.manifest.write_text(json.dumps(dict(self.record,**changed)))
                self.assertIsNone(self.selected())
        self.manifest.write_text('{')
        self.assertIsNone(self.selected())
        self.manifest.write_bytes(b' '*65537)
        self.assertIsNone(self.selected())

    def test_symlink_selection_runtime_parent_and_capability_are_rejected(self):
        self.publish();saved=self.root/'record.json';self.manifest.rename(saved)
        self.manifest.symlink_to(saved);self.assertIsNone(self.selected());self.manifest.unlink()
        for path in (self.frontend,self.fixture.sidecar):
            saved=path.with_suffix('.held');path.rename(saved);path.symlink_to(saved)
            self.publish();self.assertIsNone(self.selected());path.unlink();saved.rename(path)
        alias=self.root/'frontend-link';alias.symlink_to(self.frontend.parent,target_is_directory=True)
        self.record['runtime']='frontend-link/halo2_native';self.publish();self.assertIsNone(self.selected())

    def test_frontend_resource_changes_expire_only_frontend_admission(self):
        self.publish()
        for name in ('halo2/h2_fonts/nonactive-language/fixture.font','halo2/h2_fonts/fixture.font'):
            path=self.cache/name;before=path.read_bytes();path.write_bytes(before+b'changed')
            with self.subTest(name=name):
                self.assertIsNone(self.selected())
                self.assertTrue(admission.native_gameplay_test_available(self.root,self.gameplay,self.cache))
            path.write_bytes(before)

    def test_explicit_invalid_choice_does_not_reuse_valid_gameplay_frontend(self):
        self.manifest.write_text('{')
        self.assertIsNone(selection.selected_frontend_runtime(self.root,self.frontend,self.cache))
        self.manifest.unlink()
        self.assertEqual(selection.selected_frontend_runtime(self.root,self.frontend,self.cache),self.frontend)

    def test_registry_change_during_underlying_admission_is_rejected(self):
        self.publish()
        original=selection._check
        def change_after_check(*args,**kwargs):
            original(*args,**kwargs)
            self.manifest.write_text(json.dumps(dict(self.record,runtime_sha256='0'*64)))
        with patch.object(selection,'_check',side_effect=change_after_check):
            self.assertIsNone(self.selected())


if __name__=='__main__':unittest.main()
