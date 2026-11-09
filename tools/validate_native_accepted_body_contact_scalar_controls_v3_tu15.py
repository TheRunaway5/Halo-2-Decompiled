#!/usr/bin/env python3
"""Native accepted listener: genuine input plus separate declared scalar controls.

Only the four authenticated body+4C/+50 input cells change in declared cases.
They are offline BEFORE controls; no runtime state or expected outcome enters.
"""
import inspect,json,os,struct
from pathlib import Path
import validate_native_accepted_body_contact_actual_tu15 as base
import validate_native_accepted_body_contact_bound_actual_tu15 as bound
ROOT=Path(__file__).resolve().parents[1]
STAGE=ROOT/'analysis/physics/native-accepted-body-contact-semantic-stage-v3-20261103/staged'

def observer_source():
 text=bound.observer_source()
 text=text.replace('const std::filesystem::path&directory){','const std::filesystem::path&directory,const J&control=J{}){',1)
 marker='request(PTRACE_SETFPREGS,c.pid,nullptr,&f);'
 assert text.count(marker)==1
 text=text.replace(marker,marker+'if(!control.is_null())for(const auto&cell:control.at("cells")){const std::array<std::uint32_t,1>value{cell.at("bits").get<std::uint32_t>()};c.write(cell.at("address"),std::as_bytes(std::span(value)));}')
 marker=' const std::array<std::pair<std::uint32_t,bool>,11>faults'
 assert text.count(marker)==1
 text=text.replace(marker,''' if(mutation.empty())for(const auto&control:meta.at("declared_scalar_controls"))for(char c:std::string(argv[5]))if(c!=',')cases.push_back(J{{"mode",c-'0'}, {"fault_pc",0u},{"write",false},{"control",control}});
'''+marker)
 text=text.replace('start(original,argv[1],typed,mode,directory);','start(original,argv[1],typed,mode,directory,test.value("control",J{}));')
 text=text.replace('start(native,argv[1],typed,mode,directory);','start(native,argv[1],typed,mode,directory,test.value("control",J{}));')
 text=text.replace('{"actual_input_fp",mode==0},{"rounding_control",mode}', '{"actual_input_fp",mode==0&&!test.contains("control")},{"declared_scalar_control",test.value("control",J{})},{"rounding_control",mode}')
 text=text.replace(' if(numeric_mutant)check(mutant_profiles',' {\n struct RequirementMemory final:SourcePhysicsContactMemory {\n  bool unknown{};unsigned writes{};\n  void read(std::uint32_t p,std::span<std::byte>out)const override {\n   std::uint32_t value{};\n   if(p==0x52be5cu)value=0x2000u;if(p==0x2044u)value=0x3000u;if(p==0x3004u)value=1u;if(p==0x3008u)value=0xffffffffu;\n   if(p==0x4ecbd0u)value=0x4000u;if(p==0x4044u)value=0x4100u;\n   if(p==0x55d728u)value=56u;if(p==0x46a0d0u)value=0x3f800000u;\n   if(p==0x1200u)value=0x5000u;if(p==0x1300u)value=0x5100u;\n   if(p==0x5000u)value=0x6000u;if(p==0x5100u)value=0x6100u;\n   if(p==0x6014u)value=unknown?0x0badcafeu:0x31ede0u;if(p==0x6114u)value=0x31ede0u;\n   if(p==0x308cu)value=28u;\n   check(out.size()<=4u,"Unexpected requirement model vector read");for(unsigned n=0;n<out.size();++n)out[n]=std::byte(value>>(n*8u));\n  }\n  void write(std::uint32_t,std::span<const std::byte>)override{++writes;throw std::runtime_error("Requirement model retained write");}\n };\n for(bool unknown:{false,true}){\n  RequirementMemory memory;memory.unknown=unknown;constexpr std::uint32_t p=0x40000000u;\n  const std::array<NativeTu15ContactPrivateRange,1>owned{{{p,0x1000u}}};NativeTu15ContactCandidatePrivateData view(memory,owned,{});\n  NativeTu15AcceptedContactScratch scratch{p+0x100u,p+0x104u,p+0x108u,p+0x10cu,p+0x110u,p+0x114u,p+0x118u,p+0x11cu,{p+0x200u},{p+0x300u,p+0x310u}};\n  NativeTu15CreatedBodyContact request{0x7000u,0u,0x3f800000u,0x8000u,0u,0x1200u,0x1300u,0x1000u,0x1100u,0x1300u};\n  NativeTu15PhysicsFrameFp fp{0x27fu,0x20u,0x1fa0u};NativeTu15AcceptedBodyContactStats out;bool refused{};\n  const auto owner=unknown?0x0badcafeu:0x1d86b0u;\n  try{native_tu15_publish_accepted_body_contact(view,request,scratch,fp,0,{}, {},&out);}catch(const std::runtime_error&e){refused=std::string(e.what())=="Native accepted body contact requires owner="+std::to_string(owner);}\n  check(refused&&out.calls==1u&&out.refusals==1u&&out.completed==0u&&out.unqualified_owners.size()==1u&&out.unqualified_owners.at(owner)==1u&&memory.writes==0u,"Named requirement counter/parent boundary model failed");\n }\n report["named_requirement_interface_controls"]=2;\n }\n'+' if(numeric_mutant)check(mutant_profiles')
 return text

def main():
 text=inspect.getsource(base.main)
 text=text.replace("'include/halo2/native_tu15_retained_store_observer.hpp'];pins", "'include/halo2/native_tu15_retained_store_observer.hpp','include/halo2/native_tu15_accepted_body_contact_binding.hpp'];pins")
 text=text.replace("driver='#include", "driver='#include \"halo2/native_tu15_accepted_body_contact_binding.hpp\"\\n#include",1)
 needle="(ROOT/'tools/observe_native_accepted_body_contact_actual_tu15.cpp').read_text()";assert text.count(needle)==1;text=text.replace(needle,'BOUND_OBSERVER_SOURCE')
 # Derive body identities from genuine current payload DATA. These reads are
 # preparation/provenance only, never duplicated semantic reads in the kernel.
 marker="save(scratch/'metadata.json',dict(sites=sites))"
 assert text.count(marker)==1
 replacement='''
  def root_body(payload):
   parent=before.word(payload+0xcu)
   while parent:payload=parent;parent=before.word(payload+0xcu)
   assert before.word(payload+0x18u)==1
   return before.word(payload+0x20u)
  first_body=root_body(before.event_fields['first_payload']);second_body=root_body(before.event_fields['second_payload'])
  inputs=(('unequal_products',[0x3e800000,0x3d800000,0x3f800000,0x3e800000]),('zero_nonzero',[0,0x3f800000,0x40800000,0x40800000]),('signed_zero',[0x80000000,0,0x3f800000,0x3f800000]),('quiet_nan_priority',[0x7fc12345,0x3f800000,0x7fc54321,0x40800000]),('signaling_nan',[0x7f812345,0x3f800000,0x3f000000,0x3e800000]),('infinity_zero',[0x7f800000,0x3f000000,0,0x40000000]))
  addresses=[first_body+0x4c,first_body+0x50,second_body+0x4c,second_body+0x50]
  controls=[dict(name=name,scope='Declared offline BEFORE scalar input control, not actual capture/runtime',cells=[dict(address=p,bits=v)for p,v in zip(addresses,values)])for name,values in inputs]
  save(scratch/'metadata.json',dict(sites=sites,declared_scalar_controls=controls));report['declared_scalar_controls']=controls
'''.replace('0xcu','0xc').replace('0x18u','0x18').replace('0x20u','0x20')
 text=text.replace(marker,replacement)
 text=text.replace("'tools/run_test.py');tool_pins", "'tools/run_test.py','tools/validate_native_accepted_body_contact_bound_actual_tu15.py','tools/validate_native_accepted_body_contact_scalar_controls_tu15.py','tools/validate_native_accepted_body_contact_scalar_controls_v3_tu15.py');tool_pins")
 text=text.replace("assert all(sha(paths[n])==h for n,h in pins.items());source=", "assert all(sha(paths[n])==h for n,h in pins.items())and all(sha(ROOT/n)==h for n,h in tool_pins.items());source=")
 env=dict(base.__dict__,DRAFT=STAGE,BOUND_OBSERVER_SOURCE=observer_source(),__doc__=__doc__);exec(text,env);return env['main']()
if __name__=='__main__':main()
