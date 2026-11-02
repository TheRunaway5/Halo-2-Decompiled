#!/usr/bin/env python3
"""Prepare native282C10 Frame route; retain actual-input observer and refusals."""
import argparse,difflib,shutil
from pathlib import Path
from build_native_tu15_constraint_before_reference import sha,save
ROOT=Path(__file__).resolve().parents[1]
FIELDS={'attempts':'attempts','calls':'completed','refusals':'refusals','kernel_entries':'update.calls','pending_skips':'update.pending_skips','reentrant_skips':'update.reentrant_skips','island_rows':'update.island_rows','dirty_islands':'update.dirty_islands','body_rows':'update.body_rows','candidate_objects':'update.candidate_objects','deferred_objects':'update.deferred_objects','component_destructions':'update.component_destructions','component_frees':'update.component_frees','object_deletions':'update.object_deletions','recursive_updates':'update.recursive_updates','flag_calls':'state.flag_calls','defer_calls':'state.defer_calls','state_removals':'state.removals','state_resets':'state.resets','state_refreshes':'state.refreshes','state_activations':'state.activations','unqualified_services':'update.unbound_services+activity.state.unbound_services','unsupported_local_sizes':'update.unsupported_local_set_sizes'}
PREFIX='native_dirty_island_objects_'
def once(s,a,b,n=1):assert s.count(a)==n,(a,s.count(a));return s.replace(a,b)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source-root',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.source_root=a.source_root.resolve();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);stage=a.output/'staged';before={};base={}
 files=['src/physics/native_tu15_physics_frame_residuals.cpp','include/halo2/native_tu15_physics_frame_residuals.hpp','tests/physics_multiplayer_acceptance.cpp','cmake/PhysicsLiveWorldTu15.cmake','tests/native_tu15_physics_frame_tests.cpp']
 for name in files:
  source=a.source_root/name;before[name]=source.read_text();base[name]=sha(source);target=stage/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(source,target)
 new=['include/halo2/native_tu15_dirty_island_objects.hpp','include/halo2/native_tu15_component_state.hpp','include/halo2/native_tu15_dirty_island_objects_binding.hpp','src/physics/native_tu15_dirty_island_objects.cpp','src/physics/native_tu15_component_state.cpp']
 for name in new:target=stage/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,target)
 r=files[0];s=once(before[r],'#include "halo2/native_tu15_physics_frame_residuals.hpp"','#include "halo2/native_tu15_physics_frame_residuals.hpp"\n#include "halo2/native_tu15_dirty_island_objects_binding.hpp"');old='''  s.post_step=[&](auto&f){
    native_tu15_observe_dirty_island_objects_input(m,{f,state.x87_depth});
    call(0x282c10u,{},{},{},0,f);
  };'''
 newblock='''  s.post_step=[&](auto&f){
    native_tu15_observe_dirty_island_objects_input(m,{f,state.x87_depth});
    NativeTu15DirtyIslandObjectsBindingActivity activity;
    const auto aggregate=[&]{if(stats){
'''+''.join('      stats->'+PREFIX+name+'+=activity.'+field+';\n'for name,field in FIELDS.items())+'''      for(const auto&[owner,count]:activity.update.unbound_owners)stats->native_dirty_island_objects_unqualified_owners[owner]+=count;
      for(const auto&[owner,count]:activity.state.unbound_owners)stats->native_dirty_island_objects_unqualified_owners[owner]+=count;
    }};
    try{native_tu15_process_bound_dirty_island_objects(m,f,state.x87_depth,observe,&activity);}
    catch(...){aggregate();throw;}
    aggregate();
  };''';s=once(s,old,newblock);(stage/r).write_text(s)
 r=files[1];s=once(before[r],'  std::uint64_t frame_calls{},residual_calls{};','  std::uint64_t frame_calls{},residual_calls{};\n  std::uint64_t '+','.join(PREFIX+k+'{}'for k in FIELDS)+';\n  std::map<std::uint32_t,std::uint64_t> native_dirty_island_objects_unqualified_owners;');(stage/r).write_text(s)
 r=files[2];s=before[r];anchor='        {"native_contact_body_attempts",native_frames.native_contact_body_attempts},';lines=''.join('        {"'+PREFIX+k+'",native_frames.'+PREFIX+k+'},\n'for k in FIELDS)+'        {"native_dirty_island_objects_unqualified_owners",native_frames.native_dirty_island_objects_unqualified_owners},\n';s=once(s,anchor,lines+anchor,2)
 gate='''  if(std::getenv("HALO2_REQUIRE_NATIVE_DIRTY_ISLAND_OBJECTS")) {
    const bool native_dirty=native_frames.native_dirty_island_objects_calls>0&&
      native_frames.native_dirty_island_objects_attempts==native_frames.native_dirty_island_objects_calls&&
      native_frames.native_dirty_island_objects_refusals==0&&native_frames.native_dirty_island_objects_unqualified_services==0&&
      native_frames.native_dirty_island_objects_unqualified_owners.empty()&&native_frames.native_dirty_island_objects_unsupported_local_sizes==0&&
      !native_frames.residual_owner_calls.contains(0x282c10u);
    report["native_dirty_island_objects_gate"]=native_dirty;passed&=native_dirty;
  }
  if(std::getenv("HALO2_REQUIRE_NATIVE_DIRTY_ISLAND_CLEANUP")) {
    const bool cleanup=native_frames.native_dirty_island_objects_dirty_islands>0&&
      native_frames.native_dirty_island_objects_attempts==native_frames.native_dirty_island_objects_calls&&
      native_frames.native_dirty_island_objects_refusals==0&&native_frames.native_dirty_island_objects_unqualified_services==0&&
      native_frames.native_dirty_island_objects_unqualified_owners.empty();
    report["native_dirty_island_cleanup_gate"]=cleanup;passed&=cleanup;
  }
''';anchor='  if(std::getenv("HALO2_REQUIRE_NATIVE_LOOK_INITIALIZATION")) {';s=once(s,anchor,gate+anchor);(stage/r).write_text(s)
 r=files[3];s=before[r]+'\n# Sole native282C10 semantic parent and known component-state children.\ntarget_sources(halo2_native_pair_consumer PRIVATE src/physics/native_tu15_dirty_island_objects.cpp src/physics/native_tu15_component_state.cpp)\n';(stage/r).write_text(s)
 r=files[4];s=before[r];s=once(s,'#include "halo2/native_tu15_physics_frame_residuals.hpp"','#include "halo2/native_tu15_physics_frame_residuals.hpp"\n#include "halo2/native_tu15_dirty_island_objects_input_observer.hpp"');s=once(s,'0x31b1a0,0x282c10,0x2e2b30','0x31b1a0,0x2e2b30');anchor='  check(stats.frame_calls==1&&stats.residual_calls==calls.size()&&publications==2,"Residual native/mixed counters");';s=once(s,anchor,anchor+'\n  check(stats.native_dirty_island_objects_attempts==1&&stats.native_dirty_island_objects_calls==1&&stats.native_dirty_island_objects_pending_skips==1&&stats.native_dirty_island_objects_refusals==0&&stats.native_dirty_island_objects_unqualified_services==0&&stats.native_dirty_island_objects_unqualified_owners.empty()&&!stats.residual_owner_calls.contains(0x282c10u),"Frame did not select native282C10 no-pending return");');s=once(s,'Memory m;m.banks[0x400000]', 'Memory m;m.banks[0x56dfc0u].resize(0x100u);m.banks[0x400000]');s=once(s,'  std::vector<std::uint32_t> calls;std::uint32_t saved_step{};unsigned publications{};',"  std::vector<std::uint32_t> calls;std::uint32_t saved_step{};unsigned publications{};bool post_observed{};\n  NativeTu15DirtyIslandObjectsInputObserverBinding pending_observer(m,[&](const auto&owner,const auto&input){check(&owner==&m&&input.preserved_x87_depth==2&&input.floating_point.status==0x2b00u&&(input.floating_point.mxcsr&0x3fu)==0&&input.floating_point.control_word==0x27fu,\"Native post-step Source FP clear phase\");post_observed=true;});");s=once(s,anchor,'  check(post_observed,\"Native Frame lost actual post-step observation\");\n'+anchor);(stage/r).write_text(s)
 (a.output/'delta.patch').write_text(''.join(''.join(difflib.unified_diff(before[name].splitlines(True),(stage/name).read_text().splitlines(True),fromfile='a/'+name,tofile='b/'+name))for name in files))
 save(a.output/'source-pins.json',dict(qualified=False,root_reviewed=False,production_applied=False,live_qualified=False,base_source_root=str(a.source_root),base_hashes=base,delta_files=files,new_files=new,fields=FIELDS,prefix=PREFIX,staged_hashes={str(p.relative_to(stage)):sha(p)for p in stage.rglob('*')if p.is_file()},scope='Exact actual Frame post_step typednative282C10 through sole native-only builder; readonly actual-input observer preserved. Known1CC310/BA8C0 native helpers, all unconverted lifetime/allocator/GC owners named count+throw before any legacy executor. Actual pending0 measured profile distinct from declared positive cleanup and unqualified deep owners. Internal generated282C10 callers remain explicit.'))
if __name__=='__main__':main()
