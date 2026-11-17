#!/usr/bin/env python3
"""Strict owner/prefix drift controls; no generated build or reference copying."""
import importlib.util,sys,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
import native_arena_initialize_replacements as binding

class ArenaReplacements(unittest.TestCase):
    def setUp(self):
        self.generated=(ROOT/'src/physics/physics_game_arena_runtime_generated.cpp').read_text()
        self.owner=(ROOT/'src/physics/physics_game_arena_native.cpp').read_text()
    def test_generated_roundtrip(self):
        original=binding.remove_arena_initialize(self.generated)
        bound=binding.replace_arena_initialize(original)
        self.assertEqual(binding.installed_entries(bound),['126640'])
        self.assertEqual(binding.replace_arena_initialize(bound),bound)
        self.assertEqual(binding.remove_arena_initialize(bound),original)
    def test_generated_drift(self):
        bound=binding.replace_arena_initialize(self.generated);body=binding.BODY.search(bound)[0]
        controls={
            'body':bound.replace(body,body.replace('0xff800u','0xff801u',1),1),
            'resolver':bound.replace('case 0x125280u:return 0x126640u;','case 0x125280u:return 0x126641u;',1),
            'dispatcher':bound.replace('s.required(site,target,tail);}','s.required(site,target+1u,tail);}',1),
            'export':bound.replace('case 0x125280u:run_125280(s);break;','case 0x125280u:run_125280(s);return;',1),
            'missing_include':bound.replace(binding.INCLUDE,'',1),
            'relocated_include':bound[len(binding.INCLUDE):]+binding.INCLUDE,
            'partial_prefix':bound.replace(binding.PREFIX,binding.PREFIX+';',1),
            'duplicate_body':bound+'\n'+body,
        }
        for label,changed in controls.items():
            with self.subTest(label=label):
                self.assertNotEqual(changed,bound)
                with self.assertRaises(ValueError):binding.replace_arena_initialize(changed)
                with self.assertRaises(ValueError):binding.installed_entries(changed)
    def test_fixed_mappack_refused(self):
        source=(ROOT/'src/gameplay/units/death_startup_runtime_generated.cpp').read_text()
        with self.assertRaises(ValueError):binding.replace_arena_initialize(source)
    def test_producer_roundtrip(self):
        original=binding.canonical_owner(self.owner);bound=binding.replace_arena_owner(original)
        self.assertEqual(binding.canonical_owner(bound),original)
        self.assertEqual(binding.replace_arena_owner(bound),bound)
    def test_producer_drift(self):
        bound=binding.replace_arena_owner(self.owner)
        controls={
            'named_capture':bound.replace('Impl *const owner;','Impl *owner;',1),
            'const_local':bound.replace('const auto joined=impl_->services();','auto joined=impl_->services();',1),
            'private_publication':bound.replace('native_arena_publication_detail::Borrow published{','native_arena_publication_detail::Borrow changed{',1),
            'lower':bound.replace('preserves_instruction_policy(lower)','preserves_instruction_policy(SourcePhysicsWorldRuntimeServices{})',1),
            'platform':bound.replace('if(call.target==0x21b920u)','if(call.target==0x21b921u)',1),
            'field':bound.replace(binding.PARENT_EXECUTOR,'',1),
            'include':bound.replace(binding.INCLUDES,'',1),
            'duplicate':bound+binding.PUBLISHED_CALL,
        }
        for label,changed in controls.items():
            with self.subTest(label=label):
                self.assertNotEqual(changed,bound)
                with self.assertRaises(ValueError):binding.replace_arena_owner(changed)
    def test_bootstrap_roundtrip_and_drift(self):
        source=(ROOT/'src/physics/native_tu15_arena_bootstrap.cpp').read_text()
        original=binding.canonical_bootstrap(source);bound=binding.replace_arena_bootstrap(original)
        self.assertEqual(binding.canonical_bootstrap(bound),original)
        self.assertEqual(binding.replace_arena_bootstrap(bound),bound)
        for old,new in((binding.BOOTSTRAP_ALIAS,binding.BOOTSTRAP_ALIAS+';'),(binding.BOOTSTRAP_NAMED,binding.BOOTSTRAP_NAMED+' '),('idle->finish_startup();','idle->before_wait(0);'),(binding.BOOTSTRAP_INCLUDE,'')):
            with self.subTest(old=old):
                changed=bound.replace(old,new,1);self.assertNotEqual(changed,bound)
                with self.assertRaises(ValueError):binding.replace_arena_bootstrap(changed)

if __name__=='__main__':unittest.main()
