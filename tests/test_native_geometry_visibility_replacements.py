#!/usr/bin/env python3
"""Complete geometry admission ownership and exact public overlay controls."""
from pathlib import Path
import re,sys,unittest
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
import native_geometry_visibility_replacements as binding
import native_geometry_visibility_public_replacements as public
UNITS=('src/rendering/shadows/source_shadow_frame_tu15_generated.cpp','src/rendering/effects/source_effect_particle_draw_tu15_generated.cpp')
class GeometryBinding(unittest.TestCase):
    def source(self,unit):
        source=(ROOT/unit).read_text();match,body=binding.authenticate(source)
        return source[:match.start()]+body+source[match.end():]
    def test_complete_two_owner_roundtrip(self):
        for unit in UNITS:
            source=self.source(unit);bound=binding.replace_geometry_visibility(source)
            self.assertEqual(binding.replace_geometry_visibility(bound),bound)
            self.assertEqual(binding.authenticate(source)[1],binding.authenticate(bound)[1])
    def test_changed_data_or_real_child_contract_rejects(self):
        for unit in UNITS:
            source=self.source(unit);match,body=binding.authenticate(source)
            for old,new in(('0x2a7cu','0x2a78u'),('0xffffu','0xfffeu'),('0x1359b1u','0x1359b2u')):
                self.assertIn(old,body)
                with self.assertRaises(ValueError):binding.replace_geometry_visibility(source.replace(match[0],body.replace(old,new,1),1))
    def test_entire_resolver_dispatcher_are_required(self):
        for unit in UNITS:
            source=self.source(unit)
            resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)[0]
            dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)[0]
            for old,new in((resolver,resolver.replace('return p;','return p+1;',1)),(dispatcher,dispatcher.replace('{','{if(false)return;',1))):
                with self.assertRaises(ValueError):binding.replace_geometry_visibility(source.replace(old,new,1))
    def test_exact_native_prefix_only(self):
        for unit in UNITS:
            source=binding.replace_geometry_visibility(self.source(unit))
            with self.assertRaises(ValueError):binding.replace_geometry_visibility(source.replace(binding.PREFIX,binding.PREFIX+'s.gp[0]=0;',1))
    def test_complete_public_predecessor_roundtrip(self):
        for kind,path in(('bsp','src/rendering/world/source_bsp_admission.cpp'),('object','src/gameplay/objects/source_object_admission.cpp')):
            source=(ROOT/path).read_text();bound=public.replace_public(source,kind);owner=public.OWNERS[kind]
            self.assertEqual(public.replace_public(bound,kind),bound)
            self.assertEqual(public.canonical_public(public.unique_function(bound,owner),kind),public.canonical_public(public.unique_function(source,owner),kind))
    def test_other_public_queue_policies_remain_pinned(self):
        for kind,path in(('bsp','src/rendering/world/source_bsp_admission.cpp'),('object','src/gameplay/objects/source_object_admission.cpp')):
            source=public.replace_public((ROOT/path).read_text(),kind);owner=public.OWNERS[kind];body=public.unique_function(source,owner)
            for old,new in(('in.capacity'if kind=='object'else'in.instance_capacity','0u'),('out.associations','out.associations /* altered owner */')):
                self.assertIn(old,body)
                with self.assertRaises(ValueError):public.replace_public(source.replace(body,body.replace(old,new,1),1),kind)
    def test_exact_allocator_and_geometry_overlays_compose(self):
        import native_visibility_association_public_replacements as allocation
        for kind,path in(('bsp','src/rendering/world/source_bsp_admission.cpp'),('object','src/gameplay/objects/source_object_admission.cpp')):
            source=(ROOT/path).read_text()
            self.assertEqual(public.replace_public(allocation.replace_public(source,kind),kind),source)
            self.assertEqual(allocation.replace_public(public.replace_public(source,kind),kind),source)
if __name__=='__main__':unittest.main()
