#!/usr/bin/env python3
"""Narrow World wake factory routing and truthful typed activity transport."""
import argparse,difflib,shutil,json
from pathlib import Path
from build_native_tu15_constraint_before_reference import sha,save
ROOT=Path(__file__).resolve().parents[1]
FIELDS={'agent_factories':'native_agent_factories','bound_factory_attempts':'native_bound_agent_factory_attempts','bound_factory_calls':'native_bound_agent_factory_calls','bound_factory_refusals':'native_bound_agent_factory_refusals'}
PREFIX='native_world_activation_'
def once(s,a,b):assert s.count(a)==1,(a,s.count(a));return s.replace(a,b)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source-root',type=Path,required=True);p.add_argument('--api-stage',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.source_root=a.source_root.resolve();a.api_stage=a.api_stage.resolve();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);stage=a.output/'staged'
 shared=['src/physics/native_tu15_world_integration_residuals.cpp','include/halo2/native_tu15_physics_frame_residuals.hpp','src/physics/physics_live_world_tu15.cpp','tests/physics_multiplayer_acceptance.cpp','tests/helpers/native_world_convex_factory_prefix_tu15.hpp'];before={};paths={}
 for relative in shared:
  source=a.source_root/relative
  if not source.exists()and relative=='src/physics/physics_live_world_tu15.cpp':source=a.source_root/'src/physics_live_world_tu15.cpp'
  before[relative]=source.read_text();paths[relative]=source;target=stage/relative;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(source,target)
 for source in(a.api_stage/'staged').rglob('*'):
  if source.is_file():target=stage/source.relative_to(a.api_stage/'staged');target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(source,target)
 helper='include/halo2/native_tu15_pair_island_wake_native_factory_binding.hpp';shutil.copyfile(ROOT/helper,stage/helper)
 # Preserve the sealed readonly unknown-factory observer used by the live capture.
 diagnostic=ROOT/'analysis/physics/native-pair-wake-factory-capture-stage-20261102/revision-02/staged'
 inherited=['include/halo2/native_tu15_pair_island_wake_binding.hpp','include/halo2/native_tu15_contact_agent_factory_input_observer.hpp']
 for relative in inherited:shutil.copyfile(diagnostic/relative,stage/relative)
 models=['cmake/PhysicsLiveWorldTu15.cmake','tests/native_tu15_world_residuals_tests.cpp','tests/helpers/native_world_island_merge_prefix_tu15.hpp','tests/helpers/native_contact_process_binding_model_tu15.hpp']
 for relative in models:
  target=stage/relative;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(a.source_root/relative,target)
 relative=shared[0];s=once(before[relative],'#include <algorithm>','#include "halo2/native_tu15_pair_island_wake_native_factory_binding.hpp"\n#include <algorithm>')
 old='''        native_tu15_activate_pair_island(m,w,island,current,state.x87_depth,
          {wake_data+0x20u,wake_data+0x24u,wake_data+0x40u,wake_data+0x80u,0x800u},
          observe,stats?&stats->pair_island_wake:nullptr,stats?&stats->world_activation:nullptr);'''
 new='''        NativeTu15ObjectActivationNativeServices native_factories;
        native_factories.create_agent=[&](auto owner,auto first,auto second,auto settings,
            auto manager,auto&context,unsigned depth){
          check(depth==state.x87_depth,"Native TU1.5 wake factory lost actual x87 depth");
          return create.agents.create_agent(owner,first,second,settings,manager,context);
        };
        native_tu15_activate_pair_island_with_native_factories(m,w,island,current,state.x87_depth,
          {wake_data+0x20u,wake_data+0x24u,wake_data+0x40u,wake_data+0x80u,0x800u},native_factories,
          observe,stats?&stats->pair_island_wake:nullptr,stats?&stats->world_activation:nullptr);''';s=once(s,old,new)
 anchor='      if(target.identity==0x2f8030u||target.identity==0x2f81f0u){'
 builtin='''      if(target.identity==0x303c10u||target.identity==0x3040d0u){
        return native_tu15_create_contact_agent(m,target.identity==0x303c10u?
          NativeTu15ContactAgentFactory::owner_303c10:NativeTu15ContactAgentFactory::owner_3040d0,
          NativeTu15ContactAgentFactoryArguments{[&]{return manager.identity;}},current,pair_storage,observe);
      }
''';s=once(s,anchor,builtin+anchor);(stage/relative).write_text(s)
 relative=shared[1];s=once(before[relative],'  std::uint64_t native_pair_island_wake_attempts{}','  std::uint64_t '+','.join(PREFIX+n+'{}'for n in FIELDS)+';\n  std::uint64_t native_pair_island_wake_attempts{}');(stage/relative).write_text(s)
 relative=shared[2];s=once(before[relative],'    result.native_world_activation_calls=world_integration.native_world_activation_calls;',''.join('    result.'+PREFIX+n+'=world_activation.'+field+';\n'for n,field in FIELDS.items())+'    result.native_world_activation_calls=world_integration.native_world_activation_calls;');(stage/relative).write_text(s)
 relative=shared[3];s=before[relative];anchor='        {"native_world_activation_calls",native_frames.native_world_activation_calls},';assert s.count(anchor)==2;s=s.replace(anchor,''.join('        {"'+PREFIX+n+'",native_frames.'+PREFIX+n+'},\n'for n in FIELDS)+anchor)
 gate='''  if(std::getenv("HALO2_REQUIRE_NATIVE_WORLD_ACTIVATION_AGENT_FACTORY")) {
    const bool native_wake_factory=native_frames.native_world_activation_bound_factory_calls>0&&
      native_frames.native_world_activation_bound_factory_attempts==native_frames.native_world_activation_bound_factory_calls&&
      native_frames.native_world_activation_bound_factory_refusals==0&&native_frames.native_world_activation_agent_factories>=native_frames.native_world_activation_bound_factory_calls&&
      native_frames.native_world_activation_refusals==0&&native_frames.native_world_activation_residual_calls==0;
    report["native_world_activation_agent_factory_gate"]=native_wake_factory;passed&=native_wake_factory;
  }
''';s=once(s,'  if(std::getenv("HALO2_REQUIRE_NATIVE_PAIR_ISLAND_WAKE")) {',gate+'  if(std::getenv("HALO2_REQUIRE_NATIVE_PAIR_ISLAND_WAKE")) {');(stage/relative).write_text(s)
 relative=shared[4];s=before[relative]
 s=once(s,'if(!wake_prefix&&e.instruction==0x3210deu){','if(wake_prefix&&e.instruction==0x320dd9u){reached=true;throw Published{};}if(!wake_prefix&&e.instruction==0x3210deu){')
 s=once(s,'m.get(island1+0x78u)==0,"World native wake/deferred pair DATA differs"','m.get(island1+0x78u)==1,"World native wake/deferred pair DATA differs"')
 s=once(s,'check(std::find(calls.begin(),calls.end(),0x320da0u)!=calls.end(),"World wake did not reach source deferred-pair service");','check(reached&&std::find(calls.begin(),calls.end(),0x320da0u)==calls.end(),"World wake did not publish its now-native deferred pair");')
 s=s.replace('Declared valid inactive-island World PREFIX at remaining320DA0 request; no queue result supplied or live wake claim','Declared valid inactive-island World PREFIX at native320DD9 publication; no supplied queue result or live wake claim');(stage/relative).write_text(s)
 api_pins=json.loads((a.api_stage/'source-pins.json').read_text());api_files=list(api_pins['base_hashes']);all_delta=[*api_files,*shared]
 (a.output/'delta.patch').write_text((a.api_stage/'delta.patch').read_text()+''.join(''.join(difflib.unified_diff(before[r].splitlines(True),(stage/r).read_text().splitlines(True),fromfile='a/'+r,tofile='b/'+r))for r in shared))
 save(a.output/'source-pins.json',dict(qualified=False,production_applied=False,base_source_root=str(a.source_root),base_hashes={**api_pins['base_hashes'],**{r:sha(p)for r,p in paths.items()}},delta_files=all_delta,new_files=[helper],inherited_files=[*inherited,*models],
  staged_hashes={str(p.relative_to(stage)):sha(p)for p in stage.rglob('*')if p.is_file()},fields={PREFIX+n:field for n,field in FIELDS.items()},
  scope='Existing exact native World factory closure supplied to shared activation via ordinary typed DATA/FP/depth; old APIs and sealed unknown observer preserved.6file narrowdelta/newinlinehelper; no Root application/live parity claim.'))
 print(dict(prepared=True,delta_files=len(all_delta),fields=len(FIELDS)));return 0
if __name__=='__main__':raise SystemExit(main())
