#!/usr/bin/env python3
"""Stage native World merge/activation wiring and scoped diagnostics privately."""
import argparse,difflib,json,re
from pathlib import Path
from build_native_tu15_constraint_before_reference import sha,save
ROOT=Path(__file__).resolve().parent.parent
FIELDS=['native_world_merge_calls','native_world_merge_contact_calls','native_world_merge_body_moves','native_world_merge_constraint_moves',
 'native_world_merge_disposals','native_world_merge_refusals','native_world_merge_unbound_services','native_world_activation_calls',
 'native_world_activation_refusals','native_world_activation_body_updates','native_world_activation_constraint_rows','native_world_activation_residual_calls']
def once(text,needle,replacement):
 assert text.count(needle)==1,needle[:100];return text.replace(needle,replacement,1)
WORLD_BINDING='''  NativeTu15IslandLifetimeServices merge_lifetime;
  merge_lifetime.allocation=arena_storage;
  const auto unqualified_merge=[&](const char*why){
    if(stats)++stats->island_merge.unbound_services;
    throw std::runtime_error(why);
  };
  merge_lifetime.allocate_arena=[&](auto arena,auto owner,auto bytes,auto&context){
    if(owner.identity!=0x2e10a0u)unqualified_merge("Native TU1.5 island merge requires actual native arena allocation");
    const auto data=native_tu15_allocate_stack_arena(m,arena,bytes,context,arena_storage,observe);
    if(stats)++stats->native_stack_arena_allocations;return data;
  };
  merge_lifetime.release_arena=[&](auto arena,auto owner,auto data,auto bytes,auto&context){
    if(owner.identity!=0x2e1110u)unqualified_merge("Native TU1.5 island merge requires actual native arena release");
    native_tu15_release_stack_arena(m,arena,data,bytes,context,arena_storage,observe);
    if(stats)++stats->native_stack_arena_releases;
  };
  const auto merge_data=(std::uint32_t(end)-0x1800u)&~15u;
  const auto merge_service_stack=merge_data-0x800u;
  NativeTu15IslandContactMergeServices merge_contacts;
  merge_contacts.lifetime=merge_lifetime;
  const auto reserve_merge_array=[&](auto header,auto capacity,auto stride,auto&context){
    if(stats)++stats->native_array_reserve_calls;
    native_tu15_reserve_array(m,header,capacity,stride,context,arena_storage,observe);
  };
  merge_contacts.reserve_array=reserve_merge_array;
  const auto cleanup_merge_duplicate=[&](auto object,auto owner,auto&context){
    const auto target=owner.identity;
    if(target!=0x31d400u&&target!=0x2f3100u&&target!=0x2f7a20u&&target!=0x323220u&&
       target!=0x2f82d0u&&target!=0x2f6c80u)
      unqualified_merge("Native TU1.5 island merge requires a qualified duplicate cleanup owner");
    invalidate_agent(object,target,context);
  };
  merge_contacts.cleanup_duplicate_agent=cleanup_merge_duplicate;
  merge_contacts.release_duplicate_manager=cleanup_merge_duplicate;
  NativeTu15IslandMergeServices merge_services;
  merge_services.merge_contact_records=[&](auto first,auto second,auto&context){
    native_tu15_merge_island_contact_records(m,first,second,{{merge_data}},context,
      merge_contacts,observe,stats?&stats->island_contact_merge:nullptr);
  };
  merge_services.reserve_array=reserve_merge_array;
  merge_services.destroy_island=[&](auto island,auto owner,auto disposing,auto&context){
    if(owner.identity!=0x322100u)unqualified_merge("Native TU1.5 island merge requires actual322100 island lifetime");
    (void)native_tu15_destroy_physics_island(m,island,disposing,context,merge_lifetime,
      observe,stats?&stats->island_lifetime:nullptr);
  };
  merge_services.activate_island=[&](auto current_world,auto island,auto&context){
    try{
      // The shared service builder owns the same body/constraint lifecycle
      // as B9800. No velocity packet or guest postimage represents this call.
      native_tu15_activate_island_with_residuals(m,current_world,island,context,state.x87_depth,
        {merge_data+0x20u,merge_data+0x24u,merge_data+0x40u,merge_data+0x80u,0x800u},
        [&](auto,std::span<const std::uint32_t>,std::optional<std::uint32_t>,unsigned,
            NativeTu15PhysicsFrameFp&)->std::uint32_t{
          throw std::runtime_error("Native TU1.5 World activation requires an additional native typed owner");
        },observe,stats?&stats->world_activation:nullptr);
      if(stats)++stats->native_world_activation_calls;
    }catch(...){if(stats)++stats->native_world_activation_refusals;throw;}
  };
  deferred.merge_islands=[&](auto current_world,auto first,auto second,auto&context){
    const auto previous=native_merge_scope;native_merge_scope=true;
    try{
      check(scratch.bytes>=0x2400u&&merge_service_stack>=scratch.first+0x200u&&
        merge_data>=merge_service_stack+0x100u&&std::uint64_t(merge_data)+0x880u<=end-0x800u,
        "Native TU1.5 World merge requires distinct aligned retained DATA and service arena");
      (void)native_tu15_merge_physics_islands(m,current_world,first,second,context,
        merge_services,observe,stats?&stats->island_merge:nullptr);
    }catch(...){native_merge_scope=previous;if(stats)++stats->native_world_merge_refusals;throw;}
    native_merge_scope=previous;
  };
'''
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 paths=['src/physics/native_tu15_world_integration_residuals.cpp','include/halo2/native_tu15_world_integration.hpp',
  'include/halo2/native_tu15_physics_frame_residuals.hpp','src/physics/physics_live_world_tu15.cpp',
  'tests/native_tu15_world_residuals_tests.cpp','tests/physics_multiplayer_acceptance.cpp','cmake/PhysicsLiveWorldTu15.cmake']
 original={p:(ROOT/p).read_text()for p in paths};staged=dict(original)
 cpp=paths[0];assert sha(ROOT/cpp)=='906768a20425451f849643714d06153ad853ce075720d739f0bc715f6d8768a4'
 text=staged[cpp]
 text=once(text,'  auto call_at=[&]','  bool native_merge_scope=false;\n  auto call_at=[&]')
 text=once(text,'    check(bool(invoke),"Native TU1.5 world residual owner unavailable");',
  '    if(native_merge_scope){if(stats)++stats->island_merge.unbound_services;throw std::runtime_error("Native TU1.5 merged lifecycle refuses an unconverted child owner");}\n    check(bool(invoke),"Native TU1.5 world residual owner unavailable");')
 text=once(text,'''  deferred.merge_islands=[&](auto w,auto first,auto second,auto&){
    const std::array args{first.identity,second.identity};call(0x3190d0u,args,w.identity,8);
  };
''','')
 text=once(text,'  broad_phase.allocate_block=allocate_arena;',WORLD_BINDING+'  broad_phase.allocate_block=allocate_arena;');staged[cpp]=text
 header=paths[1];text=staged[header];text=once(text,'#include <map>','#include "halo2/native_tu15_island_contact_merge.hpp"\n#include "halo2/native_tu15_object_activation_residuals.hpp"\n#include <map>')
 text=once(text,'  NativeTu15WorldDeferredStats deferred;','''  NativeTu15IslandMergeStats island_merge;
  NativeTu15IslandContactMergeStats island_contact_merge;
  NativeTu15IslandLifetimeStats island_lifetime;
  NativeTu15ObjectActivationBindingActivity world_activation;
  std::uint64_t native_world_merge_refusals{},native_world_activation_calls{},native_world_activation_refusals{};
  NativeTu15WorldDeferredStats deferred;''');staged[header]=text
 header=paths[2];text=staged[header];text=once(text,'  std::uint64_t native_world_collision_calls{};',
  '  std::uint64_t '+','.join(k+'{}'for k in FIELDS)+';\n  std::uint64_t native_world_collision_calls{};');staged[header]=text
 getter=paths[3];text=staged[getter];needle='    const auto&velocity=impl_->native_object_velocity_transport;'
 additions='''    const auto&world_integration=impl_->native_physics_integrations;
    const auto&world_activation=world_integration.world_activation;
    result.native_world_merge_calls=world_integration.island_merge.calls;
    result.native_world_merge_contact_calls=world_integration.island_contact_merge.calls;
    result.native_world_merge_body_moves=world_integration.island_merge.moved_bodies;
    result.native_world_merge_constraint_moves=world_integration.island_merge.moved_constraints;
    result.native_world_merge_disposals=world_integration.island_lifetime.island_disposals;
    result.native_world_merge_refusals=world_integration.native_world_merge_refusals;
    result.native_world_merge_unbound_services=world_integration.island_merge.unbound_services+
      world_integration.island_contact_merge.unbound_services+world_integration.island_contact_merge.lifetime.unbound_services+
      world_integration.island_lifetime.unbound_services;
    result.native_world_activation_calls=world_integration.native_world_activation_calls;
    result.native_world_activation_refusals=world_integration.native_world_activation_refusals;
    result.native_world_activation_body_updates=world_activation.services.body_updates;
    result.native_world_activation_constraint_rows=world_activation.body_constraints.rows;
    result.native_world_activation_residual_calls=world_activation.residual_calls;
'''
 text=once(text,needle,additions+needle);begin=text.index('    const auto&removal=');end=text.index('    result.native_object_velocity_payload_predicates=',begin)
 block=text[begin:end];mapping={'velocity.binding.':'world_activation.','creation.':'world_activation.constraint_creation.','removal.':'world_activation.constraint_removal.',
  'handles.':'world_activation.constraint_runtime_handles.','destruction.':'world_activation.constraint_handle_destruction.','effect_selection.':'world_activation.constraint_effect_selector.',
  'runtime.':'world_activation.constraint_runtime_update.'};audit=[]
 def aggregate(match):
  field,expression=match.groups();extra=[]
  for term in expression.split('+'):
   term=term.strip()
   for old,new in mapping.items():
    if term.startswith(old):extra.append(new+term[len(old):]);break
  if not extra:return match.group(0)
  audit.append(dict(field=field,added_world_terms=extra));return 'result.'+field+'='+expression+'+\n      '+'+'.join(extra)+';'
 block=re.sub(r'result\.(native_constraint_\w+|native_gameplay_policy_destructions)=([^;]+);',aggregate,block)
 text=text[:begin]+block+text[end:];staged[getter]=text
 test=paths[4];text=staged[test]
 text=once(text,'m.put(world+0x24,1);','m.put(world+0x24,0); /* This callback fixture does not own a consistent merge pair. */')
 text=once(text,'expected{0x3190d0,shapeBounds','expected{shapeBounds')
 text=once(text,'stats.deferred.merges==1','stats.deferred.merges==0')
 text=once(text,'stats.deferred.operation_calls==1','stats.deferred.operation_calls==0')
 needle='int main(int argc,char**argv){try{if(argc==2)return native_world_convex_factory_prefix(argv[1]);'
 text=once(text,needle,'#include "helpers/native_world_island_merge_prefix_tu15.hpp"\nint main(int argc,char**argv){try{if(argc==4&&std::string(argv[1])=="merge-prefix")return native_world_island_merge_prefix(argv[2],argv[3]);if(argc==2)return native_world_convex_factory_prefix(argv[1]);')
 first=text.index('    else if(target==0x3190d0)');end=text.index('    else if(target==0x3198a0',first)
 text=text[:first]+'''    else if(target==0x3190d0)throw std::runtime_error("General World fixture must not invoke an opaque island merge");
'''+text[end:];staged[test]=text
 acceptance=paths[5];text=staged[acceptance];needle='        {"native_object_velocity_handoff_attempts",native_frames.native_object_velocity_handoff_attempts},';assert text.count(needle)==2
 rows=''.join('        {"'+field+'",native_frames.'+field+'},\n'for field in FIELDS);text=text.replace(needle,rows+needle)
 gate='''  if(std::getenv("HALO2_REQUIRE_NATIVE_WORLD_MERGE")) {
    const bool merge_passed=native_frames.native_world_merge_calls>0&&
      native_frames.native_world_merge_contact_calls>0&&native_frames.native_world_merge_body_moves>0&&
      native_frames.native_world_merge_disposals>0&&native_frames.native_world_merge_refusals==0&&
      native_frames.native_world_merge_unbound_services==0&&native_frames.native_world_activation_refusals==0&&
      !native_frames.residual_owner_calls.contains(0x3190d0u)&&!native_frames.residual_owner_calls.contains(0x318de0u)&&
      !native_frames.residual_owner_calls.contains(0x322100u)&&!native_frames.residual_owner_calls.contains(0x3213a0u);
    report["native_world_merge_gate"]=merge_passed;passed&=merge_passed;
  }
  if(std::getenv("HALO2_REQUIRE_NATIVE_WORLD_ACTIVATION")) {
    const bool wake_passed=native_frames.native_world_activation_calls>0&&
      native_frames.native_world_activation_body_updates>0&&native_frames.native_world_activation_refusals==0&&
      native_frames.native_world_activation_residual_calls==0&&native_frames.native_world_merge_unbound_services==0;
    report["native_world_activation_gate"]=wake_passed;passed&=wake_passed;
  }
'''
 text=once(text,'  if(std::getenv("HALO2_REQUIRE_NATIVE_CONSTRAINT_ARENA")) {',gate+'  if(std::getenv("HALO2_REQUIRE_NATIVE_CONSTRAINT_ARENA")) {');staged[acceptance]=text
 cmake=paths[6];staged[cmake]=once(staged[cmake],'add_library(halo2_native_object_velocity STATIC\n','add_library(halo2_native_object_velocity STATIC\n    src/physics/native_tu15_island_merge.cpp\n    src/physics/native_tu15_island_contact_merge.cpp\n    src/physics/native_tu15_island_lifetime.cpp\n')
 patches=[]
 for relative,text in staged.items():
  out=a.output/'staged'/relative;out.parent.mkdir(parents=True,exist_ok=True);out.write_text(text)
  patches.extend(difflib.unified_diff(original[relative].splitlines(True),text.splitlines(True),fromfile='a/'+relative,tofile='b/'+relative))
 (a.output/'world-merge-binding.inc').write_text(WORLD_BINDING);(a.output/'native-world-island-merge-binding.diff').write_text(''.join(patches));save(a.output/'aggregation-audit.json',dict(fields=audit,total_general_fields=len(audit),object_velocity_specific_fields_modified=False))
 save(a.output/'source-pins.json',dict(qualified=False,working_tree_modified=False,base_hashes={p:sha(ROOT/p)for p in paths},staged_hashes={p:sha(a.output/'staged'/p)for p in paths},source_hashes={str(Path(__file__).relative_to(ROOT)):sha(Path(__file__))},remaining=['Private default and genuine09 World-prefix model','Getter/serializer/gate compile/model validation','Root application and visible live native merge/wake']))
 print(dict(staged_files=len(paths),general_aggregation_fields=len(audit)));return 0
if __name__=='__main__':raise SystemExit(main())
