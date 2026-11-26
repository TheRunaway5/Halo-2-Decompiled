import hashlib
from pathlib import Path
import sys
import unittest

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
import native_mopp_ray_traversal_replacements as binder

class MoppRayBindings(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.sources=[(ROOT/name).read_text()for name in(
            'src/campaign/ai/source_ai_frame_tu15_generated.cpp',
            'src/gameplay/units/death_cycle_runtime_tu15_generated.cpp')]
    def rejected(self,text):
        with self.assertRaises(ValueError):binder.replace_mopp_ray_traversal(text)
    def test_complete_original_reconstruction_and_idempotence(self):
        for source in self.sources:
            bound=binder.replace_mopp_ray_traversal(source)
            self.assertEqual(bound,binder.replace_mopp_ray_traversal(bound))
            self.assertEqual(bound.count(binder.PREFIX),1)
            body=binder.function(bound,'run_30aa70')
            restored=bound.replace(body,binder.canonical_body(body),1).replace(binder.INCLUDE,'',1)
            self.assertEqual(restored,source)
    def test_separate_legacy_c1_bodies_remain_exact(self):
        bodies=[binder.function(source,'run_30aa70')for source in self.sources]
        # Both copies clear C1 at their four FSTP ST(0) sites (hardware: a
        # register store does not round). Only the AI-frame copy also repeats
        # the clear after each load, which ExtendedStack::push already does.
        self.assertEqual([body.count('s.fp.status&=~0x200u;')for body in bodies],[94,4])
        self.assertEqual([body.count('s.fp.st(0)=s.fp.st(0);s.fp.status&=~0x200u;s.fp.pop();')for body in bodies],[4,4])
        self.assertEqual(bodies[0].replace('s.fp.status&=~0x200u;',''),bodies[1].replace('s.fp.status&=~0x200u;',''))
        for source,body in zip(self.sources,bodies):self.rejected(source.replace(body,body.replace('s.fp.status&=~0x200u;','',1)if 's.fp.status&=~0x200u;'in body else body.replace('s.fp.push(','s.fp.status&=~0x200u;s.fp.push(',1),1))
    def test_complete_body_mutation(self):
        for source in self.sources:
            body=binder.function(source,'run_30aa70')
            self.rejected(source.replace(body,body.replace('0x6bu','0x6cu',1),1))
    def test_final_routes_bind_both_owner_layouts(self):
        from specialize_native_operands import specialize
        for source in self.sources:
            bound,_=specialize(source)
            body=binder.function(bound,'run_30aa70')
            for entry,target in (
                    ('30acc6','30adc9'),('30ad0c','30ade0'),('30b5a3','30b5f5'),('30b5a9','30b5f5'),
                    ('30b5b8','30b5f5'),('30b5d0','30b5f5'),('30b5f2','30b5f5'),
                    ('30aede','30af16'),('30af42','30af7c'),('30af94','30ae02')):
                line=next(row for row in body.splitlines() if f's.instruction_literal(0x{entry}u);' in row)
                self.assertIn(f'case 0x{target}u: goto L{target};',line)
                self.assertIn('Invalid MOPP final route continuation',line)
            for frontier in ('30ad10','30ad2c','30b5e1','30aeee','30af55','30afd7','30b004'):
                self.assertEqual(body.count(f'L_native_final_{frontier}:'),1)
    def test_call_site_setup_preserves_each_actual_child(self):
        from specialize_native_operands import specialize
        support=(ROOT/'src/physics/physics_live_support_tu15_generated.cpp').read_text()
        for source in (*self.sources,support):
            bound,receipt=specialize(source)
            body=binder.function(bound,'run_30b890')
            self.assertEqual(bound.count(binder.SETUP_INCLUDE),1)
            self.assertIn('30b890',receipt['native_partial_frame_owners'])
            self.assertIn('case setup_complete: return;',body)
            child='dispatch(s,0x30aa70u,0x30b952u,false);' if source is support else 'run_30aa70(s);'
            self.assertIn(f'[&]{{{child}}}',body)
            for pc,dst,src in (('30b909',4,1),('30b944',1,2)):
                self.assertIn(f's.instruction_literal(0x{pc}u); s.xmm[{dst}]=halo2::native_tu15_leaves::ordered_mul_ps(s.xmm[{dst}],s.xmm[{src}])',body)
            for frontier in binder.SETUP_FRONTIERS:
                self.assertEqual(body.count(f'L_native_setup_{frontier}:'),1)
    def test_full_resolver_and_dispatcher_mutations(self):
        for source in self.sources:
            for name in('selected','dispatch'):
                body=binder.function(source,name)
                self.rejected(source.replace(body,body.replace('{','{if(false)return;',1),1))
    def test_state_ownership_mutation(self):
        for source in self.sources:self.rejected(source.replace('using WorldState=LiveState;','using WorldState=physics_world_runtime_detail::WorldState;',1))
    def test_duplicate_and_wrong_prefix(self):
        for source in self.sources:
            bound=binder.replace_mopp_ray_traversal(source)
            self.rejected(bound.replace(binder.PREFIX,binder.PREFIX+binder.PREFIX,1))
            self.rejected(bound.replace(binder.PREFIX,binder.PREFIX.replace('bridge(s,','bridge( s,',1),1))
            self.rejected(binder.INCLUDE+bound)
    def test_missing_owner(self):
        for source in self.sources:self.rejected(source.replace(binder.function(source,'run_30aa70'),'',1))

if __name__=='__main__':unittest.main()
