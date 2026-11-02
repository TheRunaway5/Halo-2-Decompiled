#!/usr/bin/env python3
"""Stage native320260 World routing without changing an active source cohort."""
import argparse,difflib,json,shutil
from pathlib import Path
from build_native_tu15_constraint_before_reference import sha,save
ROOT=Path(__file__).resolve().parents[1]
PREFIX='native_pair_deletion_'
FIELDS={'attempts':'pair_deletion.attempts','calls':'pair_deletion.completed',
 'refusals':'pair_deletion.refusals','agent_cleanups':'pair_deletion.agent_cleanups',
 'manager_releases':'pair_deletion.manager_releases','unqualified_owners':'pair_deletion.unqualified_owners',
 'nested_refusals':'pair_deletion_scope.refused'}
def once(s,a,b):assert s.count(a)==1,(a,s.count(a));return s.replace(a,b)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source-root',type=Path,required=True);p.add_argument('--formal-stage',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.source_root=a.source_root.resolve();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);stage=a.output/'staged'
 shared=['include/halo2/native_tu15_world_integration.hpp','include/halo2/native_tu15_physics_frame_residuals.hpp','src/physics/native_tu15_world_integration_residuals.cpp','src/physics/physics_live_world_tu15.cpp','tests/physics_multiplayer_acceptance.cpp','tests/native_tu15_world_residuals_tests.cpp'];inherited=['cmake/PhysicsLiveWorldTu15.cmake','tests/helpers/native_world_convex_factory_prefix_tu15.hpp','tests/helpers/native_world_island_merge_prefix_tu15.hpp','tests/helpers/native_contact_process_binding_model_tu15.hpp'];before={};base_paths={}
 for relative in shared+inherited:
  path=a.source_root/relative
  if not path.exists()and relative=='src/physics/physics_live_world_tu15.cpp':path=a.source_root/'src/physics_live_world_tu15.cpp'
  target=stage/relative;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(path,target);base_paths[relative]=path;before[relative]=path.read_text()
 formal=['include/halo2/native_tu15_pair_handlers.hpp','src/physics/native_tu15_pair_handlers.cpp']
 for relative in formal:
  before[relative]=(a.source_root/relative).read_text();base_paths[relative]=a.source_root/relative;target=stage/relative;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(a.formal_stage/'staged'/relative,target)
 helpers=['include/halo2/native_tu15_pair_deletion_binding.hpp','include/halo2/native_tu15_pair_deletion_native_scope.hpp']
 for relative in helpers:shutil.copyfile(ROOT/relative,stage/relative)
 relative=shared[0];s=once(before[relative],'#include <map>','#include "halo2/native_tu15_pair_deletion_native_scope.hpp"\n#include <map>');s=once(s,'  NativeTu15PairIslandWakeActivity pair_island_wake;','  NativeTu15PairDeletionBindingActivity pair_deletion;\n  NativeTu15PairDeletionNativeScopeActivity pair_deletion_scope;\n  NativeTu15PairIslandWakeActivity pair_island_wake;');(stage/relative).write_text(s)
 relative=shared[1];s=once(before[relative],'  std::map<std::uint32_t,std::uint64_t> residual_owner_calls,shape_bounds_residual_owners;','  std::map<std::uint32_t,std::uint64_t> native_pair_deletion_refused_owners;\n  '+''.join('std::uint64_t '+PREFIX+n+'{};\n  'for n in FIELDS)+'std::map<std::uint32_t,std::uint64_t> residual_owner_calls,shape_bounds_residual_owners;');(stage/relative).write_text(s)
 relative=shared[2];s=once(before[relative],'  bool native_merge_scope=false,native_regroup_scope=false;','  NativeTu15PairDeletionNativeScope pair_deletion_scope;\n  bool native_merge_scope=false,native_regroup_scope=false;')
 anchor='    if(native_regroup_scope){';s=once(s,anchor,'    pair_deletion_scope.reject_unconverted(target); // BEFORE transport mutations or original execution.\n'+anchor)
 old='    const std::array args{pair};call(owner.identity,args,handler,4);'
 new='''    // Four bytes of ordinary formal DATA, outside all live pair/point packets.
    const auto formal=std::uint32_t(end)-0xa4u;put(m,formal,pair);
    NativeTu15PairDeletionBindingServices deletion;
    deletion.cleanup_agent=[&](auto agent,auto target,auto&current){invalidate_agent(agent.identity,target.identity,current);};
    deletion.release_manager=[&](auto manager,auto target,auto&current){invalidate_agent(manager.identity,target.identity,current);};
    native_tu15_delete_bound_pair_in_native_scope(m,handler,pair,{formal},context,deletion,
      pair_deletion_scope,observe,stats?&stats->pair_deletion:nullptr,stats?&stats->pair_deletion_scope:nullptr);'''
 s=once(s,old,new);(stage/relative).write_text(s)
 relative=shared[3];s=before[relative];anchor='    result.native_world_activation_calls=world_integration.native_world_activation_calls;';s=once(s,anchor,''.join('    result.'+PREFIX+n+'=world_integration.'+field+';\n'for n,field in FIELDS.items())+'    result.native_pair_deletion_refused_owners=world_integration.pair_deletion_scope.owners;\n'+anchor);(stage/relative).write_text(s)
 relative=shared[4];s=before[relative];anchor='        {"named_residual_owner_calls",native_frames.residual_owner_calls},';assert s.count(anchor)==2;s=s.replace(anchor,''.join('        {"'+PREFIX+n+'",native_frames.'+PREFIX+n+'},\n'for n in FIELDS)+'        {"native_pair_deletion_refused_owners",native_frames.native_pair_deletion_refused_owners},\n'+anchor)
 gate='''  if(std::getenv("HALO2_REQUIRE_NATIVE_PAIR_DELETION")) {
    const bool native_deletion=native_frames.native_pair_deletion_calls>0&&
      native_frames.native_pair_deletion_attempts==native_frames.native_pair_deletion_calls&&
      native_frames.native_pair_deletion_refusals==0&&native_frames.native_pair_deletion_unqualified_owners==0&&
      native_frames.native_pair_deletion_nested_refusals==0&&native_frames.native_pair_deletion_refused_owners.empty()&&
      !native_frames.residual_owner_calls.contains(0x320260u);
    report["native_pair_deletion_gate"]=native_deletion;passed&=native_deletion;
  }
''';s=once(s,'  if(std::getenv("HALO2_REQUIRE_NATIVE_PAIR_ISLAND_WAKE")) {',gate+'  if(std::getenv("HALO2_REQUIRE_NATIVE_PAIR_ISLAND_WAKE")) {');(stage/relative).write_text(s)
 relative=shared[5];s=before[relative]
 s=once(s,'delete_pair_case=true;m.put(body2+0x20u,0);','delete_pair_case=true;m.put(body2+0x20u,0);\n    // The former opaque process-only agent does not define deletion owners.\n    // This declared parent profile has no agent; genuine child cleanup is proved independently.\n    m.put(world+0x7108u,0);')
 s=once(s,'std::count(calls.begin(),calls.end(),0x320260u)==1&&','stats.pair_deletion.attempts==1&&stats.pair_deletion.completed==1&&\n      stats.pair_deletion.refusals==0&&stats.pair_deletion_scope.refused==0&&\n      !stats.residual_owner_calls.contains(0x320260u)&&\n      std::count(calls.begin(),calls.end(),0x320260u)==0&&')
 # A declared unsupported descendant must be refused before entering invoke.
 anchor='    delete_pair_case=false;\n  }\n  {\n    m=model();a=initial();fp={0x27f,0x2800,0x1f80};stats={};residuals=0;calls.clear();checks=0;\n    constexpr std::uint32_t pool='
 control='''    // Explicit unsupported-destructor control, not an authenticated object class.
    m=model();a=initial();fp={0x27f,0x2800,0x1f80};stats={};residuals=0;calls.clear();checks=0;
    m.put(body2+0x20u,0);m.put(world+0x7054u,0x2f3100u);m.put(world+0x7040u,0x2301a0u);
    bool refused=false;try{native_tu15_step_physics_world_with_residuals(m,{world},{0x3d088889,0x41f00000},fp,a,0x60300000,{stack,0x10000},invoke,body_observer,&stats,&residuals,[&]{return ++timestamps;});}
    catch(const std::runtime_error&error){refused=std::string(error.what()).find("2294176")!=std::string::npos;}
    check(refused&&stats.pair_deletion.attempts==1&&stats.pair_deletion.completed==0&&stats.pair_deletion.refusals==1&&
      stats.pair_deletion_scope.refused==1&&stats.pair_deletion_scope.owners.at(0x2301a0u)==1&&
      !stats.residual_owner_calls.contains(0x2301a0u)&&std::find(calls.begin(),calls.end(),0x2301a0u)==calls.end(),
      "Native pair deletion entered original transport for an unsupported nested owner");
'''
 s=once(s,anchor,control+anchor);(stage/relative).write_text(s)
 delta=[*shared,*formal];(a.output/'delta.patch').write_text(''.join(''.join(difflib.unified_diff(before[r].splitlines(True),(stage/r).read_text().splitlines(True),fromfile='a/'+r,tofile='b/'+r))for r in delta))
 save(a.output/'source-pins.json',dict(qualified=False,production_applied=False,base_source_root=str(a.source_root),base_hashes={r:sha(p)for r,p in base_paths.items()},delta_files=delta,new_files=helpers,inherited_files=inherited,staged_hashes={str(p.relative_to(stage)):sha(p)for p in stage.rglob('*')if p.is_file()},fields=FIELDS,prefix=PREFIX,scope='Narrow native320260 World route with formal4B DATA and guard before generic transport; same native helper, truthful separate nested-refused owner map; bounded188/247sites source proof,59 remain open; no live claim.'))
 print(dict(prepared=True,delta_files=len(delta),counters=len(FIELDS),refused_owner_maps=1));return 0
if __name__=='__main__':raise SystemExit(main())
