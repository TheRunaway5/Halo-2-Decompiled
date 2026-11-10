#!/usr/bin/env python3
"""Complete association bodies and their two distinct owning contexts."""
from pathlib import Path
import re,sys,unittest
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
import native_visibility_association_replacements as binding
import native_visibility_association_public_replacements as public
UNITS=('src/rendering/shadows/source_shadow_frame_tu15_generated.cpp','src/runtime/source_map_world_support_tu15_generated.cpp')
class AssociationBinding(unittest.TestCase):
    def source(self,unit):
        source=(ROOT/unit).read_text();match,body=binding.authenticate(source)
        return source[:match.start()]+body+source[match.end():]
    def test_roundtrip_both_owner_contexts(self):
        for unit in UNITS:
            source=self.source(unit);bound=binding.replace_visibility_association(source)
            self.assertEqual(binding.replace_visibility_association(bound),bound)
            self.assertEqual(binding.authenticate(bound)[1],binding.authenticate(source)[1])
    def test_literal_pin_is_exact_identity_projection(self):
        for unit in UNITS:
            body=binding.authenticate(self.source(unit))[1]
            literal=re.sub(r'selected\((0x[0-9a-f]+u),s.image\)',r'\1',body)
            self.assertEqual(binding.digest(literal),binding.LITERAL_PIN)
            self.assertEqual(binding.canonical_body(binding.ordered_reference(literal)),literal)
    def test_body_mutations_reject(self):
        for unit in UNITS:
            source=self.source(unit);match,body=binding.authenticate(source)
            for old,new in(('0xa5eu','0xa60u'),('0x200u','0x201u'),('0xa6cu','0xa68u')):
                self.assertIn(old,body)
                with self.assertRaises(ValueError):binding.replace_visibility_association(source[:match.start()]+body.replace(old,new,1)+source[match.end():])
    def test_entire_resolver_and_dispatcher_are_pinned(self):
        for unit in UNITS:
            source=self.source(unit)
            resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)[0]
            with self.assertRaises(ValueError):binding.replace_visibility_association(source.replace(resolver,resolver.replace('return p;','return p+1;',1),1))
            dispatch=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)[0]
            with self.assertRaises(ValueError):binding.replace_visibility_association(source.replace(dispatch,dispatch.replace('{','{if(false)return;',1),1))
    def test_native_prefix_is_exact(self):
        for unit in UNITS:
            source=binding.replace_visibility_association(self.source(unit))
            with self.assertRaises(ValueError):binding.replace_visibility_association(source.replace(binding.PREFIX,binding.PREFIX+'s.gp[0]=0;',1))
    def test_public_complete_owners_roundtrip(self):
        for kind,path in(('bsp','src/rendering/world/source_bsp_admission.cpp'),('object','src/gameplay/objects/source_object_admission.cpp')):
            source=(ROOT/path).read_text();bound=public.replace_public(source,kind)
            self.assertEqual(public.replace_public(bound,kind),bound)
            owner,pin=public.OWNERS[kind]
            self.assertEqual(public.digest(public.canonical_public(public.unique_function(bound,owner),kind)),pin)
    def test_public_unrelated_semantics_remain_pinned(self):
        for kind,path in(('bsp','src/rendering/world/source_bsp_admission.cpp'),('object','src/gameplay/objects/source_object_admission.cpp')):
            source=public.replace_public((ROOT/path).read_text(),kind)
            owner,_=public.OWNERS[kind];body=public.unique_function(source,owner)
            for old,new in(('out.associations','out.associations /* changed owner */'),('ordinal','ordinal /* altered */')):
                self.assertIn(old,body)
                with self.assertRaises(ValueError):public.replace_public(source.replace(body,body.replace(old,new,1),1),kind)
if __name__=='__main__':unittest.main()
