#!/usr/bin/env python3
"""Physical17A4A0 destruction on real handles, with staged shared17A030 API."""
from pathlib import Path
def once(text,needle,replacement):
 if text.count(needle)!=1:raise RuntimeError('Handle destruction qualification seam changed: '+needle[:100])
 return text.replace(needle,replacement,1)
def main():
 base=Path(__file__).with_name('validate_native_model_constraint_removal_tu15.py');text=base.read_text()
 first=text.index(' units={');last=text.index('\n observer=',first)
 text=text[:first]+''' units={
  'destruction':ROOT/'src/physics/native_tu15_constraint_handle_destruction.cpp',
  'handles':ROOT/'analysis/physics/native-constraint-handle-cleanup-api-20261101/staged/src/physics/native_tu15_constraint_runtime_handles.cpp',
  'delete':ROOT/'src/core/native_tu15_data_array_delete.cpp'}'''+text[last:]
 text=once(text,"default=r'(gemini-column[01]_[01]|elongation-crates_[12]|actual-warthog_[1-4])_[0-3]'","default=r'(gemini-column[01]_0|elongation-crates_0)_[0-3]'")
 text=once(text,"observer=ROOT/'tools/observe_native_model_constraint_removal_tu15.cpp'","observer=ROOT/'tools/observe_native_constraint_handle_destruction_tu15.cpp'")
 text=once(text,' pins={'," files.extend([ROOT/'tools/validate_native_model_constraint_removal_tu15.py',ROOT/'include/halo2/native_tu15_constraint_handle_destruction.hpp',ROOT/'include/halo2/native_tu15_constraint_tag_cleanup.hpp',ROOT/'include/halo2/native_tu15_constraint_runtime_handles.hpp'])\n pins={")
 text=once(text,'(0x22d0a0,0x22e1b0,0x22e390,0xde8c0,0x22fe30,0x16e8a0)','(0x17a4a0,0x17a420,0xbcdf0,0x17a380,0x17a030,0x17a230,0x17a2f0,0x16e8a0)')
 text=once(text,'(0x22d13f,0x22d155,0x22d233,0x22e1fc)','(0x17a0b5,0x17a329,0x17a32f)')
 text=once(text,"    metadata[str(pc)]=dict(bytes=i.bytes.hex(),size=i.size,memory=operands)",
  '''    if i.mnemonic in ('rep stosd','rep stosb'):
     operands=[dict(base='edi',index='',scale=1,displacement=0,bytes=4 if i.mnemonic.endswith('stosd')else 1,read=False,write=True)]
    metadata[str(pc)]=dict(bytes=i.bytes.hex(),size=i.size,memory=operands,repeat_count=i.mnemonic in ('rep stosd','rep stosb'))''')
 text=once(text,"  driver=scratch/'driver.cpp';",'''  prefix=once(prefix,'for(const auto&m:i.at("memory")){const auto p=',
   'for(const auto&m:i.at("memory")){if(i.value("repeat_count",false)&&std::uint32_t(r.rcx)==0u)continue;const auto p=')
  driver=scratch/'driver.cpp';''')
 text=once(text,'#include "halo2/native_tu15_model_constraint_removal.hpp"\\n','#include "halo2/native_tu15_constraint_handle_destruction.hpp"\\n')
 text=once(text,"text=units['removal'].read_text();needle='read(m,list_row+2u,2)-1u'","text=units['destruction'].read_text();needle='if(stats)++stats->calls;const auto found=handle_record(m,handle,false)'")
 text=once(text,"text.replace(needle,'read(m,list_row+2u,2)-2u')","text.replace(needle,'if(stats)++stats->calls;(void)read(m,0x4f720cu);const auto found=handle_record(m,handle,false)')")
 text=once(text,"first='removal.cpp'if variant=='native'else'wrong_list_count.cpp'","first='destruction.cpp'if variant=='native'else'wrong_list_count.cpp'")
 text=once(text,'{first} release.cpp slots.cpp cache.cpp delete.cpp','{first} handles.cpp delete.cpp')
 text=once(text,"remaining=['Actual17A0F0 and22FEA0 lifetime callbacks if reached','Positive authored constraint creation','Fault/alias/mutable-observer domains','Production integration and visible multiplayer']","remaining=['Positive177FA0/177020 child-link and tagged-value branches remain explicit services','Deferred destruction and other unseen input domains','Production integration and visible multiplayer','Fault/alias/mutable-observer domains']")
 text=text.replace('wrong_list_count','wrong_read_owner').replace('Original22D0A0 rejects incorrect native list removal count','Original17A4A0 rejects incorrect native read ownership')
 namespace={'__file__':__file__,'__name__':'native_constraint_handle_destruction_base','once':once}
 exec(compile(text,str(base),'exec'),namespace);return namespace['main']()
if __name__=='__main__':raise SystemExit(main())
