#!/usr/bin/env python3
"""Physical230A60/230AE0 composition on genuine existing descriptor fields."""
from pathlib import Path
def once(text,needle,replacement):
 if text.count(needle)!=1:raise RuntimeError('Handle clear qualification seam changed: '+needle[:100])
 return text.replace(needle,replacement,1)
def main():
 base=Path(__file__).with_name('validate_native_model_constraint_removal_tu15.py');text=base.read_text()
 first=text.index(' units={');last=text.index('\n observer=',first)
 text=text[:first]+" units={'handles':ROOT/'analysis/physics/native-constraint-handle-cleanup-api-20261101/staged/src/physics/native_tu15_constraint_runtime_handles.cpp'}"+text[last:]
 text=once(text,"observer=ROOT/'tools/observe_native_model_constraint_removal_tu15.cpp'","observer=ROOT/'tools/observe_native_constraint_handle_clear_tu15.cpp'")
 text=once(text,' pins={'," files.extend([ROOT/'tools/validate_native_model_constraint_removal_tu15.py',ROOT/'include/halo2/native_tu15_constraint_runtime_handles.hpp',ROOT/'include/halo2/native_tu15_constraint_tag_cleanup.hpp'])\n pins={")
 text=once(text,'(0x22d0a0,0x22e1b0,0x22e390,0xde8c0,0x22fe30,0x16e8a0)','(0x230a60,0x230ae0,0x17a0f0,0x17a030)')
 text=once(text,"  for pc in (0x22d13f,0x22d155,0x22d233,0x22e1fc):metadata[str(pc)]['stop_before']=True",'  # No child is skipped; unexpected lifetime inputs fail this cohort.')
 text=once(text,'#include "halo2/native_tu15_model_constraint_removal.hpp"\\n','#include "halo2/native_tu15_constraint_runtime_handles.hpp"\\n')
 text=once(text,"text=units['removal'].read_text();needle='read(m,list_row+2u,2)-1u'","text=units['handles'].read_text();needle='return read(m,instance+0x3cu)!=0xffffffffu'")
 text=once(text,"text.replace(needle,'read(m,list_row+2u,2)-2u')","text.replace(needle,'return read(m,instance+0x3cu)==0xffffffffu')")
 text=once(text,"first='removal.cpp'if variant=='native'else'wrong_list_count.cpp'","first='handles.cpp'if variant=='native'else'wrong_list_count.cpp'")
 text=once(text,'{first} release.cpp slots.cpp cache.cpp delete.cpp','{first}')
 text=once(text,"remaining=['Actual17A0F0 and22FEA0 lifetime callbacks if reached','Positive authored constraint creation','Fault/alias/mutable-observer domains','Production integration and visible multiplayer']","remaining=['Actual descriptor positive link/handle fields; retained input fields areNone','Actual22FEA0 caller composition','Production integration and visible multiplayer','Fault/alias/mutable-observer domains']")
 text=text.replace('wrong_list_count','wrong_presence').replace('Original22D0A0 rejects incorrect native list removal count','Original230AE0 rejects incorrect native presence result')
 namespace={'__file__':__file__,'__name__':'native_constraint_handle_clear_base'}
 exec(compile(text,str(base),'exec'),namespace);return namespace['main']()
if __name__=='__main__':raise SystemExit(main())
