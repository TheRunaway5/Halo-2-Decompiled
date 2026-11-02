#!/usr/bin/env python3
"""Use the same genuine BEFORE cohort for native22EBA0 frame/point composition."""
from pathlib import Path
def once(text,needle,replacement):
 if text.count(needle)!=1:raise RuntimeError('Point qualification seam changed: '+needle[:100])
 return text.replace(needle,replacement,1)
def main():
 base=Path(__file__).with_name('validate_native_model_constraint_removal_tu15.py');text=base.read_text()
 first=text.index(' units={');last=text.index('\n observer=',first)
 text=text[:first]+''' units={name:ROOT/path for name,path in {
  'points':'src/physics/native_tu15_constraint_local_points.cpp',
  'frames':'src/physics/native_tu15_body_constraint_update.cpp',
  'slots':'src/physics/native_tu15_constraint_body_slots.cpp',
  'geometry':'src/physics/native_tu15_geometry.cpp',
  'allocator':'src/core/native_tu15_data_array_allocate.cpp',
  'inverse':'src/rendering/world/native_tu15_camera_projection.cpp'}.items()}
'''.rstrip()+text[last:]
 text=once(text,"observer=ROOT/'tools/observe_native_model_constraint_removal_tu15.cpp'","observer=ROOT/'tools/observe_native_constraint_local_points_tu15.cpp'")
 text=once(text," pins={", " files.extend([ROOT/'tools/validate_native_model_constraint_removal_tu15.py',ROOT/'include/halo2/native_tu15_constraint_local_points.hpp',ROOT/'include/halo2/native_tu15_camera_projection.hpp',ROOT/'include/halo2/native_tu15_geometry.hpp',ROOT/'include/halo2/native_tu15_data_array_allocate.hpp',ROOT/'include/halo2/native_tu15_body_constraint_update.hpp'])\n pins={")
 text=once(text,'(0x22d0a0,0x22e1b0,0x22e390,0xde8c0,0x22fe30,0x16e8a0)','(0x22eba0,0x1d8b90,0x2e4650,0x2e44d0,0x144090)')
 text=once(text,"  for pc in (0x22d13f,0x22d155,0x22d233,0x22e1fc):metadata[str(pc)]['stop_before']=True",'  # This point composition has no unconverted lifetime callback.')
 text=once(text,'#include "halo2/native_tu15_model_constraint_removal.hpp"\\n',' #include "halo2/native_tu15_constraint_local_points.hpp"\\n')
 text=once(text,"text=units['removal'].read_text();needle='read(m,list_row+2u,2)-1u'","text=units['points'].read_text();needle='instance+destination+n*4u'")
 text=once(text,"text.replace(needle,'read(m,list_row+2u,2)-2u')","text.replace(needle,'instance+destination+n*4u+4u')")
 text=once(text,"first='removal.cpp'if variant=='native'else'wrong_list_count.cpp'","first='points.cpp'if variant=='native'else'wrong_list_count.cpp'")
 text=once(text,'{first} release.cpp slots.cpp cache.cpp delete.cpp','{first} frames.cpp slots.cpp geometry.cpp allocator.cpp inverse.cpp')
 text=once(text,"remaining=['Actual17A0F0 and22FEA0 lifetime callbacks if reached','Positive authored constraint creation','Fault/alias/mutable-observer domains','Production integration and visible multiplayer']","remaining=['Positive authored22CEA0 caller composition','Nonfinite/scale/fault/alias/mutable-observer domains beyond actual input cohort','Production integration and visible multiplayer']")
 namespace={'__file__':__file__,'__name__':'native_constraint_local_points_base'}
 exec(compile(text,str(base),'exec'),namespace);return namespace['main']()
if __name__=='__main__':raise SystemExit(main())
