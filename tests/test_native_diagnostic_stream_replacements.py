import importlib.util
from pathlib import Path
import re
import sys
import unittest

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
import native_diagnostic_stream_replacements as binding


class DiagnosticBindings(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source=(ROOT/'src/runtime/source_map_world_support_tu15_generated.cpp').read_text()

    def rejected(self,source):
        with self.assertRaises(ValueError):binding.replace_diagnostic_stream(source)

    def test_complete_original_and_idempotent_bound(self):
        bound=binding.replace_diagnostic_stream(self.source)
        self.assertEqual(bound,binding.replace_diagnostic_stream(bound))
        self.assertEqual(bound.count(binding.INCLUDE),1)
        for entry in binding.ENTRIES:
            original=binding.canonical_body(binding.function(self.source,f'run_{entry:x}'),entry)
            self.assertEqual(binding.canonical_body(binding.function(bound,f'run_{entry:x}'),entry),original)
            self.assertEqual(bound.count(binding.prefix(entry)),1)
        for name in('selected','dispatch',*binding.PUBLIC_PINS):
            self.assertEqual(binding.function(bound,name),binding.function(self.source,name))

    def test_full_literal_body_pins(self):
        source=self.source
        for entry in binding.ENTRIES:
            original=binding.function(source,f'run_{entry:x}')
            literal=re.sub(r'selected\((0x[0-9a-f]+u),s.image\)',r'\1',original)
            source=source.replace(original,literal,1)
        bound=binding.replace_diagnostic_stream(source)
        self.assertEqual(bound,binding.replace_diagnostic_stream(bound))

    def test_each_whole_body_mutation(self):
        for entry in binding.ENTRIES:
            with self.subTest(entry=entry):
                body=binding.function(self.source,f'run_{entry:x}')
                changed=body.replace('0x32ba80u','0x32ba81u',1)if entry==0x2e2d10 else body.replace('s.set(1,','s.set(2,',1)if entry in(0x2e3080,0x2e30e0)else body.replace('s.set(0,','s.set(2,',1)
                self.assertNotEqual(changed,body)
                self.rejected(self.source.replace(body,changed,1))

    def test_wrong_or_duplicate_prefix(self):
        bound=binding.replace_diagnostic_stream(self.source)
        first=binding.prefix(0x2e3080)
        self.rejected(bound.replace(first,first+first,1))
        self.rejected(bound.replace(first,binding.prefix(0x2e30e0),1))

    def test_complete_context_mutations(self):
        for name in('selected','dispatch',*binding.PUBLIC_PINS):
            with self.subTest(name=name):
                body=binding.function(self.source,name)
                self.rejected(self.source.replace(body,body.replace('{','{if(false)return;',1),1))

    def test_live_state_and_duplicate_include(self):
        self.rejected(self.source.replace('partial_valid=0;','partial_valid=1;',1))
        self.rejected(self.source.replace('using WorldState=LiveState;','using WorldState=physics_world_runtime_detail::WorldState;',1))
        self.rejected(binding.INCLUDE+binding.INCLUDE+self.source)

    def test_missing_owner(self):
        body=binding.function(self.source,'run_2e30e0')
        self.rejected(self.source.replace(body,'',1))


if __name__=='__main__':unittest.main()
