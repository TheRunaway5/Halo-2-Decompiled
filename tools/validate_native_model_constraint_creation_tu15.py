#!/usr/bin/env python3
"""Qualify full22CEA0 with declared request DATA from genuine descriptors."""
from pathlib import Path
def once(text,needle,replacement):
 if text.count(needle)!=1:raise RuntimeError('Creation qualification seam changed: '+needle[:100])
 return text.replace(needle,replacement,1)
def main():
 base=Path(__file__).with_name('validate_native_model_constraint_removal_tu15.py');text=base.read_text()
 first=text.index(' units={');last=text.index('\n observer=',first)
 text=text[:first]+''' units={name:ROOT/path for name,path in {
  'creation':'src/physics/native_tu15_model_constraint_creation.cpp',
  'frames':'src/physics/native_tu15_body_constraint_update.cpp',
  'geometry':'src/physics/native_tu15_geometry.cpp',
  'allocator':'src/core/native_tu15_data_array_allocate.cpp',
  'points':'src/physics/native_tu15_constraint_local_points.cpp',
  'inverse':'src/rendering/world/native_tu15_camera_projection.cpp',
  'normalize':'src/physics/native_tu15_vector_normalize.cpp',
  'motion':'src/physics/native_tu15_constraint_motion_compare.cpp',
  'release':'src/physics/native_tu15_constraint_reference_release.cpp',
  'slots':'src/physics/native_tu15_constraint_body_slots.cpp',
  'cache':'src/physics/native_tu15_constraint_scalar_cache.cpp'}.items()}
'''.rstrip()+text[last:]
 text=once(text,"observer=ROOT/'tools/observe_native_model_constraint_removal_tu15.cpp'","observer=ROOT/'tools/observe_native_model_constraint_creation_tu15.cpp'")
 text=once(text,' pins={'," files.extend([ROOT/'tools/validate_native_model_constraint_removal_tu15.py',*[ROOT/'include/halo2'/name for name in ('native_tu15_model_constraint_creation.hpp','native_tu15_constraint_local_points.hpp','native_tu15_camera_projection.hpp','native_tu15_geometry.hpp','native_tu15_data_array_allocate.hpp','native_tu15_body_constraint_update.hpp','native_tu15_vector_normalize.hpp','native_tu15_constraint_motion_compare.hpp')]])\n pins={")
 text=once(text,'(0x22d0a0,0x22e1b0,0x22e390,0xde8c0,0x22fe30,0x16e8a0)',
  '(0x22cea0,0x16e6a0,0x230f60,0x122b60,0x22ede0,0x22eba0,0x1d8b90,0x2e4650,0x2e44d0,0x144090,0x22e1b0,0x22e390,0xde8c0,0x22fe30)')
 text=once(text,"    metadata[str(pc)]=dict(bytes=i.bytes.hex(),size=i.size,memory=operands)",
  '''    if i.mnemonic in ('rep stosd','rep stosb'):
     operands=[dict(base='edi',index='',scale=1,displacement=0,bytes=4 if i.mnemonic.endswith('stosd')else 1,read=False,write=True)]
    metadata[str(pc)]=dict(bytes=i.bytes.hex(),size=i.size,memory=operands,repeat_count=i.mnemonic in ('rep stosd','rep stosb'))''')
 text=once(text,"  for pc in (0x22d13f,0x22d155,0x22d233,0x22e1fc):metadata[str(pc)]['stop_before']=True",'  # The native constructor closes its actual child path; no child is skipped.')
 text=once(text,"  driver=scratch/'driver.cpp';",'''  prefix=once(prefix,'for(const auto&m:i.at("memory")){const auto p=',
   'for(const auto&m:i.at("memory")){if(i.value("repeat_count",false)&&std::uint32_t(r.rcx)==0u)continue;const auto p=')
  driver=scratch/'driver.cpp';''')
 text=once(text,'#include "halo2/native_tu15_model_constraint_removal.hpp"\\n','#include "halo2/native_tu15_model_constraint_creation.hpp"\\n')
 text=once(text,"text=units['removal'].read_text();needle='read(m,list_row+2u,2)-1u'","text=units['creation'].read_text();needle='previous&0xfdu'")
 text=once(text,"text.replace(needle,'read(m,list_row+2u,2)-2u')","text.replace(needle,'previous|2u')")
 text=once(text,"first='removal.cpp'if variant=='native'else'wrong_list_count.cpp'","first='creation.cpp'if variant=='native'else'wrong_list_count.cpp'")
 text=once(text,'{first} release.cpp slots.cpp cache.cpp delete.cpp','{first} frames.cpp geometry.cpp allocator.cpp points.cpp inverse.cpp normalize.cpp motion.cpp release.cpp slots.cpp cache.cpp')
 text=once(text,"remaining=['Actual17A0F0 and22FEA0 lifetime callbacks if reached','Positive authored constraint creation','Fault/alias/mutable-observer domains','Production integration and visible multiplayer']","remaining=['Genuine authored+80 positive22D460 caller input; requests here are explicitly declared','Retained-node/model-key/fault/alias/mutable-observer and other numerical-control domains','Production integration and visible multiplayer']")
 text=text.replace('wrong_list_count','wrong_motion_flag').replace('Original22D0A0 rejects incorrect native list removal count','Original22CEA0 rejects incorrect native motion flag')
 namespace={'__file__':__file__,'__name__':'native_model_constraint_creation_base','once':once}
 exec(compile(text,str(base),'exec'),namespace);return namespace['main']()
if __name__=='__main__':raise SystemExit(main())
