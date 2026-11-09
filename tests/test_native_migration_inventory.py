#!/usr/bin/env python3
"""The migration inventory preserves uncertainty and detects source drift."""
import importlib.util
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from audit_native_migration_inventory import inventory, PRODUCTION_EVIDENCE


class NativeMigrationInventoryTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(dir=os.environ.get('HALO2_TEST_SCRATCH'))
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        (self.root / 'src/gameplay').mkdir(parents=True)
        (self.root / 'cmake').mkdir()
        (self.root / 'CMakeLists.txt').write_text('''
add_executable(halo2_native src/app/main.cpp)
target_link_libraries(halo2_native PRIVATE halo2_game)
add_library(halo2_game src/gameplay/native_generated.cpp)
add_library(test_only src/gameplay/oracle_generated.cpp)
''')
        self.source = self.root / 'src/gameplay/native_generated.cpp'
        self.source.write_text('''
#include "gate.hpp"
static void run_100(WorldState &s) {
    if(!some_native::reference()) {
        some_native::bridge(s);return;
    }
if(s.services.instruction){s.services.instruction(0x100u);} run_200(s);
if(s.services.instruction){s.services.instruction(0x105u);} {const auto target=0x300u;dispatch(s,target,0x105u,false);}
}
static void run_200(WorldState &s) {
if(s.services.instruction){s.services.instruction(0x200u);} return;
}
''')
        (self.source.parent / 'gate.hpp').write_text('// HALO2_REFERENCE_EXAMPLE\n')
        (self.source.parent / 'oracle_generated.cpp').write_text('''
static void run_100(State &s) {
if(s.services.instruction){s.services.instruction(0x100u);} return;
}
''')

    def test_generated_units_and_hooks_keep_production_uncertainty(self):
        manifest, outputs = inventory(self.root)
        self.assertEqual(manifest['totals']['generated_units'], 2)
        self.assertEqual(manifest['totals']['function_bodies'], 3)
        self.assertEqual(manifest['totals']['production_candidate_function_bodies'], 2)
        self.assertFalse(manifest['acceptance']['native_only_complete'])
        native = next(unit for unit in manifest['units'] if unit['source'].endswith('native_generated.cpp'))
        oracle = next(unit for unit in manifest['units'] if unit['source'].endswith('oracle_generated.cpp'))
        self.assertEqual(native['production_scope'], 'static_production_candidate')
        self.assertEqual(oracle['production_scope'], 'registered_outside_static_production_closure')
        table = json.loads(outputs[native['tables'][0]['path']])
        routine = table['routines'][0]
        self.assertEqual(routine['observed_tu15_entry'], '100')
        self.assertEqual(routine['direct_run_symbols'], ['200'])
        self.assertEqual(routine['literal_dispatch_required_targets'], ['300'])
        self.assertIn('requires_audit', routine['route_status'])
        self.assertIn('some_native::bridge', routine['native_admission_hints'])
        self.assertIn('HALO2_REFERENCE_EXAMPLE', table['reference_verification_gate_names'])
        self.assertTrue(all(len(content.encode()) <= 1_048_576 for content in outputs.values()))

    def test_authenticated_source_prefixes_are_distinct_from_closure(self):
        from native_source_prefixes import authenticated_prefixes
        path=ROOT/'src/physics/physics_live_support_tu15_generated.cpp'
        text=path.read_text();evidence=authenticated_prefixes(ROOT,path,text)
        for entry in('126200','1264e0','126060','1e5c80','1e5af0','1e6cb0','2dbf74','13fe50','1e68e0','1262b0','1e3150','15cb60','15cc70','15cdf0','15cd30','bf0c0','bfb90','b8780','b87b0'):
            self.assertTrue(evidence[entry]['authenticated'],entry)
            self.assertFalse(evidence[entry]['runtime_admission_proved'])
            self.assertFalse(evidence[entry]['native_dependency_closure_proved'])
            self.assertIn('native_source_prefixes.py',evidence[entry]['validation_dependency_sha256'])
        from native_resource_page_replacements import PREFIX
        from specialize_native_operands import leaf_body
        body=leaf_body(text,'126060')[0]
        changed=text.replace(body,body.replace('0x474d78u','0x474d7cu',1),1)
        rejected=authenticated_prefixes(ROOT,path,changed)
        self.assertFalse(rejected['126060']['authenticated'])
        self.assertIn('body drift',rejected['126060']['error'])
        # A similar-looking uninstalled hook cannot be promoted by its name.
        changed=text.replace(PREFIX,PREFIX.replace('bridge(s,','bridge( s,',1),1)
        self.assertNotIn('126060',authenticated_prefixes(ROOT,path,changed))

    def test_nested_vehicle_admission_requires_its_complete_guard_and_body(self):
        from native_source_prefixes import authenticated_prefixes
        from native_vehicle_choice_data_replacements import prefix
        from generated_routine_ownership import function_bodies
        path=ROOT/'src/physics/physics_live_support_tu15_generated.cpp';text=path.read_text()
        body=next(body for signature,body in function_bodies(text)if signature[1]=='15cb60')
        evidence=authenticated_prefixes(ROOT,path,text)['15cb60']
        self.assertTrue(evidence['authenticated'])
        self.assertEqual(evidence['admission_placement'],'inside_complete_authenticated_tu15_guard')
        for old,new in(('if(s.image==SourcePhysicsContactImage::TitleUpdate15)','if(true)'),('0x19660du','0x19660cu')):
            changed=text.replace(body,body.replace(old,new,1),1)
            self.assertFalse(authenticated_prefixes(ROOT,path,changed)['15cb60']['authenticated'])
        forged='static void run_15cb60(WorldState &s) {\n'+prefix('light')+'return;\n}\n'
        self.assertFalse(authenticated_prefixes(ROOT,path,forged)['15cb60']['authenticated'])

    def test_canonical_state_mopp_has_its_distinct_exact_owner(self):
        from native_source_prefixes import authenticated_prefixes
        path=ROOT/'src/physics/runtime/physics_contact_runtime_generated.cpp'
        evidence=authenticated_prefixes(ROOT,path,path.read_text())['305e40']
        self.assertTrue(evidence['authenticated'])
        self.assertTrue(evidence['target_body_rebinding_proved'])
        self.assertEqual(evidence['binder'],'tools/native_mopp_aabb_canonical_replacements.py')
        self.assertFalse(evidence['native_dependency_closure_proved'])

    def test_multi_entry_object_family_rebinds_each_whole_owner(self):
        from native_source_prefixes import authenticated_prefixes
        from native_object_node_rebuild_replacements import PREFIXES
        from generated_routine_ownership import function_bodies
        path=ROOT/'src/physics/physics_live_support_tu15_generated.cpp';text=path.read_text()
        body=next(body for signature,body in function_bodies(text)if signature[1]=='bfb90')
        changed=text.replace(body,body.replace(PREFIXES['bfb90'],PREFIXES['bfb90'].replace('dispatch(v,target,site,false)','dispatch(v,target,site,true)'),1),1)
        rejected=authenticated_prefixes(ROOT,path,changed)
        for entry in PREFIXES:self.assertFalse(rejected.get(entry,{}).get('authenticated',False))
        sphere=ROOT/'src/rendering/effects/source_effect_particle_draw_tu15_generated.cpp'
        evidence=authenticated_prefixes(ROOT,sphere,sphere.read_text())['184690']
        self.assertTrue(evidence['authenticated'])
        self.assertFalse(evidence['native_dependency_closure_proved'])

    def test_typed_async_prefixes_keep_each_source_namespace_and_owner(self):
        from native_source_prefixes import authenticated_prefixes
        from native_async_order_replacements import authenticate as order
        from native_async_insert_replacements import authenticate as insertion
        arena=ROOT/'src/physics/physics_game_arena_runtime_generated.cpp';text=arena.read_text()
        evidence=authenticated_prefixes(ROOT,arena,text)
        for entry in('1a5080','122020'):
            self.assertTrue(evidence[entry]['authenticated'])
            self.assertTrue(evidence[entry]['target_body_rebinding_proved'])
            self.assertFalse(evidence[entry]['native_dependency_closure_proved'])
            self.assertIn('include/halo2/native_tu15_async_queue_kernel.hpp',evidence[entry]['runtime_include_source_sha256'])
        match,_=order(text);changed=text[:match.start()]+match[0].replace('0x1a47d0u','0x1a47d1u',1)+text[match.end():]
        self.assertFalse(authenticated_prefixes(ROOT,arena,changed)['1a5080']['authenticated'])
        shadow=ROOT/'src/rendering/shadows/source_shadow_frame_tu15_generated.cpp';text=shadow.read_text()
        evidence=authenticated_prefixes(ROOT,shadow,text)['1232c0']
        self.assertTrue(evidence['authenticated'])
        match,_=insertion(text,'1232c0');changed=text[:match.start()]+match[0].replace('0x123339u,false','0x123339u,true',1)+text[match.end():]
        self.assertFalse(authenticated_prefixes(ROOT,shadow,changed).get('1232c0',{}).get('authenticated',False))

    def test_typed_promotion_prefix_is_not_a_whole_mutex_closure_claim(self):
        from native_source_prefixes import authenticated_prefixes
        from native_async_promote_replacements import authenticate
        path=ROOT/'src/physics/physics_live_support_tu15_generated.cpp';text=path.read_text()
        evidence=authenticated_prefixes(ROOT,path,text)['1236a0']
        self.assertTrue(evidence['authenticated'])
        self.assertTrue(evidence['target_body_rebinding_proved'])
        self.assertIn('include/halo2/native_tu15_async_promotion_kernel.hpp',evidence['runtime_include_source_sha256'])
        self.assertFalse(evidence['native_dependency_closure_proved'])
        match,_=authenticate(text);changed=text[:match.start()]+match[0].replace('0x2e0478u','0x2e0342u',1)+text[match.end():]
        self.assertFalse(authenticated_prefixes(ROOT,path,changed)['1236a0']['authenticated'])

    def test_instanced_child_prefixes_preserve_image_and_runtime_scope(self):
        from native_source_prefixes import authenticated_prefixes
        from native_instanced_ray_child_replacements import authenticate
        for name in('src/physics/physics_live_support_tu15_generated.cpp','src/campaign/ai/source_ai_frame_tu15_generated.cpp'):
            path=ROOT/name;text=path.read_text();evidence=authenticated_prefixes(ROOT,path,text)['24dec0']
            self.assertTrue(evidence['authenticated'])
            self.assertTrue(evidence['target_body_rebinding_proved'])
            self.assertIn('include/halo2/native_tu15_instanced_ray_child_kernel.hpp',evidence['runtime_include_source_sha256'])
            self.assertFalse(evidence['runtime_admission_proved'])
            self.assertFalse(evidence['native_dependency_closure_proved'])
        match,_=authenticate(text);changed=text[:match.start()]+match[0].replace('0x46625cu','0x466260u',1)+text[match.end():]
        self.assertFalse(authenticated_prefixes(ROOT,path,changed)['24dec0']['authenticated'])

    def test_polygon_prefixes_are_authenticated_without_unbound_parent_closure(self):
        from native_source_prefixes import authenticated_prefixes
        for name in('src/physics/runtime/physics_world_runtime_generated.cpp','src/physics/runtime/physics_contact_runtime_generated.cpp'):
            path=ROOT/name;text=path.read_text();evidence=authenticated_prefixes(ROOT,path,text)
            for entry in('1f1d20','1f1da0'):
                self.assertTrue(evidence[entry]['authenticated'])
                self.assertTrue(evidence[entry]['target_body_rebinding_proved'])
                self.assertFalse(evidence[entry]['runtime_admission_proved'])
                self.assertFalse(evidence[entry]['native_dependency_closure_proved'])

    def test_check_detects_source_and_gate_drift(self):
        output = self.root / 'inventory-docs'
        command = [sys.executable, str(ROOT / 'tools/audit_native_migration_inventory.py'),
                   '--root', str(self.root), '--output', str(output)]
        subprocess.run(command, check=True, capture_output=True)
        subprocess.run(command + ['--check'], check=True, capture_output=True)
        (self.source.parent / 'gate.hpp').write_text('// HALO2_REFERENCE_CHANGED\n')
        changed = subprocess.run(command + ['--check'], capture_output=True, text=True)
        self.assertEqual(changed.returncode, 1)
        self.assertTrue(json.loads(changed.stdout)['stale_files'])

    def test_prefix_binder_noop_cannot_authenticate_an_unmatched_owner(self):
        from native_source_prefixes import authenticated_prefixes
        from native_resource_page_replacements import PREFIX
        # This exact hook is real, but the owner accepts only WorldState.
        # Its deliberate no-op for State cannot qualify the forged body.
        text='static void run_126060(State &s) {'+PREFIX+'\nreturn;\n}\n'
        evidence=authenticated_prefixes(ROOT,self.source,text)['126060']
        self.assertFalse(evidence['authenticated'])
        self.assertIn('reconstruct',evidence['error'])
        unrelated='static void run_1(WorldState &s) {'+PREFIX+'\nreturn;\n}\n'
        evidence=authenticated_prefixes(ROOT,self.source,unrelated)['126060']
        self.assertFalse(evidence['authenticated'])
        self.assertIn('target body',evidence['error'])

    def test_legacy_attributed_functions_are_counted_and_image_qualified(self):
        self.source.write_text('''
struct State : Base { State(M&m,O&o,S&shared):State{m,o,shared,SourcePhysicsContactImage::MapPack11}{} };
[[gnu::noinline]] static void run_12f5e0(State &s) {
if(s.services.instruction){s.services.instruction(0x12f5e0u);} return;
}
''')
        manifest,outputs=inventory(self.root)
        unit=next(row for row in manifest['units'] if row['source'].endswith('native_generated.cpp'))
        self.assertEqual(unit['routine_count'],1)
        self.assertEqual(unit['callback_image_scope'],'map-pack11')
        table=json.loads(outputs[unit['tables'][0]['path']]);routine=table['routines'][0]
        self.assertIsNone(routine['observed_tu15_entry'])
        self.assertEqual(routine['observed_map_pack11_entry'],'12f5e0')
        self.assertEqual(routine['observed_entry'],'12f5e0')
        self.assertEqual(unit['production_scope'],'static_production_candidate')

    def test_descriptive_and_local_state_owners_are_not_dropped(self):
        self.source.write_text('''
void producer(WorldState &s) {
if(s.services.instruction)s.services.instruction(0x1482a0u);
}
void source_run_tu15_power_dpc(M &memory,const S &services) {
WorldState s{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};
if(services.instruction)services.instruction(0x2e009du);
}
''')
        manifest,outputs=inventory(self.root)
        self.assertEqual(manifest['totals']['descriptive_function_bodies'],2)
        unit=next(row for row in manifest['units'] if row['source'].endswith('native_generated.cpp'))
        table=json.loads(outputs[unit['tables'][0]['path']])
        self.assertEqual([row['observed_tu15_entry'] for row in table['routines']],['1482a0','2e009d'])
        self.assertTrue(all(row['symbol_kind']=='descriptive' for row in table['routines']))

    def test_configured_production_proof_retains_scope_and_detects_cmake_drift(self):
        path = self.root / PRODUCTION_EVIDENCE
        path.parent.mkdir(parents=True)
        cmake = self.root / 'CMakeLists.txt'
        path.write_text(json.dumps({'schema': 'native-production-cmake-closure-v1',
            'source_root': str(self.root), 'root_target': 'halo2_native',
            'configuration': 'Release', 'host': {'system': 'Linux', 'machine': 'fixture'},
            'target_count': 2, 'configured_generated_units': 1,
            'scope': {'runtime_execution_proved': False},
            'collection_context': {'cmake_source_hashes': {
                'CMakeLists.txt': hashlib.sha256(cmake.read_bytes()).hexdigest()}},
            'canonical_units': [{'source': str(self.source.relative_to(self.root)),
                'source_sha256': hashlib.sha256(self.source.read_bytes()).hexdigest(), 'targets': ['runtime']}]}))
        manifest, _ = inventory(self.root)
        self.assertTrue(manifest['configured_production_evidence']['current_source_and_configuration'])
        self.assertEqual(manifest['totals']['configured_production_function_bodies'], 2)
        self.assertFalse(manifest['configured_production_evidence']['scope']['runtime_execution_proved'])
        cmake.write_text(cmake.read_text() + '\n# A new source selection decision\n')
        changed, _ = inventory(self.root)
        self.assertFalse(changed['configured_production_evidence']['current_source_and_configuration'])
        self.assertIn('CMakeLists.txt', changed['configured_production_evidence']['source_or_configuration_drift'])

    def test_nongenerated_cpp_and_header_literal_owners_are_preserved(self):
        ordinary=self.source.parent/'ordinary.cpp';header=self.source.parent/'retained.hpp'
        with (self.root/'CMakeLists.txt').open('a')as stream:stream.write('\nadd_library(halo2_extra src/gameplay/ordinary.cpp)\ntarget_link_libraries(halo2_native PRIVATE halo2_extra)\n')
        ordinary.write_text('''#include "retained.hpp"
[[gnu::noinline]] static unsigned find(State &s) {
s.instruction(0x300u);return s.gp[0];
}
''')
        header.write_text('''template<class T>
inline unsigned retained(T &s) {instruction(0x400u);return 0;}
''')
        manifest,outputs=inventory(self.root)
        self.assertEqual(manifest['totals']['generated_units'],2)
        self.assertEqual(manifest['totals']['nongenerated_units'],2)
        self.assertEqual(manifest['totals']['function_bodies'],5)
        extra=next(unit for unit in manifest['units']if unit['source'].endswith('ordinary.cpp'))
        included=next(unit for unit in manifest['units']if unit['source'].endswith('retained.hpp'))
        self.assertEqual(extra['production_scope'],'static_production_candidate')
        self.assertEqual(included['production_scope'],'static_production_include_candidate')
        table=json.loads(outputs[extra['tables'][0]['path']])
        self.assertEqual(table['routines'][0]['observed_entry'],'300')
        self.assertIn('independent qualification',table['routines'][0]['observed_entry_semantics'])
        included_table=json.loads(outputs[included['tables'][0]['path']])
        self.assertEqual(included_table['source_inclusion_candidates'][0]['registered_source'],'src/gameplay/ordinary.cpp')

    def test_old_callback_pc_is_not_promoted_from_state_image_to_actual_tu15(self):
        path=self.root/'src/gameplay/weapons/unit_pickup_runtime.cpp';path.parent.mkdir()
        path.write_text('''struct State {State():Base{m,o,shared,SourcePhysicsContactImage::TitleUpdate15}{}};
static void run_cd1a0(State &s) {s.instruction(0xcd1a0u);}
''')
        manifest,outputs=inventory(self.root);unit=next(row for row in manifest['units']if row['source'].endswith('unit_pickup_runtime.cpp'))
        routine=json.loads(outputs[unit['tables'][0]['path']])['routines'][0]
        self.assertIsNone(routine['observed_tu15_entry']);self.assertIsNone(routine['actual_tu15_function_entry'])
        self.assertEqual(routine['canonical_callback_entry'],'cd1a0');self.assertFalse(routine['actual_tu15_identity_qualified'])

    def test_conditional_internal_owner_targets_are_separate_from_callback_tags(self):
        self.source.write_text(self.source.read_text()+'''
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage image){if(image==SourcePhysicsContactImage::MapPack11)return p;switch(p){case 0x100u:return 0x200u;default:throw std::runtime_error("unbound");}}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail){if(target==selected(0x100u,s.image)){run_100(s);return;}s.required(site,target,tail);}
''')
        manifest,outputs=inventory(self.root);unit=next(row for row in manifest['units']if row['source'].endswith('native_generated.cpp'))
        table=json.loads(outputs[unit['tables'][0]['path']]);routine=table['routines'][0]
        self.assertEqual(routine['observed_tu15_entry'],'100')
        self.assertEqual(routine['internal_dispatch_owner_entries'],{'tu15':'200','map-pack11':'100'})
        self.assertIn('dispatcher',table['internal_dispatch_identity_evidence']['tu15']['basis'])
        self.assertFalse(manifest['acceptance']['native_only_complete'])
        self.source.write_text(self.source.read_text().replace('switch(p)','p^=1u;switch(p)'))
        _,outputs=inventory(self.root);table=json.loads(outputs[unit['tables'][0]['path']])
        self.assertNotIn('internal_dispatch_owner_entries',table['routines'][0])


if __name__ == '__main__':
    unittest.main()
