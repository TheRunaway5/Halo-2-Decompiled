#!/usr/bin/env python3
"""Keep both original fog owners and the exact flags correction authenticated."""
from pathlib import Path
import re,sys,unittest
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
import native_fog_membership_replacements as binding
UNITS=('src/rendering/shadows/source_shadow_frame_tu15_generated.cpp','src/rendering/effects/source_effect_particle_draw_tu15_generated.cpp')
class FogBinding(unittest.TestCase):
    def source(self,path):
        text=(ROOT/path).read_text();match,body=binding.authenticate(text)
        return text[:match.start()]+body+text[match.end():]
    def test_both_exact_owners_install_roundtrip(self):
        for unit in UNITS:
            source=self.source(unit);bound=binding.replace_fog_membership(source)
            self.assertEqual(binding.replace_fog_membership(bound),bound)
            self.assertEqual(binding.authenticate(source)[1],binding.authenticate(bound)[1])
            self.assertEqual(binding.BODY.search(bound)[0].count(binding.ORDERED_SHIFT),1)
    def test_complete_body_and_context_drift_reject(self):
        for unit in UNITS:
            source=self.source(unit);match,body=binding.authenticate(source)
            for old,new in(('0xb0u','0xb4u'),('0x4ecc18u','0x4ecc1cu'),('0x34cu','0x350u')):
                self.assertIn(old,body)
                changed=source[:match.start()]+body.replace(old,new,1)+source[match.end():]
                with self.assertRaises(ValueError):binding.replace_fog_membership(changed)
            resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)[0]
            with self.assertRaises(ValueError):binding.replace_fog_membership(source.replace(resolver,resolver.replace('return p;','return p+1;',1),1))
            dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)[0]
            self.assertIn('171070',dispatcher)
            with self.assertRaises(ValueError):binding.replace_fog_membership(source.replace(dispatcher,dispatcher.replace('171070','171071',1),1))
    def test_only_exact_retained_shift_spelling_is_canonicalized(self):
        for unit in UNITS:
            bound=binding.replace_fog_membership(self.source(unit))
            for old,new in((binding.ORDERED_SHIFT,binding.ORDERED_SHIFT+'s.gp[1]=0;'),(binding.ORDERED_SHIFT,binding.ORDERED_SHIFT.replace('(s)','( s)')),(binding.PREFIX,binding.PREFIX+'s.gp[0]=0;')):
                with self.assertRaises(ValueError):binding.replace_fog_membership(bound.replace(old,new,1))
    def test_public_owner_is_pinned_before_data_reuse(self):
        source=(ROOT/'src/rendering/world/bsp_atmosphere.cpp').read_text();bound=binding.replace_public(source)
        self.assertEqual(binding.replace_public(bound),bound)
        body=binding.PUBLIC_BODY.search(bound)[0]
        with self.assertRaises(ValueError):binding.replace_public(bound.replace(body,body.replace('planar_member','changed_planar_member'),1))
if __name__=='__main__':unittest.main()
