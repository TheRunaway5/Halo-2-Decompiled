#!/usr/bin/env python3
import hashlib
import re
from pathlib import Path
import sys
import unittest
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
import native_instanced_ray_child_replacements as binding
class InstancedChildBindings(unittest.TestCase):
    def test_exact_prepared_owners(self):
        from specialize_native_operands import specialize
        from validate_native_instanced_ray_child_binding import instrument_retained_body
        bodies=[]
        for name in('src/physics/physics_live_support_tu15_generated.cpp','src/campaign/ai/source_ai_frame_tu15_generated.cpp'):
            source=(ROOT/name).read_text();prepared,_=specialize(binding.replace_instanced_ray_child(source))
            match=binding.BODY.search(prepared);body=binding.canonical_body(match[0]);bodies.append(body)
            self.assertEqual(binding.digest(body),binding.PREPARED_BODY_PIN)
            self.assertIn(binding.PREFIX,match[0])
            for target in(binding.authenticate(source)[0][0],match[0]):
                counted=instrument_retained_body(target)
                self.assertEqual(counted.count('++literal_body_statements;'),13)
                self.assertEqual(counted.replace('++literal_body_statements;',''),target)
            changed=prepared.replace(match[0],match[0].replace('0x46625cu','0x466260u',1),1)
            with self.assertRaises(ValueError):binding.replace_instanced_ray_child(changed)
        self.assertEqual(bodies[0],bodies[1])
        original=binding.authenticate((ROOT/'src/physics/physics_live_support_tu15_generated.cpp').read_text())[1]
        derived,count=re.subn(r'if\(s\.services\.instruction\)\{s\.services\.instruction\((0x[0-9a-f]+u)\);\}',r's.instruction_literal(\1);',original)
        self.assertEqual(count,13)
        self.assertEqual(bodies[0],derived)
    def test_both_complete_owners_and_idempotence(self):
        for name in('src/physics/physics_live_support_tu15_generated.cpp','src/campaign/ai/source_ai_frame_tu15_generated.cpp'):
            source=(ROOT/name).read_text();match,body=binding.authenticate(source);installed=binding.replace_instanced_ray_child(source)
            self.assertEqual(binding.replace_instanced_ray_child(installed),installed)
            self.assertEqual(binding.canonical_body(binding.authenticate(installed)[0][0]),body)
            for old,new in(('0x46625cu','0x466260u'),('0x24deceu','0x24decfu')):
                changed=source.replace(match[0],match[0].replace(old,new,1),1);self.assertNotEqual(changed,source)
                with self.assertRaises(ValueError):binding.replace_instanced_ray_child(changed)
            changed=installed.replace(binding.PREFIX,binding.PREFIX.replace('bridge(s)','bridge(s,false)'),1)
            with self.assertRaises(ValueError):binding.replace_instanced_ray_child(changed)
    def test_full_resolver_and_dispatcher_are_not_subset_witnesses(self):
        source=(ROOT/'src/campaign/ai/source_ai_frame_tu15_generated.cpp').read_text()
        for old,new in(('case 0x46625cu:return 0x46625cu;','case 0x46625cu:return 0x466260u;'),
            ('if(image==SourcePhysicsContactImage::MapPack11)return p;','if(image==SourcePhysicsContactImage::MapPack11)return p+1u;'),
            ('if(target==selected(0x24dec0u,s.image))','if(target==selected(0x24dec4u,s.image))')):
            changed=source.replace(old,new,1);self.assertNotEqual(changed,source)
            with self.assertRaises(ValueError):binding.replace_instanced_ray_child(changed)
    def test_public_facade_only_round_trip_and_reference(self):
        source=(ROOT/'src/physics/native_tu15_instanced_ray_callbacks.cpp').read_text();canonical=binding.canonical_public(source)
        installed=binding.replace_public(canonical);self.assertEqual(binding.replace_public(installed),installed)
        self.assertEqual(binding.canonical_public(installed),canonical)
        ref=ROOT/'tests/reference/native_tu15_instanced_ray_child_value_reference.hpp'
        self.assertEqual(hashlib.sha256(ref.read_bytes()).hexdigest(),binding.PUBLIC_REFERENCE_PIN)
        for changed in(canonical.replace('child+16u,key','child+16u,key+1u',1),
            installed.replace('words,narrow_store,collection,key,child','words,narrow_store,collection,key+1u,child',1),
            canonical.replace('services.collect(owner,collector,body,packet,fp);','services.collect(owner,collector,body+1u,packet,fp);',1)):
            self.assertNotIn(changed,(canonical,installed))
            with self.assertRaises(ValueError):binding.replace_public(changed)
if __name__=='__main__':unittest.main()
