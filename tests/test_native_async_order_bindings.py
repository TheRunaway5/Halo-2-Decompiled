#!/usr/bin/env python3
"""Native async comparison binds only its complete, mapped source body."""
from pathlib import Path
import re
import sys
import unittest
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from native_async_order_replacements import install_native_async_order,canonical_body,PREFIX


class AsyncBinding(unittest.TestCase):
    def setUp(self):
        self.source=(ROOT/'src/physics/physics_game_arena_runtime_generated.cpp').read_text()
        self.bound=install_native_async_order(self.source)
        self.body=re.search(r'static void run_1a5080\(WorldState &s\) \{.*?\n\}',self.bound,re.S)[0]

    def test_idempotence_and_complete_body(self):
        self.assertIn(PREFIX,self.body)
        self.assertEqual(self.bound,install_native_async_order(self.bound))
        original=canonical_body(self.body)
        self.assertEqual(self.bound,install_native_async_order(self.bound.replace(self.body,original)))
        for body in (self.body,original):
            for old,new in (('0x1a47d0u','0x1a47d4u'),('0x10u+s.gp[4]','0xcu+s.gp[4]'),
                ('if(s.flags.cf||s.flags.zf)','if(s.flags.cf)'),('s.gp[4]+=12u','s.gp[4]+=8u'),
                ('0x1a50fbu','0x1a50fcu'),('s.reg(5,4,0)','s.reg(3,4,0)')):
                changed=body.replace(old,new,1)
                self.assertNotEqual(body,changed,'Missing async drift witness '+old)
                with self.assertRaises(ValueError):install_native_async_order(self.bound.replace(self.body,changed))

    def test_complete_resolver(self):
        resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',self.bound,re.S)[0]
        for old,new in (('0x1aaa80u','0x1aa500u'),('0x1ab330u','0x1ab334u'),('{','{if(p==0)return 3u;')):
            changed=resolver.replace(old,new,1)
            with self.assertRaises(ValueError):install_native_async_order(self.bound.replace(resolver,changed))

    def test_duplicate_or_missing_owner(self):
        with self.assertRaises(ValueError):install_native_async_order(self.bound+'\n'+self.body)
        with self.assertRaises(ValueError):install_native_async_order(self.bound.replace(self.body,''))


if __name__=='__main__':unittest.main()
