#!/usr/bin/env python3
"""Stage the narrow native wrapped/compound process World binding."""
import argparse,difflib,hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
FIELDS={
 'native_contact_process_calls':'attempts',
 'native_contact_process_completions':'completed',
 'native_contact_process_refusals':'refusals',
 'native_wrapped_contact_process_calls':'wrapped.calls',
 'native_compound_contact_process_calls':'compound.calls',
 'native_contact_process_geometry_calls':'wrapped.geometry_calls',
 'native_contact_process_child_calls':'wrapped.child_calls+impl_->native_physics_integrations.contact_process.compound.child_calls',
 'native_contact_process_child_residuals':'remaining_child_calls',
}
def sha(data):return hashlib.sha256(data).hexdigest()
def replace(text,before,after,count=1):
 assert text.count(before)==count,(before,text.count(before),count)
 return text.replace(before,after)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source-root',type=Path,default=ROOT);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=False);stage=out/'staged'
 files=['src/physics/native_tu15_world_integration_residuals.cpp','include/halo2/native_tu15_world_integration.hpp','include/halo2/native_tu15_physics_frame_residuals.hpp','src/physics/physics_live_world_tu15.cpp','tests/physics_multiplayer_acceptance.cpp','cmake/PhysicsLiveWorldTu15.cmake']
 base={};pins={};patch=[]
 for relative in files:
  path=a.source_root/relative
  if not path.exists() and relative=='src/physics/physics_live_world_tu15.cpp':path=a.source_root/'src/physics_live_world_tu15.cpp'
  raw=path.read_bytes();text=raw.decode();base[relative]=sha(raw)
  if relative.endswith('native_tu15_world_integration_residuals.cpp'):
   before='''  const auto agent_call=[&](auto agent,auto owner,auto first,auto second,auto configuration,auto output,auto&){
    const std::array args{first.identity,second.identity,configuration.identity,output.identity};
    call(owner.identity,args,agent.identity,16);
  };'''
   after='''  NativeTu15CompoundProcessServices remaining_contact_process;
  remaining_contact_process.process_child=[&](const auto&child,auto&){
    const std::array args{child.first.identity,child.second.identity,
      child.configuration.identity,child.output.identity};
    // All retained native descriptors and geometry stay above this genuine
    // child's arguments and downward-growing private control frame.
    call_at(std::uint32_t(end)-0x1200u,child.owner.identity,args,child.receiver.identity,16);
  };
  NativeTu15ContactProcessBinding native_contact_process(m,
    {std::uint32_t(end)-0x1000u,0x600u},remaining_contact_process,observe,
    stats?&stats->contact_process:nullptr);
  const auto agent_call=[&](auto agent,auto owner,auto first,auto second,auto configuration,auto output,auto&context){
    if(NativeTu15ContactProcessBinding::owns(owner)){
      check(scratch.bytes>=0x1600u,"Native TU1.5 contact process requires retained DATA above its genuine child frames");
      native_contact_process.process({agent,owner,{first.identity},second,configuration,output},context);
      return;
    }
    const std::array args{first.identity,second.identity,configuration.identity,output.identity};
    call(owner.identity,args,agent.identity,16);
  };'''
   text=replace(text,before,after)
  elif relative.endswith('native_tu15_world_integration.hpp'):
   text=replace(text,'#include <map>','#include "halo2/native_tu15_contact_process_binding.hpp"\n#include <map>')
   text=replace(text,'  NativeTu15IslandAgentPhaseStats island_agents;','  NativeTu15ContactProcessBindingStats contact_process;\n  NativeTu15IslandAgentPhaseStats island_agents;')
  elif relative.endswith('native_tu15_physics_frame_residuals.hpp'):
   needle='  std::uint64_t native_contact_agent_cached_cleanups{}'
   at=text.index(needle);text=text[:at]+'  std::uint64_t '+','.join(k+'{}'for k in FIELDS)+';\n'+text[at:]
  elif relative.endswith('physics_live_world_tu15.cpp'):
   needle='    result.native_contact_agent_point_residuals=impl_->native_physics_integrations.native_contact_agent_point_residuals;'
   text=replace(text,needle,needle+'\n'+''.join('    result.'+key+'=impl_->native_physics_integrations.contact_process.'+value+';\n'for key,value in FIELDS.items()).rstrip())
  elif relative.endswith('physics_multiplayer_acceptance.cpp'):
   needle='        {"native_contact_agent_point_residuals",native_frames.native_contact_agent_point_residuals},'
   text=replace(text,needle,needle+'\n'+''.join('        {"'+key+'",native_frames.'+key+'},\n'for key in FIELDS).rstrip(),2)
   needle='  if(std::getenv("HALO2_REQUIRE_NATIVE_CONVEX_AGENT_FACTORY")) {'
   gate=''
   for kind,owner in [('COMPOUND',0x2f7a50),('WRAPPED',0x2f6ca0)]:
    key='native_'+kind.lower()+'_contact_process_calls'
    gate+='  if(std::getenv("HALO2_REQUIRE_NATIVE_'+kind+'_CONTACT_PROCESS")) {\n'
    gate+='    const bool process_passed=native_frames.'+key+'>0 &&\n'
    gate+='      native_frames.native_contact_process_calls==native_frames.native_contact_process_completions &&\n'
    gate+='      native_frames.native_contact_process_refusals==0 &&\n'
    gate+='      !native_frames.residual_owner_calls.contains(0x'+format(owner,'x')+'u);\n'
    gate+='    report["native_'+kind.lower()+'_contact_process_gate"]=process_passed;passed&=process_passed;\n  }\n'
   text=replace(text,needle,gate+needle)
  else:
   needle='add_library(halo2_native_object_velocity STATIC'
   text=replace(text,needle,needle+'\n    src/physics/native_tu15_compound_process.cpp\n    src/physics/native_tu15_wrapped_contact_process.cpp')
  dest=stage/relative;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_text(text);pins[relative]=sha(text.encode());patch.extend(difflib.unified_diff(raw.decode().splitlines(True),text.splitlines(True),fromfile='a/'+relative,tofile='b/'+relative))
 for relative in ['include/halo2/native_tu15_contact_process_binding.hpp','include/halo2/native_tu15_wrapped_contact_process.hpp','src/physics/native_tu15_wrapped_contact_process.cpp','include/halo2/native_tu15_compound_process.hpp','src/physics/native_tu15_compound_process.cpp']:
  raw=(ROOT/relative).read_bytes();dest=stage/relative;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(raw);pins[relative]=sha(raw)
 for relative in ['tests/native_tu15_world_residuals_tests.cpp','tests/helpers/native_world_convex_factory_prefix_tu15.hpp','tests/helpers/native_world_island_merge_prefix_tu15.hpp']:
  source=a.source_root/relative;raw=source.read_bytes();base[relative]=sha(raw)
  if relative=='tests/native_tu15_world_residuals_tests.cpp':
   text=raw.decode();needle='#include "helpers/native_world_island_merge_prefix_tu15.hpp"';text=replace(text,needle,needle+'\n#include "helpers/native_contact_process_binding_model_tu15.hpp"');text=replace(text,'int main(int argc,char**argv){try{','int main(int argc,char**argv){try{native_contact_process_binding_model();');raw=text.encode()
  dest=stage/relative;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(raw);pins[relative]=sha(raw)
 relative='tests/helpers/native_contact_process_binding_model_tu15.hpp';raw=(ROOT/relative).read_bytes();dest=stage/relative;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(raw);pins[relative]=sha(raw)
 (out/'native-contact-process-world-binding.diff').write_text(''.join(patch))
 (out/'source-pins.json').write_text(json.dumps(dict(base_source_root=str(a.source_root.resolve()),base_hashes=base,staged_hashes=pins,production_applied=False,component_parent_prefix_qualified=False,world_model_qualified=False,live_qualified=False,scope='Native2F6CA0+2F7A50 typed parents, native2E44D0 geometry, explicit real remaining children. No full producer/native-only closure.'),indent=2)+'\n')
 print(out)
if __name__=='__main__':main()
