#include "halo2/native_tu15_physics_frame_residuals.hpp"
#include "halo2/native_tu15_dirty_island_objects_binding.hpp"
#include "halo2/native_tu15_dirty_island_objects_input_observer.hpp"
#include "halo2/native_tu15_contact_pressure_binding.hpp"
#include "halo2/native_tu15_memory_pressure.hpp"
#include "halo2/native_tu15_object_publication.hpp"
#include <array>
#include <limits>
#include <stdexcept>
namespace halo2 {
namespace {
void check(bool ok,const char*why){if(!ok)throw std::runtime_error(why);}
void put(SourcePhysicsContactMemory&m,std::uint32_t address,std::uint32_t value){
  std::array<std::byte,4>b{};for(unsigned i=0;i<4;++i)b[i]=std::byte(value>>(i*8u));m.write(address,b);
}
std::uint32_t word(SourcePhysicsContactMemory&m,std::uint32_t address){
  std::array<std::byte,4>b{};m.read(address,b);std::uint32_t value{};for(unsigned i=0;i<4;++i)value|=std::to_integer<unsigned>(b[i])<<(i*8u);return value;
}
void valid_live_values(const SourcePhysicsContactRuntimeAbi&a){
  check(a.x87_depth<=8,"Native physics frame residual has invalid live x87 depth");
  check(!a.active_mmx||!a.x87_depth,"Native physics frame residual mixes MMX and logical x87 ownership");
  if(a.x87_depth)check(sizeof(long double)>=10,"Native physics frame residual requires original extended precision live values");
}
}
void native_tu15_step_physics_frame_with_residuals(SourcePhysicsContactMemory&m,
  NativeTu15PhysicsFrameFp&fp,SourcePhysicsContactRuntimeAbi&state,
  NativeTu15PhysicsFrameScratch scratch,const NativeTu15PhysicsFrameResidualCall&invoke,
  const SourcePhysicsContactRecordObserver&observe,NativeTu15PhysicsFrameExecutionStats*stats,
  const NativeTu15PhysicsFrameWorldStep&native_world_step){
  if(stats)++stats->frame_calls;
  NativeTu15PhysicsFrameMemory storage(m,observe);
  // Preserve the original null-world prefix: it needs no stack/backend owner.
  if(!storage.world().identity)return;
  valid_live_values(state);
  const auto end=std::uint64_t(scratch.first)+scratch.bytes;
  check(scratch.bytes>=0x800u&&end<=std::numeric_limits<std::uint32_t>::max()&&end<=state.gp[4],
    "Native physics frame requires an owned retained stack interval below the active caller");
  const auto call_stack=std::uint32_t(end)-0x40u,step_packet=std::uint32_t(end)-0x10u;
  // Reuse the actual caller-owned continuation token as the bridge return
  // protocol. It is not an invented original root result or guest execution.
  const auto continuation=word(m,state.gp[4]);
  auto call=[&](std::uint32_t entry,std::span<const std::uint32_t>args,
      std::optional<std::uint32_t>eax,std::optional<std::uint32_t>ecx,
      unsigned callee_pop,NativeTu15PhysicsFrameFp&context){
    check(bool(invoke),"Native physics frame residual owner unavailable");valid_live_values(state);
    state.gp[4]=call_stack;if(eax)state.gp[0]=*eax;if(ecx)state.gp[1]=*ecx;
    state.x87_status=context.status;state.floating_point=SourcePhysicsFactoryFloatingPoint{context.control_word,context.mxcsr};
    put(m,call_stack,continuation);for(unsigned n=0;n<args.size();++n)put(m,call_stack+4u+n*4u,args[n]);
    if(stats){++stats->residual_calls;++stats->residual_owner_calls[entry];}
    try{invoke(entry,state,observe);}catch(...){
      // A real missing-owner/fault prefix keeps all actual callee FP effects.
      if(state.floating_point){context.control_word=state.floating_point->x87_control_word;context.mxcsr=state.floating_point->mxcsr;context.status=state.x87_status;}
      throw;
    }
    check(bool(state.floating_point),"Native physics frame residual lost actual FP policy");valid_live_values(state);
    context.control_word=state.floating_point->x87_control_word;context.mxcsr=state.floating_point->mxcsr;context.status=state.x87_status;
    check(state.gp[4]==call_stack+4u+callee_pop,"Native physics frame residual return stack differs from its original ABI");
  };
  NativeTu15PhysicsFrameServices s;
  s.resize_debug_arena=[&](auto bytes,auto&f){const std::array args{bytes};call(0x2e2990u,args,{},{},0,f);};
  s.reset_debug_arena=[&](auto&f){call(0x2e2910u,{},{},{},0,f);};
  s.publish_debug_arena=[&](auto arena,auto&f){call(0x2e2b30u,{},{},arena,0,f);};
  s.drain_commands=[&](auto&f){
    NativeTu15MemoryPressureStats activity;
    const auto aggregate=[&]{if(stats){
      stats->native_memory_pressure_attempts+=activity.calls;
      stats->native_memory_pressure_level_queries+=activity.level_queries;
      stats->native_memory_pressure_pool_queries+=activity.pool_queries;
      stats->native_memory_pressure_reentrant_skips+=activity.reentrant_skips;
      stats->native_memory_pressure_unbound_services+=activity.unbound_services;
    }};
    try{
      // Object trimming/removal/cleanup children remain deliberately empty.
      // A selected source child counts/refuses at its actual boundary.
      native_tu15_collect_memory_pressure(m,f,{},observe,&activity);
    }catch(...){aggregate();if(stats)++stats->native_memory_pressure_refusals;throw;}
    aggregate();if(stats)++stats->native_memory_pressure_completions;
  };
  s.step_world=[&](auto world,const auto&step,auto&f){
    if(native_world_step){native_world_step(world,step,f);return;}
    put(m,step_packet,step.seconds_bits);put(m,step_packet+4u,step.inverse_seconds_bits);
    const std::array args{step_packet};call(0x31b1a0u,args,{},world.identity,4,f);
  };
  s.post_step=[&](auto&f){
    native_tu15_observe_dirty_island_objects_input(m,{f,state.x87_depth});
    NativeTu15DirtyIslandObjectsBindingActivity activity;
    const auto aggregate=[&]{if(stats){
      stats->native_dirty_island_objects_attempts+=activity.attempts;
      stats->native_dirty_island_objects_calls+=activity.completed;
      stats->native_dirty_island_objects_refusals+=activity.refusals;
      stats->native_dirty_island_objects_kernel_entries+=activity.update.calls;
      stats->native_dirty_island_objects_pending_skips+=activity.update.pending_skips;
      stats->native_dirty_island_objects_reentrant_skips+=activity.update.reentrant_skips;
      stats->native_dirty_island_objects_island_rows+=activity.update.island_rows;
      stats->native_dirty_island_objects_dirty_islands+=activity.update.dirty_islands;
      stats->native_dirty_island_objects_body_rows+=activity.update.body_rows;
      stats->native_dirty_island_objects_candidate_objects+=activity.update.candidate_objects;
      stats->native_dirty_island_objects_deferred_objects+=activity.update.deferred_objects;
      stats->native_dirty_island_objects_component_destructions+=activity.update.component_destructions;
      stats->native_dirty_island_objects_component_frees+=activity.update.component_frees;
      stats->native_dirty_island_objects_object_deletions+=activity.update.object_deletions;
      stats->native_dirty_island_objects_recursive_updates+=activity.update.recursive_updates;
      stats->native_dirty_island_objects_flag_calls+=activity.state.flag_calls;
      stats->native_dirty_island_objects_defer_calls+=activity.state.defer_calls;
      stats->native_dirty_island_objects_state_removals+=activity.state.removals;
      stats->native_dirty_island_objects_state_resets+=activity.state.resets;
      stats->native_dirty_island_objects_state_refreshes+=activity.state.refreshes;
      stats->native_dirty_island_objects_state_activations+=activity.state.activations;
      stats->native_dirty_island_objects_unqualified_services+=activity.update.unbound_services+activity.state.unbound_services;
      stats->native_dirty_island_objects_unsupported_local_sizes+=activity.update.unsupported_local_set_sizes;
      for(const auto&[owner,count]:activity.update.unbound_owners)stats->native_dirty_island_objects_unqualified_owners[owner]+=count;
      for(const auto&[owner,count]:activity.state.unbound_owners)stats->native_dirty_island_objects_unqualified_owners[owner]+=count;
    }};
    try{native_tu15_process_bound_dirty_island_objects(m,f,state.x87_depth,observe,&activity);}
    catch(...){aggregate();throw;}
    aggregate();
  };
  s.update_contacts=[&](auto&f){
    NativeTu15ContactPressureStats activity;
    const auto aggregate=[&]{if(stats){
      stats->native_contact_pressure_attempts+=activity.calls;
      stats->native_contact_pressure_null_worlds+=activity.null_worlds;
      stats->native_contact_pressure_reentrant_skips+=activity.reentrant_skips;
      stats->native_contact_pressure_islands+=activity.islands;
      stats->native_contact_pressure_dense_islands+=activity.dense_islands;
      stats->native_contact_pressure_shape_queries+=activity.shape_queries;
      stats->native_contact_pressure_candidate_queries+=activity.candidate_queries;
      stats->native_contact_pressure_unbound_services+=activity.unbound_services;
    }};
    try{
      // Candidate/deletion children remain deliberately empty.
      // A selected source child counts/refuses at its actual boundary.
      native_tu15_prune_contact_pressure(m,f,native_tu15_contact_pressure_native_services(&activity),observe,&activity);
    }catch(...){aggregate();if(stats)++stats->native_contact_pressure_refusals;throw;}
    aggregate();if(stats)++stats->native_contact_pressure_completions;
  };
  s.publish_object=[&](auto object,auto&){
    if(stats)++stats->native_object_publication_attempts;
    NativeTu15ObjectPublicationStats activity;
    const auto record_activity=[&]{if(stats){
      stats->native_object_publication_root_queries+=activity.root_queries;
      stats->native_object_publication_tree_calls+=activity.tree_calls;
      stats->native_object_publication_ancestor_wakes+=activity.ancestor_wakes;
      stats->native_object_publication_marked_headers+=activity.marked_headers;
    }};
    try{native_tu15_publish_object_update(m,object,observe,&activity);}
    catch(...){record_activity();if(stats)++stats->native_object_publication_refusals;throw;}
    record_activity();if(stats)++stats->native_object_publication_calls;
  };
  s.destroy_body=[&](auto body,bool deleting,auto&f){
    const auto vtable=word(m,body.identity),target=word(m,vtable);
    const std::array args{std::uint32_t(deleting)};call(target,args,{},body.identity,4,f);
  };
  native_tu15_step_physics_frame(storage,fp,s);
  // Only real FP state is published to the residual context. Its other fields
  // remain the actual last callee output; callers must not use them as if the
  // native root had reproduced the original dead register file.
  state.x87_status=fp.status;state.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};
}
} // namespace halo2
