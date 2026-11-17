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
