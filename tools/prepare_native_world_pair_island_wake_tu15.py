#!/usr/bin/env python3
"""Stage a wake-only World route against an explicitly frozen live cohort."""
import argparse,difflib,shutil
from pathlib import Path
from build_native_tu15_constraint_before_reference import sha,save
ROOT=Path(__file__).resolve().parents[1]
SHARED=['include/halo2/native_tu15_world_integration.hpp',
 'include/halo2/native_tu15_physics_frame_residuals.hpp',
 'src/physics/native_tu15_world_integration_residuals.cpp',
 'src/physics/physics_live_world_tu15.cpp','tests/physics_multiplayer_acceptance.cpp',
 'tests/helpers/native_world_convex_factory_prefix_tu15.hpp',
 'tests/native_tu15_world_residuals_tests.cpp']
def once(s,a,b):assert s.count(a)==1,(a,s.count(a));return s.replace(a,b)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source-root',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.source_root=a.source_root.resolve();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);stage=a.output/'staged';before={};paths={}
 for relative in SHARED:
  source=a.source_root/relative
  if not source.exists()and relative=='src/physics/physics_live_world_tu15.cpp':source=a.source_root/'src/physics_live_world_tu15.cpp'
  before[relative]=source.read_text();paths[relative]=source;target=stage/relative;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(source,target)
 inherited=['cmake/PhysicsLiveWorldTu15.cmake','tests/helpers/native_world_island_merge_prefix_tu15.hpp','tests/helpers/native_contact_process_binding_model_tu15.hpp']
 for relative in inherited:
  target=stage/relative;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(a.source_root/relative,target)
 helper='include/halo2/native_tu15_pair_island_wake_binding.hpp';shutil.copyfile(ROOT/helper,stage/helper)
 relative=SHARED[0];s=once(before[relative],'#include <map>','#include "halo2/native_tu15_pair_island_wake_binding.hpp"\n#include <map>');s=once(s,'  NativeTu15ObjectActivationBindingActivity world_activation;','  NativeTu15PairIslandWakeActivity pair_island_wake;\n  NativeTu15ObjectActivationBindingActivity world_activation;');(stage/relative).write_text(s)
 relative=SHARED[1];s=once(before[relative],'  std::uint64_t native_world_merge_calls{}','  std::uint64_t native_pair_island_wake_attempts{},native_pair_island_wake_calls{},native_pair_island_wake_refusals{};\n  std::uint64_t native_world_merge_calls{}');(stage/relative).write_text(s)
 relative=SHARED[2];s=before[relative];old='    create.activate_island=[&](auto w,auto island,auto&){const std::array args{island.identity};call(0x316d40u,args,w.identity,4);};'
 new='''    create.activate_island=[&](auto w,auto island,auto&current){
      // This call precedes all pair factory DATA. The low scratch segment is
      // disjoint from the merge segment, high pair packets and lifecycle DATA.
      const auto wake_data=(scratch.first+0x1000u+15u)&~15u;
      check(scratch.bytes>=0x5000u&&std::uint64_t(wake_data)+0x880u<=end-0x2800u,
        "Native TU1.5 pair wake requires distinct caller-owned retained DATA");
      try{
        native_tu15_activate_pair_island(m,w,island,current,state.x87_depth,
          {wake_data+0x20u,wake_data+0x24u,wake_data+0x40u,wake_data+0x80u,0x800u},
          observe,stats?&stats->pair_island_wake:nullptr,stats?&stats->world_activation:nullptr);
        if(stats)++stats->native_world_activation_calls;
      }catch(...){if(stats)++stats->native_world_activation_refusals;throw;}
    };''';s=once(s,old,new);(stage/relative).write_text(s)
 relative=SHARED[3];s=before[relative];anchor='    result.native_world_activation_calls=world_integration.native_world_activation_calls;';s=once(s,anchor,''.join('    result.native_pair_island_wake_'+name+'=world_integration.pair_island_wake.'+field+';\n'for name,field in [('attempts','attempts'),('calls','completed'),('refusals','refusals')])+anchor);(stage/relative).write_text(s)
 relative=SHARED[4];s=before[relative];anchor='        {"native_world_activation_calls",native_frames.native_world_activation_calls},';assert s.count(anchor)==2;rows=''.join('        {"native_pair_island_wake_'+name+'",native_frames.native_pair_island_wake_'+name+'},\n'for name in('attempts','calls','refusals'));s=s.replace(anchor,rows+anchor)
 gate='''  if(std::getenv("HALO2_REQUIRE_NATIVE_PAIR_ISLAND_WAKE")) {
    const bool native_pair_wake=native_frames.native_pair_island_wake_calls>0&&
      native_frames.native_pair_island_wake_attempts==native_frames.native_pair_island_wake_calls&&
      native_frames.native_pair_island_wake_refusals==0&&native_frames.native_world_activation_refusals==0&&
      native_frames.native_world_activation_residual_calls==0&&!native_frames.residual_owner_calls.contains(0x316d40u);
    report["native_pair_island_wake_gate"]=native_pair_wake;passed&=native_pair_wake;
  }
''';s=once(s,'  if(std::getenv("HALO2_REQUIRE_NATIVE_WORLD_ACTIVATION")) {',gate+'  if(std::getenv("HALO2_REQUIRE_NATIVE_WORLD_ACTIVATION")) {');(stage/relative).write_text(s)
 relative=SHARED[5];s=once(before[relative],'const std::filesystem::path&reference){','const std::filesystem::path&reference,bool wake_prefix=false){')
 needle='            constexpr auto context=world+0xe800u';extra='''            if(wake_prefix){
                // Declared valid inactive-island DATA; no live flags or rows are injected.
                m.put(world+0x10u,3u);m.put(world+0x14u,world+0x950u);m.put(world+0x18u,1u);
                m.put(world+0x1cu,1u);m.put(world+0x950u,island1);
                m.put(body1+0x44u,island1);m.put(island1+0x24u,0u);m.put(island1+0x29u,0u,1);
                m.put(island1+0x3cu,world+0xdb00u);m.put(world+0xdb00u,body1);m.put(island1+0x40u,1u);
                m.put(island1+0x74u,world+0xdb20u);m.put(island1+0x78u,0u);m.put(island1+0x7cu,0x80000004u);
            }
''';s=once(s,needle,extra+needle)
 old='if(e.instruction==0x3210deu){';s=once(s,old,'if(!wake_prefix&&e.instruction==0x3210deu){')
 s=once(s,'        }else if(target==0x3232e0u){','''        }else if(wake_prefix&&target==0x320da0u){
            check(a.gp[1]==island1&&m.get(m.get(sp+4u))==body0+0x1cu&&
              m.get(m.get(sp+4u)+4u)==body1+0x1cu,"Native wake lost the actual deferred-pair request DATA");
            throw Published{}; // Read-only stop at the remaining source service.
        }else if(target==0x3232e0u){''')
 anchor='    if(published_agent)published_manager=m.get(published_agent+8u);';early='''    if(wake_prefix){
        check(stats.pair_island_wake.attempts==1&&stats.pair_island_wake.completed==1&&
          stats.pair_island_wake.refusals==0&&stats.world_activation.activation.island_calls==1&&
          stats.world_activation.residual_calls==0&&stats.native_world_activation_calls==1&&
          stats.native_world_activation_refusals==0,"World native pair wake completion/refusal differs");
        check(m.get(island1+0x29u,1)==1&&m.get(world+0xcu)==2&&m.get(world+0x18u)==0&&
          m.get(island1+0x78u)==0,"World native wake/deferred pair DATA differs");
        check(std::find(calls.begin(),calls.end(),0x316d40u)==calls.end(),"World pair wake invoked old316D40");
        check(std::find(calls.begin(),calls.end(),0x320da0u)!=calls.end(),"World wake did not reach source deferred-pair service");
        std::cout<<J{{"passed",true},{"native_pair_island_wakes",1},{"deferred_pair_requested",true},
          {"scope","Declared valid inactive-island World PREFIX at remaining320DA0 request; no queue result supplied or live wake claim"}}.dump()<<'\\n';return 0;
    }
''';s=once(s,anchor,early+anchor);(stage/relative).write_text(s)
 relative=SHARED[6];s=once(before[relative],'try{native_contact_process_binding_model();','try{native_contact_process_binding_model();if(argc==3&&std::string(argv[1])=="pair-wake-prefix")return native_world_convex_factory_prefix(argv[2],true);');(stage/relative).write_text(s)
 (a.output/'delta.patch').write_text(''.join(''.join(difflib.unified_diff(before[p].splitlines(True),(stage/p).read_text().splitlines(True),fromfile='a/'+p,tofile='b/'+p))for p in SHARED))
 save(a.output/'source-pins.json',dict(qualified=False,production_applied=False,working_tree_modified=False,
  base_source_root=str(a.source_root),base_paths={k:str(v)for k,v in paths.items()},base_hashes={k:sha(v)for k,v in paths.items()},
  delta_files=SHARED,new_files=[helper],inherited_files=inherited,
  staged_hashes={str(p.relative_to(stage)):sha(p)for p in stage.rglob('*')if p.is_file()},
  scope='Wake-only native316D40 pair-create route using sole qualified shared activation builder; distinct low retained DATA; native remaining-owner count+throw; existing world_activation aggregates all lifecycle counters; no Root/MP application claimed.'))
 print(dict(prepared=True,files=len(SHARED)));return 0
if __name__=='__main__':raise SystemExit(main())
