#!/usr/bin/env python3
"""Physical22FEA0 native parent and explicit effect-lifetime service frontier."""
from pathlib import Path
def once(text,needle,replacement):
 if text.count(needle)!=1:raise RuntimeError('Point basis qualification seam changed: '+needle[:100])
 return text.replace(needle,replacement,1)
def main():
 base=Path(__file__).with_name('validate_native_model_constraint_removal_tu15.py');text=base.read_text()
 first=text.index(' units={');last=text.index('\n observer=',first)
 text=text[:first]+" units={name:ROOT/path for name,path in {'runtime':'src/physics/native_tu15_constraint_runtime_update.cpp','parameters':'src/physics/native_tu15_constraint_runtime_parameters.cpp','handles':'src/physics/native_tu15_constraint_runtime_handles.cpp','frame':'src/physics/native_tu15_constraint_geometry_frame.cpp','basis':'src/physics/native_tu15_constraint_point_basis.cpp','math':'src/physics/native_tu15_constraint_auxiliary_math.cpp','frames':'src/physics/native_tu15_body_constraint_update.cpp','slots':'src/physics/native_tu15_constraint_body_slots.cpp','geometry':'src/physics/native_tu15_geometry.cpp','allocator':'src/core/native_tu15_data_array_allocate.cpp'}.items()}"+text[last:]
 text=once(text,"observer=ROOT/'tools/observe_native_model_constraint_removal_tu15.cpp'","observer=ROOT/'tools/observe_native_constraint_runtime_update_tu15.cpp'")
 text=once(text,' pins={'," files.extend([ROOT/'tools/validate_native_model_constraint_removal_tu15.py',ROOT/'include/halo2/native_tu15_constraint_runtime_update.hpp',ROOT/'include/halo2/native_tu15_constraint_runtime_parameters.hpp',ROOT/'include/halo2/native_tu15_constraint_runtime_handles.hpp',ROOT/'include/halo2/native_tu15_constraint_geometry_frame.hpp',ROOT/'include/halo2/native_tu15_body_constraint_update.hpp',ROOT/'include/halo2/native_tu15_geometry.hpp',ROOT/'include/halo2/native_tu15_data_array_allocate.hpp',ROOT/'include/halo2/native_tu15_constraint_point_basis.hpp',ROOT/'include/halo2/native_tu15_constraint_auxiliary_math.hpp'])\n pins={")
 text=once(text,'(0x22d0a0,0x22e1b0,0x22e390,0xde8c0,0x22fe30,0x16e8a0)','(0x22fea0,0x230a60,0x230ae0,0x17a0f0,0x17a030,0x17dec0,0x18d4b0,0x18d510,0x16e6a0,0x18bbc0,0x22e4f0,0x22e210,0x30bd0,0x1d8b90,0x2e4650,0x2e44d0,0x1d8c80)')
 text=once(text,"  for pc in (0x22d13f,0x22d155,0x22d233,0x22e1fc):metadata[str(pc)]['stop_before']=True","  for pc in (0x230099,0x2300d5,0x230115,0x230152,0x2301f0,0x230288,0x230320,0x23059a,0x2306bd,0x230751,0x17a0b5,0x17a18c):metadata[str(pc)]['stop_before']=True")
 text=once(text,'#include "halo2/native_tu15_model_constraint_removal.hpp"\\n','#include "halo2/native_tu15_constraint_runtime_update.hpp"\\n#include "halo2/native_tu15_constraint_body_slots.hpp"\\n')
 text=once(text,"text=units['removal'].read_text();needle='read(m,list_row+2u,2)-1u'","text=units['runtime'].read_text();needle='value>126?126:value'")
 text=once(text,"text.replace(needle,'read(m,list_row+2u,2)-2u')","text.replace(needle,'value>125?125:value')")
 text=once(text,"first='removal.cpp'if variant=='native'else'wrong_list_count.cpp'","first='runtime.cpp'if variant=='native'else'wrong_list_count.cpp'")
 text=once(text,'{first} release.cpp slots.cpp cache.cpp delete.cpp','{first} parameters.cpp handles.cpp frame.cpp basis.cpp math.cpp frames.cpp slots.cpp geometry.cpp allocator.cpp')
 text=once(text,"remaining=['Actual17A0F0 and22FEA0 lifetime callbacks if reached','Positive authored constraint creation','Fault/alias/mutable-observer domains','Production integration and visible multiplayer']","remaining=['Full22FEA0 caller composition','Additional nonfinite/axis/alias/fault/observer domains beyond actual descriptor inputs','Production integration and visible multiplayer']")
 text=text.replace('wrong_list_count','wrong_counter').replace('Original22D0A0 rejects incorrect native list removal count','Original22FEA0 rejects incorrect native timer cap')
 namespace={'__file__':__file__,'__name__':'native_constraint_point_basis_base'}
 exec(compile(text,str(base),'exec'),namespace);return namespace['main']()
if __name__=='__main__':raise SystemExit(main())
