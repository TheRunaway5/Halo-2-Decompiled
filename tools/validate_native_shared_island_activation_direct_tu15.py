#!/usr/bin/env python3
"""Direct typed316D40 shared binding on genuine inactive Gemini islands."""
from pathlib import Path
from prepare_native_shared_island_activation_binding_tu15 import once
def direct_body(body):
 body=once(body,'put(sp+4u,with_linear?linear_data:0u);','put(sp+4u,island);')
 body=once(body,'object_entry.rip=0xb9800u;','object_entry.rip=0x316d40u;object_entry.rcx=world;')
 body=once(body,'native_tu15_set_object_velocities_with_residuals(memory,object,with_linear?&linear:nullptr,with_angular?&angular:nullptr,fp,0,',
  'native_tu15_activate_island_with_residuals(memory,{world},{island},fp,incoming_depth,')
 body=once(body,'{"entry",0xb9800u}','{"entry",0x316d40u},{"direct_typed_island_binding",true},{"captured316D40_caller_ABI",false},{"declared_incoming_x87_depth",incoming_depth}')
 return body
def direct_validator(text):
 text=once(text,'and(a.transport or a.live_depths==[0])','')
 text=once(text,'variations=(2,3)if a.transport else range(5)','variations=(2,)')
 text=once(text,"mutation_unit='activation_contact_manager'if a.manager else'body_policy'if a.policy else'object_motion'","mutation_unit='island_activation'")
 first=text.index('  mutations=(');end=text.index('\n  if a.policy:mutations=',first)
 text=text[:first]+"  mutations=(('wrong_active_flag','m.word(0x316d97u,island.identity+0x29u,1,1)','m.word(0x316d97u,island.identity+0x29u,0,1)'),)"+text[end:]
 text=once(text,"(2 if a.transport else 0)",'2')
 text=once(text,'  driver.write_text(','  body=direct_body(body)\n  driver.write_text(')
 return text
def main():
 base=Path(__file__).with_name('validate_native_constraint_effect_selector_binding_tu15.py');text=base.read_text()
 needle=" namespace={'__file__':__file__,'__name__':'native_constraint_effect_selector_binding_base'}"
 text=once(text,needle," text=direct_validator(text)\n namespace={'__file__':__file__,'__name__':'native_shared_island_activation_direct_base','direct_body':direct_body}")
 namespace={'__file__':__file__,'__name__':'native_shared_island_activation_direct_stage','direct_body':direct_body,'direct_validator':direct_validator}
 exec(compile(text,str(base),'exec'),namespace);return namespace['main']()
if __name__=='__main__':raise SystemExit(main())
