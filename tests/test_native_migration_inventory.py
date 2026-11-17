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

    def test_settled_renderer_prefixes_keep_each_source_scope(self):
        from native_source_prefixes import authenticated_prefixes
        for source,entries in (
                ('src/rendering/shadows/source_shadow_frame_tu15_generated.cpp',('166c00','171070','136e10','135970','136ea0')),
                ('src/runtime/source_map_world_support_tu15_generated.cpp',('136e10','2e2d10','2e2e00','2e3080','2e30e0')),
                ('src/rendering/effects/source_effect_particle_draw_tu15_generated.cpp',('167ef0','171070','135970','136ea0')),
                ('src/physics/physics_live_support_tu15_generated.cpp',('16b9a0','120fa0','16c000','24def0','24df20')),
                ('src/campaign/ai/source_ai_frame_tu15_generated.cpp',('120fa0','16c000','24def0','24df20'))):
            path=ROOT/source;evidence=authenticated_prefixes(ROOT,path,path.read_text())
            for entry in entries:
                self.assertTrue(evidence[entry]['authenticated'],(source,entry))
                self.assertTrue(evidence[entry]['target_body_rebinding_proved'])
                self.assertFalse(evidence[entry]['runtime_admission_proved'])
                self.assertFalse(evidence[entry]['native_dependency_closure_proved'])

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

    def test_closest_segments_all_three_contexts_and_sibling_include_order(self):
        from native_source_prefixes import authenticated_prefixes
        from native_closest_segments_replacements import BODY
        for name,entry in(
            ('src/physics/runtime/physics_world_runtime_generated.cpp','30eb30'),
            ('src/physics/runtime/physics_contact_runtime_generated.cpp','30eb30'),
            ('src/rendering/renderer/source_scene_device_constructor_tu15_generated.cpp','312620')):
            path=ROOT/name;text=path.read_text();proof=authenticated_prefixes(ROOT,path,text)[entry]
            self.assertTrue(proof['authenticated'])
            self.assertTrue(proof['target_body_rebinding_proved'])
            self.assertFalse(proof['runtime_admission_proved'])
            self.assertFalse(proof['native_dependency_closure_proved'])
            self.assertIn('include/halo2/native_tu15_closest_segments_kernel.hpp',proof['runtime_include_source_sha256'])
            sibling='// independent sibling include-position witness\n'+text
            self.assertTrue(authenticated_prefixes(ROOT,path,sibling)[entry]['authenticated'])
            body=BODY.search(text);changed=body[0].replace('0x10u','0x20u',1)
            self.assertNotEqual(changed,body[0])
            changed=text[:body.start()]+changed+text[body.end():]
            self.assertFalse(authenticated_prefixes(ROOT,path,changed)[entry]['authenticated'])

    def test_simplex_paired_both_contexts_and_sibling_include_order(self):
        from native_source_prefixes import authenticated_prefixes
        from native_simplex_paired_replacements import BODY
        for name in('src/physics/runtime/physics_world_runtime_generated.cpp','src/physics/runtime/physics_contact_runtime_generated.cpp'):
            path=ROOT/name;text=path.read_text();proof=authenticated_prefixes(ROOT,path,text)['309d40']
            self.assertTrue(proof['authenticated'],proof)
            self.assertTrue(proof['target_body_rebinding_proved'])
            self.assertFalse(proof['runtime_admission_proved'])
            self.assertFalse(proof['native_dependency_closure_proved'])
            self.assertIn('include/halo2/native_tu15_simplex_paired_kernel.hpp',proof['runtime_include_source_sha256'])
            sibling='// independent paired sibling include-position witness\n'+text
            self.assertTrue(authenticated_prefixes(ROOT,path,sibling)['309d40']['authenticated'])
            body=BODY.search(text);changed=body[0].replace('0xa4u','0xa8u',1)
            self.assertNotEqual(changed,body[0]);changed=text[:body.start()]+changed+text[body.end():]
            self.assertFalse(authenticated_prefixes(ROOT,path,changed)['309d40']['authenticated'])

    def test_solver_hull_append_actual_identity_and_sibling_position(self):
        from native_source_prefixes import authenticated_prefixes
        from native_solver_hull_append_replacements import BODY
        path=ROOT/'src/physics/physics_live_support_tu15_generated.cpp';text=path.read_text()
        proof=authenticated_prefixes(ROOT,path,text)['313290']
        self.assertTrue(proof['authenticated'],proof)
        self.assertTrue(proof['target_body_rebinding_proved'])
        self.assertFalse(proof['runtime_admission_proved'])
        self.assertFalse(proof['native_dependency_closure_proved'])
        self.assertIn('include/halo2/native_tu15_solver_hull_append_kernel.hpp',proof['runtime_include_source_sha256'])
        sibling='// independent actual append include-position witness\n'+text
        self.assertTrue(authenticated_prefixes(ROOT,path,sibling)['313290']['authenticated'])
        body=BODY.search(text);changed=body[0].replace('0x48u','0x4cu',1)
        self.assertNotEqual(changed,body[0]);changed=text[:body.start()]+changed+text[body.end():]
        self.assertFalse(authenticated_prefixes(ROOT,path,changed)['313290']['authenticated'])
        world=ROOT/'src/physics/runtime/physics_world_runtime_generated.cpp'
        self.assertNotIn('313290',authenticated_prefixes(ROOT,world,world.read_text()))

    def test_zero_support_actual_identity_and_sibling_position(self):
        from native_source_prefixes import authenticated_prefixes
        from native_zero_shape_support_replacements import BODY
        path=ROOT/'src/physics/physics_live_support_tu15_generated.cpp';text=path.read_text()
        proof=authenticated_prefixes(ROOT,path,text)['2e8d20']
        self.assertTrue(proof['authenticated'],proof)
        self.assertTrue(proof['target_body_rebinding_proved'])
        self.assertFalse(proof['runtime_admission_proved'])
        self.assertFalse(proof['native_dependency_closure_proved'])
        self.assertIn('include/halo2/native_tu15_zero_shape_support_kernel.hpp',proof['runtime_include_source_sha256'])
        sibling='// independent actual zero-support include-position witness\n'+text
        self.assertTrue(authenticated_prefixes(ROOT,path,sibling)['2e8d20']['authenticated'])
        body=BODY.search(text);changed=body[0].replace('0xcu','0x8u',1)
        self.assertNotEqual(changed,body[0]);changed=text[:body.start()]+changed+text[body.end():]
        self.assertFalse(authenticated_prefixes(ROOT,path,changed)['2e8d20']['authenticated'])
        world=ROOT/'src/physics/runtime/physics_world_runtime_generated.cpp'
        self.assertNotIn('2e8d20',authenticated_prefixes(ROOT,world,world.read_text()))

    def test_box_support_actual_identity_and_sibling_position(self):
        from native_source_prefixes import authenticated_prefixes
        from native_box_shape_support_replacements import BODY
        path=ROOT/'src/physics/physics_live_support_tu15_generated.cpp';text=path.read_text()
        proof=authenticated_prefixes(ROOT,path,text)['306790']
        self.assertTrue(proof['authenticated'],proof)
        self.assertTrue(proof['target_body_rebinding_proved'])
        self.assertFalse(proof['runtime_admission_proved'])
        self.assertFalse(proof['native_dependency_closure_proved'])
        self.assertIn('include/halo2/native_tu15_box_shape_support_kernel.hpp',proof['runtime_include_source_sha256'])
        sibling='// independent actual box-support include-position witness\n'+text
        self.assertTrue(authenticated_prefixes(ROOT,path,sibling)['306790']['authenticated'])
        body=BODY.search(text);changed=body[0].replace('0xcu','0x8u',1)
        self.assertNotEqual(changed,body[0]);changed=text[:body.start()]+changed+text[body.end():]
        self.assertFalse(authenticated_prefixes(ROOT,path,changed)['306790']['authenticated'])
        world=ROOT/'src/physics/runtime/physics_world_runtime_generated.cpp'
        self.assertNotIn('306790',authenticated_prefixes(ROOT,world,world.read_text()))

    def test_support_pair_actual_identity_sibling_and_relocated_label(self):
        from native_source_prefixes import authenticated_prefixes
        from native_solver_hull_support_pair_replacements import BODY,LABELS
        path=ROOT/'src/physics/physics_live_support_tu15_generated.cpp';text=path.read_text()
        proof=authenticated_prefixes(ROOT,path,text)['313390']
        self.assertTrue(proof['authenticated'],proof)
        self.assertTrue(proof['target_body_rebinding_proved'])
        self.assertFalse(proof['runtime_admission_proved'])
        self.assertFalse(proof['native_dependency_closure_proved'])
        self.assertIn('include/halo2/native_tu15_solver_hull_support_pair_kernel.hpp',proof['runtime_include_source_sha256'])
        sibling='// independent support-pair include-position witness\n'+text
        self.assertTrue(authenticated_prefixes(ROOT,path,sibling)['313390']['authenticated'])
        body=BODY.search(text);label=LABELS[0x313391]
        target=next(line for line in body[0].splitlines()if '0x313393u'in line)
        changed=body[0].replace(label,'',1).replace(target,target[:len(target)-len(target.lstrip())]+label+target.lstrip(),1)
        self.assertNotEqual(changed,body[0])
        self.assertFalse(authenticated_prefixes(ROOT,path,text[:body.start()]+changed+text[body.end():])['313390']['authenticated'])
        world=ROOT/'src/physics/runtime/physics_world_runtime_generated.cpp'
        self.assertNotIn('313390',authenticated_prefixes(ROOT,world,world.read_text()))

    def test_solver_hull_initialize_actual_spill_profile_and_class(self):
        from native_source_prefixes import authenticated_prefixes
        from native_solver_hull_initialize_replacements import STATE
        path=ROOT/'src/physics/physics_live_support_tu15_generated.cpp';text=path.read_text()
        families={'313560':'initialize','3136a0':'line','313300':'sample','3131b0':'minimum'}
        proofs=authenticated_prefixes(ROOT,path,text)
        for entry,name in families.items():
            proof=proofs[entry]
            self.assertTrue(proof['authenticated'],proof)
            self.assertTrue(proof['target_body_rebinding_proved'])
            self.assertFalse(proof['runtime_admission_proved'])
            self.assertFalse(proof['native_dependency_closure_proved'])
            self.assertIn(f'include/halo2/native_tu15_solver_hull_{name}_kernel.hpp',proof['runtime_include_source_sha256'])
        sibling='// independent hull-initialize include-position witness\n'+text
        sibling_proofs=authenticated_prefixes(ROOT,path,sibling)
        for entry in families:self.assertTrue(sibling_proofs[entry]['authenticated'])
        state=STATE.search(text);self.assertIsNotNone(state)
        start=state[0].index('{')+1
        changed=state[0][:start]+(' void instruction_literal(std::uint32_t pc) {'
            ' if(pc==0x3135dbu) gp[6]^=4u; WorldState::instruction_literal(pc); }')+state[0][start:]
        mutated=text[:state.start()]+changed+text[state.end():]
        changed_proofs=authenticated_prefixes(ROOT,path,mutated)
        for entry in families:self.assertFalse(changed_proofs[entry]['authenticated'])
        world=ROOT/'src/physics/runtime/physics_world_runtime_generated.cpp'
        self.assertFalse(families.keys()&authenticated_prefixes(ROOT,world,world.read_text()).keys())

    def test_effect_reference_multiple_exact_owners_and_external_dispatch(self):
        from native_source_prefixes import authenticated_prefixes
        from native_effect_reference_element_replacements import authenticate,function
        for name in('src/gameplay/units/source_combat_action_tu15_generated.cpp',
                    'src/gameplay/units/death_cycle_runtime_tu15_generated.cpp',
                    'src/rendering/effects/source_effect_particle_frame_tu15_generated.cpp'):
            path=ROOT/name;text=path.read_text();proofs=authenticated_prefixes(ROOT,path,text)
            for _,entry,_ in authenticate(text):
                proof=proofs[f'{entry:x}'];self.assertTrue(proof['authenticated'],proof)
                self.assertTrue(proof['target_body_rebinding_proved'])
                self.assertFalse(proof['runtime_admission_proved']);self.assertFalse(proof['native_dependency_closure_proved'])
                self.assertIn('include/halo2/native_tu15_effect_reference_element_kernel.hpp',proof['runtime_include_source_sha256'])
            sibling='// independent effect-reference include-position witness\n'+text
            self.assertTrue(authenticated_prefixes(ROOT,path,sibling)['178070']['authenticated'])
            body=function(text,'run_178070');changed=body.replace('0x4f0414u','0x4f0418u',1)
            self.assertNotEqual(changed,body)
            self.assertFalse(authenticated_prefixes(ROOT,path,text.replace(body,changed,1))['178070']['authenticated'])
        particle=ROOT/'src/rendering/effects/source_effect_particle_frame_tu15_generated.cpp'
        self.assertNotIn('run_22aca0(s);',function(particle.read_text(),'dispatch'))

    def test_simplex_boundary_both_actual_contexts(self):
        from native_source_prefixes import authenticated_prefixes
        from native_simplex_boundary_replacements import BODY,remove_simplex_boundary,replace_simplex_boundary
        for name in('src/physics/physics_live_support_tu15_generated.cpp','src/rendering/renderer/source_scene_device_constructor_tu15_generated.cpp'):
            path=ROOT/name;text=path.read_text();proof=authenticated_prefixes(ROOT,path,text)['30dc10']
            self.assertTrue(proof['authenticated'],proof)
            self.assertTrue(proof['target_body_rebinding_proved'])
            self.assertFalse(proof['runtime_admission_proved'])
            self.assertFalse(proof['native_dependency_closure_proved'])
            self.assertIn('include/halo2/native_tu15_simplex_boundary_kernel.hpp',proof['runtime_include_source_sha256'])
            body=BODY.search(text);changed=body[0].replace('0x74u','0x78u',1)
            self.assertNotEqual(changed,body[0]);changed=text[:body.start()]+changed+text[body.end():]
            self.assertFalse(authenticated_prefixes(ROOT,path,changed)['30dc10']['authenticated'])
            if name.startswith('src/rendering/'):
                unbound=remove_simplex_boundary(text)
                self.assertEqual(replace_simplex_boundary(unbound),text)
                lookup='#include "gameplay/objects/native_tu15_object_lookup_copies_source_owner.hpp"\n'
                # The older generated stage and the installed later sibling
                # both preserve Boundary's exact historical include position.
                older=unbound.replace(lookup,'',1)
                self.assertEqual(replace_simplex_boundary(older),text.replace(lookup,'',1))
                for bad in ('#include "unknown_scene_sibling.hpp"\n'+unbound,
                            lookup+unbound,
                            unbound.replace(lookup,'',1)+lookup):
                    with self.assertRaisesRegex(ValueError,'Scene sibling include order drift'):
                        replace_simplex_boundary(bad)

    def test_marker_lookup_six_contexts_and_real_effect_remain_scoped(self):
        from native_source_prefixes import authenticated_prefixes
        from native_model_marker_lookup_inputs import UNITS,HELPER
        from native_model_marker_lookup_replacements import BODY,effect_body
        for name in (*UNITS,HELPER):
            path=ROOT/name;text=path.read_text()
            entry='marker_lookup'if name==HELPER else BODY.search(text)[1]
            proof=authenticated_prefixes(ROOT,path,text)[entry]
            self.assertTrue(proof['authenticated'],(name,proof))
            self.assertTrue(proof['target_body_rebinding_proved'])
            self.assertFalse(proof['runtime_admission_proved'])
            self.assertFalse(proof['native_dependency_closure_proved'])
            self.assertIn('instruction-shaped',proof['retained_fallback_scope'])
            self.assertIn('include/halo2/native_tu15_model_marker_lookup_kernel.hpp',proof['runtime_include_source_sha256'])
            body=effect_body(text)if name==HELPER else BODY.search(text)[0]
            changed=body.replace('0x58','0x54',1)
            self.assertNotEqual(changed,body)
            forged=text.replace(body,changed,1)
            self.assertFalse(authenticated_prefixes(ROOT,path,forged)[entry]['authenticated'])
        effect=ROOT/HELPER
        self.assertIn('validation_source_sha256',authenticated_prefixes(ROOT,effect,effect.read_text())['marker_lookup'])

    def test_object_body_unit_view_prefix_stays_source_scoped(self):
        from native_source_prefixes import authenticated_prefixes
        from native_object_body_replacements import parts,FRONTIERS
        path=ROOT/'src/gameplay/units/source_unit_view_reset_tu15_generated.cpp'
        text=path.read_text();proof=authenticated_prefixes(ROOT,path,text)['bcdf0']
        self.assertTrue(proof['authenticated'],proof)
        self.assertTrue(proof['target_body_rebinding_proved'])
        self.assertFalse(proof['runtime_admission_proved'])
        self.assertFalse(proof['native_dependency_closure_proved'])
        self.assertIn('include/halo2/native_tu15_object_body_kernel.hpp',proof['runtime_include_source_sha256'])
        shifted='// independent native include-position witness\n'+text
        self.assertTrue(authenticated_prefixes(ROOT,path,shifted)['bcdf0']['authenticated'])
        for name,part in parts(text).items():
            at=part.index('{')+1
            changed=text.replace(part,part[:at]+'/* context drift */'+part[at:],1)
            rejected=authenticated_prefixes(ROOT,path,changed)['bcdf0']
            self.assertFalse(rejected['authenticated'],(name,rejected))
        for pc in FRONTIERS:
            label=f'NativeObjectBody{pc:x}: '
            entry='if(s.services.instruction){s.services.instruction(0xbcdf0u);}'
            changed=text.replace(label,'',1).replace(entry,label+entry,1)
            rejected=authenticated_prefixes(ROOT,path,changed)['bcdf0']
            self.assertFalse(rejected['authenticated'],(pc,rejected))

    def test_object_material_lookup_prefix_preserves_owner_identity(self):
        from native_source_prefixes import authenticated_prefixes
        from native_object_material_lookup_replacements import parts,FRONTIERS
        path=ROOT/'src/gameplay/objects/source_object_material_tu15_generated.cpp'
        text=path.read_text();proof=authenticated_prefixes(ROOT,path,text)['bcdf0']
        self.assertTrue(proof['authenticated'],proof)
        self.assertFalse(proof['runtime_admission_proved'])
        self.assertFalse(proof['native_dependency_closure_proved'])
        self.assertIn('Public18B4C0 parent',proof['retained_fallback_scope'])
        for name,part in parts(text).items():
            at=part.index('{')+1;changed=text.replace(part,part[:at]+'/* context drift */'+part[at:],1)
            self.assertFalse(authenticated_prefixes(ROOT,path,changed)['bcdf0']['authenticated'],name)
        for pc in FRONTIERS:
            label=f'NativeObjectBody{pc:x}: '
            entry='if(s.services.instruction){s.services.instruction(0xbcdf0u);}'
            changed=text.replace(label,'',1).replace(entry,label+entry,1)
            self.assertFalse(authenticated_prefixes(ROOT,path,changed)['bcdf0']['authenticated'],pc)
        sibling='// independent native include position\n'+text
        self.assertTrue(authenticated_prefixes(ROOT,path,sibling)['bcdf0']['authenticated'])

    def test_object_lookup_copies_preserve_complete_source_owners(self):
        from native_source_prefixes import authenticated_prefixes
        from native_object_lookup_copies_replacements import parts,FRONTIERS
        owners={
            'scene_device_constructor':'src/rendering/renderer/source_scene_device_constructor_tu15_generated.cpp',
            'sandbox_special':'src/gameplay/objects/source_sandbox_special_tu15_generated.cpp',
            'effect_particle_draw':'src/rendering/effects/source_effect_particle_draw_tu15_generated.cpp',
            'ai_frame':'src/campaign/ai/source_ai_frame_tu15_generated.cpp',
        }
        for owner,name in owners.items():
            path=ROOT/name;text=path.read_text()
            proof=authenticated_prefixes(ROOT,path,text)['bcdf0']
            self.assertTrue(proof['authenticated'],(owner,proof))
            self.assertTrue(proof['target_body_rebinding_proved'])
            self.assertFalse(proof['runtime_admission_proved'])
            self.assertFalse(proof['native_dependency_closure_proved'])
            self.assertIn('Four exact',proof['retained_fallback_scope'])
            self.assertTrue(authenticated_prefixes(ROOT,path,'// independent include position\n'+text)['bcdf0']['authenticated'])
            for key,part in parts(text,owner).items():
                at=part.index('{')+1
                changed=text.replace(part,part[:at]+'/* exact owner drift */'+part[at:],1)
                self.assertFalse(authenticated_prefixes(ROOT,path,changed)['bcdf0']['authenticated'],(owner,key))
            for pc in FRONTIERS:
                label=f'NativeObjectBody{pc:x}: '
                entry='if(s.services.instruction){s.services.instruction(0xbcdf0u);}'
                changed=text.replace(label,'',1).replace(entry,label+entry,1)
                self.assertFalse(authenticated_prefixes(ROOT,path,changed)['bcdf0']['authenticated'],(owner,pc))

    def test_canonical_object_lookup_prefix_preserves_image_and_format(self):
        from native_source_prefixes import authenticated_prefixes
        import native_object_lookup_canonical_replacements as binding
        owners={
            'direct_damage':'src/gameplay/units/source_direct_damage_runtime_generated.cpp',
            'vitality':'src/gameplay/units/source_vitality_runtime_generated.cpp',
        }
        for owner,name in owners.items():
            path=ROOT/name;text=path.read_text()
            proof=authenticated_prefixes(ROOT,path,text)['bbcb0']
            self.assertTrue(proof['authenticated'],(owner,proof))
            self.assertTrue(proof['target_body_rebinding_proved'])
            self.assertFalse(proof['runtime_admission_proved'])
            self.assertFalse(proof['native_dependency_closure_proved'])
            self.assertIn('canonical BBCB0 observation PCs',proof['retained_fallback_scope'])
            self.assertEqual(binding.installed_entries(text),['bbcb0'])
            body=binding.parts(text,owner)['body']
            self.assertEqual(binding.entry_prefix(body),binding.prefix(False))
            for key,part in binding.parts(text,owner).items():
                at=part.index('{')+1
                changed=text.replace(part,part[:at]+'/* exact context drift */'+part[at:],1)
                self.assertFalse(authenticated_prefixes(ROOT,path,changed)['bbcb0']['authenticated'],(owner,key))
            for pc in binding.FRONTIERS:
                label=f'CanonicalObjectBody{pc:x}: '
                entry='if(s.services.instruction){s.services.instruction(0xbbcb0u);}'
                changed=text.replace(label,'',1).replace(entry,label+entry,1)
                self.assertFalse(authenticated_prefixes(ROOT,path,changed)['bbcb0']['authenticated'],(owner,pc))
            prefix=binding.prefix(False)
            moved=body.replace(prefix,'',1);at=moved.rfind('\n}')
            moved=moved[:at]+prefix+moved[at:]
            self.assertFalse(authenticated_prefixes(ROOT,path,text.replace(body,moved,1))['bbcb0']['authenticated'])
            wrong=text.replace(prefix,binding.prefix(True),1)
            self.assertFalse(authenticated_prefixes(ROOT,path,wrong)['bbcb0']['authenticated'])

    def test_arena_initializer_prefix_is_private_borrow_scoped(self):
        from native_source_prefixes import authenticated_prefixes
        from native_arena_initialize_replacements import BODY
        path=ROOT/'src/physics/physics_game_arena_runtime_generated.cpp'
        text=path.read_text();proof=authenticated_prefixes(ROOT,path,text)['125280']
        self.assertTrue(proof['authenticated'],proof)
        self.assertTrue(proof['target_body_rebinding_proved'])
        self.assertFalse(proof['runtime_admission_proved'])
        self.assertFalse(proof['native_dependency_closure_proved'])
        self.assertIn('callback-free',proof['retained_fallback_scope'])
        self.assertEqual(proof['binder'],'tools/native_arena_world_initialize_replacements.py')
        for header in('native_tu15_arena_world_initialize_owner.hpp','native_tu15_arena_world_publication.hpp','native_tu15_arena_initialize_source_frame.hpp'):
            self.assertIn('src/physics/runtime/'+header,proof['runtime_include_source_sha256'])
        self.assertIn('native_arena_owner_policy_prototype.py',proof['validation_dependency_sha256'])
        body=BODY.search(text)[0];at=body.index('{')+1
        changed=text.replace(body,body[:at]+'/* exact context drift */'+body[at:],1)
        self.assertFalse(authenticated_prefixes(ROOT,path,changed)['125280']['authenticated'])
        fixed=ROOT/'src/gameplay/units/death_startup_runtime_generated.cpp'
        self.assertNotIn('125280',authenticated_prefixes(ROOT,fixed,fixed.read_text()))

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

    def test_parent_mode_primary_and_real_effect_remain_scoped(self):
        from native_source_prefixes import authenticated_prefixes
        from native_object_parent_mode_inputs import UNIT,HELPER
        from native_object_parent_mode_replacements import BODY,effect_body
        for name in(UNIT,HELPER):
            path=ROOT/name;text=path.read_text()
            entry='fc510'if name==UNIT else'marker_parent_mode'
            proof=authenticated_prefixes(ROOT,path,text)[entry]
            self.assertTrue(proof['authenticated'],proof)
            self.assertTrue(proof['target_body_rebinding_proved'])
            self.assertFalse(proof['runtime_admission_proved'])
            self.assertFalse(proof['native_dependency_closure_proved'])
            self.assertIn('private MapPack',proof['retained_fallback_scope'])
            self.assertIn('include/halo2/native_tu15_object_parent_mode_kernel.hpp',proof['runtime_include_source_sha256'])
            body=BODY.search(text)[0]if name==UNIT else effect_body(text)
            changed=body.replace('0x12c','0x128',1)
            self.assertNotEqual(changed,body)
            self.assertFalse(authenticated_prefixes(ROOT,path,text.replace(body,changed,1))[entry]['authenticated'])
        effect=ROOT/HELPER
        self.assertIn('validation_source_sha256',authenticated_prefixes(ROOT,effect,effect.read_text())['marker_parent_mode'])

    def test_marker_owner_actual_canonical_and_effect_are_source_qualified(self):
        from native_source_prefixes import authenticated_prefixes
        from native_marker_owner_inputs import UNITS,HELPER
        from native_marker_owner_replacements import BODY,effect_body
        for name in(*UNITS,HELPER):
            path=ROOT/name;text=path.read_text()
            entry='marker_parent_unit'if name==HELPER else'b9b90'if name==UNITS[3]else'bacd0'
            proof=authenticated_prefixes(ROOT,path,text)[entry]
            self.assertTrue(proof['authenticated'],proof)
            self.assertTrue(proof['target_body_rebinding_proved'])
            self.assertFalse(proof['runtime_admission_proved'])
            self.assertFalse(proof['native_dependency_closure_proved'])
            self.assertIn('required-owner',proof['retained_fallback_scope'])
            self.assertIn('include/halo2/native_tu15_marker_owner_kernel.hpp',proof['runtime_include_source_sha256'])
            body=effect_body(text)if name==HELPER else BODY.search(text)[0]
            changed=body.replace('0x14','0x18',1)
            self.assertNotEqual(changed,body)
            self.assertFalse(authenticated_prefixes(ROOT,path,text.replace(body,changed,1))[entry]['authenticated'])
        path=ROOT/HELPER
        self.assertIn('validation_source_sha256',authenticated_prefixes(ROOT,path,path.read_text())['marker_parent_unit'])

    def test_sphere_source_prefix_has_exact_authority_without_runtime_closure(self):
        from native_source_prefixes import authenticated_prefixes
        from native_sphere_ray_cast_inputs import UNIT
        from native_sphere_ray_cast_replacements import BODY
        path=ROOT/UNIT;text=path.read_text();proof=authenticated_prefixes(ROOT,path,text)['2e8dd0']
        self.assertTrue(proof['authenticated'],proof)
        self.assertTrue(proof['target_body_rebinding_proved'])
        self.assertFalse(proof['runtime_admission_proved'])
        self.assertFalse(proof['native_dependency_closure_proved'])
        self.assertIn('C1/spill',proof['retained_fallback_scope'])
        self.assertIn('native_instanced_ray_child_replacements.py',proof['validation_dependency_sha256'])
        self.assertIn('include/halo2/native_tu15_sphere_ray_cast_kernel.hpp',proof['runtime_include_source_sha256'])
        body=BODY.search(text)[0];changed=body.replace('0xcu','0x10u',1)
        self.assertNotEqual(body,changed)
        self.assertFalse(authenticated_prefixes(ROOT,path,text.replace(body,changed,1))['2e8dd0']['authenticated'])

    def test_packed_rotation_prefixes_keep_exact_source_authority(self):
        from native_source_prefixes import authenticated_prefixes
        from native_packed_rotation_source_inputs import UNITS
        from native_packed_rotation_replacements import BODY
        for name in UNITS:
            with self.subTest(owner=name):
                path=ROOT/name;text=path.read_text();proof=authenticated_prefixes(ROOT,path,text)['296180']
                self.assertTrue(proof['authenticated'],proof)
                self.assertTrue(proof['target_body_rebinding_proved'])
                self.assertFalse(proof['runtime_admission_proved'])
                self.assertFalse(proof['native_dependency_closure_proved'])
                self.assertIn('native_packed_rotation_source_inputs.py',proof['validation_dependency_sha256'])
                self.assertIn('include/halo2/native_tu15_packed_rotation_kernel.hpp',proof['runtime_include_source_sha256'])
                self.assertIn('intermediate MMX',proof['retained_fallback_scope'])
                body=BODY.search(text)[0];changed=body.replace('0x8u','0xcu',1)
                self.assertNotEqual(body,changed)
                self.assertFalse(authenticated_prefixes(ROOT,path,text.replace(body,changed,1))['296180']['authenticated'])

    def test_codec_translation_exact_owners_and_continuation_adjacency(self):
        from native_source_prefixes import authenticated_prefixes
        from native_codec_translation_replacements import BODY,LABELS,inputs
        for name in inputs.UNITS:
            with self.subTest(owner=name):
                path=ROOT/name;text=path.read_text();proof=authenticated_prefixes(ROOT,path,text)['2961f0']
                self.assertTrue(proof['authenticated'],proof)
                self.assertTrue(proof['target_body_rebinding_proved'])
                self.assertFalse(proof['runtime_admission_proved'])
                self.assertFalse(proof['native_dependency_closure_proved'])
                self.assertIn('native_packed_rotation_source_inputs.py',proof['validation_dependency_sha256'])
                self.assertIn('include/halo2/native_tu15_codec_translation_kernel.hpp',proof['runtime_include_source_sha256'])
                self.assertIn('readonly source FP-metadata',proof['retained_fallback_scope'])
                body=BODY.search(text)[0];changed=body.replace('0xcu','0x8u',1)
                self.assertNotEqual(body,changed)
                self.assertFalse(authenticated_prefixes(ROOT,path,text.replace(body,changed,1))['2961f0']['authenticated'])
                label=LABELS[0x2961f5];changed=body.replace(label,'',1)
                target=next(line for line in changed.splitlines()if 'instruction(0x2961f8u)'in line)
                changed=changed.replace(target,label+target,1)
                self.assertFalse(authenticated_prefixes(ROOT,path,text.replace(body,changed,1))['2961f0']['authenticated'])

    def test_codec_scale_exact_owners_and_retired_continuations(self):
        from native_source_prefixes import authenticated_prefixes
        from native_codec_scale_replacements import BODY,LABELS,UNITS
        for name in UNITS:
            with self.subTest(owner=name):
                path=ROOT/name;text=path.read_text();proof=authenticated_prefixes(ROOT,path,text)['296220']
                self.assertTrue(proof['authenticated'],proof)
                self.assertTrue(proof['target_body_rebinding_proved'])
                self.assertFalse(proof['runtime_admission_proved'])
                self.assertFalse(proof['native_dependency_closure_proved'])
                self.assertIn('include/halo2/native_tu15_codec_scale_kernel.hpp',proof['runtime_include_source_sha256'])
                body=BODY.search(text)[0];changed=body.replace('0x10u','0x14u',1)
                self.assertNotEqual(body,changed)
                self.assertFalse(authenticated_prefixes(ROOT,path,text.replace(body,changed,1))['296220']['authenticated'])
                label=LABELS[0x296225];changed=body.replace(label,'',1)
                target=next(line for line in changed.splitlines()if 'instruction(0x296228u)'in line)
                changed=changed.replace(target,label+target,1)
                self.assertFalse(authenticated_prefixes(ROOT,path,text.replace(body,changed,1))['296220']['authenticated'])

    def test_codec_interpolation_exact_tagged_owners_and_retired_continuations(self):
        from native_source_prefixes import authenticated_prefixes
        from native_codec_rotation_interpolation_replacements import BODY,LABELS,ORDERED,UNITS
        from specialize_native_operands import specialize
        for name in UNITS:
            path=ROOT/name;raw=path.read_text();prepared,_=specialize(raw)
            for form,text in(('raw',raw),('prepared',prepared)):
                with self.subTest(owner=name,form=form):
                    proof=authenticated_prefixes(ROOT,path,text)['296240']
                    self.assertTrue(proof['authenticated'],proof)
                    self.assertTrue(proof['target_body_rebinding_proved'])
                    self.assertFalse(proof['runtime_admission_proved'])
                    self.assertFalse(proof['native_dependency_closure_proved'])
                    self.assertIn('native_packed_rotation_source_inputs.py',proof['validation_dependency_sha256'])
                    self.assertIn('include/halo2/native_tu15_codec_rotation_interpolation_kernel.hpp',proof['runtime_include_source_sha256'])
                    self.assertIn('ordered-SSE',proof['retained_fallback_scope'])
                    body=BODY.search(text)[0];label=LABELS[0x296248]
                    changed=body.replace(label,'',1);at=changed.index('{')+1
                    changed=changed[:at]+'\n'+label+changed[at:]
                    self.assertFalse(authenticated_prefixes(ROOT,path,text.replace(body,changed,1))['296240']['authenticated'])
                    original,ordered=next(iter(ORDERED.items()))
                    self.assertFalse(authenticated_prefixes(ROOT,path,text.replace(body,body.replace(ordered,original,1),1))['296240']['authenticated'])

    def test_codec_translation_interpolation_exact_tagged_owners(self):
        from native_source_prefixes import authenticated_prefixes
        import native_codec_translation_interpolation_replacements as binding
        from specialize_native_operands import specialize
        for name in binding.UNITS:
            path=ROOT/name;raw=path.read_text();prepared,_=specialize(raw)
            for form,text in (('raw',raw),('prepared',prepared)):
                with self.subTest(owner=name,form=form):
                    proof=authenticated_prefixes(ROOT,path,text)['2962f0']
                    self.assertTrue(proof['authenticated'],proof)
                    self.assertTrue(proof['target_body_rebinding_proved'])
                    self.assertFalse(proof['runtime_admission_proved'])
                    self.assertFalse(proof['native_dependency_closure_proved'])
                    self.assertIn('native_packed_rotation_source_inputs.py',proof['validation_dependency_sha256'])
                    self.assertIn('include/halo2/native_tu15_codec_translation_interpolation_kernel.hpp',proof['runtime_include_source_sha256'])
                    self.assertIn('twenty-nine',proof['retained_fallback_scope'])
                    body=binding.BODY.search(text)[0];label=binding.LABELS[0x2962f6]
                    prefix=binding.RAW_PREFIX if form=='raw' else binding.PREPARED_PREFIX
                    changed=body.replace(label,'',1);at=changed.index('{')+1+len(prefix)
                    changed=changed[:at]+'\n'+label+changed[at:]
                    self.assertFalse(authenticated_prefixes(ROOT,path,text.replace(body,changed,1))['2962f0']['authenticated'])
                    moved=binding.INCLUDE+text.replace(binding.INCLUDE,'',1)
                    self.assertNotEqual(moved,text)
                    self.assertFalse(authenticated_prefixes(ROOT,path,moved)['2962f0']['authenticated'])

    def test_codec_scale_interpolation_exact_tagged_owner(self):
        from native_source_prefixes import authenticated_prefixes
        import native_codec_scale_interpolation_replacements as binding
        from specialize_native_operands import specialize
        path=ROOT/binding.UNITS[0];raw=path.read_text();prepared,_=specialize(raw)
        for form,text in (('raw',raw),('prepared',prepared)):
            with self.subTest(form=form):
                proof=authenticated_prefixes(ROOT,path,text)['296620']
                self.assertTrue(proof['authenticated'],proof)
                self.assertTrue(proof['target_body_rebinding_proved'])
                self.assertFalse(proof['runtime_admission_proved'])
                self.assertFalse(proof['native_dependency_closure_proved'])
                self.assertIn('include/halo2/native_tu15_codec_scale_interpolation_kernel.hpp',proof['runtime_include_source_sha256'])
                self.assertIn('fifteen',proof['retained_fallback_scope'])
                body=binding.BODY.search(text)[0];label=binding.LABELS[0x29666d]
                prefix=binding.RAW_PREFIX if form=='raw' else binding.PREPARED_PREFIX
                changed=body.replace(label,'',1);at=changed.index('{')+1+len(prefix)
                changed=changed[:at]+'\n'+label+changed[at:]
                self.assertFalse(authenticated_prefixes(ROOT,path,text.replace(body,changed,1))['296620']['authenticated'])
                moved=binding.INCLUDE+text.replace(binding.INCLUDE,'',1)
                self.assertFalse(authenticated_prefixes(ROOT,path,moved)['296620']['authenticated'])

    def test_codec_copy_exact_groups_and_retired_continuations(self):
        from native_source_prefixes import authenticated_prefixes
        import native_codec_copy_replacements as binding
        from specialize_native_operands import specialize
        for name,group in zip(binding.UNITS,binding.ENTRY_GROUPS,strict=True):
            path=ROOT/name;raw=path.read_text();prepared,receipt=specialize(raw)
            expected=[f'{entry:x}' for entry in group]
            self.assertEqual(receipt['native_codec_copy_entries'],expected)
            for literal,text in ((False,raw),(True,prepared)):
                proof=authenticated_prefixes(ROOT,path,text)
                for entry in group:
                    key=f'{entry:x}'
                    self.assertTrue(proof[key]['authenticated'],proof[key])
                    self.assertTrue(proof[key]['target_body_rebinding_proved'])
                    self.assertFalse(proof[key]['runtime_admission_proved'])
                    self.assertFalse(proof[key]['native_dependency_closure_proved'])
                    self.assertIn('thirty-three',proof[key]['retained_fallback_scope'])
                    self.assertIn('src/physics/runtime/native_tu15_codec_copy_source_owner.hpp',
                                  proof[key]['runtime_include_source_sha256'])
                first=group[0];label=next(iter(binding.LABELS[first].values()))
                body=binding.BODY[first].search(text)[0]
                changed=body.replace(label,'',1)
                at=changed.index('{')+1+len(binding.PREFIXES[first][literal])
                changed=changed[:at]+'\n'+label+changed[at:]
                bad=authenticated_prefixes(ROOT,path,text.replace(body,changed,1))
                self.assertTrue(all(not bad[key]['authenticated'] for key in expected))

    def test_projectile_query_preserves_symbol_and_ordered_fallback_owners(self):
        from native_source_prefixes import authenticated_prefixes
        import native_projectile_function_query_replacements as projectile
        from specialize_native_operands import specialize
        path=ROOT/'src/gameplay/units/source_combat_action_tu15_generated.cpp';raw=path.read_text()
        for text in (raw,specialize(raw)[0]):
            proof=authenticated_prefixes(ROOT,path,text)['100e70']
            self.assertTrue(proof['authenticated'],proof)
            self.assertTrue(proof['target_body_rebinding_proved'])
            self.assertFalse(proof['runtime_admission_proved'])
            self.assertFalse(proof['native_dependency_closure_proved'])
            self.assertIn('ordered-SSE',proof['retained_fallback_scope'])
            self.assertIn('src/rendering/effects/source_effect_compiled_symbols_tu15.cpp',proof['validation_source_sha256'])
            self.assertIn('include/halo2/native_tu15_projectile_function_query_kernel.hpp',proof['runtime_include_source_sha256'])
            bad=text.replace(projectile.ORDERED_MULTIPLY,projectile.LEGACY_MULTIPLY,1)
            self.assertNotEqual(bad,text)
            self.assertFalse(authenticated_prefixes(ROOT,path,bad)['100e70']['authenticated'])

    def test_flight_query_distinguishes_primary_and_death_cycle_owners(self):
        from native_source_prefixes import authenticated_prefixes
        import native_flight_property_query_replacements as primary
        import native_death_cycle_flight_query_replacements as death
        from specialize_native_operands import specialize
        owners=(('src/physics/physics_live_support_tu15_generated.cpp',primary,'Primary'),
                ('src/gameplay/units/death_cycle_runtime_tu15_generated.cpp',death,'DeathCycle'))
        for name,binding,owner in owners:
            path=ROOT/name;raw=path.read_text()
            for text in (raw,specialize(raw)[0]):
                proofs=authenticated_prefixes(ROOT,path,text);proof=proofs['e0900']
                self.assertTrue(proof['authenticated'],proof)
                self.assertTrue(proof['target_body_rebinding_proved'])
                self.assertFalse(proof['runtime_admission_proved'])
                self.assertIn(owner,proof['retained_fallback_scope'])
                self.assertIn('include/halo2/native_tu15_flight_property_query_kernel.hpp',proof['runtime_include_source_sha256'])
                if binding is primary:self.assertTrue(proofs['10de40']['authenticated'])
                bad=text+'\nusing namespace halo2::'+binding.OWNER+';\n'
                self.assertFalse(authenticated_prefixes(ROOT,path,bad)['e0900']['authenticated'])

    def test_map_world_property_query_pair_preserves_child_and_image(self):
        from native_source_prefixes import authenticated_prefixes
        import native_map_world_property_query_replacements as binding
        from specialize_native_operands import specialize
        path=ROOT/'src/runtime/source_map_world_support_tu15_generated.cpp';raw=path.read_text()
        for literal,text in ((False,raw),(True,specialize(raw)[0])):
            proofs=authenticated_prefixes(ROOT,path,text)
            for entry in binding.ENTRIES:
                proof=proofs[entry]
                self.assertTrue(proof['authenticated'],proof)
                self.assertTrue(proof['target_body_rebinding_proved'])
                self.assertFalse(proof['runtime_admission_proved'])
                self.assertFalse(proof['native_dependency_closure_proved'])
                self.assertIn('genuine 11E4DA to 1DB800',proof['retained_fallback_scope'])
                self.assertIn('MapPack11',proof['retained_fallback_scope'])
                self.assertIn('src/rendering/effects/source_object_function_symbols_tu15.inc',proof['validation_source_sha256'])
            bad=text.replace(binding.prefix('1db800',literal),'',1)
            self.assertNotEqual(bad,text)
            self.assertFalse(authenticated_prefixes(ROOT,path,bad)['11e490']['authenticated'])



    def test_hull_clip_catalog_preserves_exact_formats(self):
        from native_source_prefixes import authenticated_prefixes
        from specialize_native_operands import specialize
        import native_solver_hull_clip_replacements as clip
        path=ROOT/'src/physics/physics_live_support_tu15_generated.cpp'
        raw=path.read_text()
        for text,mode in ((raw,False),(specialize(raw)[0],True)):
            evidence=authenticated_prefixes(ROOT,path,text)
            proof=evidence['3147d0']
            self.assertTrue(proof['authenticated'],proof)
            self.assertTrue(proof['target_body_rebinding_proved'])
            self.assertFalse(proof['runtime_admission_proved'])
            self.assertFalse(proof['native_dependency_closure_proved'])
            self.assertTrue(evidence['314870']['authenticated'])
            changed=text.replace(clip.PREFIXES[mode,False],clip.PREFIXES[not mode,False],1)
            self.assertFalse(authenticated_prefixes(ROOT,path,changed)['3147d0']['authenticated'])

    def test_hull_clear_catalog_preserves_exact_formats(self):
        from native_source_prefixes import authenticated_prefixes
        from specialize_native_operands import specialize
        import native_solver_hull_clear_replacements as clear
        path=ROOT/'src/physics/physics_live_support_tu15_generated.cpp'
        raw=path.read_text()
        for text,mode in ((raw,False),(specialize(raw)[0],True)):
            evidence=authenticated_prefixes(ROOT,path,text)
            proof=evidence['314870']
            self.assertTrue(proof['authenticated'],proof)
            self.assertTrue(proof['target_body_rebinding_proved'])
            self.assertFalse(proof['runtime_admission_proved'])
            self.assertFalse(proof['native_dependency_closure_proved'])
            changed=text.replace(clear.PREFIXES[mode],clear.PREFIXES[not mode],1)
            self.assertFalse(authenticated_prefixes(ROOT,path,changed)['314870']['authenticated'])

    def test_device_property_query_catalog_keeps_map_world_siblings(self):
        from native_source_prefixes import authenticated_prefixes
        import native_device_property_query_replacements as device
        from specialize_native_operands import specialize
        path=ROOT/'src/runtime/source_map_world_support_tu15_generated.cpp';raw=path.read_text()
        for literal,text in ((False,raw),(True,specialize(raw)[0])):
            proofs=authenticated_prefixes(ROOT,path,text)
            proof=proofs['10be20']
            self.assertTrue(proof['authenticated'],proof)
            self.assertTrue(proof['target_body_rebinding_proved'])
            self.assertFalse(proof['runtime_admission_proved'])
            self.assertFalse(proof['native_dependency_closure_proved'])
            for entry in device.pair.ENTRIES:self.assertTrue(proofs[entry]['authenticated'],proofs[entry])
            bad=text.replace(device.prefix(literal,False),device.prefix(literal,True),1)
            self.assertNotEqual(bad,text)
            self.assertFalse(authenticated_prefixes(ROOT,path,bad)['10be20']['authenticated'])

    def test_definition_mode_catalog_preserves_exact_combat_action_sibling(self):
        from native_source_prefixes import authenticated_prefixes
        import native_object_definition_mode_query_replacements as definition
        from specialize_native_operands import specialize
        path=ROOT/'src/gameplay/units/source_combat_action_tu15_generated.cpp';raw=path.read_text()
        for literal,text in ((False,raw),(True,specialize(raw)[0])):
            evidence=authenticated_prefixes(ROOT,path,text)
            self.assertTrue(evidence['100e70']['authenticated'],evidence['100e70'])
            proof=evidence['165590']
            self.assertTrue(proof['authenticated'],proof)
            self.assertTrue(proof['target_body_rebinding_proved'])
            self.assertFalse(proof['runtime_admission_proved'])
            self.assertFalse(proof['native_dependency_closure_proved'])
            bad=text.replace(definition.prefix(literal),definition.prefix(not literal),1)
            self.assertNotEqual(text,bad)
            self.assertFalse(authenticated_prefixes(ROOT,path,bad)['165590']['authenticated'])

    def test_object_function_scalar_catalog_keeps_real_children(self):
        from native_source_prefixes import authenticated_prefixes
        import native_object_function_scalar_replacements as scalar
        from specialize_native_operands import specialize
        path=ROOT/'src/physics/physics_live_support_tu15_generated.cpp';raw=path.read_text()
        for literal,text in ((False,raw),(True,specialize(raw)[0])):
            rows=authenticated_prefixes(ROOT,path,text)
            for entry in ('bcb70','c9070','10de40','d3950','107140','1046c0'):
                self.assertTrue(rows[entry]['authenticated'],rows[entry])
                self.assertTrue(rows[entry]['target_body_rebinding_proved'],rows[entry])
            self.assertFalse(rows['bcb70']['runtime_admission_proved'])
            self.assertFalse(rows['bcb70']['native_dependency_closure_proved'])
            self.assertNotIn('10fb70',rows)
            self.assertNotIn('bc8f0',rows)
            self.assertEqual(scalar.replace_object_function_scalar(scalar.remove_object_function_scalar(text)),text)
            bad=text.replace(scalar.prefix(literal),scalar.prefix(not literal),1)
            self.assertFalse(authenticated_prefixes(ROOT,path,bad)['bcb70']['authenticated'])

    def test_object_function_query_catalog_preserves_genuine_siblings(self):
        from native_source_prefixes import authenticated_prefixes
        import native_object_function_query_replacements as query
        from specialize_native_operands import specialize
        path=ROOT/'src/physics/physics_live_support_tu15_generated.cpp';raw=path.read_text()
        for literal,text in ((False,raw),(True,specialize(raw)[0])):
            rows=authenticated_prefixes(ROOT,path,text)
            proof=rows['c9070']
            for entry in ('c9070','d3950','107140','1046c0'):
                self.assertTrue(rows[entry]['authenticated'],rows[entry])
                self.assertTrue(rows[entry]['target_body_rebinding_proved'],rows[entry])
            self.assertFalse(proof['runtime_admission_proved'])
            self.assertFalse(proof['native_dependency_closure_proved'])
            self.assertIn('independent 1046C0',proof['retained_fallback_scope'])
            import native_object_ratio_caller_replacements as caller
            self.assertEqual(caller.replace_object_ratio_caller(caller.remove_object_ratio_caller(text)),text)
            wrong_caller=text.replace(caller.prefix(literal),caller.prefix(not literal),1)
            self.assertFalse(authenticated_prefixes(ROOT,path,wrong_caller)['1046c0']['authenticated'])
            self.assertEqual(query.replace_object_function_query(query.remove_object_function_query(text)),text)
            wrong=text.replace(query.prefix(literal),query.prefix(not literal),1)
            self.assertFalse(authenticated_prefixes(ROOT,path,wrong)['c9070']['authenticated'])

    def test_node_progress_query_catalog_keeps_local_and_external_routes_distinct(self):
        from native_source_prefixes import authenticated_prefixes
        import native_node_progress_query_replacements as node
        from specialize_native_operands import specialize
        path=ROOT/'src/campaign/ai/source_ai_frame_tu15_generated.cpp';raw=path.read_text()
        for prepared,text in ((False,raw),(True,specialize(raw)[0])):
            evidence=authenticated_prefixes(ROOT,path,text)
            proof=evidence['1cf0e0']
            self.assertTrue(proof['authenticated'],proof)
            self.assertTrue(proof['target_body_rebinding_proved'])
            self.assertFalse(proof['runtime_admission_proved'])
            self.assertFalse(proof['native_dependency_closure_proved'])
            prior=node.remove_node_progress_query(text)
            prior_child=authenticated_prefixes(ROOT,path,prior)['1e3150']
            child=evidence['1e3150']
            if not prepared:self.assertTrue(prior_child['authenticated'],prior_child)
            self.assertEqual((child['authenticated'],child.get('error')),
                             (prior_child['authenticated'],prior_child.get('error')))
            wrong=text.replace('return p==0x1d0670u ||','return p==0x1cf0e0u || p==0x1d0670u ||',1)
            self.assertNotEqual(text,wrong)
            self.assertFalse(authenticated_prefixes(ROOT,path,wrong)['1cf0e0']['authenticated'])

    def test_vehicle_lookup_authenticates_whole_public_owner(self):
        from native_source_prefixes import authenticated_prefixes
        import native_vehicle_object_lookup_replacements as vehicle
        path=ROOT/'src/physics/vehicles/physics_vehicle_environment_state_tu15_generated.cpp';text=path.read_text()
        proof=authenticated_prefixes(ROOT,path,text)['bcdf0']
        self.assertTrue(proof['authenticated'],proof)
        self.assertTrue(proof['target_body_rebinding_proved'])
        self.assertFalse(proof['runtime_admission_proved'])
        self.assertFalse(proof['native_dependency_closure_proved'])
        self.assertIn('all35 actual callbacks',proof['retained_fallback_scope'])
        self.assertIn('src/gameplay/objects/native_tu15_object_body_raw_frame.hpp',proof['runtime_include_source_sha256'])
        body=vehicle.PATTERN.findall(text)[0]
        changed=body.replace('0x4ecbd0u','0x4ecbd4u',1)
        self.assertFalse(authenticated_prefixes(ROOT,path,text.replace(body,changed,1))['bcdf0']['authenticated'])
        changed=text.replace(' auto bound=services;',' auto &bound=services;',1)
        self.assertNotEqual(changed,text)
        self.assertFalse(authenticated_prefixes(ROOT,path,changed)['bcdf0']['authenticated'])

    def test_type_query_parent_prefix_keeps_genuine_children_separate(self):
        from native_source_prefixes import authenticated_prefixes
        import native_object_type_query_replacements as query
        from specialize_native_operands import specialize
        path=ROOT/'src/physics/physics_live_support_tu15_generated.cpp';raw=path.read_text()
        prepared,receipt=specialize(raw)
        self.assertEqual(receipt['native_object_type_query_entries'],['10de40'])
        for form,text in(('raw',raw),('prepared',prepared)):
            with self.subTest(form=form):
                proof=authenticated_prefixes(ROOT,path,text)['10de40']
                self.assertTrue(proof['authenticated'],proof)
                self.assertTrue(proof['target_body_rebinding_proved'])
                self.assertFalse(proof['runtime_admission_proved'])
                self.assertFalse(proof['native_dependency_closure_proved'])
                self.assertIn('genuine E0900',proof['retained_fallback_scope'])
                self.assertIn('include/halo2/native_tu15_object_type_query_kernel.hpp',proof['runtime_include_source_sha256'])
                body=query.parts(text)['body'];changed=body.replace('NativeObjectTypeQuery10de46: ','',1)
                self.assertFalse(authenticated_prefixes(ROOT,path,text.replace(body,changed,1))['10de40']['authenticated'])

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
