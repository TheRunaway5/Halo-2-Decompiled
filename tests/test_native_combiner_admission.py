"""Source-defined combiner admission, including invalid synthetic controls."""
import json
from pathlib import Path
import sys
import unittest

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from native_world_fragment_kernel import lower_combiner_program
from native_combiner_contract import validate_combiner_controls
from check_native_fragment_semantics import directed,unproved_directed


class NativeCombinerAdmission(unittest.TestCase):
    def test_reserved_registers_mappings_and_dependencies_cannot_compile(self):
        base=[0]*60;base[53]=1;base[8]=0x0c200000;base[9]=0x1c00
        cases=[]
        for index,value in ((53,9),(53,0x201),(34,6<<24),(0,14),(45,3),(26,1<<4),
                            (45,5<<15),(26,7<<15),(8,0x40000000),(9,15<<24),
                            (8,14<<24),(8,0x1f000000),(45,1<<20),
                            (26,1<<12),(26,1<<18),(9,1)):
            changed=base.copy();changed[index]=value;cases.append(changed)
        for words in cases:
            with self.subTest(changed=[(i,v) for i,v in enumerate(words) if v!=base[i]]):
                with self.assertRaises(ValueError):validate_combiner_controls(words)
                with self.assertRaises(ValueError):lower_combiner_program(words)

    def test_defined_directed_cases_keep_all_input_mappings_and_output_modes(self):
        rows=directed()
        self.assertEqual(len(rows),24)
        for words in rows:validate_combiner_controls(words)
        self.assertEqual({(word[34]>>29)&7 for word in rows[:8]},set(range(8)))
        self.assertEqual({(word[45]>>15)&7 for word in rows[:8]},{0,1,2,3,4,6})

    def test_original_unsupported_fuzz_cases_remain_explicit_rejections(self):
        rejected=0
        for words in unproved_directed():
            try:validate_combiner_controls(words)
            except ValueError:rejected+=1
        self.assertGreater(rejected,0)

    def test_all_source_owned_compiler_registries_pass_defined_encoding_contract(self):
        files=[ROOT/'shaders/native-programs/world/programs.json',
               *sorted((ROOT/'shaders/native-programs/fragments').glob('*.json'))]
        count=0
        for path in files:
            document=json.loads(path.read_bytes())
            for entry in document['entries']:
                validate_combiner_controls(entry['controls']);count+=1
        self.assertGreater(count,0)


if __name__=='__main__':unittest.main()
