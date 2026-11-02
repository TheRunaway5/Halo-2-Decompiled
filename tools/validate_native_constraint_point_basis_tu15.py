#!/usr/bin/env python3
"""Physical22E210/native30BD0 basis composition using genuine descriptors."""
from pathlib import Path
def once(text,needle,replacement):
 if text.count(needle)!=1:raise RuntimeError('Point basis qualification seam changed: '+needle[:100])
 return text.replace(needle,replacement,1)
def main():
 base=Path(__file__).with_name('validate_native_model_constraint_removal_tu15.py');text=base.read_text()
 first=text.index(' units={');last=text.index('\n observer=',first)
 text=text[:first]+" units={'basis':ROOT/'src/physics/native_tu15_constraint_point_basis.cpp','math':ROOT/'src/physics/native_tu15_constraint_auxiliary_math.cpp'}"+text[last:]
 text=once(text,"observer=ROOT/'tools/observe_native_model_constraint_removal_tu15.cpp'","observer=ROOT/'tools/observe_native_constraint_point_basis_tu15.cpp'")
 text=once(text,' pins={'," files.extend([ROOT/'tools/validate_native_model_constraint_removal_tu15.py',ROOT/'include/halo2/native_tu15_constraint_point_basis.hpp',ROOT/'include/halo2/native_tu15_constraint_auxiliary_math.hpp'])\n pins={")
 text=once(text,'(0x22d0a0,0x22e1b0,0x22e390,0xde8c0,0x22fe30,0x16e8a0)','(0x22e210,0x30bd0)')
 text=once(text,"  for pc in (0x22d13f,0x22d155,0x22d233,0x22e1fc):metadata[str(pc)]['stop_before']=True",'  # Native30BD0 closes both actual normalization calls.')
 text=once(text,'#include "halo2/native_tu15_model_constraint_removal.hpp"\\n','#include "halo2/native_tu15_constraint_point_basis.hpp"\\n')
 text=once(text,"text=units['removal'].read_text();needle='read(m,list_row+2u,2)-1u'","text=units['basis'].read_text();needle='auto axis=object+0x70u'")
 text=once(text,"text.replace(needle,'read(m,list_row+2u,2)-2u')","text.replace(needle,'auto axis=object+0x7cu')")
 text=once(text,"first='removal.cpp'if variant=='native'else'wrong_list_count.cpp'","first='basis.cpp'if variant=='native'else'wrong_list_count.cpp'")
 text=once(text,'{first} release.cpp slots.cpp cache.cpp delete.cpp','{first} math.cpp')
 text=once(text,"remaining=['Actual17A0F0 and22FEA0 lifetime callbacks if reached','Positive authored constraint creation','Fault/alias/mutable-observer domains','Production integration and visible multiplayer']","remaining=['Full22E4F0/22FEA0 caller composition','Additional nonfinite/axis/alias/fault/observer domains beyond actual descriptor inputs','Production integration and visible multiplayer']")
 text=text.replace('wrong_list_count','wrong_axis').replace('Original22D0A0 rejects incorrect native list removal count','Original22E210 rejects incorrect native selected axis')
 namespace={'__file__':__file__,'__name__':'native_constraint_point_basis_base'}
 exec(compile(text,str(base),'exec'),namespace);return namespace['main']()
if __name__=='__main__':raise SystemExit(main())
