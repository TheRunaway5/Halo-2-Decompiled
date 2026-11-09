#!/usr/bin/env python3
"""Exact prepared polygon owners remain idempotent and reject source drift."""
import hashlib,os,re,sys,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
import native_polygon_support_replacements as support
import native_polygon_features_replacements as features
from native_polygon_prepared_owners import installed,PREPARED
from specialize_native_operands import specialize

class PreparedPolygonOwners(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        import fix_physics_world_observation_tu15 as fixer
        world=(ROOT/'src/physics/runtime/physics_world_runtime_generated.cpp').read_text()
        contact=(ROOT/'src/physics/runtime/physics_contact_runtime_generated.cpp').read_text()
        fresh=features.replace_polygon_features(support.replace_polygon_support(fixer.canonical(world)))
        cls.sources={'ObservedWorldState':world,'State':contact,'WorldState':fresh}
        cls.prepared={state:specialize(source)[0]for state,source in cls.sources.items()}

    def test_second_pass_preserves_both_receipts_and_all_bytes(self):
        for state,source in self.prepared.items():
            with self.subTest(state=state):
                again,receipt=specialize(source)
                self.assertEqual(source,again)
                self.assertEqual(receipt['native_polygon_support_entries'],['1f6610'])
                self.assertEqual(receipt['native_polygon_feature_entries'],['1f6690'])

    def test_output_triples_match_first_pass_of_strict_raw_installers(self):
        for state,source in self.sources.items():
            for family,binding in(('support',support),('features',features)):
                with self.subTest(state=state,family=family):
                    self.assertTrue(installed(source,binding,family))
                    match,original=binding.authenticate(source)
                    self.assertEqual(hashlib.sha256(original.encode()).hexdigest(),binding.BODY_PINS[state])
                    prepared=self.prepared[state];body=binding.BODY.search(prepared)[0]
                    self.assertEqual(hashlib.sha256(body.encode()).hexdigest(),PREPARED[family][state][0])
                    self.assertTrue(installed(prepared,binding,family))
                    # Output recognition must not broaden the source installer.
                    with self.assertRaises(ValueError):getattr(binding,'replace_polygon_'+family)(prepared)

    def test_raw_and_prepared_body_prefix_include_and_context_mutations_fail(self):
        for state,source in self.sources.items():
            for variant in(source,self.prepared[state]):
                for family,binding in(('support',support),('features',features)):
                    match=binding.BODY.search(variant);body=match[0]
                    changed_body=body.replace('return;','return ;',1)
                    changed_prefix=body.replace(binding.PREFIX,binding.PREFIX.replace('bridge(s)','bridge( s)'),1)
                    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',variant,re.S)[0]
                    dispatcher=re.search(r'static void dispatch\('+state+r' &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',variant)[0]
                    for name,changed in(
                        ('body',variant.replace(body,changed_body,1)),
                        ('prefix',variant.replace(body,changed_prefix,1)),
                        ('missing_include',variant.replace(binding.INCLUDE,'',1)),
                        ('duplicate_include',binding.INCLUDE+variant),
                        ('resolver',variant.replace(resolver,resolver.replace('return p;','return p+1u;',1),1)),
                        ('dispatcher',variant.replace(dispatcher,dispatcher.replace('s.required(site,target,tail);','s.required(site,target+1u,tail);',1),1))):
                        with self.subTest(state=state,family=family,name=name):
                            self.assertNotEqual(variant,changed)
                            with self.assertRaises(ValueError):installed(changed,binding,family)
                            with self.assertRaises(ValueError):specialize(changed)

if __name__=='__main__':unittest.main()
