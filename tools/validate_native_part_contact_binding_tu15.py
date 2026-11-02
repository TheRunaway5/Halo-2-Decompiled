#!/usr/bin/env python3
"""Qualify the same production PART/evaluator service builder on all217 inputs."""
from pathlib import Path
import os
import validate_native_part_response_conveyor_tu15 as parent

def main():
 base=parent.parent.parent.parent.prop.base
 base.FILES=(*base.FILES,'include/halo2/native_tu15_part_contact_binding.hpp','tools/validate_native_part_contact_binding_tu15.py')
 write=Path.write_text;scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
 def staged_write(path,text,*args,**kwargs):
  if path.is_relative_to(scratch)and path.name=='native_tu15_array_growth_tests.cpp':
   text='#include "halo2/native_tu15_part_contact_binding.hpp"\n'+text
   before='native_tu15_update_part_contact_response(memory,args[0],std::bit_cast<std::int32_t>(args[1]),args[2],{PRIVATE+0x100u,PRIVATE,{PRIVATE+0x200u,PRIVATE+0x204u}},fp,0,growth,evaluator,observe)'
   assert text.count(before)==1
   text=text.replace(before,'native_tu15_update_bound_part_contact_response(memory,args[0],std::bit_cast<std::int32_t>(args[1]),args[2],{PRIVATE+0x100u,PRIVATE,{PRIVATE+0x200u,PRIVATE+0x204u}},fp,0,growth,observe,&part_binding_activity)')
   text=text.replace('      const auto host_sw=', '      NativeTu15PartContactBindingActivity part_binding_activity;\n      const auto host_sw=')
   anchor='      check(calls==c.at("allocator_calls"),name+" actual allocator request order differs");'
   assert text.count(anchor)==1
   text=text.replace(anchor,anchor+'\n      check(part_binding_activity.attempts==1&&part_binding_activity.completed==1&&part_binding_activity.refusals==0&&part_binding_activity.response.calls==1&&part_binding_activity.function.unsupported_kinds==0&&part_binding_activity.function.unsupported_fp_profiles==0&&part_binding_activity.response.conveyor.function_calls==part_binding_activity.function.calls,name+" production part builder activity differs");')
  return write(path,text,*args,**kwargs)
 Path.write_text=staged_write
 try:return parent.main()
 finally:Path.write_text=write

if __name__=='__main__':raise SystemExit(main())
