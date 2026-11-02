#!/usr/bin/env python3
"""Physical17DEC0 parameter and actual frame-chain publication."""
from pathlib import Path
def once(text,needle,replacement):
 if text.count(needle)!=1:raise RuntimeError('Runtime handle qualification seam changed: '+needle[:100])
 return text.replace(needle,replacement,1)
def main():
 base=Path(__file__).with_name('validate_native_model_constraint_removal_tu15.py');text=base.read_text()
 first=text.index(' units={');last=text.index('\n observer=',first)
 text=text[:first]+" units={'parameters':ROOT/'src/physics/native_tu15_constraint_runtime_parameters.cpp','allocator':ROOT/'src/core/native_tu15_data_array_allocate.cpp'}"+text[last:]
 text=once(text,"default=r'(gemini-column[01]_[01]|elongation-crates_[12]|actual-warthog_[1-4])_[0-3]'","default=r'(gemini-column[01]_0|elongation-crates_0)_[0-3]'")
 text=once(text,"observer=ROOT/'tools/observe_native_model_constraint_removal_tu15.cpp'","observer=ROOT/'tools/observe_native_constraint_runtime_parameters_tu15.cpp'")
 text=once(text,' pins={'," files.extend([ROOT/'tools/validate_native_model_constraint_removal_tu15.py',ROOT/'include/halo2/native_tu15_constraint_runtime_parameters.hpp'])\n pins={")
 text=once(text,'(0x22d0a0,0x22e1b0,0x22e390,0xde8c0,0x22fe30,0x16e8a0)','(0x17dec0,)')
 text=once(text,"  for pc in (0x22d13f,0x22d155,0x22d233,0x22e1fc):metadata[str(pc)]['stop_before']=True",'  # The parameter owner has no external callback.')
 text=once(text,"    metadata[str(pc)]=dict(bytes=i.bytes.hex(),size=i.size,memory=operands)",
  "    if i.mnemonic=='rep movsd':operands=[dict(base='esi',index='',scale=1,displacement=0,bytes=4,read=True,write=False),dict(base='edi',index='',scale=1,displacement=0,bytes=4,read=False,write=True)]\n    metadata[str(pc)]=dict(bytes=i.bytes.hex(),size=i.size,memory=operands,repeat_count=i.mnemonic=='rep movsd')")
 text=once(text,"  driver=scratch/'driver.cpp';",'''  prefix=once(prefix,'for(const auto&m:i.at("memory")){const auto p=',
   'for(const auto&m:i.at("memory")){if(i.value("repeat_count",false)&&std::uint32_t(r.rcx)==0u)continue;const auto p=')
  driver=scratch/'driver.cpp';''')
 text=once(text,'#include "halo2/native_tu15_model_constraint_removal.hpp"\\n','#include "halo2/native_tu15_constraint_runtime_parameters.hpp"\\n')
 text=once(text,"text=units['removal'].read_text();needle='read(m,list_row+2u,2)-1u'","text=units['parameters'].read_text();needle='record+3u,read(m,record+3u,1)|2u'")
 text=once(text,"text.replace(needle,'read(m,list_row+2u,2)-2u')","text.replace(needle,'record+3u,read(m,record+3u,1)|4u')")
 text=once(text,"first='removal.cpp'if variant=='native'else'wrong_list_count.cpp'","first='parameters.cpp'if variant=='native'else'wrong_list_count.cpp'")
 text=once(text,'{first} release.cpp slots.cpp cache.cpp delete.cpp','{first} allocator.cpp')
 text=once(text,"remaining=['Actual17A0F0 and22FEA0 lifetime callbacks if reached','Positive authored constraint creation','Fault/alias/mutable-observer domains','Production integration and visible multiplayer']","remaining=['Full22FEA0 caller composition','Non-forward-copy,overlap,fault and mutable-observer domains','Production caller composition and visible multiplayer']")
 text=text.replace('wrong_list_count','wrong_update_flag').replace('Original22D0A0 rejects incorrect native list removal count','Original17DEC0 rejects incorrect native parameter dirty flag')
 namespace={'__file__':__file__,'__name__':'native_constraint_runtime_parameters_base','once':once}
 exec(compile(text,str(base),'exec'),namespace);return namespace['main']()
if __name__=='__main__':raise SystemExit(main())
