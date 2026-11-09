#!/usr/bin/env python3
"""Configured graph closure and canonical specialization/partition provenance."""
import json
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import audit_native_production_closure as closure
import collect_native_production_closure as collector

SOURCE = '''namespace halo2 {
namespace {
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage image){if(image==SourcePhysicsContactImage::MapPack11)return p;switch(p){case 0x1u:return 0x1u;default:throw std::runtime_error("unbound");}}
static void run_100(WorldState &);
static void run_100(WorldState &s) {
if(s.services.instruction){s.services.instruction(0x100u);} s.gp[0]=selected(0x1u,s.image);
throw std::runtime_error("Original solver fell outside authenticated CFG");
}
} // namespace
void external_entry() {}
} // namespace halo2
'''


class NativeProductionClosureTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(dir=os.environ.get('HALO2_TEST_SCRATCH'))
        self.addCleanup(self.temporary.cleanup)
        self.owner = Path(self.temporary.name)
        self.root, self.build = self.owner / 'source', self.owner / 'build'
        self.unit = self.root / 'src/physics/fixture_generated.cpp'
        self.unit.parent.mkdir(parents=True)
        self.unit.write_text(SOURCE)
        self.orphan = self.unit.with_name('orphan_generated.cpp')
        self.orphan.write_text(SOURCE)
        self.reply = self.build / '.cmake/api/v1/reply'
        self.reply.mkdir(parents=True)
        self.save('index-fixture.json', {'cmake': {'version': {'string': 'fixture'}, 'generator': {'name': 'fixture'}},
            'objects': [{'kind': kind, 'jsonFile': kind + '.json'} for kind in ('codemodel', 'cache', 'cmakeFiles', 'toolchains')]})
        self.save('codemodel.json', {'paths': {'source': str(self.root), 'build': str(self.build)},
            'configurations': [{'name': 'Release', 'targets': [
                {'id': name, 'name': target, 'jsonFile': name + '.json'} for name, target in
                [('root', 'halo2_native'), ('runtime', 'halo2_runtime'), ('orphan', 'oracle_only')]]}]})
        self.save('cache.json', {'entries': [{'name': 'HALO2_SPECIALIZE_NATIVE_OPERANDS', 'value': 'ON'}]})
        self.save('cmakeFiles.json', {'inputs': []})
        self.save('toolchains.json', {'toolchains': []})
        self.save('root.json', {'id': 'root', 'name': 'halo2_native', 'type': 'EXECUTABLE',
            'dependencies': [{'id': 'runtime'}], 'sources': []})
        self.save('runtime.json', {'id': 'runtime', 'name': 'halo2_runtime', 'type': 'STATIC_LIBRARY',
            'sources': [{'path': str(self.build / 'native-translations/fixture_generated' / name), 'isGenerated': True}
                        for name in ('router.cpp', 'body_000.cpp', 'body_001.cpp')]})
        self.save('orphan.json', {'id': 'orphan', 'name': 'oracle_only', 'type': 'STATIC_LIBRARY',
            'sources': [{'path': str(self.orphan)}]})

    def save(self, name, value):
        (self.reply / name).write_text(json.dumps(value))

    def test_target_closure_normalizes_prepared_source_aliases_and_receipts(self):
        report, content = closure.export(self.root, self.build, prepare=True)
        self.assertEqual(report['target_count'], 2)
        self.assertEqual(report['configured_generated_units'], 1)
        self.assertEqual(report['generated_units_not_in_configured_closure'], ['src/physics/orphan_generated.cpp'])
        unit = report['canonical_units'][0]
        self.assertEqual(unit['targets'], ['runtime'])
        self.assertEqual(len(unit['configured_sources']), 3)
        self.assertEqual({row['kind'] for row in unit['configured_sources']}, {'partitioned'})
        self.assertEqual({row['schema'] for row in unit['generator_receipts']},
                         {'native-operand-specialization-v1', 'native-translation-partition-v1'})
        self.assertTrue(report['scope']['configured_target_dependency_closure'])
        self.assertTrue(report['scope']['canonical_alias_bindings_complete'])
        self.assertFalse(report['scope']['game_compiled'])
        self.assertFalse(report['scope']['runtime_execution_proved'])
        self.assertLess(len(content.encode()), 1_048_576)

    def test_missing_graph_nodes_stay_explicitly_unqualified(self):
        self.save('root.json', {'id': 'root', 'name': 'halo2_native', 'type': 'EXECUTABLE',
            'dependencies': [{'id': 'unknown'}], 'sources': [{'path': str(self.unit)}]})
        report, _ = closure.export(self.root, self.build)
        self.assertEqual(report['unresolved_dependency_ids'], ['unknown'])
        self.assertFalse(report['scope']['configured_target_dependency_closure'])

    def test_all_native_specialization_groups_survive_direct_and_partitioned_aliases(self):
        closure.export(self.root, self.build, prepare=True)
        operand=self.build/'native-operands'/self.unit.relative_to(self.root/'src')
        receipt_path=operand.with_suffix('.operands.json');receipt=json.loads(receipt_path.read_text())
        receipt.update(native_allocator_actual_owner_entries=['149930'],
            native_physical_epoch_actual_owner_entries=['12f040','12f0c0','13fd10'],
            native_future_family_leaves=['0x100'], unrelated_debug_metadata='not a binding')
        receipt_path.write_text(json.dumps(receipt))
        runtime=json.loads((self.reply/'runtime.json').read_text())
        runtime['sources'].append({'path':str(operand),'isGenerated':True});self.save('runtime.json',runtime)
        report,_=closure.export(self.root,self.build)
        saved=[row for row in report['canonical_units'][0]['generator_receipts']
            if row['schema']=='native-operand-specialization-v1']
        self.assertEqual(len(saved),1)
        groups=saved[0]['native_binding_groups']
        self.assertEqual(groups['native_allocator_actual_owner_entries'],['149930'])
        self.assertEqual(groups['native_physical_epoch_actual_owner_entries'],['12f040','12f0c0','13fd10'])
        self.assertEqual(groups['native_future_family_leaves'],['0x100'])
        self.assertEqual(groups['native_leaves'],saved[0]['native_leaves'])
        self.assertNotIn('unrelated_debug_metadata',groups)
        self.assertFalse(report['scope']['native_only_complete'])
        receipt['native_physical_epoch_actual_owner_entries']=['not-an-address'];receipt_path.write_text(json.dumps(receipt))
        with self.assertRaisesRegex(ValueError,'Malformed native specialization entry group'):
            closure.export(self.root,self.build)

    def test_accounted_raw_specialized_and_partitioned_aliases_keep_each_receipt(self):
        self.save('cache.json',{'entries':[
            {'name':'HALO2_SPECIALIZE_NATIVE_OPERANDS','value':'ON'},
            {'name':'HALO2_NATIVE_EXECUTION_ACCOUNTING','value':'ON'}]})
        runtime=json.loads((self.reply/'runtime.json').read_text())
        for source in (self.unit,self.build/'native-operands'/self.unit.relative_to(self.root/'src')):
            runtime['sources'].append({'path':str(closure.execution_alias(self.unit,source,self.root,self.build)),'isGenerated':True})
        self.save('runtime.json',runtime)
        report,_=closure.export(self.root,self.build,prepare=True)
        unit=report['canonical_units'][0]
        self.assertEqual({row['kind'] for row in unit['configured_sources']},{'partitioned','accounted'})
        self.assertEqual({row['schema'] for row in unit['generator_receipts']},
            {'native-operand-specialization-v1','native-translation-partition-v1','native-execution-instrumentation-v1'})
        execution=[row for row in unit['generator_receipts'] if row['schema']=='native-execution-instrumentation-v1']
        self.assertEqual(len(execution),2)
        self.assertTrue(all(row['function_count']==1 and row['literal_sites']==1 for row in execution))
        self.assertIsNone(closure.execution_owner(self.build/'native-execution'/('0'*16)/self.unit.relative_to(self.root),{str(self.unit):self.unit},self.root,self.build))
        input_source=self.build/'native-operands'/self.unit.relative_to(self.root/'src')
        input_source.write_text(input_source.read_text()+'\n// drift after accounting\n')
        with self.assertRaisesRegex(ValueError,'actual input|canonical/output source'):
            closure.export(self.root,self.build)

    def test_wrong_configuration_and_unowned_preparation_are_rejected(self):
        with self.assertRaisesRegex(ValueError, 'configuration'):
            closure.export(self.root, self.build, configuration='Debug')
        with mock.patch.dict(os.environ, {'HALO2_TEST_SCRATCH': str(self.owner / 'other-owner')}):
            with self.assertRaisesRegex(ValueError, 'HALO2_TEST_SCRATCH'):
                closure.export(self.root, self.build, prepare=True)

    def test_nongenerated_and_embedded_header_bytes_are_authenticated(self):
        ordinary=self.unit.with_name('ordinary.cpp');header=self.unit.with_name('retained.hpp')
        ordinary.write_text('#include "retained.hpp"\nunsigned owner(State &s){s.instruction(0x200u);return helper(s);}\n')
        header.write_text('inline unsigned helper(State &s){s.instruction(0x100u);return 1;}\n')
        alias=closure.execution_alias(ordinary,ordinary,self.root,self.build)
        self.save('cache.json',{'entries':[{'name':'HALO2_NATIVE_EXECUTION_ACCOUNTING','value':'ON'}]})
        self.save('runtime.json',{'id':'runtime','name':'halo2_runtime','type':'STATIC_LIBRARY',
            'sources':[{'path':str(alias),'isGenerated':True}]})
        report,_=closure.export(self.root,self.build,prepare=True)
        self.assertEqual(report['configured_generated_units'],0)
        self.assertEqual(report['configured_literal_instruction_source_units'],2)
        by_source={row['source']:row for row in report['canonical_units']}
        included=by_source['src/physics/retained.hpp']
        self.assertEqual(included['targets'],['runtime'])
        self.assertEqual(included['configured_sources'][0]['kind'],'embedded-accounted-header')
        self.assertIn('not a direct CMake',included['source_relationship'])
        alias.write_text(alias.read_text().replace('s.instruction(0x100u)','s.instruction(0x101u)'))
        with self.assertRaisesRegex(ValueError,'canonical/output source|header bytes'):
            closure.export(self.root,self.build)

    def test_changed_canonical_source_cannot_reuse_an_old_alias_receipt(self):
        closure.export(self.root, self.build, prepare=True)
        self.unit.write_text(SOURCE.replace('0x100u', '0x101u'))
        with self.assertRaisesRegex(ValueError, 'differs from canonical'):
            closure.export(self.root, self.build)

    def test_growing_graph_retains_every_field_with_compact_encoding(self):
        value={'targets':[{'name':'halo2_native','sources':['src/a.cpp','src/b.cpp'],'alias_sha256':'f'*64}for _ in range(20)]}
        compact=json.dumps(value,sort_keys=True,separators=(',',':'))+'\n';limit=len(compact.encode())
        self.assertGreater(len(json.dumps(value,indent=2,sort_keys=True).encode()),limit)
        retained=closure.retained_json(value,limit)
        self.assertEqual(json.loads(retained),value)
        self.assertEqual(len(retained.encode()),limit)
        with self.assertRaisesRegex(ValueError,'retention budget'):closure.retained_json(value,limit-1)

    def test_configuration_drift_is_rejected_before_publishing_closure(self):
        closure.export(self.root, self.build, prepare=True)
        (self.root / 'CMakeLists.txt').write_text('# changed configure inputs\n')
        with self.assertRaisesRegex(ValueError, 'configuration inputs changed'):
            closure.export(self.root, self.build, collection_context={
                'cmake_source_hashes': {'CMakeLists.txt': '0' * 64}})

    def test_collector_rejects_concurrent_preparation_tool_changes(self):
        (self.root/'CMakeLists.txt').write_text('# stable configuration\n');(self.root/'cmake').mkdir(exist_ok=True)
        output=self.owner/'collection';evidence=output/'cmake/evidence/native-production-closure.json';evidence.parent.mkdir(parents=True);evidence.write_text('{}')
        with mock.patch.object(sys,'argv',['collect','--source',str(self.root),'--output',str(output)]), \
             mock.patch.object(collector,'prepare_peer_source',return_value=(self.owner/'peer',[])), \
             mock.patch.object(collector,'preparation_tool_hashes',side_effect=[{'tools/prepare.py':'a'*64},{'tools/prepare.py':'b'*64}]), \
             mock.patch.object(collector.subprocess,'run',return_value=mock.Mock(returncode=0)):
            self.assertEqual(collector.main(),1)
        report=json.loads((output/'collection.json').read_text())
        self.assertFalse(report['configured_closure_collected'])
        self.assertEqual(report['source_preparation_tools_changed_during_collection'],['tools/prepare.py'])
        self.assertEqual(report['source_configuration_changed_during_collection'],[])


if __name__ == '__main__':
    unittest.main()
