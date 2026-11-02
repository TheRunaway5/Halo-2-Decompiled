#!/usr/bin/env python3
"""Create, publish, observe, then remove an actual authored constraint.

Both independent executions consume their own algorithm's newly published
handle. No original outcome, row, or guest postimage is supplied to native.
"""
from pathlib import Path
from validate_native_body_activation_actual_before_tu15 import once,listener_fragment
def main():
 base=Path(__file__).with_name('validate_native_body_constraint_actual_before_tu15.py');text=base.read_text()
 text=once(text,"'parent':'src/physics/native_tu15_body_constraint_update.cpp'","'listener':'src/physics/native_tu15_activation_services.cpp','removal':'src/physics/native_tu15_model_constraint_removal.cpp','delete':'src/core/native_tu15_data_array_delete.cpp','parent':'src/physics/native_tu15_body_constraint_update.cpp'")
 text=once(text,"names=['body_constraint_update'","names=['activation_services','model_constraint_removal','data_array_delete','body_constraint_update'")
 text=once(text,"files=[production_parent,*units.values()","files=[ROOT/'tools/validate_native_body_constraint_actual_before_tu15.py',ROOT/'tools/validate_native_body_activation_actual_before_tu15.py',production_parent,*units.values()")
 text=once(text,'owners=[0x22d460,','owners=[0x2816a0,0xb9390,0x22d0a0,0x16e8a0,0x22d460,')
 text=once(text,"driver=scratch/'driver.cpp';driver.write_text(","fragment=lifetime_fragment(listener_fragment((ROOT/'tools/observe_native_body_constraint_actual_before_tu15.cpp').read_text()))\n  driver=scratch/'driver.cpp';driver.write_text('#include \\\"halo2/native_tu15_activation_services.hpp\\\"\\n#include \\\"halo2/native_tu15_model_constraint_removal.hpp\\\"\\n'+")
 text=once(text,"+(ROOT/'tools/observe_native_body_constraint_actual_before_tu15.cpp').read_text())",'+fragment)')
 text=once(text,"(project/'wrong_flag.cpp').write_text(parent.replace(needle,'word(component+4u)&0x400u'))",'''removal=units['removal'].read_text();clear='store(m,observer,0x22d23fu,bank+offset+4u,0xffffffffu)';assert removal.count(clear)==1
  (project/'wrong_flag.cpp').write_text(removal.replace(clear,'store(m,observer,0x22d23fu,bank+offset+4u,0u)'))''')
 text=once(text,"n+'.cpp'for n in units if n!='parent'","n+'.cpp'for n in units if n not in ['parent','removal']")
 text=once(text,"cpp='parent.cpp'if variant=='native'else'wrong_flag.cpp'","cpp='parent.cpp removal.cpp'if variant=='native'else'parent.cpp wrong_flag.cpp'")
 text=text.replace('wrong_flag','wrong_global_clear').replace('Original22D460 rejects wrong native positive-loop gate','Original22D0A0 rejects wrong native authored reference clear')
 namespace={'__file__':__file__,'__name__':'native_constraint_authored_lifetime_base','listener_fragment':listener_fragment,'lifetime_fragment':lifetime_fragment}
 exec(compile(text,str(base),'exec'),namespace);return namespace['main']()
def lifetime_fragment(text):
 text=once(text,'std::size_t compared{},different{};J byte_differences=J::array();','''const auto source_handle=original.word(original.word(component+0x7cu)+4u),native_handle=native.word(native.word(component+0x7cu)+4u);
  check(source_handle!=0xffffffffu&&native_handle!=0xffffffffu,"Creation did not publish an owned authored reference");
  put(sp,std::uint32_t(0x1ccacau));put(sp+4u,source_handle);entry=original.regs();entry.rsp=sp;entry.rip=0x22d0a0u;
  request(PTRACE_SETREGS,original.pid,nullptr,&entry);const auto removed=activation_trace(original,metadata,memory,calls,steps);for(const auto&e:removed)ledger.push_back(e);
  NativeTu15ModelConstraintRemovalServices removal;NativeTu15ModelConstraintRemovalStats removal_stats;
  removal.release_runtime_handle=[&](auto,auto,auto&){++unbound_removals;throw std::runtime_error("Authored lifetime requires an additional handle owner");};
  removal.reference.remove_runtime_constraint=[&](auto,auto,auto,auto&){++unbound_runtime;throw std::runtime_error("Authored lifetime requires an additional runtime owner");};
  native_tu15_remove_model_constraint(memory,native_handle,fp,depth,removal,{},&removal_stats);
  std::size_t compared{},different{};J byte_differences=J::array();''')
 text=once(text,'{"parent_calls",parent.calls}','{"created_native_handle",native_handle},{"source_created_handle",source_handle},{"removal_calls",removal_stats.calls},{"global_rows",removal_stats.global_rows},{"global_references",removal_stats.global_references},{"list_removals",removal_stats.list_removals},{"list_deletions",removal_stats.list_deletions},{"instance_deletions",removal_stats.instance_deletions},{"parent_calls",parent.calls}')
 return text
if __name__=='__main__':raise SystemExit(main())
