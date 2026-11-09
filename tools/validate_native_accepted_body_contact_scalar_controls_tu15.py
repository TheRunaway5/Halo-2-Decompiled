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
STAGE=ROOT/'analysis/physics/native-accepted-body-contact-semantic-stage-v2-20261103/staged'

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
 text=text.replace("'tools/run_test.py');tool_pins", "'tools/run_test.py','tools/validate_native_accepted_body_contact_bound_actual_tu15.py','tools/validate_native_accepted_body_contact_scalar_controls_tu15.py');tool_pins")
 text=text.replace("assert all(sha(paths[n])==h for n,h in pins.items());source=", "assert all(sha(paths[n])==h for n,h in pins.items())and all(sha(ROOT[n])==h for n,h in tool_pins.items());source=")
 env=dict(base.__dict__,DRAFT=STAGE,BOUND_OBSERVER_SOURCE=observer_source(),__doc__=__doc__);exec(text,env);return env['main']()
if __name__=='__main__':main()
