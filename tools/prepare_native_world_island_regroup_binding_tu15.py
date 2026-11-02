#!/usr/bin/env python3
"""Stage narrow World3187C0 replacement on immutable point07/06, never Root files."""
import argparse,difflib,hashlib,json,shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
BASE06=ROOT/'analysis/physics/native-reversed-list-transformed-world-binding-stage-20261101'
BASE07=ROOT/'analysis/physics/native-contact-point-world-binding-stage-v3-20261101'
UNITS=['island_partition_graph','island_partition_metadata','island_partition_build','raw_allocator','island_partition_allocator_binding','island_partition_temporary','island_partition_edges','island_construction','island_regroup','island_regroup_binding']
FIELDS={
 'attempts':'attempts','calls':'completed','refusals':'refusals','active_visits':'regroup.active_visits',
 'flag_skips':'regroup.flag_skips','fully_connected':'regroup.fully_connected','empty_islands':'regroup.empty_islands',
 'created_islands':'regroup.created_islands','moved_bodies':'regroup.moved_bodies','compacted_bodies':'regroup.compacted_bodies',
 'moved_actions':'regroup.moved_actions','moved_constraints':'regroup.moved_constraints','moved_contacts':'regroup.moved_contacts',
 'retained_contacts':'regroup.retained_contacts','graph_builds':'regroup.partition.builds','edge_collections':'regroup.edges.calls',
 'array_growths':'regroup.array_growths','group_allocations':'regroup.partition.group_allocations','group_releases':'regroup.partition.group_releases',
 'temporary_constructors':'regroup.lifetime.temporary_constructors','temporary_destructors':'regroup.lifetime.temporary_destructors',
 'pool_allocations':'allocator.pool_allocations','pool_releases':'allocator.pool_releases'}
UNKNOWN=['regroup.unbound_services','regroup.lifetime.unbound_services','regroup.partition.unbound_services','regroup.edges.unbound_services','allocator.unbound_services','allocator.raw.unbound_services']
SHARED=['cmake/PhysicsLiveWorldTu15.cmake','include/halo2/native_tu15_physics_frame_residuals.hpp','include/halo2/native_tu15_world_integration.hpp','src/physics/native_tu15_world_integration_residuals.cpp','src/physics/physics_live_world_tu15.cpp','tests/physics_multiplayer_acceptance.cpp','tests/native_tu15_world_residuals_tests.cpp']
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def save(p,j):p.write_text(json.dumps(j,indent=2,sort_keys=True)+'\n')
def replace(s,a,b,n=1):assert s.count(a)==n,(a,s.count(a));return s.replace(a,b)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);stage=a.output/'staged';shutil.copytree(BASE06/'staged',stage)
 inherited={}
 for base in(BASE06,BASE07):
  for path in(base/'staged').rglob('*'):
   if path.is_file():target=stage/path.relative_to(base/'staged');target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(path,target);inherited[str(path.relative_to(base/'staged'))]=sha(path)
 before={p:(stage/p).read_text()for p in SHARED};base_hashes={p:sha(stage/p)for p in SHARED}
 for unit in UNITS:
  for relative in('include/halo2/native_tu15_'+unit+'.hpp','src/physics/native_tu15_'+unit+'.cpp'):
   target=stage/relative;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/relative,target)
 relative=SHARED[0];s=before[relative];anchor='target_include_directories(halo2_native_object_velocity PUBLIC include)'
 extra=[f'src/physics/native_tu15_{n}.cpp'for n in UNITS if f'src/physics/native_tu15_{n}.cpp'not in s];s=replace(s,anchor,'target_sources(halo2_native_object_velocity PRIVATE\n    '+'\n    '.join(extra)+')\n'+anchor);(stage/relative).write_text(s)
 fields=['native_world_regroup_'+n for n in FIELDS]+['native_world_regroup_unbound_services']
 relative=SHARED[1];s=before[relative];anchor='  std::uint64_t native_world_merge_calls{}';assert s.count(anchor)==1;s=s.replace(anchor,'  std::uint64_t '+','.join(n+'{}'for n in fields)+';\n'+anchor);(stage/relative).write_text(s)
 relative=SHARED[2];s=before[relative];s=replace(s,'#include <map>','#include "halo2/native_tu15_island_regroup_binding.hpp"\n#include <map>');s=replace(s,'  NativeTu15IslandMergeStats island_merge;','  NativeTu15IslandRegroupBindingActivity island_regroup;\n  NativeTu15IslandMergeStats island_merge;');(stage/relative).write_text(s)
 relative=SHARED[3];s=before[relative];s=replace(s,'  bool native_merge_scope=false;','  bool native_merge_scope=false,native_regroup_scope=false;');anchor='    if(native_merge_scope){';assert s.count(anchor)==1;s=s.replace(anchor,'    if(native_regroup_scope){if(stats)++stats->island_regroup.regroup.unbound_services;throw std::runtime_error("Native TU1.5 regroup refuses an unconverted child owner");}\n'+anchor)
 s=replace(s,'  s.regroup_islands=[&](auto w,auto&){world_call(0x3187c0u,w);};','''  s.regroup_islands=[&](auto w,auto&context){
    const auto previous=native_regroup_scope;native_regroup_scope=true;
    try{
      // Every packet is caller-owned ordinary DATA, disjoint from retained
      // merge/activation packets and any active original callback frame.
      const auto data=(std::uint32_t(end)-0x2400u)&~15u;
      const bool admitted=scratch.bytes>=0x2800u&&data>=scratch.first+0x200u&&
        std::uint64_t(data)+0x180u<=merge_service_stack;
      if(!admitted&&stats){++stats->island_regroup.attempts;++stats->island_regroup.refusals;}
      check(admitted,"Native TU1.5 regroup requires distinct aligned retained DATA");
      native_tu15_regroup_physics_islands_with_native_services(m,w,
        {{data},{data+0x20u},{data+0x40u},{data+0x60u},{data+0x80u},
         {{data+0xa0u},data+0xc0u},{data+0x100u},data+0x120u,{data+0x160u}},
        context,observe,stats?&stats->island_regroup:nullptr);
    }catch(...){native_regroup_scope=previous;throw;}
    native_regroup_scope=previous;
  };''');(stage/relative).write_text(s)
 relative=SHARED[4];s=before[relative];anchor='    result.native_world_merge_calls=world_integration.island_merge.calls;';fragment='    const auto&world_regroup=world_integration.island_regroup;\n'+''.join('    result.native_world_regroup_'+n+'=world_regroup.'+path+';\n'for n,path in FIELDS.items())+'    result.native_world_regroup_unbound_services='+(' +\n      '.join('world_regroup.'+v for v in UNKNOWN))+';\n';s=replace(s,anchor,fragment+anchor);(stage/relative).write_text(s)
 relative=SHARED[5];s=before[relative];anchor='        {"native_world_merge_calls",native_frames.native_world_merge_calls},';rows=''.join('        {"'+f+'",native_frames.'+f+'},\n'for f in fields);s=replace(s,anchor,rows+anchor,2)
 anchor='  if(std::getenv("HALO2_REQUIRE_NATIVE_WORLD_MERGE")) {';gate='''  if(std::getenv("HALO2_REQUIRE_NATIVE_WORLD_REGROUP")||std::getenv("HALO2_REQUIRE_NATIVE_WORLD_REPARTITION")) {
    const bool native_regroup=native_frames.native_world_regroup_calls>0&&
      native_frames.native_world_regroup_attempts==native_frames.native_world_regroup_calls&&
      native_frames.native_world_regroup_active_visits>0&&native_frames.native_world_regroup_refusals==0&&
      native_frames.native_world_regroup_unbound_services==0&&
      !native_frames.residual_owner_calls.contains(0x3187c0u)&&!native_frames.residual_owner_calls.contains(0x321eb0u);
    report["native_world_regroup_gate"]=native_regroup;passed&=native_regroup;
    if(std::getenv("HALO2_REQUIRE_NATIVE_WORLD_REPARTITION")) {
      const bool split=native_regroup&&native_frames.native_world_regroup_graph_builds>0&&
        native_frames.native_world_regroup_created_islands>0&&native_frames.native_world_regroup_moved_bodies>0&&
        native_frames.native_world_regroup_group_allocations==native_frames.native_world_regroup_group_releases&&
        native_frames.native_world_regroup_temporary_constructors==native_frames.native_world_regroup_temporary_destructors;
      report["native_world_repartition_gate"]=split;passed&=split;
    }
  }
''';s=replace(s,anchor,gate+anchor);(stage/relative).write_text(s)
 relative=SHARED[6];s=before[relative];s=replace(s,'m.put(world+0x23c,1,1);','m.put(world+0x23c,0,1); /* Opaque callback fixture has inconsistent empty-island lifetimes; actual regroup is proved separately. */');s=replace(s,'else if(target==0x3187c0)check(actual.gp[1]==world,"World ECX service ABI");','else if(target==0x3187c0)throw std::runtime_error("Old regroup parent must not execute");');s=replace(s,'islandStepListener,0x3187c0,destroy','islandStepListener,destroy');anchor='  check(stats.world_integrations==1';s=replace(s,anchor,'  check(stats.island_regroup.attempts==0&&!stats.residual_owner_calls.contains(0x3187c0u),"Opaque callback fixture unexpectedly admitted regroup");\n'+anchor);(stage/relative).write_text(s)
 patch=''.join(''.join(difflib.unified_diff(before[p].splitlines(True),(stage/p).read_text().splitlines(True),fromfile='a/'+p,tofile='b/'+p))for p in SHARED);(a.output/'delta.patch').write_text(patch)
 manifest=dict(qualified=False,production_applied=False,live_qualified=False,base_source_roots=[str(BASE06/'staged'),str(BASE07/'staged')],base_hashes=base_hashes,inherited_hashes=inherited,delta_files=SHARED,source_units=UNITS,fields=fields,staged_hashes={str(p.relative_to(stage)):sha(p)for p in stage.rglob('*')if p.is_file()},scope='Sole native3187C0 service builder on immutable point07 plus06; no old parent fallback, retained DATA only, unknown owners refuse; default10 callback fixture disables opaque inconsistent regroup. Actual split controls remain declared, not live.',remaining=['Both-compiler production-builder physical qualification','Coherent World default/factory/merge models and Getter/gate fragments','Visible multiplayer normal regroup and genuine positive repartition','Positive action/constraint/contact redistribution and callback fault/unwind domains'])
 save(a.output/'source-pins.json',manifest);print(dict(prepared=True,delta_files=len(SHARED),source_units_added=len(extra),fields=len(fields)))
 return 0
if __name__=='__main__':raise SystemExit(main())
