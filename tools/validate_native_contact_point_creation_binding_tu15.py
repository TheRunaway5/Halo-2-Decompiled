#!/usr/bin/env python3
"""Qualify the production native322D20 binding with its unchanged physical cases."""
import inspect,json,re,sys
from pathlib import Path
import validate_native_contact_point_create_tu15 as component

def main():
    pending=[component.ROOT/'include/halo2/native_tu15_contact_point_create_binding.hpp',
        *(component.ROOT/'src/physics'/name for name in (*component.UNITS,'native_tu15_contact_point_removal.cpp'))]
    pinned={}
    while pending:
        path=pending.pop();name=str(path.relative_to(component.ROOT))
        if name in pinned:continue
        pinned[name]=component.sha(path)
        for included in re.findall(r'^#include "([^"]+)"',path.read_text(),re.M):
            child=component.ROOT/'include'/included if included.startswith('halo2/')else path.parent/included
            if child.is_file():pending.append(child)
    source=inspect.getsource(component.main)
    source=source.replace(' try:\n  assert sha(NEW_DEFAULT)'," try:\n  (a.output/'input-source-pins.json').write_text(json.dumps(pinned,indent=2)+'\\n')\n  assert sha(NEW_DEFAULT)")
    old='result=native_tu15_create_contact_point(memory,c.at("receiver"),{args[0]},{args[1]},{args[2]},args[3],{PRIVATE},fp,0,services,{},&activity);'
    new='result=native_tu15_create_bound_contact_point(memory,c.at("receiver"),{args[0]},{args[1]},{args[2]},args[3],{{PRIVATE},{PRIVATE+0x100u,PRIVATE+0x120u,PRIVATE+0x140u}},fp,0,bound,scope,{},&binding_activity);'
    assert source.count(old)==1
    source=source.replace(old,new)
    anchor="  (project/'driver.cpp').write_text(driver);source="
    assert source.count(anchor)==1
    injection='''  driver='#include "halo2/native_tu15_contact_point_create_binding.hpp"\\n'+driver
  driver=driver.replace('NativeTu15ContactPointCreateStats activity;','NativeTu15ContactPointCreationBindingActivity binding_activity;auto&activity=binding_activity.creation;')
  marker='      try{const auto args=c.at("arguments").get<std::vector<std::uint32_t>>();result=native_tu15_create_bound_contact_point'
  assert driver.count(marker)==1
  providers='''+repr('''      NativeTu15ContactPointCreationNativeServices bound;NativeTu15PairDeletionNativeScope scope;
      bound.allocation=[&](auto&){return growth;};
      bound.observe_listener=[&](const auto&view,auto listener,auto target,auto event){std::array<std::byte,2>id{};std::array<std::byte,32>fields{};view.read(event,id);view.read(event+4u,fields);listener_calls.push_back(J{{"owner",target.identity},{"receiver",listener},{"id",hex(id)},{"fields",hex(fields)}});};
''')+'''
  driver=driver.replace(marker,providers+marker)
  checks='''+repr('''      if(!read_pass&&name=="create_1_2_0.25"){
        const auto reset_bound=[&]{memory.banks.clear();for(const auto&bank:c.at("banks"))memory.banks.emplace(bank.at("address"),file(path.parent_path()/bank.at("before_file").get<std::string>()));memory.banks.emplace(PRIVATE,std::vector<std::byte>(4096,std::byte(0x7d)));memory.reads=J::array();memory.actual_writes=J::array();memory.fault=J{};memory.log_reads=true;fp={c.at("before_fp")[0],c.at("before_fp")[1],c.at("before_fp")[2]};};
        const auto args=c.at("arguments").get<std::vector<std::uint32_t>>();
        const auto invoke_bound=[&](const SourcePhysicsContactRecordObserver&observer,auto&stats){return native_tu15_create_bound_contact_point(memory,c.at("receiver"),{args[0]},{args[1]},{args[2]},args[3],{{PRIVATE},{PRIVATE+0x100u,PRIVATE+0x120u,PRIVATE+0x140u}},fp,0,bound,scope,observer,&stats);};
        reset_bound();NativeTu15ContactPointCreationBindingActivity raw;bool refused=false;
        try{invoke_bound([](const auto&){},raw);}catch(const std::runtime_error&e){refused=std::string(e.what()).find("raw private")!=std::string::npos;}
        check(refused&&raw.refusals==1&&raw.creation.refusals==1&&memory.reads.empty()&&memory.actual_writes.empty()&&!scope.active(),"Bound raw observer refusal/restore differs");
        reset_bound();const auto saved_listener=bound.observe_listener;
        bound.observe_listener=[](const auto&view,auto,auto,auto event){std::array<std::byte,1>b{};view.read(event+2u,b);};
        NativeTu15ContactPointCreationBindingActivity padding;refused=false;
        try{invoke_bound({},padding);}catch(const std::runtime_error&e){refused=std::string(e.what()).find("unprovided DATA")!=std::string::npos;}
        check(refused&&padding.refusals==1&&padding.creation.unprovided_event_reads==1&&!scope.active(),"Bound listener lost shared padding guard");bound.observe_listener=saved_listener;
        reset_bound();const auto saved_allocation=bound.allocation;
        bound.allocation=[&](auto&)->NativeTu15ArrayGrowthServices{scope.reject_unconverted(0x149da0u);throw std::runtime_error("Unreachable source execution");};
        NativeTu15ContactPointCreationBindingActivity native_only;refused=false;
        try{invoke_bound({},native_only);}catch(const std::runtime_error&e){refused=std::string(e.what()).find("refuses unconverted")!=std::string::npos;}
        check(refused&&native_only.refusals==1&&native_only.native_only.refused==1&&native_only.native_only.owners.at(0x149da0u)==1&&!scope.active(),"Bound native-only source refusal/restore differs");bound.allocation=saved_allocation;
      }
''')+'''
  marker='    }\\n    ++cases;faults+='
  assert driver.count(marker)==1
  driver=driver.replace(marker,checks+marker)
  (project/'driver.cpp').write_text(driver);source='''
    source=source.replace(anchor,injection)
    source=source.replace("'include/halo2/native_tu15_contact_point_create.hpp',*[", "'include/halo2/native_tu15_contact_point_create_binding.hpp','include/halo2/native_tu15_contact_point_create.hpp',*[")
    source=source.replace("scope='Declared coherent manager", "scope='Sole production native point-creation binding; unchanged guarded EventMemory and typed native children. Declared coherent manager")
    needle=';report.update(passed=True,profiles='
    assert source.count(needle)==1
    source=source.replace(needle,";assert all(sha(ROOT/name)==value for name,value in pinned.items()),'Binding source inputs changed';report.update(input_source_pins=pinned,source_inputs_unchanged=True,passed=True,profiles=")
    environment=dict(component.__dict__,__file__=__file__,pinned=pinned,UNITS=(*component.UNITS,'native_tu15_contact_point_removal.cpp'))
    exec(source,environment)
    return environment['main']()

if __name__=='__main__':main()
