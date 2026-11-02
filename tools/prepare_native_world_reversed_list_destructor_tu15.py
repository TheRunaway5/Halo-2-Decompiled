#!/usr/bin/env python3
"""Narrow qualified-native2F81B0 callback route and truthful activity transport."""
import argparse,difflib,shutil
from pathlib import Path
from build_native_tu15_constraint_before_reference import sha,save
ROOT=Path(__file__).resolve().parents[1]
PREFIX='native_reversed_list_agent_destructor_'
FIELDS={'attempts':'attempts','calls':'completed','refusals':'refusals','unqualified_owners':'native_only.refused'}
def once(s,a,b):assert s.count(a)==1,(a,s.count(a));return s.replace(a,b)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source-root',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.source_root=a.source_root.resolve();a.output=a.output.resolve();a.output.mkdir(exist_ok=False,parents=True);stage=a.output/'staged'
 shared=['include/halo2/native_tu15_world_integration.hpp','include/halo2/native_tu15_physics_frame_residuals.hpp','src/physics/native_tu15_world_integration_residuals.cpp','src/physics/physics_live_world_tu15.cpp','tests/physics_multiplayer_acceptance.cpp','tests/native_tu15_world_residuals_tests.cpp'];before={};paths={};inherited=['cmake/PhysicsLiveWorldTu15.cmake','tests/helpers/native_world_convex_factory_prefix_tu15.hpp','tests/helpers/native_world_island_merge_prefix_tu15.hpp','tests/helpers/native_contact_process_binding_model_tu15.hpp']
 for relative in shared+inherited:
  path=a.source_root/relative
  if not path.exists()and relative=='src/physics/physics_live_world_tu15.cpp':path=a.source_root/'src/physics_live_world_tu15.cpp'
  target=stage/relative;target.parent.mkdir(exist_ok=True,parents=True);shutil.copyfile(path,target);paths[relative]=path;before[relative]=path.read_text()
 helpers=['include/halo2/native_tu15_reversed_list_agent_destructor.hpp','include/halo2/native_tu15_reversed_list_agent_destructor_binding.hpp']
 for relative in helpers:shutil.copyfile(ROOT/relative,stage/relative)
 relative=shared[0];s=once(before[relative],'#include <map>','#include "halo2/native_tu15_reversed_list_agent_destructor_binding.hpp"\n#include <map>');s=once(s,'  NativeTu15PairDeletionBindingActivity pair_deletion;','  NativeTu15ReversedListAgentDestructorActivity reversed_list_agent_destructor;\n  NativeTu15PairDeletionBindingActivity pair_deletion;');(stage/relative).write_text(s)
 relative=shared[1];s=once(before[relative],'  std::map<std::uint32_t,std::uint64_t> native_pair_deletion_refused_owners;','  std::map<std::uint32_t,std::uint64_t> '+PREFIX+'refused_owners;\n  '+''.join('std::uint64_t '+PREFIX+n+'{};\n  'for n in FIELDS)+'std::map<std::uint32_t,std::uint64_t> native_pair_deletion_refused_owners;');(stage/relative).write_text(s)
 relative=shared[2];s=before[relative];anchor='    if(owner.identity==0x2f8090u||owner.identity==0x3051a0u||owner.identity==0x2fc540u){';new='''    if(owner.identity==0x2f81b0u){
      // The same native-only guard applies even outside pair deletion. Any
      // deeper unconverted allocator owner refuses before legacy transport.
      (void)native_tu15_delete_bound_reversed_list_contact_agent(m,{object},disposing,
        context,agent_children,pair_deletion_scope,observe,
        stats?&stats->reversed_list_agent_destructor:nullptr);
      if(stats)++stats->native_contact_agent_native_destructors;return;
    }
''';s=once(s,anchor,new+anchor);(stage/relative).write_text(s)
 relative=shared[3];s=before[relative];anchor='    result.native_pair_deletion_attempts=world_integration.pair_deletion.attempts;';s=once(s,anchor,''.join('    result.'+PREFIX+n+'=world_integration.reversed_list_agent_destructor.'+field+';\n'for n,field in FIELDS.items())+'    result.'+PREFIX+'refused_owners=world_integration.reversed_list_agent_destructor.native_only.owners;\n'+anchor);(stage/relative).write_text(s)
 relative=shared[4];s=before[relative];anchor='        {"native_pair_deletion_attempts",native_frames.native_pair_deletion_attempts},';assert s.count(anchor)==2;s=s.replace(anchor,''.join('        {"'+PREFIX+n+'",native_frames.'+PREFIX+n+'},\n'for n in[*FIELDS,'refused_owners'])+anchor)
 gate='''  if(std::getenv("HALO2_REQUIRE_NATIVE_REVERSED_LIST_AGENT_DESTRUCTOR")) {
    const bool native_destructor=native_frames.native_reversed_list_agent_destructor_calls>0&&
      native_frames.native_reversed_list_agent_destructor_attempts==native_frames.native_reversed_list_agent_destructor_calls&&
      native_frames.native_reversed_list_agent_destructor_refusals==0&&native_frames.native_reversed_list_agent_destructor_unqualified_owners==0&&
      native_frames.native_reversed_list_agent_destructor_refused_owners.empty()&&
      !native_frames.residual_owner_calls.contains(0x2f81b0u);
    report["native_reversed_list_agent_destructor_gate"]=native_destructor;passed&=native_destructor;
  }
''';s=once(s,'  if(std::getenv("HALO2_REQUIRE_NATIVE_PAIR_DELETION")) {',gate+'  if(std::getenv("HALO2_REQUIRE_NATIVE_PAIR_DELETION")) {');(stage/relative).write_text(s)
 relative=shared[5];s=before[relative];s=once(s,'  unsigned inactive_cleanup_policy_refs=2;','  bool reversed_list_cleanup_case{};\n  unsigned inactive_cleanup_policy_refs=2;');s=once(s,'m.put(world+0xd8c0u,0x2f8090u);','m.put(world+0xd8c0u,reversed_list_cleanup_case?0x2f81b0u:0x2f8090u);')
 s=once(s,'std::pair{1u,8u},std::pair{1u,9u}}){','std::pair{1u,8u},std::pair{1u,9u},std::pair{1u,10u}}){')
 s=once(s,'const auto policy_refs=profile.first;inactive_cleanup_agent_kind=profile.second;','const auto policy_refs=profile.first;reversed_list_cleanup_case=profile.second==10u;\n    inactive_cleanup_agent_kind=reversed_list_cleanup_case?5u:profile.second;')
 start=s.index('    check(stats.inactive_island_cleanup.calls==1');end=s.index('  for(const auto phantom_owner:',start);part=s[start:end].replace('profile.second','inactive_cleanup_agent_kind');needle='    inactive_cleanup_case=false;';assert part.count(needle)==1;part=part.replace(needle,'''    check(stats.reversed_list_agent_destructor.attempts==unsigned(reversed_list_cleanup_case)&&
      stats.reversed_list_agent_destructor.completed==unsigned(reversed_list_cleanup_case)&&
      stats.reversed_list_agent_destructor.refusals==0&&stats.reversed_list_agent_destructor.native_only.refused==0&&
      !stats.residual_owner_calls.contains(0x2f81b0u),"World reversed list scalar destructor bypassed native binding");
    reversed_list_cleanup_case=false;
'''+needle);s=s[:start]+part+s[end:];(stage/relative).write_text(s)
 (a.output/'delta.patch').write_text(''.join(''.join(difflib.unified_diff(before[r].splitlines(True),(stage/r).read_text().splitlines(True),fromfile='a/'+r,tofile='b/'+r))for r in shared))
 save(a.output/'source-pins.json',dict(qualified=False,production_applied=False,root_reviewed=False,base_source_root=str(a.source_root),base_hashes={r:sha(p)for r,p in paths.items()},delta_files=shared,new_files=helpers,inherited_files=inherited,staged_hashes={str(p.relative_to(stage)):sha(p)for p in stage.rglob('*')if p.is_file()},fields=FIELDS,prefix=PREFIX,scope='Actual2F81B0 World scalar-destructor route through same native helper+strict scope; preserves00ROcapture/pair/refusedcensus, adds4counters/map/twoJSON/gate. Eleven declared default cleanup profiles; no live proof yet.'))
 print(dict(prepared=True,delta_files=len(shared),new_headers=len(helpers)));return 0
if __name__=='__main__':raise SystemExit(main())
