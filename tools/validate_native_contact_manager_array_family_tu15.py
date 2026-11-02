#!/usr/bin/env python3
"""Genuine original/native reserve-to-manager-destruction capacity families.

Each pipeline starts from the same actual typed BEFORE. Both sides call their
own2E31E0 reserve owners, then3232B0; no original allocation/capacity postimage
is supplied to native code. Original phase transitions are ordinary typed ABI
marshaling between genuine source leaves, not a virtual CPU physics program.
"""
import inspect,struct
import validate_native_contact_manager_cleanup_typed_before_tu15 as parent
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT
from generate_physics_live_support_tu15 import complete_body

def patch_child(text,banks,capture,typed,environment):
 child=parent.parent.collector.child;child.OWNERS=(*child.OWNERS,0x2e31e0);environment['OWNERS']=child.OWNERS
 def word(p):
  for first,raw in banks.items():
   if first<=p and p+4<=first+len(raw):return int.from_bytes(raw[p-first:p-first+4],'little')
  raise AssertionError(hex(p))
 image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
 for pc in complete_body(code,0x2e31e0).code:
  i=code.at(pc)
  for first,raw in banks.items():
   if first<=pc and pc+i.size<=first+len(raw):assert raw[pc-first:pc-first+i.size]==i.bytes
 manager=typed['manager'];collector=manager+0x10;arrays=[(collector+0x90,28),(collector+0x34,32),(collector+0x20,1)];rows=[]
 profiles=[('actual',[],0),('points',arrays[:1],1),('records',arrays[1:2],1),('bytes',arrays[2:],1),('all9',arrays,9),('all33',arrays,33),('all64',arrays,64),('zero_points',arrays[:1],-1)]
 if word(arrays[1][0]+4)==0:profiles.append(('zero_records',arrays[1:2],-1))
 for name,selected,minimum in profiles:
  if minimum<0:assert all(word(p+4)==0 for p,stride in selected),'Zero reserve requires genuinely empty current DATA'
  phases=[dict(entry=0x2e31e0,receiver=0,arguments=[p,0 if minimum<0 else max((word(p+8)&0x7fffffff)+1,minimum),stride])for p,stride in selected]
  phases.append(dict(entry=0x3232b0,receiver=manager,arguments=[typed['disposing']]))
  rows.append(dict(name='array_family_'+name,entry=phases[0]['entry'],receiver=phases[0]['receiver'],arguments=phases[0]['arguments'],pipeline=phases,user_words=[],before_fp_input=[typed['floating_point']['x87_control_word'],typed['floating_point']['x87_status'],typed['floating_point']['mxcsr']]))
 global FAMILY_INPUTS
 FAMILY_INPUTS=dict(actual_manager=manager,actual_array_headers=[dict(header=p,stride=stride,count=word(p+4),capacity_flags=word(p+8))for p,stride in arrays],pipelines=rows)
 child.cases=lambda:rows;environment['cases']=child.cases
 # The original observer executes only authenticated source bodies. At each
 # completed leaf it marshals the next ordinary receiver/formal argument ABI.
 phase='''if(pc==stop&&m.contains("pipeline")&&phase+1u<m.at("pipeline").size()){
 ++phase;const auto&next=m.at("pipeline")[phase];const auto args=next.at("arguments").get<std::vector<std::uint32_t>>();std::vector<std::uint32_t>packet{std::uint32_t(stop)};packet.insert(packet.end(),args.begin(),args.end());
 regs.rsp=0x0200f000u;regs.rip=next.at("entry").get<std::uint32_t>();regs.rcx=next.at("receiver").get<std::uint32_t>();regs.rax=0;regs.rdx=0;regs.rbx=0;regs.rbp=0;regs.rsi=0;regs.rdi=0;regs.eflags&=~0x400u;
 iovec local{packet.data(),packet.size()*4u},remote{reinterpret_cast<void*>(std::uintptr_t(regs.rsp)),packet.size()*4u};check(process_vm_writev(child.pid,&local,1,&remote,1,0)==std::ptrdiff_t(packet.size()*4u),"Cannot marshal original next pipeline ABI");request(PTRACE_SETREGS,child.pid,nullptr,&regs);continue;
}if(pc==stop)break;'''
 needle="observer=(ROOT/'tools/observe_native_array_growth_tu15.cpp').read_text();needle="
 replacement="observer=(ROOT/'tools/observe_native_array_growth_tu15.cpp').read_text().replace('unsigned count=0,fault_count=0;','unsigned count=0,fault_count=0,phase=0;').replace('if(pc==stop)break;',"+repr(phase)+");needle="
 assert text.count(needle)==1;text=text.replace(needle,replacement)
 # Preserve the default observer's exact one-stop anchor for the other
 # authenticated RET callback instrumentation; the replacement ends in it.
 needle="('fault','mutations')";assert text.count(needle)==0 # Source original() is loaded later, not inlined here.
 needle="environment=dict(base.__dict__,observer_source=observer_source);"
 change="source=source.replace(\"('fault','mutations')\",\"('fault','mutations','pipeline')\");"+needle
 assert text.count(needle)==1;text=text.replace(needle,change)
 text=text.replace("'tools/validate_native_contact_manager_cleanup_typed_before_tu15.py'", "'tools/validate_native_contact_manager_cleanup_typed_before_tu15.py','tools/validate_native_contact_manager_array_family_tu15.py','include/halo2/native_tu15_array_reserve.hpp','src/physics/native_tu15_array_reserve.cpp'")
 text=text.replace("'native_tu15_contact_manager_cleanup.cpp'):shutil.copyfile", "'native_tu15_contact_manager_cleanup.cpp','native_tu15_array_reserve.cpp'):shutil.copyfile")
 text=text.replace('native_tu15_contact_manager_cleanup.cpp)', 'native_tu15_contact_manager_cleanup.cpp native_tu15_array_reserve.cpp)')
 text=text.replace('#include "halo2/native_tu15_contact_manager_cleanup.hpp"', '#include "halo2/native_tu15_contact_manager_cleanup.hpp"'+chr(92)+'n#include "halo2/native_tu15_array_reserve.hpp"')
 dispatch='''if(c.contains("pipeline")){for(const auto&phase:c.at("pipeline")){const auto e=phase.at("entry").get<std::uint32_t>();const auto args=phase.at("arguments").get<std::vector<std::uint32_t>>();if(e==0x2e31e0u)native_tu15_reserve_array(memory,NativeTu15ArrayHeader{args[0]},args[1],args[2],fp,growth,observe);else{check(e==0x3232b0u,"Unqualified native manager pipeline phase");NativeTu15ContactManagerCleanupServices manager_services;manager_services.collector=collector_services;result=native_tu15_delete_contact_manager(memory,phase.at("receiver"),args[0],{},fp,manager_services,observe,&collector_stats);}}check(result==c.at("return_word"),"Native manager pipeline return differs");}else '''
 needle='if(entry==0x323220u)native_tu15_dispose_contact_reference';assert text.count(needle)==1;text=text.replace(needle,dispatch+needle)
 return text

def main():
 carrier=parent.parent.collector;carrier.patch_native_manager_array_family=patch_child
 original=inspect.getsource
 def getsource(obj):
  text=original(obj)
  if obj is carrier.main:
   needle="exec(source,env);return env['main']()";assert text.count(needle)==1
   text=text.replace(needle,"exec(patch_native_manager_array_family(source,banks,capture,typed,env),env);return env['main']()")
  return text
 inspect.getsource=getsource
 result=parent.main()
 if result==0:
  import json,sys
  from pathlib import Path
  output=Path(sys.argv[sys.argv.index('--output')+1]);(output/'family-input-provenance.json').write_text(json.dumps(FAMILY_INPUTS,indent=2)+'\n')
 return result
if __name__=='__main__':raise SystemExit(main())
