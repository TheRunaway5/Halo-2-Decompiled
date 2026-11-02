#!/usr/bin/env python3
"""Narrow retained-DATA native2E5320 stage over authenticated final1480 files."""
from pathlib import Path
import difflib,hashlib,json,re
import validate_native_moving_shape_process_full_native_actual_tu15 as full
import validate_native_moving_shape_process_solved_actual_tu15 as prefix
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'analysis/physics/native-moving-shape-world-stage-20261102'
FIELDS={
 'attempts':'native_moving_process_dispatches','calls':'moving_shape_process.completed',
 'refusals':'moving_shape_process.refusals+world_integration.native_moving_process_observer_refusals+world_integration.native_moving_process_fp_refusals+world_integration.native_moving_process_resource_refusals',
 'observer_refusals':'native_moving_process_observer_refusals','entry_fp_refusals':'native_moving_process_fp_refusals','entry_resource_refusals':'native_moving_process_resource_refusals',
 'helper_entries':'moving_shape_process.attempts',
 **{name:'moving_shape_process.'+name for name in ('resource_refusals','depth_refusals','fp_refusals','unprovided_reads','classifications','cache_initializations','contact_publications','distance_publications','activation_requests','unqualified_owners','flag_domain_refusals')},
 'native_scope_refusals':'moving_shape_process.native_only.refused','kernel_calls':'moving_shape_process.moving.calls','kernel_completed':'moving_shape_process.moving.completed','convex_calls':'moving_shape_process.convex.calls','convex_completed':'moving_shape_process.convex.completed','simplex_calls':'moving_shape_process.solver.attempts','simplex_completed':'moving_shape_process.solver.completed','point_creations':'moving_shape_process.publication.creation.completed','point_rejections':'moving_shape_process.publication.creation.creation.rejected_points','point_removals':'moving_shape_process.removal.completed','world_listeners':'moving_shape_process.gameplay.world_calls','limit_listeners':'moving_shape_process.gameplay.limit_calls','body_early_outs':'moving_shape_process.gameplay.body_rejected_early_outs','pool_allocations':'moving_shape_process.pool.pool_allocations','pool_releases':'moving_shape_process.pool.pool_releases'}
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 base=OUT/'base';stage=OUT/'staged';stage.mkdir(exist_ok=True);before={};after={};diffs=[]
 def replace(s,a,b,count=1):assert s.count(a)==count,(a[:100],s.count(a),count);return s.replace(a,b)
 def edit(name,fn):
  old=(base/name).read_text();new=fn(old);assert new!=old;target=stage/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_text(new);before[name]=sha(base/name);after[name]=sha(target);diffs.extend(difflib.unified_diff(old.splitlines(True),new.splitlines(True),fromfile='a/'+name,tofile='b/'+name))
 def world(s):
  anchor='  NativeTu15CompoundProcessServices remaining_contact_process;'
  route='''  // One retained availability view spans the whole PROCESS and all later
  // result/publication/listener/removal consumers. No raw-private observer is
  // accepted in this bounded contract; preserve the named refusal explicitly.
  const auto process_moving=[&](auto agent,auto first,auto second,auto configuration,auto output,auto&current){
    if(stats)++stats->native_moving_process_dispatches;
    if(observe){if(stats)++stats->native_moving_process_observer_refusals;
      throw std::runtime_error("Native moving PROCESS requires its retained-only observer domain");}
    if((current.control_word&~0x0c00u)!=0x027fu||(current.status&0xb8c0u)||
        (current.mxcsr&0xffff9fc0u)!=0x1f80u){
      if(stats)++stats->native_moving_process_fp_refusals;
      throw std::runtime_error("Native moving PROCESS requires its qualified entry FP policy");}
    const auto data=(scratch.first+0x1000u+15u)&~15u;
    const auto wake=(scratch.first+0xa000u+15u)&~15u;
    if(scratch.bytes<0x10000u||std::uint64_t(data)+0x8000u>wake||
        std::uint64_t(wake)+0x880u>end-0x2800u){
      if(stats)++stats->native_moving_process_resource_refusals;
      throw std::runtime_error("Native moving PROCESS requires distinct caller-owned32KiB DATA");}
    NativeTu15MovingShapeNativeServices native;native.timestamp_counter=collision.timestamp_counter;
    native.activate_island=[&](auto&view,auto current_world,auto island,auto&context){
      native_tu15_activate_island_with_residuals(view,{current_world},{island},context,state.x87_depth,
        {wake,wake+4u,wake+0x20u,wake+0x80u,0x800u},
        [](auto,std::span<const std::uint32_t>,std::optional<std::uint32_t>,unsigned,auto&)->std::uint32_t{
          throw std::runtime_error("Native moving wake requires an additional typed native owner");
        },{},stats?&stats->world_activation:nullptr);
    };
    native_tu15_process_bound_moving_shape_contact(m,{agent.identity},{first.identity},{second.identity},
      {configuration.identity},{output.identity},{data,0x8000u},current,state.x87_depth,native,pair_deletion_scope,
      stats?&stats->moving_shape_process:nullptr);
  };
'''
  s=replace(s,anchor,route+anchor)
  anchor='    const std::array args{child.first.identity,child.second.identity,\n      child.configuration.identity,child.output.identity};'
  s=replace(s,anchor,'    if(child.owner.identity==0x2e5320u){process_moving(child.receiver,child.first,child.second,child.configuration,child.output,current);return;}\n'+anchor)
  anchor='    const std::array args{first.identity,second.identity,configuration.identity,output.identity};\n    call(owner.identity,args,agent.identity,16);'
  return replace(s,anchor,'    if(owner.identity==0x2e5320u){process_moving(agent,first,second,configuration,output,context);return;}\n'+anchor)
 edit('src/physics/native_tu15_world_integration_residuals.cpp',world)
 def stats(s):
  s=replace(s,'#include <map>','#include "halo2/native_tu15_moving_shape_process_binding.hpp"\n#include <map>')
  return replace(s,'  NativeTu15ContactProcessBindingStats contact_process;','  NativeTu15MovingShapeBindingActivity moving_shape_process;\n  std::uint64_t native_moving_process_dispatches{},native_moving_process_observer_refusals{},native_moving_process_fp_refusals{},native_moving_process_resource_refusals{};\n  NativeTu15ContactProcessBindingStats contact_process;')
 edit('include/halo2/native_tu15_world_integration.hpp',stats)
 declaration='  std::uint64_t '+','.join('native_moving_process_'+name+'{}'for name in FIELDS)+';\n  std::map<std::uint32_t,std::uint64_t> native_moving_process_refused_owners;\n'
 edit('include/halo2/native_tu15_physics_frame_residuals.hpp',lambda s:replace(s,'  std::uint64_t native_broad_phase_calls{};','  std::uint64_t native_broad_phase_calls{};\n'+declaration))
 getter=''.join('    result.native_moving_process_'+name+'=world_integration.'+path+';\n'for name,path in FIELDS.items())
 getter+='''    for(const auto*owners:{&world_integration.moving_shape_process.refused_owners,
        &world_integration.moving_shape_process.native_only.owners,
        &world_integration.moving_shape_process.gameplay.unqualified_owners})
      for(const auto&[owner,count]:*owners)result.native_moving_process_refused_owners[owner]+=count;
'''
 edit('src/physics_live_world_tu15.cpp',lambda s:replace(s,'    const auto&world_integration=impl_->native_physics_integrations;','    const auto&world_integration=impl_->native_physics_integrations;\n'+getter))
 gate='''  if(std::getenv("HALO2_REQUIRE_NATIVE_MOVING_SHAPE_PROCESS")) {
    const bool moving=native_frames.native_moving_process_calls>0&&
      native_frames.native_moving_process_attempts==native_frames.native_moving_process_calls&&
      native_frames.native_moving_process_helper_entries==native_frames.native_moving_process_calls&&
      native_frames.native_moving_process_kernel_calls==native_frames.native_moving_process_calls&&
      native_frames.native_moving_process_kernel_completed==native_frames.native_moving_process_calls&&
      native_frames.native_moving_process_refusals==0&&native_frames.native_moving_process_unprovided_reads==0&&
      native_frames.native_moving_process_native_scope_refusals==0&&native_frames.native_moving_process_unqualified_owners==0&&
      native_frames.native_moving_process_flag_domain_refusals==0&&native_frames.native_moving_process_refused_owners.empty()&&
      !native_frames.residual_owner_calls.contains(0x2e5320u);
    report["native_moving_shape_process_gate"]=moving;passed&=moving;
  }
'''
 def accept(s):
  anchor='        {"native_broad_phase_calls",native_frames.native_broad_phase_calls},';extra=''.join('        {"native_moving_process_'+name+'",native_frames.native_moving_process_'+name+'},\n'for name in FIELDS)+'        {"native_moving_process_refused_owners",native_frames.native_moving_process_refused_owners},'
  return replace(replace(s,anchor,anchor+'\n'+extra,2),'  if(std::getenv("HALO2_REQUIRE_NATIVE_BROAD_PHASE_EDGES")) {',gate+'  if(std::getenv("HALO2_REQUIRE_NATIVE_BROAD_PHASE_EDGES")) {')
 edit('tests/physics_multiplayer_acceptance.cpp',accept)
 manifest=json.loads((ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-broad-phase-edges-original-v1480-20261102/source-manifest.json').read_text())['source_files_sha256']
 # Recursively pin every native arithmetic/data dependency actually compiled by
 # the same-helper original proof, comparing with the exact frozen source set.
 units=tuple(dict.fromkeys(prefix.UNITS+full.EXTRA+('native_tu15_constraint_body_slots.cpp','native_tu15_constraint_scalar_cache.cpp','native_tu15_contact_creation_gameplay.cpp','native_tu15_activation_payload_filter.cpp','native_tu15_activation_agents.cpp','native_tu15_contact_cache.cpp')))
 pending=[(ROOT/'src/physics'/name).resolve()for name in units]+[ROOT/'include/halo2/native_tu15_moving_shape_process_binding.hpp'];visited={};unit_names=[]
 qualified=ROOT/'analysis/physics/native-moving-shape-process-qualified-actual1479-20261102/source'
 baseline=qualified/'src/physics/native_tu15_convex_simplex_solver.cpp'
 while pending:
  original=pending.pop();name=str(original.relative_to(ROOT));path=qualified/name if(qualified/name).is_file()else original
  if name in visited:continue
  digest=sha(path);visited[name]=digest
  if manifest.get(name)!=digest:
   target=stage/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(path.read_bytes());before[name]=manifest.get(name);after[name]=digest
  for inc in re.findall(r'^#include "([^"\n]+)"',path.read_text(),re.M):
   child=ROOT/'include'/inc if inc.startswith('halo2/')else original.parent/inc
   if child.is_file():pending.append(child.resolve())
 for name in units:unit_names.append(str((ROOT/'src/physics'/name).resolve().relative_to(ROOT)))
 def cmake(s):
  existing=set(re.findall(r'src/[^\s)\"]+\.cpp',s));new=[name for name in unit_names if name not in existing]
  return s+'\n# Native moving PROCESS: sole semantic kernels; same retained validity view.\ntarget_sources(halo2_native_pair_consumer PRIVATE\n'+''.join('  '+name+'\n'for name in new)+')\n# Reuse the existing sole feature archive; do not duplicate its CPP.\ntarget_link_libraries(halo2_native_pair_consumer PUBLIC halo2_native_contact_features)\n'
 edit('cmake/PhysicsLiveWorldTu15.cmake',cmake)
 # These small test-only headers were inherited by the qualified1480 model,
 # but are deliberately absent from its runtime-only source checkpoint.
 inherited=json.loads((ROOT/'analysis/physics/native-broad-phase-edges-world-stage-20261102/source-pins.json').read_text())['inherited_source_sha256']
 for name,digest in inherited.items():
  path=ROOT/'analysis/physics/native-broad-phase-edges-world-stage-20261102/staged'/name;assert sha(path)==digest
  target=stage/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(path.read_bytes());before[name]=manifest.get(name);after[name]=digest
 (OUT/'native-moving-shape-world.diff').write_text(''.join(diffs))
 r=dict(qualified=False,root_reviewed=False,no_root_source_changes=True,base_1480_manifest_sha256='b5a7f99df8cae90cda36884056d1e8aa5aa0b0e5cbe7007d24327a8d0b8e1ec1',base_hashes=before,source_sha256=after,dependency_source_sha256=visited,fields=FIELDS,new_gate='HALO2_REQUIRE_NATIVE_MOVING_SHAPE_PROCESS',compiled_solver_baseline_sha256=sha(baseline),remaining=['Exact staged World models and dispatch/counters/FP/resource/observer tests','Independent source/resource peer','Fresh imported-MCC visible strict original/live','Wider solver/count/shape/gameplay/profile/exception observer domains remain refused'])
 (OUT/'source-pins.json').write_text(json.dumps(r,indent=2)+'\n');print(dict(staged_files=len(after),fields=len(FIELDS),manifest_sha256=sha(OUT/'source-pins.json')))
if __name__=='__main__':main()
