#!/usr/bin/env python3
"""Complete owner, resolver, dispatcher and exact prefix authentication."""
import re,sys,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
import native_bsp_queue_find_replacements as binding
class Binding(unittest.TestCase):
    def setUp(self):
        self.source=(ROOT/'src/rendering/shadows/source_shadow_frame_tu15_generated.cpp').read_text()
        match,body=binding.authenticate(self.source)
        self.source=self.source[:match.start()]+body+self.source[match.end():]
    def test_exact_install_roundtrip(self):
        bound=binding.replace_bsp_queue_find(self.source);self.assertEqual(binding.replace_bsp_queue_find(bound),bound)
        self.assertEqual(binding.authenticate(bound)[1],binding.authenticate(self.source)[1])
        self.assertEqual(bound.count(binding.INCLUDE),1)
    def test_mutated_complete_owners_reject(self):
        body=binding.authenticate(self.source)[1]
        for old,new in(('selected(0x4u,s.image)','selected(0x8u,s.image)'),('s.reg(6,4,0)','s.reg(7,4,0)'),('s.pop();','s.pop();s.gp[4]+=4u;')):
            self.assertIn(old,body)
            with self.assertRaises(ValueError):binding.replace_bsp_queue_find(self.source.replace(body,body.replace(old,new,1),1))
        resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',self.source,re.S)[0]
        with self.assertRaises(ValueError):binding.replace_bsp_queue_find(self.source.replace(resolver,resolver.replace('return p;','return p+1;',1),1))
        for old,new in(('if(target==selected(0x166c00u,s.image)){run_166c00(s);return;}','if(target==selected(0x166c00u,s.image)){run_166c00(s);run_166c00(s);return;}'),):
            self.assertEqual(self.source.count(old),1)
            with self.assertRaises(ValueError):binding.replace_bsp_queue_find(self.source.replace(old,new,1))
    def test_installed_prefix_cannot_be_extended_or_changed(self):
        bound=binding.replace_bsp_queue_find(self.source)
        for old,new in((binding.PREFIX,binding.PREFIX+'s.gp[0]=0;'),('native_bsp_queue_find_detail::bridge(s)','native_bsp_queue_find_detail::bridge(s,1)')):
            with self.assertRaises(ValueError):binding.replace_bsp_queue_find(bound.replace(old,new,1))
    def test_public_owners_keep_exact_original_pins(self):
        for name,path in(('instance','src/rendering/world/source_bsp_admission.cpp'),('object','src/gameplay/objects/source_object_admission.cpp')):
            source=(ROOT/path).read_text();bound=binding.replace_public(source,name)
            self.assertEqual(binding.replace_public(bound,name),bound)
            original=binding.public_body(source,name)
            if original!=binding.public_body(bound,name):
                with self.assertRaises(ValueError):binding.replace_public(source.replace(original,original.replace('r.id==id','r.id!=id')),name)
            installed=binding.public_body(bound,name)
            with self.assertRaises(ValueError):binding.replace_public(bound.replace(installed,installed.replace('native_id_search_detail::record','native_id_search_detail::changed_record')),name)
if __name__=='__main__':unittest.main()
