#!/usr/bin/env python3
"""Native30BD0/18BBC0 against unchanged original hardware arithmetic."""
import argparse,sys
from pathlib import Path
def once(text,needle,replacement):
 if text.count(needle)!=1:raise RuntimeError('Auxiliary math qualification seam changed: '+needle[:100])
 return text.replace(needle,replacement,1)
def main():
 parser=argparse.ArgumentParser(add_help=False);parser.add_argument('--operation',choices=['axis','pack'],default='axis')
 options,remaining=parser.parse_known_args();sys.argv=[sys.argv[0],*remaining]
 base=Path(__file__).with_name('validate_native_model_constraint_removal_tu15.py');text=base.read_text()
 first=text.index(' units={');last=text.index('\n observer=',first)
 text=text[:first]+" units={'math':ROOT/'src/physics/native_tu15_constraint_auxiliary_math.cpp'}"+text[last:]
 text=once(text,"default=r'(gemini-column[01]_[01]|elongation-crates_[12]|actual-warthog_[1-4])_[0-3]'","default=r'gemini-column0_0_[0-3]'")
 observer='observe_native_constraint_axis_normalize_tu15.cpp'if options.operation=='axis'else'observe_native_constraint_direction_pack_tu15.cpp'
 text=once(text,"observer=ROOT/'tools/observe_native_model_constraint_removal_tu15.cpp'",f"observer=ROOT/{('tools/'+observer)!r}")
 text=once(text,' pins={'," files.extend([ROOT/'tools/validate_native_model_constraint_removal_tu15.py',ROOT/'include/halo2/native_tu15_constraint_auxiliary_math.hpp'])\n pins={")
 text=once(text,'(0x22d0a0,0x22e1b0,0x22e390,0xde8c0,0x22fe30,0x16e8a0)','(0x30bd0,)'if options.operation=='axis'else'(0x18bbc0,)')
 text=once(text,"  for pc in (0x22d13f,0x22d155,0x22d233,0x22e1fc):metadata[str(pc)]['stop_before']=True",'  # Native arithmetic has no remaining child callback.')
 text=once(text,'#include "halo2/native_tu15_model_constraint_removal.hpp"\\n','#include "halo2/native_tu15_constraint_auxiliary_math.hpp"\\n')
 if options.operation=='axis':
  text=once(text,"text=units['removal'].read_text();needle='read(m,list_row+2u,2)-1u'","text=units['math'].read_text();needle='divide(one,scalar(length))'")
  text=once(text,"text.replace(needle,'read(m,list_row+2u,2)-2u')","text.replace(needle,'divide(scalar(0x40000000u),scalar(length))')")
 else:
  text=once(text,"text=units['removal'].read_text();needle='read(m,list_row+2u,2)-1u'","text=units['math'].read_text();needle='packed_z<<11u'")
  text=once(text,"text.replace(needle,'read(m,list_row+2u,2)-2u')","text.replace(needle,'packed_z<<10u')")
 text=once(text,"first='removal.cpp'if variant=='native'else'wrong_list_count.cpp'","first='math.cpp'if variant=='native'else'wrong_list_count.cpp'")
 text=once(text,'{first} release.cpp slots.cpp cache.cpp delete.cpp','{first}')
 text=once(text,"remaining=['Actual17A0F0 and22FEA0 lifetime callbacks if reached','Positive authored constraint creation','Fault/alias/mutable-observer domains','Production integration and visible multiplayer']","remaining=['Full22E4F0/22FEA0 native caller composition','Fault/alias/mutable-observer and additional numerical-control domains','Production integration and visible multiplayer']")
 text=text.replace('wrong_list_count','wrong_arithmetic').replace('Original22D0A0 rejects incorrect native list removal count','Original TU1.5 rejects incorrect native auxiliary arithmetic')
 namespace={'__file__':__file__,'__name__':'native_constraint_auxiliary_math_base'}
 exec(compile(text,str(base),'exec'),namespace);return namespace['main']()
if __name__=='__main__':raise SystemExit(main())
