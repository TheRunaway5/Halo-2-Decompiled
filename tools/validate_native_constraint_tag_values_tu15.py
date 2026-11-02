#!/usr/bin/env python3
"""PhysicalC6120 lookup proof; the genuine tagged-value pools are empty."""
from pathlib import Path
def once(text,needle,replacement):
 if text.count(needle)!=1:raise RuntimeError('Tagged value qualification seam changed: '+needle[:100])
 return text.replace(needle,replacement,1)
def main():
 base=Path(__file__).with_name('validate_native_model_constraint_removal_tu15.py');text=base.read_text()
 first=text.index(' units={');last=text.index('\n observer=',first)
 text=text[:first]+" units={'tag_value':ROOT/'src/physics/native_tu15_constraint_tag_values.cpp','iteration':ROOT/'src/core/native_tu15_data_array_iteration.cpp'}"+text[last:]
 text=once(text,"default=r'(gemini-column[01]_[01]|elongation-crates_[12]|actual-warthog_[1-4])_[0-3]'","default=r'(gemini-column[01]_0|elongation-crates_0)_[0-3]'")
 text=once(text,"observer=ROOT/'tools/observe_native_model_constraint_removal_tu15.cpp'","observer=ROOT/'tools/observe_native_constraint_tag_values_tu15.cpp'")
 text=once(text,' pins={'," files.extend([ROOT/'tools/validate_native_model_constraint_removal_tu15.py',ROOT/'include/halo2/native_tu15_constraint_tag_values.hpp',ROOT/'include/halo2/native_tu15_data_array_iteration.hpp'])\n pins={")
 text=once(text,'(0x22d0a0,0x22e1b0,0x22e390,0xde8c0,0x22fe30,0x16e8a0)','(0xc6120,0x16ea60)')
 text=once(text,"  for pc in (0x22d13f,0x22d155,0x22d233,0x22e1fc):metadata[str(pc)]['stop_before']=True",'  # C6120 composes the existing native16EA60, with no residual owner.')
 text=once(text,'#include "halo2/native_tu15_model_constraint_removal.hpp"\\n','#include "halo2/native_tu15_constraint_tag_values.hpp"\\n')
 text=once(text,"text=units['removal'].read_text();needle='read(m,list_row+2u,2)-1u'","text=units['tag_value'].read_text();needle='if(stats)++stats->calls;const auto pool=read(m,0x4ecbecu)'")
 text=once(text,"text.replace(needle,'read(m,list_row+2u,2)-2u')","text.replace(needle,'if(stats)++stats->calls;(void)read(m,0x4f0414u);const auto pool=read(m,0x4ecbecu)')")
 text=once(text,"first='removal.cpp'if variant=='native'else'wrong_list_count.cpp'","first='tag_value.cpp'if variant=='native'else'wrong_list_count.cpp'")
 text=once(text,'{first} release.cpp slots.cpp cache.cpp delete.cpp','{first} iteration.cpp')
 text=once(text,"remaining=['Actual17A0F0 and22FEA0 lifetime callbacks if reached','Positive authored constraint creation','Fault/alias/mutable-observer domains','Production integration and visible multiplayer']","remaining=['Genuine positive tagged-value pool rows and numeric clamp/store; actual pools have extent0','Actual17A030 positive tagged-field caller','Production integration and visible multiplayer','Fault/alias/mutable-observer and additional numerical-control domains']")
 text=text.replace('wrong_list_count','wrong_read_owner').replace('Original22D0A0 rejects incorrect native list removal count','OriginalC6120 rejects incorrect native read ownership')
 namespace={'__file__':__file__,'__name__':'native_constraint_tag_values_base'}
 exec(compile(text,str(base),'exec'),namespace);return namespace['main']()
if __name__=='__main__':raise SystemExit(main())
