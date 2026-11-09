#!/usr/bin/env python3
"""Native async insertion pins complete source bodies and operand mappings."""
from pathlib import Path
import re
import sys
import unittest
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from native_async_insert_replacements import install_native_async_insert,canonical_body,prefix


class InsertBindings(unittest.TestCase):
    def test_complete_bodies(self):
        for path,entry in (('src/physics/physics_game_arena_runtime_generated.cpp','122020'),
                           ('src/rendering/shadows/source_shadow_frame_tu15_generated.cpp','1232c0')):
            source=install_native_async_insert((ROOT/path).read_text())
            body=re.search(r'static void run_'+entry+r'\(WorldState &s\) \{.*?\n\}',source,re.S)[0]
            self.assertIn(prefix(entry),body)
            self.assertEqual(source,install_native_async_insert(source))
            original=canonical_body(body,entry)
            self.assertEqual(source,install_native_async_insert(source.replace(body,original)))
            child=('run_1a5080(s)','run_1a5084(s)') if entry=='122020' else ('0x1ab330u','0x1ab334u')
            for candidate in (body,original):
                for old,new in (('0x3cu','0x38u'),('s.reg(2,4,0)','s.reg(1,4,0)'),
                                ('if(s.flags.zf)','if(!s.flags.zf)'),('s.gp[4]+=4u','s.gp[4]+=8u'),child):
                    changed=candidate.replace(old,new,1);self.assertNotEqual(candidate,changed,'Missing insert drift witness '+old)
                    with self.assertRaises(ValueError):install_native_async_insert(source.replace(body,changed))

    def test_complete_resolvers(self):
        for path in ('src/physics/physics_game_arena_runtime_generated.cpp','src/rendering/shadows/source_shadow_frame_tu15_generated.cpp'):
            source=(ROOT/path).read_text()
            resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)[0]
            changed=resolver.replace('{','{if(p==0x3cu)return 0x38u;',1)
            with self.assertRaises(ValueError):install_native_async_insert(source.replace(resolver,changed))


if __name__=='__main__':unittest.main()
