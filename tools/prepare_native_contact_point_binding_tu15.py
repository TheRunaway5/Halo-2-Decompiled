#!/usr/bin/env python3
"""Prepare a narrow point-only World overlay against an explicit frozen root.

Never changes the supplied source tree or imports the current shared World.
Existing process/factory/merge/manager code and their gates remain untouched.
"""
import argparse,difflib,hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
FIELDS={
 'native_contact_point_attempts':'attempts','native_contact_point_calls':'completed',
 'native_contact_point_refusals':'refusals','native_contact_point_profile_refusals':'unsupported_profiles',
 'native_contact_point_unqualified_services':'unqualified_services','native_contact_point_noop_listeners':'noop_listeners',
 'native_contact_point_gameplay_listeners':'gameplay_listeners','native_contact_point_gameplay_rows':'gameplay.rows',
 'native_contact_point_metadata_calls':'collector_updates+activity.detach.native_metadata_removals',
 'native_contact_point_world_updates':'world.completed','native_contact_point_world_queued_updates':'world.queued',
 'native_contact_point_island_detaches':'detach.calls','native_contact_point_info_queries':'detach.native_info_queries',
 'native_contact_point_reference_releases':'detach.reference_releases',
 'native_contact_point_effect_requests':'gameplay.constraint_releases','native_contact_point_part_requests':'gameplay.part_releases'}
UNITS=('native_tu15_contact_point_removal','native_tu15_contact_world_update','native_tu15_gameplay_contact_removal',
       'native_tu15_island_constraint_detach','native_tu15_island_partition_metadata','native_tu15_contact_collector_info')
def sha(raw):return hashlib.sha256(raw).hexdigest()
def change(text,before,after,n=1):assert text.count(before)==n,(before,text.count(before));return text.replace(before,after)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source-root',required=True,type=Path);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);stage=a.output/'staged';base_pins={};stage_pins={};patch=[]
 paths=('src/physics/native_tu15_world_integration_residuals.cpp','include/halo2/native_tu15_world_integration.hpp','include/halo2/native_tu15_physics_frame_residuals.hpp','src/physics/physics_live_world_tu15.cpp','tests/physics_multiplayer_acceptance.cpp','cmake/PhysicsLiveWorldTu15.cmake')
 for name in paths:
  source=a.source_root/name
  if not source.exists()and name=='src/physics/physics_live_world_tu15.cpp':source=a.source_root/'src/physics_live_world_tu15.cpp'
  raw=source.read_bytes();text=raw.decode();base_pins[name]=sha(raw)
  if name.endswith('world_integration_residuals.cpp'):
   anchor='#include "halo2/native_tu15_contact_point_input_observer.hpp"';text=change(text,anchor,anchor+'\n#include "halo2/native_tu15_contact_point_binding.hpp"')
   before='''    if(owner.identity==0x322f70u)native_tu15_observe_contact_point_removal_input(m,{manager,owner.identity,point,context,state.x87_depth});'''
   aggregate='\n'.join('          stats->'+field+'+=activity.'+member+';'for field,member in FIELDS.items())
   branch='''    if(owner.identity==0x322f70u){
      native_tu15_observe_contact_point_removal_input(m,{manager,owner.identity,point,context,state.x87_depth});
      NativeTu15ContactPointBindingActivity activity;
      const auto collect_point=[&]{if(stats){
'''+aggregate+'''
      }};
      try{
        // Event20B and info16B occupy the free gap after pair header[-60,-4C)
        // and before out[-20,-10). All are above the genuine call_stack.
        // Point info is dead before detach325E90 completely rewrites it.
        const NativeTu15ContactPointBindingScratch point_data{
          std::uint32_t(end)-0x4cu,std::uint32_t(end)-0x38u,
          std::uint32_t(end)-0x24u,std::uint32_t(end)-0x38u};
        native_tu15_remove_bound_contact_point(m,manager,std::uint32_t(point),point_data,
          context,state.x87_depth,arena_storage,observe,&activity);
        collect_point();
      }catch(...){collect_point();throw;}
      return;
    }'''
   text=change(text,before,branch)
  elif name.endswith(('native_tu15_world_integration.hpp','native_tu15_physics_frame_residuals.hpp')):
   anchor='  std::uint64_t native_contact_agent_cached_cleanups{}';index=text.index(anchor)
   text=text[:index]+'  std::uint64_t '+','.join(field+'{}'for field in FIELDS)+';\n'+text[index:]
  elif name.endswith('physics_live_world_tu15.cpp'):
   anchor='    result.native_contact_agent_point_residuals=impl_->native_physics_integrations.native_contact_agent_point_residuals;'
   text=change(text,anchor,anchor+'\n'+'\n'.join('    result.'+field+'=impl_->native_physics_integrations.'+field+';'for field in FIELDS))
  elif name.endswith('physics_multiplayer_acceptance.cpp'):
   anchor='        {"native_contact_agent_point_residuals",native_frames.native_contact_agent_point_residuals},'
   text=change(text,anchor,anchor+'\n'+'\n'.join('        {"'+field+'",native_frames.'+field+'},'for field in FIELDS),2)
   anchor='  if(std::getenv("HALO2_REQUIRE_NATIVE_CONTACT_MANAGER_CLEANUP")) {'
   gate='''  if(std::getenv("HALO2_REQUIRE_NATIVE_CONTACT_POINT_PIPELINE")) {
    const bool point_passed=native_frames.native_contact_point_calls>0&&
      native_frames.native_contact_point_attempts==native_frames.native_contact_point_calls&&
      native_frames.native_contact_point_refusals==0&&native_frames.native_contact_point_profile_refusals==0&&
      native_frames.native_contact_point_unqualified_services==0&&native_frames.native_contact_agent_point_residuals==0&&
      native_frames.native_contact_point_gameplay_listeners>0&&native_frames.native_contact_point_gameplay_rows>0&&
      native_frames.native_contact_point_world_updates>0&&native_frames.native_contact_point_island_detaches>0&&
      native_frames.native_contact_point_info_queries==native_frames.native_contact_point_island_detaches&&
      native_frames.native_contact_point_reference_releases==native_frames.native_contact_point_island_detaches&&
      native_frames.native_contact_point_metadata_calls>=native_frames.native_contact_point_calls+native_frames.native_contact_point_island_detaches&&
      native_frames.native_contact_point_effect_requests==0&&native_frames.native_contact_point_part_requests==0&&
      !native_frames.residual_owner_calls.contains(0x322f70u)&&!native_frames.residual_owner_calls.contains(0x281d40u)&&
      !native_frames.residual_owner_calls.contains(0x1d8880u)&&!native_frames.residual_owner_calls.contains(0x1d8970u)&&
      !native_frames.residual_owner_calls.contains(0x325d90u)&&!native_frames.residual_owner_calls.contains(0x320100u)&&
      !native_frames.residual_owner_calls.contains(0x3201b0u)&&!native_frames.residual_owner_calls.contains(0x325e90u);
    report["native_contact_point_pipeline_gate"]=point_passed;passed&=point_passed;
  }
''';text=change(text,anchor,gate+anchor)
  else:
   text+='''
# Qualified native point/gameplay/world update and typed constraint-info/detach closure.
target_sources(halo2_native_pair_consumer PRIVATE
    src/physics/native_tu15_contact_point_removal.cpp
    src/physics/native_tu15_contact_world_update.cpp
    src/physics/native_tu15_gameplay_contact_removal.cpp
)
target_sources(halo2_native_object_velocity PRIVATE
    src/physics/native_tu15_island_constraint_detach.cpp
    src/physics/native_tu15_island_partition_metadata.cpp
    src/physics/native_tu15_contact_collector_info.cpp
)
'''
  destination=stage/name;destination.parent.mkdir(parents=True,exist_ok=True);destination.write_text(text);stage_pins[name]=sha(text.encode());patch.extend(difflib.unified_diff(raw.decode().splitlines(True),text.splitlines(True),fromfile='a/'+name,tofile='b/'+name))
 for name in (*['src/physics/'+unit+'.cpp'for unit in UNITS],*['include/halo2/'+unit+'.hpp'for unit in UNITS],'include/halo2/native_tu15_contact_point_binding.hpp'):
  raw=(ROOT/name).read_bytes();destination=stage/name;destination.parent.mkdir(parents=True,exist_ok=True);destination.write_bytes(raw);stage_pins[name]=sha(raw)
 (a.output/'native-contact-point-binding.diff').write_text(''.join(patch))
 proofs=('analysis/physics/native-contact-point-production-binding-actual1470-tu15-20261101-first/report.json','analysis/physics/native-contact-point-actual1470-pipeline-tu15-20261101-first/report.json','analysis/physics/native-gameplay-contact-removal-tu15-20261101-qualified/report.json','analysis/physics/native-contact-world-update-tu15-20261101-qualified/report.json','analysis/physics/native-contact-collector-info-tu15-20261101-first/report.json','analysis/physics/native-island-constraint-detach-actual1470-qualified-20261101/source-pins.json')
 report=dict(base_source_root=str(a.source_root.resolve()),base_hashes=base_pins,staged_hashes=stage_pins,fields=list(FIELDS),source_units=list(UNITS),proofs={name:dict(sha256=sha((ROOT/name).read_bytes()),passed=json.loads((ROOT/name).read_text()).get('passed'))for name in proofs if(ROOT/name).exists()},production_applied=False,live_qualified=False,private_raw_ledger_passed=False,scope='Narrow measured point handler/native service builder only; source profiles/effect/part/zero-ref unknown owners refuse without CPU fallback. Existing factory/process/merge/manager guards unchanged. Info lifetime reuse is tested by the same production builder on every genuine1470 bank. Generic call_at native_merge_scope guard remains unchanged.',gate='HALO2_REQUIRE_NATIVE_CONTACT_POINT_PIPELINE',remaining=['Coherent World model/build','Visible multiplayer full frame and actual vehicle camera','Unqualified effect/part/zero-reference/profile branches and wider native physics closure'])
 (a.output/'source-pins.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(a.output)
if __name__=='__main__':main()
