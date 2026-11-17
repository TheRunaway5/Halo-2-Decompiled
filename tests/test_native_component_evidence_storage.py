#!/usr/bin/env python3
"""Real receipt, budget, rollback and logical-reader storage regressions."""
import hashlib
import json
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
import native_component_evidence_storage as storage
import native_metadata as metadata

def sha(data):return hashlib.sha256(data).hexdigest()

class ComponentEvidenceStorage(unittest.TestCase):
    def setUp(self):
        self.owner=tempfile.TemporaryDirectory(dir=os.environ.get('HALO2_TEST_SCRATCH'));self.addCleanup(self.owner.cleanup)
        self.base=Path(self.owner.name);self.output=self.base/'output';self.scratch=self.base/'scratch';self.scratch.mkdir()
        self.addCleanup(patch.stopall);patch.dict(os.environ,HALO2_TEST_SCRATCH=str(self.scratch)).start()
        (self.output/'cmake/evidence').mkdir(parents=True);(self.output/'report.json').write_text(json.dumps(dict(passed=False,status='components',owned_scratch=str(self.scratch)))+'\n')

    def fixture(self,files):
        rows=[]
        for name,data in files.items():
            path=self.output/'cmake/evidence'/name;path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(data)
            rows.append(dict(path=name,status='retained',bytes=len(data),sha256=sha(data),retained_path=str(path)))
        rows.append(dict(path='not-produced.json',status='not produced'))
        path=self.output/'cmake/report.json';path.write_text(json.dumps(dict(passed=True,status='passed',retained_small_reports=rows),indent=2)+'\n')
        return path.read_bytes()

    def snapshot(self):return {str(p.relative_to(self.output)):p.read_bytes()for p in self.output.rglob('*')if p.is_file()}

    def test_exact_original_receipts_and_logical_reads_are_preserved(self):
        files={'nested/report.json':b'{ "same": "'+b'a'*4000+b'" }\n','small.json':b'{}\n'}
        receipt=self.fixture(files);cohort=(self.output/'report.json').read_bytes()
        result=storage.compact_component_evidence(self.output)
        self.assertEqual((result['json_reports'],result['compressed_reports']),(2,1));self.assertFalse(result['idempotent'])
        self.assertEqual((self.output/'cmake/report.json').read_bytes(),receipt);self.assertEqual((self.output/'report.json').read_bytes(),cohort)
        for name,data in files.items():self.assertEqual(metadata.read_bytes(self.output/'cmake/evidence'/name),data)
        self.assertFalse((self.output/'cmake/evidence/nested/report.json').exists());self.assertTrue((self.output/'cmake/evidence/small.json').is_file())
        manifest=json.loads((self.output/storage.MANIFEST).read_text());self.assertEqual(manifest['runner_report_sha256'],sha(receipt))
        self.assertEqual(result['saved_bytes'],sum(len(b)for b in files.values())-result['stored_component_bytes'])
        before=self.snapshot();second=storage.compact_component_evidence(self.output);self.assertTrue(second['idempotent']);self.assertEqual(self.snapshot(),before)
        self.assertEqual(list(self.scratch.iterdir()),[])

    def test_batch_plans_an_initially_over_budget_cohort_before_any_commit(self):
        files={f'case{n}.json':b'{"value":"'+b'a'*950_000+b'"}\n'for n in range(9)}
        self.fixture(files)
        with self.assertRaises(metadata.MetadataBudgetError):metadata.check_budget(self.output)
        result=storage.compact_component_evidence(self.output)
        self.assertEqual(result['compressed_reports'],9);self.assertLess(result['retained_metadata_bytes'],30_000)
        for name,data in files.items():self.assertEqual(metadata.read_bytes(self.output/'cmake/evidence'/name),data)
        self.assertEqual(list(self.scratch.iterdir()),[])

    def test_completed_historical_cohort_is_refused_unchanged(self):
        self.fixture({'report.json':b'{"value":"'+b'a'*1000+b'"}\n'})
        for state in ({'passed':True,'status':'passed'},{'passed':False,'status':'passed'},
                      {'passed':False,'status':'failed'},{'passed':False,'status':'cancelled'},
                      {'passed':False,'status':'engine-build'},{'status':'components'}):
            (self.output/'report.json').write_text(json.dumps(state));before=self.snapshot()
            with self.assertRaisesRegex(ValueError,'Historical/terminal'):storage.compact_component_evidence(self.output)
            self.assertEqual(self.snapshot(),before)

    def test_original_receipt_mutation_missing_extra_and_unsafe_paths_refuse(self):
        self.fixture({'report.json':b'{"value":"'+b'a'*1000+b'"}\n'})
        receipt_path=self.output/'cmake/report.json';original=receipt_path.read_bytes();evidence=self.output/'cmake/evidence/report.json';payload=evidence.read_bytes()
        for mutation in ('hash','bytes','missing','extra','unsafe'):
            receipt=json.loads(original)
            if mutation=='hash':receipt['retained_small_reports'][0]['sha256']='0'*64
            elif mutation=='bytes':receipt['retained_small_reports'][0]['bytes']+=1
            elif mutation=='missing':evidence.unlink()
            elif mutation=='extra':(evidence.parent/'extra.json').write_text('{}')
            else:receipt['retained_small_reports'][0]['path']='../report.json'
            receipt_path.write_text(json.dumps(receipt));before=self.snapshot()
            with self.assertRaises(ValueError):storage.compact_component_evidence(self.output)
            self.assertEqual(self.snapshot(),before)
            receipt_path.write_bytes(original);evidence.write_bytes(payload);(evidence.parent/'extra.json').unlink(missing_ok=True)

    def test_staged_corruption_is_rejected_before_output_changes(self):
        self.fixture({'report.json':b'{"value":"'+b'a'*1000+b'"}\n'});before=self.snapshot()
        with patch.object(storage.gzip,'compress',return_value=b'bad gzip'):
            with self.assertRaises((ValueError,OSError,EOFError)):storage.compact_component_evidence(self.output)
        self.assertEqual(self.snapshot(),before);self.assertEqual(list(self.scratch.iterdir()),[])

    def test_other_metadata_cannot_exceed_the_unchanged_budget(self):
        self.fixture({'report.json':b'{"value":"'+b'a'*1000+b'"}\n'})
        for n in range(8):(self.output/f'other{n}.json').write_bytes(b'0'*metadata.FILE_LIMIT)
        before=self.snapshot()
        with self.assertRaisesRegex(metadata.MetadataBudgetError,'8MiB'):storage.compact_component_evidence(self.output)
        self.assertEqual(self.snapshot(),before);self.assertEqual(list(self.scratch.iterdir()),[])

    def test_commit_failure_rolls_back_every_original(self):
        self.fixture({f'{n}.json':b'{"value":"'+b'a'*1000+b'"}\n'for n in range(2)});before=self.snapshot();publish=storage._publish_file;calls=[]
        def fail_later(source,target,created):
            calls.append(target)
            if len(calls)==2:raise OSError('declared commit failure')
            return publish(source,target,created)
        with patch.object(storage,'_publish_file',side_effect=fail_later):
            with self.assertRaisesRegex(OSError,'declared commit'):storage.compact_component_evidence(self.output)
        self.assertEqual(self.snapshot(),before);self.assertEqual(list(self.scratch.iterdir()),[])

    def test_existing_manifest_detects_changed_receipt_and_encoded_bytes(self):
        self.fixture({'report.json':b'{"value":"'+b'a'*1000+b'"}\n'});storage.compact_component_evidence(self.output)
        path=self.output/'cmake/report.json';original=path.read_bytes();path.write_bytes(original+b' ')
        with self.assertRaisesRegex(ValueError,'runner receipt'):storage.compact_component_evidence(self.output)
        path.write_bytes(original);encoded=self.output/'cmake/evidence/report.json.gz';data=bytearray(encoded.read_bytes());data[-5]^=1;encoded.write_bytes(data)
        with self.assertRaisesRegex(ValueError,'encoded size/hash'):storage.compact_component_evidence(self.output)

    def test_source_change_during_staging_is_not_overwritten(self):
        self.fixture({'report.json':b'{"value":"'+b'a'*1000+b'"}\n'});path=self.output/'cmake/evidence/report.json';compress=storage.gzip.compress
        def external_edit(*args,**kwargs):
            result=compress(*args,**kwargs);path.write_bytes(b'{"external":"preserve"}\n');return result
        with patch.object(storage.gzip,'compress',side_effect=external_edit):
            with self.assertRaisesRegex(ValueError,'changed during'):storage.compact_component_evidence(self.output)
        self.assertEqual(path.read_bytes(),b'{"external":"preserve"}\n');self.assertFalse((self.output/storage.MANIFEST).exists());self.assertEqual(list(self.scratch.iterdir()),[])

    def test_manifest_publish_failure_rolls_back_every_original(self):
        self.fixture({'report.json':b'{"value":"'+b'a'*1000+b'"}\n'});before=self.snapshot();publish=storage._publish_file
        def fail_manifest(source,target,created):
            if target==self.output/storage.MANIFEST:raise OSError('declared manifest failure')
            return publish(source,target,created)
        with patch.object(storage,'_publish_file',side_effect=fail_manifest):
            with self.assertRaisesRegex(OSError,'manifest failure'):storage.compact_component_evidence(self.output)
        self.assertEqual(self.snapshot(),before);self.assertEqual(list(self.scratch.iterdir()),[])

    def test_rollback_preserves_recreated_original_and_encoded_replacement(self):
        original=b'{"value":"'+b'a'*1000+b'"}\n'
        self.fixture({'0.json':original,'1.json':original});publish=storage._publish_file;calls=[]
        external_raw=b'{"external":"new owner"}\n';external_encoded=b'{"external":"replacement target"}\n'
        raw=self.output/'cmake/evidence/0.json';encoded=Path(str(raw)+'.gz')
        def fail_later(source,target,created):
            calls.append(target)
            if len(calls)==2:
                raw.write_bytes(external_raw);encoded.unlink();encoded.write_bytes(external_encoded)
                raise OSError('declared concurrent owner')
            return publish(source,target,created)
        with patch.object(storage,'_publish_file',side_effect=fail_later):
            with self.assertRaisesRegex(RuntimeError,'external files preserved'):storage.compact_component_evidence(self.output)
        self.assertEqual(raw.read_bytes(),external_raw);self.assertEqual(encoded.read_bytes(),external_encoded)
        recoveries=list((self.output/'cmake').glob('component-evidence-recovery-*/recovery.json'));self.assertEqual(len(recoveries),1)
        recovery=json.loads(recoveries[0].read_text());self.assertTrue(recovery['external_files_preserved']);self.assertEqual(len(recovery['originals']),1)
        self.assertTrue(recovery['failed_output_within_budget']);self.assertEqual(recovery['individual_file_limit_bytes'],metadata.FILE_LIMIT)
        saved=self.output/recovery['originals'][0]['stored_path'];self.assertEqual(metadata.read_bytes(saved),original)
        self.assertLessEqual(saved.stat().st_size,metadata.FILE_LIMIT);self.assertEqual(metadata.digest(saved),recovery['originals'][0]['raw_sha256'])
        self.assertEqual((self.output/'cmake/evidence/1.json').read_bytes(),original);self.assertEqual(list(self.scratch.iterdir()),[])

    def test_rollback_preserves_in_place_changed_created_target(self):
        original=b'{"value":"'+b'a'*1000+b'"}\n';self.fixture({'0.json':original,'1.json':original});publish=storage._publish_file;calls=[]
        raw=self.output/'cmake/evidence/0.json';encoded=Path(str(raw)+'.gz');external=b'external same inode contents'
        def fail_later(source,target,created):
            calls.append(target)
            if len(calls)==2:encoded.write_bytes(external);raise OSError('declared in-place change')
            return publish(source,target,created)
        with patch.object(storage,'_publish_file',side_effect=fail_later):
            with self.assertRaisesRegex(RuntimeError,'external files preserved'):storage.compact_component_evidence(self.output)
        self.assertEqual(raw.read_bytes(),original);self.assertEqual(encoded.read_bytes(),external)
        self.assertEqual(list(self.scratch.iterdir()),[])

    def test_unowned_scratch_and_evidence_symlinks_are_refused(self):
        self.fixture({'report.json':b'{}\n'})
        with patch.dict(os.environ,{},clear=True):
            with self.assertRaisesRegex(RuntimeError,'HALO2_TEST_SCRATCH'):storage.compact_component_evidence(self.output)
        report=self.output/'report.json';original=report.read_bytes();changed=json.loads(original);changed['owned_scratch']=str(self.base/'another-owner');report.write_text(json.dumps(changed))
        with self.assertRaisesRegex(ValueError,'current scratch owner'):storage.compact_component_evidence(self.output)
        report.write_bytes(original)
        link=self.output/'cmake/evidence/outside.json';link.symlink_to(self.output/'report.json');before=(self.output/'report.json').read_bytes()
        with self.assertRaisesRegex(ValueError,'symlink'):storage.compact_component_evidence(self.output)
        self.assertEqual((self.output/'report.json').read_bytes(),before)

if __name__=='__main__':unittest.main()
