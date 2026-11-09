#!/usr/bin/env python3
"""Observation regeneration preserves exact independently authenticated owners."""
import hashlib
from pathlib import Path
import sys
import unittest
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
import fix_physics_world_observation_tu15 as fix

class WorldObservationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.current=(ROOT/'src/physics/runtime/physics_world_runtime_generated.cpp').read_text()
        cls.base,cls.state=fix.observation_baseline(cls.current)

    def test_same_complete_old_baseline_remains_authoritative(self):
        self.assertEqual(hashlib.sha256(self.base.encode()).hexdigest(),fix.BASELINE_SHA256)
        self.assertEqual(fix.canonical(self.current),self.base)
        self.assertTrue(self.state['renderer']);self.assertTrue(self.state['schema'])
        self.assertEqual(self.state['tails'],list(range(7)))

    def test_independent_overlay_variants_restore_exactly(self):
        for renderer,schema,tails in ((False,False,[]),(True,False,[]),(False,True,[]),(False,False,[0,1,4,5,6]),(False,False,[2,3]),(True,True,list(range(7)))):
            headers=[]
            if renderer:headers.append(fix.RENDER_HEADER)
            if schema:headers.append(fix.SCHEMA_HEADER)
            headers+=sorted({fix.TAIL_PATCHES[n][3]for n in tails})
            state=dict(renderer=renderer,schema=schema,tails=tails,headers=headers,modern_header=True)
            source=fix.restore_native_overlays(self.base.replace('WorldState','ObservedWorldState'),state)
            restored,actual=fix.observation_baseline(source)
            self.assertEqual(restored,self.base)
            self.assertEqual(actual['renderer'],renderer);self.assertEqual(actual['schema'],schema)
            self.assertEqual(actual['tails'],tails)

    def test_handoff_target_and_site_mutations_fail_closed(self):
        for old,new in (('handoff_native_simplex_helpers(s,0x30d3f0u,0x30dad9u)','handoff_native_simplex_helpers(s,0x30d3f0u,0x30dad8u)'),('handoff_native_contact_features(s,target,0x30e96du)','handoff_native_contact_features(s,target,0x30e96cu)'),('handoff_native_simplex_helpers(s,0x312620u,0x30eaccu)','handoff_native_simplex_helpers(s,0x312624u,0x30eaccu)')):
            value=self.current.replace(old,new,1);self.assertNotEqual(value,self.current)
            with self.assertRaises(ValueError):fix.observation_baseline(value)

    def test_body_and_owner_resolver_mutations_fail_closed(self):
        match=fix.body_match(self.base,'309d40');body=match[0]
        changed=body.replace('0x7u,4','0x8u,4',1);self.assertNotEqual(changed,body)
        with self.assertRaises(ValueError):fix.observation_baseline(self.base.replace(body,changed,1))
        for old,new in (('case 0x4e78d0u:return 0x4ecbd0u;','case 0x4e78d0u:return 0x4ecbd4u;'),('switch(p)','p^=1u;switch(p)')):
            changed=self.base.replace(old,new,1);self.assertNotEqual(changed,self.base)
            with self.assertRaises(ValueError):fix.observation_baseline(changed)

    def test_unknown_prefix_and_missing_include_fail_closed(self):
        for value in(self.current.replace('if(try_native_constraint_schema_emitter_tu15(s))return;','if(try_native_constraint_schema_emitter_tu15(s))return;return;',1),self.current.replace(fix.SIMPLEX_HEADER,''),self.current.replace(fix.CONTACT_HEADER,'')):
            with self.assertRaises(ValueError):fix.observation_baseline(value)

    def test_exact_repaired_quaternion_generator_layout(self):
        from fix_map_quaternion_spills_tu15 import HELPER
        old='static '+HELPER+'void run_378ab0(WorldState &s) {'
        self.assertEqual(self.base.count(old),1)
        generated=self.base.replace(old,'static void run_378ab0(WorldState &s) {',1)
        generated=generated.replace('std::uint32_t selected(',HELPER+'std::uint32_t selected(',1)
        baseline,state=fix.observation_baseline(generated)
        self.assertEqual(baseline,self.base)
        self.assertTrue(state['quaternion_helper_layout_normalized'])
        with self.assertRaises(ValueError):fix.observation_baseline(generated.replace('actual&0x200u','actual&0x100u',1))
        with self.assertRaises(ValueError):fix.observation_baseline(generated.replace('native_single_spill(s.fp.st(0),s.fp.status)','u(spill(s.fp.st(0)))',1))

    def test_mopp_reverse_runs_before_state_normalization(self):
        import native_mopp_aabb_replacements as mopp
        self.assertIn(mopp.PREFIX,self.current)
        self.assertEqual(mopp.replace_mopp_aabb(self.current),self.current)
        changed=self.current.replace(mopp.PREFIX,mopp.PREFIX.replace('dispatch(v,target,site,tail);','dispatch(v,target,site+1u,tail);'),1)
        with self.assertRaises(ValueError):fix.observation_baseline(changed)

    def test_polygon_reverse_requires_exact_installed_owner(self):
        import native_polygon_support_replacements as polygon
        self.assertIn(polygon.PREFIX,self.current)
        self.assertEqual(polygon.replace_polygon_support(self.current),self.current)
        self.assertEqual(fix.canonical(polygon.remove_polygon_support(self.current)),self.base)
        for value in(self.current.replace(polygon.INCLUDE,'',1),self.current.replace(polygon.PREFIX,'',1),
            self.current.replace(polygon.ORDERED_SHIFT,polygon.ORDERED_SHIFT+'+1u',1)):
            with self.assertRaises(ValueError):fix.observation_baseline(value)

    def test_polygon_preparation_preserves_qualified_target_bodies(self):
        import native_polygon_support_replacements as polygon
        from specialize_native_operands import specialize
        # Exact target hashes from the independent raw/prepared physical proof;
        # the source installer retains its original body/resolver/dispatch pins.
        variants=(('src/physics/runtime/physics_world_runtime_generated.cpp',
            '207092075ad2a3e1a837d4943583b21412cd336c9867b3cf13fa8023b74de551'),
            ('src/physics/runtime/physics_contact_runtime_generated.cpp',
            '171098f13a4096d7ed80aeb6e861853f574e44dcf1c9bc4d6afa50a5660983d1'))
        for name,expected in variants:
            source=(ROOT/name).read_text();self.assertEqual(polygon.replace_polygon_support(source),source)
            prepared,receipt=specialize(source)
            self.assertEqual(receipt['native_polygon_support_entries'],['1f6610'])
            body=polygon.BODY.search(prepared)[0]
            self.assertEqual(hashlib.sha256(body.encode()).hexdigest(),expected)
            changed=source.replace(polygon.ORDERED_SHIFT,polygon.ORDERED_SHIFT+'+1u',1)
            self.assertNotEqual(changed,source)
            with self.assertRaises(ValueError):specialize(changed)

if __name__=='__main__':unittest.main()
