#!/usr/bin/env python3
"""Qualify the actual production point builder and compact World DATA layout."""
import inspect
import validate_native_contact_point_actual_pipeline_tu15 as parent


def transform_driver(driver):
    old='native_tu15_remove_contact_point(memory,c.at("receiver"),c.at("arguments")[0],{PRIVATE+0x100,PRIVATE+0x200,PRIVATE+0x240},fp,point,observe);'
    new='native_tu15_remove_bound_contact_point(memory,c.at("receiver"),c.at("arguments")[0],{PRIVATE+0x3b4,PRIVATE+0x3c8,PRIVATE+0x3dc,PRIVATE+0x3c8},fp,0,{},observe,&binding);'
    assert driver.count(old)==1;driver=driver.replace(old,new)
    driver=driver.replace('check(game_stats.notifications==1', 'check(binding.attempts==1&&binding.completed==1&&binding.refusals==0&&binding.unsupported_profiles==0&&binding.unqualified_services==0,name+" production binding differs");check(game_stats.notifications==1')
    driver=driver.replace('if(!read_pass){entries[0x1d8880u]', 'if(!read_pass){entries[0x2e4d60u]=binding.noop_listeners;entries[0x281d40u]=binding.gameplay_listeners;entries[0x1d8880u]')
    guard=r'''
  const auto&initial=fixture.at("cases")[0];
  const auto value=[](Memory&m,std::uint32_t p){std::array<std::byte,4>b{};m.read(p,b);std::uint32_t v{};for(unsigned n=0;n<4;++n)v|=std::to_integer<unsigned>(b[n])<<(8*n);return v;};
  const auto put=[](Memory&m,std::uint32_t p,std::uint32_t v){std::array<std::byte,4>b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(v>>(8*n));m.write(p,b);};
  for(unsigned guard=0;guard<3;++guard){
    Memory memory;for(const auto&bank:initial.at("banks"))memory.banks.emplace(bank.at("address"),file(path.parent_path()/bank.at("before_file").get<std::string>()));memory.banks.emplace(PRIVATE,std::vector<std::byte>(0x1000,std::byte{0xa5}));
    const auto manager=initial.at("receiver").get<std::uint32_t>();
    NativeTu15ContactPointBindingScratch scratch{PRIVATE+0x3b4,PRIVATE+0x3c8,PRIVATE+0x3dc,PRIVATE+0x3c8};
    if(guard==1)scratch.result_byte=scratch.event;
    if(guard==2){const auto first=value(memory,manager+0x1c),entries=value(memory,first+0x60),listener=value(memory,entries);put(memory,listener,PRIVATE+0x800);put(memory,PRIVATE+0x804,0x2815f0u);}
    const auto before=memory.banks;NativeTu15PhysicsFrameFp fp{initial.at("before_fp")[0],initial.at("before_fp")[1],initial.at("before_fp")[2]};NativeTu15ContactPointBindingActivity activity;bool refused=false;
    try{native_tu15_remove_bound_contact_point(memory,manager,initial.at("arguments")[0],scratch,fp,guard==0?1u:0u,{}, {},&activity);}catch(const std::runtime_error&){refused=true;}
    check(refused&&activity.attempts==1&&activity.completed==0&&activity.refusals==1,"Point binding did not refuse an unqualified profile/owner");
    check(guard==2?(activity.unqualified_services==1&&activity.unsupported_profiles==0):(activity.unqualified_services==0&&activity.unsupported_profiles==1),"Point binding refusal classification differs");
    for(const auto&bank:initial.at("banks"))check(memory.banks.at(bank.at("address"))==before.at(bank.at("address")),"Refused point binding changed captured physics DATA");
    if(guard<2)check(memory.banks==before,"Early point profile guard wrote private DATA");
    check(J::array({fp.control_word,fp.status,fp.mxcsr})==initial.at("before_fp"),"Refused point binding changed FP policy");
  }
'''
    anchor='  check(cases==3,';assert driver.count(anchor)==1;driver=driver.replace(anchor,guard+anchor)
    return driver.replace('J{{"passed",true},{"strict_source_private_raw_ledger_passed",false}', 'J{{"passed",true},{"production_builder",true},{"binding_refusal_controls",3},{"strict_source_private_raw_ledger_passed",false}')


def main():
    parent.HEADERS=(*parent.HEADERS,'native_tu15_contact_point_binding.hpp')
    source=inspect.getsource(parent.main);start=source.index("  setup='''");end=source.index("''';driver=driver[:begin]+setup+driver[end:]",start)
    setup='''      NativeTu15PhysicsFrameFp fp{c.at("before_fp")[0],c.at("before_fp")[1],c.at("before_fp")[2]};NativeTu15ContactPointBindingActivity binding;
      auto&game_stats=binding.gameplay;auto&world_stats=binding.world;auto&detach_stats=binding.detach;
'''
    source=source[:start]+'  setup='+repr(setup)+source[end+3:]
    anchor="  (project/'driver.cpp').write_text(driver);";assert source.count(anchor)==1;source=source.replace(anchor,'  driver=transform_driver(driver);'+anchor.strip())
    source=source.replace("'tools/validate_native_contact_point_actual_pipeline_tu15.py','tools/validate_native_array_growth_tu15.py'", "'tools/validate_native_contact_point_actual_pipeline_tu15.py','tools/validate_native_contact_point_binding_tu15.py','tools/validate_native_array_growth_tu15.py'")
    source=source.replace("scope='Complete actual1470 typed point-parent pipeline; all captured banks+ordered captured-DATA operands/preimages/FP and logical current-owner counts. Every selected dependency native. Ordinary original leaf ABI only; no captured GP. Fresh native private DATA is intentionally independent; source-private raw ledger remains RED separately, not waived. No live multiplayer/frame/camera claim.'", "production_builder=True,binding_refusal_controls=['live_x87_profile','overlapping_DATA','unknown_point_listener2815F0'],world_relative_scratch=dict(event=-0x4c,removal_info=-0x38,result_byte=-0x24,detach_info=-0x38),scope='Actual production point binding on genuine1470 current DATA; every captured bank/ordered captured-DATA operand/preimage/FP/logical owner count compared. Exact compact World buffer offsets, permitted info lifetime reuse and independent00/A5/5A native DATA. Source-private raw ledger remains RED separately. Three refusal models select no CPU fallback and preserve all captured physics DATA. No live frame/camera claim.'")
    environment=dict(parent.__dict__,transform_driver=transform_driver);exec(source,environment);return environment['main']()


if __name__=='__main__':
    raise SystemExit(main())
