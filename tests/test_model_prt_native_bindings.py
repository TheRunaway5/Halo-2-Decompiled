#!/usr/bin/env python3
"""Regression checks for selected/literal authenticated model native routes."""
from pathlib import Path
import re
import sys
import unittest

ROOT=Path(__file__).resolve().parent.parent
sys.path.insert(0,str(ROOT/'tools'))
from model_prt_sample_native_replacements import replace_prt_sample
from model_prt_cache_native_replacements import replace_prt_cache
from model_prt_update_native_replacements import replace_prt_update


class Bindings(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source=(ROOT/'src/rendering/models/source_model_render_tu15_generated.cpp').read_text()

    def test_both_complete_body_shapes(self):
        for entry,bind in ((0x22cd0,replace_prt_sample),(0x235e0,replace_prt_cache),(0x22830,replace_prt_update)):
            with self.subTest(entry=hex(entry)):
                self.assertEqual(bind(self.source),self.source)
                pattern=rf'static void run_{entry:x}\(WorldState &s\) \{{.*?\n\}}'
                body=re.search(pattern,self.source,re.S).group()
                literal=re.sub(r'selected\((0x[0-9a-f]+u),s.image\)',r'\1',body)
                modified=self.source.replace(body,literal,1)
                self.assertEqual(bind(modified),modified)
                with self.assertRaises(ValueError):bind(modified.replace(literal,literal.replace('0x4ecc14u','0x4ecc18u',1),1)
                    if entry!=0x235e0 else modified.replace(literal,literal.replace('0x4f0414u','0x4f0418u',1),1))

    def test_selected_function_semantics_authenticated(self):
        mutations=(('switch(p){','p+=4;switch(p){'),
                   ('MapPack11)return p;','MapPack11)return p+4;'),
                   ('case 0x2cu:return 0x2cu;','case 0x2cu:return 0x28u;'),
                   ('Unbound original contact executable address','Changed default behavior'))
        for old,new in mutations:
            changed=self.source.replace(old,new,1)
            self.assertNotEqual(changed,self.source)
            for bind in (replace_prt_sample,replace_prt_cache,replace_prt_update):
                with self.subTest(mutation=old,binder=bind.__name__),self.assertRaises(ValueError):bind(changed)


if __name__=='__main__':unittest.main()
