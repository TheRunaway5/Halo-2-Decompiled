"""Exact qualified reversal of the29 Motion register-FSTP C1 corrections."""
import hashlib,importlib.util
from pathlib import Path
BASELINE='bde8a4ba2378d750fe899a29bab977dc1fdd7dd3416cdc4e6ed155e9c93cd36f'
SITES=((0x2b3f2,2),(0x2b3f4,0),(0x2b403,0),(0xf2b28,0),(0xf3387,3),(0xf3389,0),(0xf338b,0),(0xf3c1f,0),(0xf48d9,0),(0xf4991,3),(0xf4999,1),(0xf49c5,3),(0xf49cd,1),(0xf4bf1,0),(0xf5f3f,3),(0xf5f47,1),(0xf5f55,0),(0xf69c9,0),(0xf6ed5,0),(0xf6f20,0),(0xf710e,3),(0xf7110,0),(0xf7112,0),(0xf712b,0),(0x11eef5,0),(0x2094b2,0),(0x20a638,0),(0x20aa27,0),(0x20ab98,0))
def statements(pc,target):
 old=f'if(s.services.instruction){{s.services.instruction(0x{pc:x}u);}} s.fp.st({target})=s.fp.st(0);s.fp.pop();'
 return old,old.replace(';s.fp.pop();',';s.fp.status&=~0x200u;s.fp.pop();')
def digest(text):return hashlib.sha256(text.encode()).hexdigest()
def canonical_source(source):
 spec=importlib.util.spec_from_file_location('motion_fst_inverse_binding',Path(__file__).with_name('native_affine_inverse_replacements.py'))
 binding=importlib.util.module_from_spec(spec);spec.loader.exec_module(binding)
 source=binding.remove_affine_inverse(source);corrected=0
 for pc,target in SITES:
  old,new=statements(pc,target)
  if source.count(old)+source.count(new)!=1:raise ValueError('Motion FSTP/C1 site drift '+hex(pc))
  if new in source:source=source.replace(new,old,1);corrected+=1
 if corrected not in(0,len(SITES)):raise ValueError('Incomplete Motion FSTP/C1 correction set')
 if digest(source)!=BASELINE:raise ValueError('Motion whole original source drift outside qualified corrections')
 return source
def correct(source):
 canonical_source(source)
 for pc,target in SITES:
  old,new=statements(pc,target);source=source.replace(old,new,1)
 canonical_source(source);return source
