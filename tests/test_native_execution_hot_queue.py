#!/usr/bin/env python3
"""Validate source-qualified aggregation, variant grouping and frozen identity pins."""
import json
import os
from pathlib import Path
import sys
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
from aggregate_native_execution import aggregate,digest,metadata

FIELDS=['unit','symbol','image','literal_sites','dynamic_forwarders','entries','body_entries','literal_statements','returns','unwinds','returns_without_body','unwinds_without_body','active']


class Queue(unittest.TestCase):
    def setUp(self):
        self.owner=tempfile.TemporaryDirectory(dir=os.environ.get('HALO2_TEST_SCRATCH'));self.addCleanup(self.owner.cleanup)
        self.root=Path(self.owner.name);self.directory=self.root/'evidence';self.directory.mkdir()
        self.a=self.root/'src/a/shared_generated.cpp';self.b=self.root/'src/b/shared_generated.cpp'
        self.a.parent.mkdir(parents=True);self.b.parent.mkdir(parents=True)
        self.a.write_text('''static void run_100(State &s) {
if(s.services.instruction)s.services.instruction(world_observation_pc(0x100u,0x200u,s));run_300(s);
}
static void run_300(State &s) {
if(s.services.instruction)s.services.instruction(0x400u);
}
''')
        self.b.write_text('''static void run_100(State &s) {
if(s.services.instruction)s.services.instruction(0x300u);
s.required(0x300u,s.gp[0],false);
}
static void dispatch(State &s,std::uint32_t target,std::uint32_t site,bool tail){switch(target){case 0x300u:run_100(s);return;default:s.required(site,target,tail);return;}}
''')
        self.sources=[str(self.a.relative_to(self.root)),str(self.b.relative_to(self.root))]
        pins={self.sources[0]:digest(self.a),self.sources[1]:digest(self.b)}
        self.save('source-hashes.json',pins)
        self.units=[[self.sources[0],pins[self.sources[0]],'1'*64,'tu15'],
                    [self.sources[0],pins[self.sources[0]],'2'*64,'tu15'],
                    [self.sources[1],pins[self.sources[1]],'3'*64,'tu15']]
        self.save('cmake/evidence/native-execution-coverage.json',dict(canonical_source_drift=[],aliases=[dict(source=u[0],canonical_sha256=u[1],input_sha256=u[2])for u in self.units]))
        rows=[[[0,'100',1,1,0,2,1,10,2,0,1,0,0],[0,'300',1,1,0,1,0,0,1,0,1,0,0]],
              [[1,'100',1,1,0,1,1,20,1,0,0,0,0],[2,'100',1,1,0,1,1,100,1,0,0,0,0]]]
        runs=[]
        for index,values in enumerate(rows):
            p=self.save(f'case{index}.json',dict(schema='native-execution-accounting-v1',units=self.units,image_names=['map-pack11','tu15','unknown'],row_fields=FIELDS,rows=values,omitted_records=0,registration_failures=0,active_scopes=0))
            runs.append(dict(label=f'case{index}',capture=dict(valid=True),runtime_report=str(p),runtime_report_sha256=digest(p)))
        self.cohort=self.save('report.json',dict(passed=True,execution_accounting_enabled=True,bounded_runtime_accounting_recorded=True,
            source_inventory_sha256=digest(self.directory/'source-hashes.json'),accounting_runs=runs,runtime_sha256='4'*64))

    def save(self,name,value):
        p=self.directory/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(json.dumps(value));return p

    def test_alias_variants_combine_without_merging_different_source_owners(self):
        report=aggregate(self.cohort,self.root)
        self.assertEqual(report['totals']['literal_statements'],130)
        self.assertEqual(report['source_qualified_functions'],3)
        self.assertEqual(report['input_qualified_function_variants'],4)
        self.assertEqual(report['fallback_functions'],2)
        self.assertEqual(report['no_literal_body_functions'],1)
        self.assertEqual(report['mixed_body_and_no_body_functions'],1)
        self.assertEqual([r['source_audit']['actual_entry']for r in report['hot_queue']],['300','200'])
        self.assertEqual(report['hot_queue'][1]['canonical_symbol'],'100')
        self.assertEqual(report['hot_queue'][1]['per_run_literal_statements'],[10,20])
        self.assertEqual(report['hot_queue'][1]['source_audit']['static_direct_run_symbols'],['300'])
        self.assertTrue(report['hot_queue'][0]['source_audit']['unresolved_dispatch'])
        self.assertFalse(report['native_closure_proved']);self.assertFalse(report['performance_qualified'])

    def test_later_source_edits_are_marked_without_rewriting_frozen_counts(self):
        self.a.write_text('// later source edit\n'+self.a.read_text())
        report=aggregate(self.cohort,self.root)
        self.assertEqual(report['current_source_drift'],[self.sources[0]])
        self.assertIn('current-source audit',report['hot_queue'][1]['source_audit']['identity_evidence'])
        self.assertEqual(report['totals']['literal_statements'],130)

    def test_report_or_source_identity_tampering_is_rejected(self):
        p=self.directory/'case0.json';p.write_text(p.read_text()+'\n')
        with self.assertRaisesRegex(ValueError,'size/hash'):aggregate(self.cohort,self.root)

    def test_partial_exit_or_omitted_rows_are_rejected(self):
        p=self.directory/'case0.json';raw=json.loads(p.read_text());raw['omitted_records']=1;p.write_text(json.dumps(raw))
        cohort=json.loads(self.cohort.read_text());cohort['accounting_runs'][0]['runtime_report_sha256']=digest(p);self.cohort.write_text(json.dumps(cohort))
        with self.assertRaisesRegex(ValueError,'omitted'):aggregate(self.cohort,self.root)
        raw['omitted_records']=0;raw['rows'][0][5]=3;raw['rows'][0][-1]=1;raw['active_scopes']=1
        p.write_text(json.dumps(raw));cohort['accounting_runs'][0]['runtime_report_sha256']=digest(p);self.cohort.write_text(json.dumps(cohort))
        with self.assertRaisesRegex(ValueError,'active function scopes'):aggregate(self.cohort,self.root)

    def test_legacy_callback_namespace_does_not_infer_actual_tu15_identity(self):
        relative='src/gameplay/weapons/unit_pickup_runtime.cpp';path=self.root/relative;path.parent.mkdir(parents=True)
        path.write_text('static void run_cd1a0(State &s) {s.instruction(0xcd1a0u);}\n')
        row=metadata(self.root,relative,{'cd1a0'},'tu15',digest(path))['cd1a0']
        self.assertTrue(row['unit_matches_frozen']);self.assertIsNone(row['actual_entry'])
        self.assertEqual(row['canonical_callback_entry'],'cd1a0')
        self.assertIn('remapper not authenticated',row['identity_evidence'])

    def test_internal_dispatch_resolves_canonical_callbacks_and_rejects_extra_semantics(self):
        path=self.a;source='''std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage image){if(image==SourcePhysicsContactImage::MapPack11)return p;switch(p){case 0x100u:return 0x200u;default:throw std::runtime_error("unbound");}}
static void dispatch(State &s,std::uint32_t target,std::uint32_t site,bool tail){if(target==selected(0x100u,s.image)){run_100(s);return;}s.required(site,target,tail);}
static void run_100(State &s){s.instruction(0x100u);}
''';path.write_text(source);relative=str(path.relative_to(self.root))
        row=metadata(self.root,relative,{'100'},'tu15',digest(path))['100']
        self.assertEqual(row['actual_entry'],'200');self.assertEqual(row['canonical_callback_entry'],'100')
        self.assertIn('dispatcher',row['entry_identity_evidence']['basis'])
        path.write_text(source.replace('switch(p)','p^=1u;switch(p)'))
        self.assertIsNone(metadata(self.root,relative,{'100'},'tu15',digest(path))['100']['actual_entry'])
        path.write_text(source.replace('{if(target==','{target^=1u;if(target=='))
        self.assertIsNone(metadata(self.root,relative,{'100'},'tu15',digest(path))['100']['actual_entry'])

    def test_bare_callback_without_actual_owner_dispatcher_stays_unresolved(self):
        self.b.write_text('static void run_100(State &s){s.instruction(0x300u);}\n')
        row=metadata(self.root,str(self.b.relative_to(self.root)),{'100'},'tu15',digest(self.b))['100']
        self.assertEqual(row['canonical_callback_entry'],'300');self.assertIsNone(row['actual_entry'])
        self.assertFalse(row['entry_identity_qualified'])


if __name__=='__main__':unittest.main()
