#!/usr/bin/env python3
"""Qualify the exact staged point PART callback on217 complete native inputs.

This proves the callback and its actual owned scratch aliases, not a captured
positive PART point-parent invocation. The actual point input remains separate.
"""
from pathlib import Path
import hashlib,json,os,sys
import validate_native_part_contact_binding_tu15 as parent
ROOT=Path(__file__).resolve().parents[1]
def main():
 stage=Path(os.environ['HALO2_POINT_PART_STAGE']).resolve()
 base=parent.parent.parent.parent.parent.prop.base
 base.FILES=(*base.FILES,'tools/validate_native_contact_point_part_callback_tu15.py')
 header=stage/'staged/include/halo2/native_tu15_contact_point_binding.hpp';text=header.read_text();begin=text.index('        gameplay.release_part_contact=');end=text.index('        NativeTu15IslandConstraintDetachServices detach;',begin);callback=text[begin:end]
 write=Path.write_text;scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
 def staged_write(path,text,*args,**kwargs):
  if path.is_relative_to(scratch)and path.name=='native_tu15_array_growth_tests.cpp':
   text=text.replace('const auto allocation=[&]','const auto pool_allocation=[&]').replace('lower.allocate_owner=allocation;','lower.allocate_owner=pool_allocation;')
   text=text.replace('std::uint32_t result{};','std::uint32_t result_word{};').replace('check(result==c.at("return_word")','check(result_word==c.at("return_word")').replace(')result=',')result_word=').replace('result_word=allocation(','result_word=pool_allocation(')
   text='#include "halo2/native_tu15_contact_point_binding.hpp"\n'+text
   destination=path.parents[1]/'include/halo2/native_tu15_contact_point_binding.hpp';destination.write_bytes(header.read_bytes())
   before='NativeTu15PartContactBindingActivity part_binding_activity;'
   assert text.count(before)==1
   setup=r'''NativeTu15ContactPointBindingActivity result;
      auto&part_binding_activity=result.part;
      NativeTu15ContactPointBindingScratch scratch{PRIVATE+0x80u,PRIVATE+0x100u,PRIVATE+0x120u,PRIVATE+0x100u};
      constexpr unsigned incoming_x87_depth=0;const auto&allocation=growth;
      const auto valid=[](std::uint32_t p,unsigned bytes){return p&&std::uint64_t(p)+bytes<=0x100000000ull;};
      const auto overlap=[](std::uint32_t a,unsigned an,std::uint32_t b,unsigned bn){return std::uint64_t(a)<std::uint64_t(b)+bn&&std::uint64_t(b)<std::uint64_t(a)+an;};
      const auto bad_profile=[&](const char*why){++result.unsupported_profiles;throw std::runtime_error(why);};
      NativeTu15GameplayContactRemovalServices gameplay;
'''+callback
   text=text.replace(before,setup)
   before='native_tu15_update_bound_part_contact_response(memory,args[0],std::bit_cast<std::int32_t>(args[1]),args[2],{PRIVATE+0x100u,PRIVATE,{PRIVATE+0x200u,PRIVATE+0x204u}},fp,0,growth,observe,&part_binding_activity)';assert text.count(before)==1
   text=text.replace(before,'gameplay.release_part_contact(args[0],std::bit_cast<std::int32_t>(args[1]),args[2],fp)')
  return write(path,text,*args,**kwargs)
 Path.write_text=staged_write
 try:return parent.main()
 finally:
  Path.write_text=write
  p=Path(sys.argv[sys.argv.index('--output')+1])/'report.json'
  if p.exists():
   report=json.loads(p.read_text());report.update(production_point_callback_sha256=hashlib.sha256(callback.encode()).hexdigest(),production_point_header_sha256=hashlib.sha256(header.read_bytes()).hexdigest(),point_stage_pins_sha256=hashlib.sha256((stage/'source-pins.json').read_bytes()).hexdigest(),actual_point_part_invocation_claimed=False,point_scratch_layout=dict(event='private+80/20B',material='private+100/12B',removed='private+10C/4B',first='private+110/4B',second='private+120/4B',later_info='private+100/16B',later_world_result='private+120/1B'),scope=__doc__);p.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
if __name__=='__main__':raise SystemExit(main())
