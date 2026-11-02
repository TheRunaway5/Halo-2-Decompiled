#!/usr/bin/env python3
"""Same native pair wake builder under actual typed02 original320EF0 entries."""
import inspect, os, re
from pathlib import Path
import validate_native_pair_create_tu15 as parent
ROOT=Path(__file__).resolve().parents[1]

def main():
    assert os.environ.get('HALO2_TEST_SCRATCH')
    text=inspect.getsource(parent.main)
    text=text.replace('for poison in range(14):','for poison in range(16):')
    cmake=(ROOT/'cmake/PhysicsLiveWorldTu15.cmake').read_text()
    start=cmake.index('add_library(halo2_native_object_velocity STATIC');end=cmake.index(')',start)
    units=re.findall(r'src/[^\s)]+\.cpp',cmake[start:end])
    for chunk in re.findall(r'target_sources\(halo2_native_object_velocity\s+PRIVATE\s+([^)]*)\)',cmake,re.S):
        units+=re.findall(r'src/[^\s)]+\.cpp',chunk)
    units+=['src/core/native_tu15_data_array_allocate.cpp','src/core/native_tu15_data_array_delete.cpp',
        'src/core/native_tu15_data_array_iteration.cpp','src/rendering/world/native_tu15_camera_projection.cpp']
    units+=['src/physics/native_tu15_'+name+'.cpp' for name in (
        'array_growth','array_allocator','array_reserve','stack_arena','geometry','shape_bounds',
        'centered_shape_bounds','compound_shape_bounds','sphere_shape_bounds')]
    units=list(dict.fromkeys(units))
    files=['include/halo2/native_tu15_pair_island_wake_binding.hpp',str(Path(__file__).relative_to(ROOT)),*units]
    text=text.replace("'tools/validate_native_pair_create_tu15.py'", "'tools/validate_native_pair_create_tu15.py',"+','.join(repr(x)for x in files))
    needle='environment=dict(parent.__dict__);'
    text=text.replace(needle,"prefix=prefix.replace(\"  args=[typed['first']\",\"  assembly.append('.section .native_data,\\\"aw\\\"\\\\n.zero 16384');sections.append('.native_data 0x04000000:{*(.native_data)}')\\n  args=[typed['first']\");"+needle)
    old='services.activate_island=[&](auto world,auto island,auto&fp){call(0x316d40,world.identity,{island.identity},0x32103au,fp);};'
    replacement='''services.activate_island=[&](auto world,auto island,auto&fp){
      actual_calls.push_back(J{{"entry",0x316d40u},{"arguments",J::array({island.identity})},{"this",world.identity}});
      Memory child_memory(native);NativeTu15PairIslandWakeActivity activity;
      NativeTu15ObjectActivationBindingActivity activation;
      native_tu15_activate_pair_island(child_memory,world,island,fp,0,
        {0x4002020u,0x4002024u,0x4002040u,0x4002080u,0x800u},{},&activity,&activation);
      check(activity.attempts==1&&activity.completed==1&&activity.refusals==0&&
        activation.activation.island_calls==1&&activation.residual_calls==0,
        "Native pair wake lifetime/admission differs");
      ++native_wakes;native_body_updates+=activation.services.body_updates;
    };'''
    transform='driver='+repr('#include "halo2/native_tu15_pair_island_wake_binding.hpp"\n')+'+driver;'
    transform+='assert driver.count('+repr(old)+')==1;driver=driver.replace('+repr(old)+','+repr(replacement)+');'
    setup='''if(variant==7){
      const auto first=original.word(original.word(pair)-0x10u+0x20u),second=original.word(original.word(pair+4u)-0x10u+0x20u);
      const auto first_island=original.word(first+0x44u),second_island=original.word(second+0x44u);
      const auto other=first_island==handler-0x1cu?second_island:first_island,world=original.word(handler+4u);
      check(first_island!=second_island&&std::to_integer<unsigned>(original.read(other+0x29u,1)[0])==1,"Declared sleep control requires actual distinct active island");
      const auto active=original.word(world+8u),count=original.word(world+0xcu),index=original.word(other+0x24u);
      check(index<count&&original.word(active+index*4u)==other,"Declared sleep control needs coherent actual active list");
      const auto moved=original.word(active+(count-1u)*4u);put(active+index*4u,moved);put(moved+0x24u,index);put(world+0xcu,count-1u);
      const auto inactive=original.word(world+0x14u),inactive_count=original.word(world+0x18u);
      check(inactive_count<512u,"Declared inactive DATA exceeds private extent");
      for(unsigned n=0;n<inactive_count;++n)put(0x4003000u+n*4u,original.word(inactive+n*4u));
      put(0x4003000u+inactive_count*4u,other);put(world+0x14u,0x4003000u);put(world+0x18u,inactive_count+1u);put(world+0x1cu,0x80000400u);
      put(other+0x24u,inactive_count);put(other+0x29u,0u,1);
    }'''
    transform+='driver=driver.replace("const auto changed_cursor=",'+repr(setup+'const auto changed_cursor=')+');'
    transform+='driver=driver.replace("std::ifstream meta_input(argv[5]);","if(variant==7)banks.push_back({0x4003000u,original.read(0x4003000u,4096),{}});std::ifstream meta_input(argv[5]);");'
    transform+='driver=driver.replace("NativeTu15PairHandlerServices services;","unsigned native_wakes{},native_body_updates{};NativeTu15PairHandlerServices services;").replace('+repr('{"actual_original_child_entry",true}')+','+repr('{"native_island_wakes",native_wakes},{"native_body_updates",native_body_updates},{"declared_sleep_list_control",variant==7},{"actual_original_child_entry",true}')+');'
    extra='\n  for name in '+repr(units)+':\n   target=project/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,target)\n  shutil.copyfile(ROOT/"src/physics/native_tu15_contact_unwind_internal.hpp",project/"src/physics/native_tu15_contact_unwind_internal.hpp")\n  '
    text=text.replace("(project/'driver.cpp').write_text(driver)",extra+transform+"(project/'driver.cpp').write_text(driver)")
    text=text.replace('add_executable(pair_create driver.cpp native.cpp)','add_executable(pair_create driver.cpp native.cpp '+' '.join(units)+')')
    begin=text.index("[('first_record_word'");end=text.index(']:\n   assert original_kernel.count',begin)
    text=text[:begin]+'[]'+text[end+1:]
    text=text.replace("scope='Two genuine","scope='Same production native316D40 pair-wake builder, no unconverted wake child admission; two genuine")
    env=dict(parent.__dict__);env['main']=None;exec(text,env);return env['main']()

if __name__=='__main__':raise SystemExit(main())
