#!/usr/bin/env python3
"""Complete resource/BSP bodies and their operand resolvers fail closed."""
from pathlib import Path
import re
import sys
import unittest
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from native_geometry_request_replacements import install_native_geometry_request,install_native_geometry_publish,PREFIX,PUBLISH_PREFIX,canonical_body
from native_bsp_cluster_replacements import install_native_bsp_cluster_flood


class Bindings(unittest.TestCase):
    def test_publication_full_body(self):
        source=(ROOT/'src/physics/physics_live_support_tu15_generated.cpp').read_text()
        self.assertEqual(source,install_native_geometry_publish(source))
        body=re.search(r'static void run_22a680\(WorldState &s\) \{.*?\n\}',source,re.S)[0]
        self.assertIn(PUBLISH_PREFIX,body)
        original=canonical_body(body)
        self.assertEqual(source,install_native_geometry_publish(source.replace(body,original)))
        for candidate in (body,original):
            for old,new in (('0x1cu','0x18u'),('0x22a718u','0x22a714u'),('0x10u+s.gp[7]','0x14u+s.gp[7]')):
                changed=candidate.replace(old,new,1)
                self.assertTrue(changed!=candidate,'Missing publication drift witness')
                with self.assertRaises(ValueError):install_native_geometry_publish(source.replace(body,changed))

    def test_resource_full_bodies(self):
        for path in ('src/physics/physics_live_support_tu15_generated.cpp',
                     'src/rendering/shadows/source_shadow_frame_tu15_generated.cpp'):
            source=(ROOT/path).read_text()
            self.assertEqual(source,install_native_geometry_request(source))
            body=re.search(r'static void run_130980\(WorldState &s\) \{.*?\n\}',source,re.S)[0]
            self.assertIn(PREFIX,body)
            original=canonical_body(body)
            self.assertEqual(source,install_native_geometry_request(source.replace(body,original)))
            for selected in (body,original):
                child=('0x130eb0u','0x130eb4u') if '0x130eb0u' in selected else ('run_130eb0(s)','run_130eb4(s)')
                for old,new in (('0x4f2d6cu','0x4f2d68u'),child,
                                ('if(s.flags.zf)','if(!s.flags.zf)'),('0x14u','0x10u')):
                    changed=selected.replace(old,new,1)
                    self.assertTrue(selected!=changed,'Missing mutation witness '+old)
                    with self.assertRaises(ValueError):install_native_geometry_request(source.replace(body,changed))

    def test_complete_selected_resolvers(self):
        for path,install in (
            ('src/rendering/shadows/source_shadow_frame_tu15_generated.cpp',install_native_geometry_request),
            ('src/rendering/shadows/source_shadow_frame_tu15_generated.cpp',install_native_bsp_cluster_flood),
            ('src/campaign/ai/source_ai_frame_tu15_generated.cpp',install_native_bsp_cluster_flood)):
            source=(ROOT/path).read_text()
            self.assertEqual(source,install(source))
            selected=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)[0]
            # Insert a resolver control-flow change without changing any
            # existing case pair or the complete authenticated function body.
            changed=selected.replace('{','{if(p==0xcu)return 8u;',1)
            with self.assertRaises(ValueError):install(source.replace(selected,changed))


if __name__=='__main__':unittest.main()
