#!/usr/bin/env python3
from pathlib import Path
import sys
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from audit_native_instruction_shaped_owners import candidates


class InstructionShapeAudit(unittest.TestCase):
    def test_cpp_owner_and_nested_lambda(self):
        text='''void retained(Memory &m, Abi &a) {
    const auto instruction=[&](auto pc){if(s.instruction)s.instruction(pc);};
    instruction(0x100);a.gp[0]=0;
    if(a.gp[1]){instruction(0x101);iterator_flags(a,1,2,4,true);}
    instruction(0x102);a.gp[2]=3;
}
'''
        rows,unowned,count=candidates(text)
        self.assertEqual(count,3);self.assertFalse(unowned);self.assertEqual(rows[0]['function'],'retained')
        self.assertTrue(rows[0]['instruction_shape_review_required']);self.assertEqual(rows[0]['gp_operations'],3)

    def test_comments_and_nonbookkeeping_observers(self):
        text='''// instruction(0x001);a.gp[0]=0;
inline bool observe(State &s) {s.instruction(0x123u);return true;}
'''
        rows,unowned,count=candidates(text)
        self.assertEqual(count,1);self.assertFalse(unowned);self.assertFalse(rows[0]['instruction_shape_review_required'])

    def test_unrecognized_owner_is_retained(self):
        rows,unowned,count=candidates('auto callback = []{instruction(0x123);};')
        self.assertFalse(rows);self.assertEqual(count,1);self.assertEqual(unowned,[{'line':1,'pc':'0x123'}])

    def test_attributed_legacy_function_and_label(self):
        text='''[[gnu::noinline]] static void run_123(State &s) {
L123: if(s.services.instruction){s.services.instruction(0x123u);} s.gp[0]=1;
if(s.services.instruction){s.services.instruction(0x124u);} s.gp[1]=2;
if(s.services.instruction){s.services.instruction(0x125u);} s.gp[2]=3;
}
'''
        rows,unowned,count=candidates(text)
        self.assertEqual(count,3);self.assertFalse(unowned);self.assertEqual(len(rows),1)
        self.assertEqual(rows[0]['function'],'run_123');self.assertTrue(rows[0]['instruction_shape_review_required'])


if __name__=='__main__':unittest.main()
