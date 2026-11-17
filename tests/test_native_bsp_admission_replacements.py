import hashlib,sys,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
import native_bsp_admission_replacements as raw
import native_bsp_admission_public_replacements as public
from audit_native_execution_ownership import unique_function
UNITS=('src/rendering/shadows/source_shadow_frame_tu15_generated.cpp','src/rendering/effects/source_effect_particle_draw_tu15_generated.cpp')
class Binding(unittest.TestCase):
    def test_complete_owners(self):
        for name in UNITS:
            source=(ROOT/name).read_text();match,body=raw.authenticate(source);bound=raw.replace_bsp_admission(source)
            self.assertEqual(raw.replace_bsp_admission(bound),bound)
            self.assertEqual(raw.canonical_body(raw.BODY.search(bound)[0]),body)
            self.assertEqual(raw.canonical_body(raw.ordered_reference(body)),body)
            restored=bound.replace(raw.INCLUDE,'',1).replace(raw.BODY.search(bound)[0],body,1)
            pristine=source.replace(raw.INCLUDE,'',1).replace(match[0],body,1)
            self.assertEqual(restored,pristine)
    def test_body_mutations(self):
        for name in UNITS:
            source=(ROOT/name).read_text();match,body=raw.authenticate(source)
            for old,new in(('s.gp[4]','s.gp[5]'),('s.reg(3,4,0)','s.reg(2,4,0)'),('s.flags.minus(','s.flags.plus(')):
                changed=body.replace(old,new,1);self.assertNotEqual(changed,body)
                with self.assertRaises(ValueError):raw.replace_bsp_admission(source[:match.start()]+changed+source[match.end():])
    def test_resolver_mutation(self):
        source=(ROOT/UNITS[0]).read_text();resolver=unique_function(source,'selected');changed=resolver.replace('return p;','return p+1u;',1);self.assertNotEqual(resolver,changed)
        with self.assertRaises(ValueError):raw.replace_bsp_admission(source.replace(resolver,changed,1))
    def test_dispatcher_mutation(self):
        source=(ROOT/UNITS[0]).read_text();dispatcher=unique_function(source,'dispatch');changed=dispatcher.replace('run_136e10(s);return;','run_136ea0(s);return;',1);self.assertNotEqual(dispatcher,changed)
        with self.assertRaises(ValueError):raw.replace_bsp_admission(source.replace(dispatcher,changed,1))
    def test_prefix_mutation(self):
        source=raw.replace_bsp_admission((ROOT/UNITS[0]).read_text());changed=source.replace(raw.PREFIX,raw.PREFIX.replace('0x1371abu','0x1371acu'),1)
        with self.assertRaises(ValueError):raw.replace_bsp_admission(changed)
    def test_public_composition(self):
        import native_geometry_visibility_public_replacements as geometry
        import native_visibility_association_public_replacements as association
        source=(ROOT/'src/rendering/world/source_bsp_admission.cpp').read_text();bound=public.replace_public(source)
        self.assertEqual(public.replace_public(bound),bound);self.assertEqual(geometry.replace_public(bound,'bsp'),bound);self.assertEqual(association.replace_public(bound,'bsp'),bound)
        self.assertEqual(association.digest(association.canonical_public(public.NEW,'bsp')),association.OWNERS['bsp'][1])
    def test_public_mutation(self):
        source=public.replace_public((ROOT/'src/rendering/world/source_bsp_admission.cpp').read_text());changed=source.replace('values(bsp,visible,in,','values(bsp,visible,wrong,',1)
        with self.assertRaises(ValueError):public.replace_public(changed)
if __name__=='__main__':unittest.main()
