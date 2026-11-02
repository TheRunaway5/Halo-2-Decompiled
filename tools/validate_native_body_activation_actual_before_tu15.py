#!/usr/bin/env python3
"""Compose genuine body-listener DATA with native positive22D460 creation.

Uses the qualified component harness unchanged, selecting actual body slots
as declared normal2816A0 formal arguments. It does not recover a caller ABI.
"""
from pathlib import Path
def once(text,needle,replacement):
 if text.count(needle)!=1:raise RuntimeError('Actual body-listener qualification seam changed: '+needle[:100])
 return text.replace(needle,replacement,1)
def main():
 base=Path(__file__).with_name('validate_native_body_constraint_actual_before_tu15.py');text=base.read_text()
 text=once(text,"'parent':'src/physics/native_tu15_body_constraint_update.cpp'","'listener':'src/physics/native_tu15_activation_services.cpp','parent':'src/physics/native_tu15_body_constraint_update.cpp'")
 text=once(text,"names=['body_constraint_update'","names=['activation_services','body_constraint_update'")
 text=once(text,"files=[production_parent,*units.values()","files=[ROOT/'tools/validate_native_body_constraint_actual_before_tu15.py',production_parent,*units.values()")
 text=once(text,'owners=[0x22d460,','owners=[0x2816a0,0xb9390,0x22d460,')
 text=once(text,"driver=scratch/'driver.cpp';driver.write_text(","fragment=listener_fragment((ROOT/'tools/observe_native_body_constraint_actual_before_tu15.cpp').read_text())\n  driver=scratch/'driver.cpp';driver.write_text('#include \\\"halo2/native_tu15_activation_services.hpp\\\"\\n'+")
 text=once(text,"+(ROOT/'tools/observe_native_body_constraint_actual_before_tu15.cpp').read_text())",'+fragment)')
 namespace={'__file__':__file__,'__name__':'native_body_activation_actual_before_base','listener_fragment':listener_fragment}
 exec(compile(text,str(base),'exec'),namespace);return namespace['main']()
def listener_fragment(text):
 text=once(text,'const auto ledger=activation_trace(original,metadata,memory,calls,steps);','''std::vector<std::uint32_t>bodies;const auto component=capture.at("component").at("row").get<std::uint32_t>(),
   body_bank=original.word(component+0x70u),body_count=original.word(component+0x74u);check(body_count>0&&body_count<=64,"Unbounded genuine listener body cohort");
  for(unsigned n=0;n<body_count;++n)bodies.push_back(original.word(body_bank+n*0x60u+0x40u));
  J ledger=J::array();for(const auto body:bodies){put(sp,std::uint32_t(0x1ccacau));put(sp+4u,body);entry=original.regs();entry.rsp=sp;entry.rip=0x2816a0u;
   request(PTRACE_SETREGS,original.pid,nullptr,&entry);const auto part=activation_trace(original,metadata,memory,calls,steps);for(const auto&e:part)ledger.push_back(e);}''')
 text=once(text,'native_tu15_update_body_constraints(memory,datum,{{scratch}},fp,services,{},&parent);','''NativeTu15ActivationBodyServices activated;NativeTu15ActivationServiceStats listeners;
  activated.publish_body_update=[&](auto component,auto&state){native_tu15_update_body_constraints(memory,component,{{scratch}},state,services,{},&parent);};
  for(const auto body:bodies)native_tu15_body_activation_listener(memory,{body},fp,activated,{},&listeners);''')
 text=once(text,'{"parent_calls",parent.calls}','{"sequential_body_arguments",bodies},{"body_listener_calls",listeners.body_listener_calls},{"body_updates",listeners.body_updates},{"object_wakes",listeners.object_wake_calls},{"object_headers_marked",listeners.object_headers_marked},{"parent_calls",parent.calls}')
 text=once(text,'{"entry",0x22d460u},{"arguments",J::array({datum})}','{"entry",0x2816a0u},{"body_arguments_from_actual_component_slots",true}')
 return text
if __name__=='__main__':raise SystemExit(main())
