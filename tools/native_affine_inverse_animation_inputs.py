"""Extract the pinned fixed-image animation State and one complete child body."""
import native_affine_inverse_animation_replacements as binding
def prepare(root,scratch):
 source=(root/binding.UNIT).read_text();match,body,state,public=binding.authenticate(source)
 bound=binding.replace_affine_inverse_animation(source)
 assert binding.replace_affine_inverse_animation(bound)==bound
 current=binding.BODY.search(bound)[0]
 controls=(source.replace(match[0],match[0].replace('_mm_mul_ss','_mm_add_ss',1),1),
           source.replace(state,state.replace('flags.load(a.eflags)','flags.load(0u)',1),1),
           source.replace(public,public.replace('case 0x142d00u:run_142d00(s);break;','case 0x142d00u:run_142e80(s);break;',1),1),
           bound.replace(binding.PREFIX,binding.PREFIX.replace('::canonical','::actual'),1))
 for changed in controls:
  assert changed not in(source,bound)
  try:binding.replace_affine_inverse_animation(changed)
  except ValueError:pass
  else:raise ValueError('Fixed-1.1 inverse owner drift admitted')
 (scratch/'inverse_animation_state.inc').write_text(state+'\nnamespace bound {\n'+current+'\n}\n')
 return dict(unit=binding.UNIT,complete_body=binding.BODY_PIN,complete_state=binding.STATE_PIN,
             complete_public_entry=binding.PUBLIC_PIN,source_drift_rejections=len(controls),
             domain='Dense masked source FP and separate softwareMMX fields; active-MMX input remains refused by the existing animation loader')
