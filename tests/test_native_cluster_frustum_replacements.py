#!/usr/bin/env python3
"""Whole-owner/source namespace gates for native167EF0 range aggregation."""
import hashlib,re,sys,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
import native_cluster_frustum_replacements as binding
from native_sphere_frustum_replacements import PREFIX as CHILD_PREFIX

class ClusterFrustumBindings(unittest.TestCase):
    @classmethod
    def setUpClass(cls):cls.source=(ROOT/'src/rendering/effects/source_effect_particle_draw_tu15_generated.cpp').read_text()
    def test_installs_idempotently_and_preserves_original_full_body(self):
        output=binding.replace_cluster_frustum(self.source)
        self.assertEqual(output,binding.replace_cluster_frustum(output))
        self.assertEqual(binding.BODY_PIN,binding.digest(binding.canonical_body(binding.BODY.search(output)[0])))
        self.assertEqual(binding.BODY_PIN,binding.digest(binding.canonical_body(binding.BODY.search(self.source)[0])))

    def test_complete_body_child_and_namespace_drifts_are_rejected(self):
        for source in(self.source,binding.replace_cluster_frustum(self.source)):
            body=binding.BODY.search(source)[0]
            resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)[0]
            dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)[0]
            child=re.search(r'static void run_184690\(WorldState &s\) \{.*?\n\}',source,re.S)[0]
            mutations=[source.replace(body,body.replace('0xa6eu','0xa70u',1),1),source.replace(body,body.replace('s.gp[4]+=8u','s.gp[4]+=4u',1),1),source.replace(body,body.replace('run_184690(s);','run_167ef0(s);',1),1),source.replace(resolver,resolver.replace('return p;','return p+1u;',1),1),source.replace(dispatcher,dispatcher.replace('s.required(site,target,tail);','s.required(site,target+1u,tail);',1),1),source.replace(child,child.replace(CHILD_PREFIX,'',1),1),source.replace(child,child.replace('0x64u','0x65u',1),1),source+'\n'+body]
            for index,changed in enumerate(mutations):
                with self.subTest(index=index):
                    self.assertNotEqual(source,changed)
                    with self.assertRaises(ValueError):binding.replace_cluster_frustum(changed)

    def test_public_range_owner_and_independent_sphere_facade_are_pinned(self):
        from audit_native_execution_ownership import unique_function
        source=(ROOT/'src/rendering/world/source_visibility.cpp').read_text();bound=binding.replace_world_visibility(source)
        self.assertEqual(bound,binding.replace_world_visibility(bound))
        for name in('source_cluster_frustum_visibility','source_sphere_frustum_visibility'):
            body=unique_function(bound,name);changed=bound.replace(body,body.replace('radius','changed_radius',1),1)
            with self.assertRaises(ValueError):binding.replace_world_visibility(changed)

if __name__=='__main__':unittest.main()
