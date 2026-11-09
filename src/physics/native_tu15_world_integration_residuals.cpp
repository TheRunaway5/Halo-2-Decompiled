#include "halo2/native_tu15_world_integration_residuals.hpp"
#include "halo2/native_tu15_world_factory_scratch.hpp"
#include "halo2/native_tu15_pair_pool_release_input_observer.hpp"
#include "halo2/native_tu15_motion_pose.hpp"
#include "halo2/native_tu15_island_deactivation.hpp"
#include "halo2/native_tu15_body_deactivation_policy.hpp"
#include "halo2/native_tu15_island_transition.hpp"
#include "halo2/native_tu15_kinematic_motion.hpp"
#include "halo2/native_tu15_array_allocator.hpp"
#include "halo2/native_tu15_array_reserve.hpp"
#include "halo2/native_tu15_world_collision.hpp"
#include "halo2/native_tu15_world_collision_listeners.hpp"
#include "halo2/native_tu15_world_step_listeners.hpp"
#include "halo2/native_tu15_constraint_step_residuals.hpp"
#include "halo2/native_tu15_island_collision.hpp"
#include "halo2/native_tu15_broad_phase.hpp"
#include "halo2/native_tu15_pair_consumer.hpp"
#include "halo2/native_tu15_pair_consumer_input_observer.hpp"
#include "halo2/native_tu15_stack_arena.hpp"
#include "halo2/native_tu15_phantom_pair_delete.hpp"
#include "halo2/native_tu15_phantom_overlap_add.hpp"
#include "halo2/native_tu15_phantom_added_binding.hpp"
#include "halo2/native_tu15_object_deletion_spatial_binding.hpp"
#include "halo2/native_tu15_contact_agent_factory_input_observer.hpp"
#include "halo2/native_tu15_phantom_overlap_remove.hpp"
#include "halo2/native_tu15_simple_phantom_overlap_remove.hpp"
#include "halo2/native_tu15_agent_cleanup.hpp"
#include "halo2/native_tu15_agent_cleanup_children.hpp"
#include "halo2/native_tu15_gameplay_phantom_membership.hpp"
#include "halo2/native_tu15_gameplay_phantom_enter.hpp"
#include "halo2/native_tu15_phantom_callback_agent.hpp"
#include "halo2/native_tu15_contact_manager_cleanup.hpp"
#include "halo2/native_tu15_base_lock.hpp"
#include "halo2/native_tu15_contact_manager_input_observer.hpp"
#include "halo2/native_tu15_contact_point_input_observer.hpp"
#include "halo2/native_tu15_contact_point_binding.hpp"
#include "halo2/native_tu15_pair_handlers.hpp"
#include "halo2/native_tu15_convex_agent_binding.hpp"
#include "halo2/native_tu15_wrapped_contact_agent.hpp"
#include "halo2/native_tu15_wrapped_factory_binding.hpp"
#include "halo2/native_tu15_list_contact_agent.hpp"
#include "halo2/native_tu15_collection_contact_agent.hpp"
#include "halo2/native_tu15_collection_filter.hpp"
#include "halo2/native_tu15_activation_services.hpp"
#include "halo2/native_tu15_contact_time_input_observer.hpp"
#include "halo2/native_tu15_gameplay_deactivation_policy.hpp"
#include "halo2/native_tu15_pair_island_wake_native_factory_binding.hpp"
#include "halo2/native_tu15_pair_deletion_child_input_observer.hpp"
#include "halo2/native_tu15_contact_body_update_input_observer.hpp"
#include <algorithm>
#include <bit>
#include <limits>
#include <cstring>
#include <stdexcept>
#include <string>
#if defined(__i386__) || defined(__x86_64__)
#include <x86intrin.h>
#endif
namespace halo2 {
namespace {
void check(bool ok,const char*why){if(!ok)throw std::runtime_error(why);}
void put(SourcePhysicsContactMemory&m,std::uint32_t address,std::uint32_t n){std::array<std::byte,4>b{};for(unsigned i=0;i<4;++i)b[i]=std::byte(n>>(i*8u));m.write(address,b);}
std::uint32_t word(SourcePhysicsContactMemory&m,std::uint32_t address,unsigned size=4){std::array<std::byte,4>b{};m.read(address,std::span(b).first(size));std::uint32_t n{};for(unsigned i=0;i<size;++i)n|=std::to_integer<unsigned>(b[i])<<(i*8u);return n;}
void valid(const SourcePhysicsContactRuntimeAbi&a){check(a.x87_depth<=5&&!a.active_mmx,"Native TU1.5 world residual requires its unsupported x87/MMX fault owner");if(a.x87_depth)check(sizeof(long double)>=10,"Native TU1.5 world residual requires original extended live values");}
std::uint64_t timestamp(){
#if defined(__i386__) || defined(__x86_64__)
  return __rdtsc();
#else
  throw std::runtime_error("Native TU1.5 world platform timestamp owner unavailable");
#endif
}
}
void native_tu15_step_physics_world_with_residuals(SourcePhysicsContactMemory&m,
  NativeTu15PhysicsWorld world,const NativeTu15PhysicsStep&step,NativeTu15PhysicsFrameFp&fp,
  SourcePhysicsContactRuntimeAbi&state,std::uint32_t continuation,NativeTu15PhysicsFrameScratch scratch,
  const NativeTu15PhysicsFrameResidualCall&invoke,const SourcePhysicsContactRecordObserver&observe,
  NativeTu15WorldIntegrationStats*stats,std::uint64_t*residual_calls,
  const std::function<std::uint64_t()>&timestamp_counter,
  const std::function<std::uint64_t()>&actual_filetime){
  valid(state);const auto end=std::uint64_t(scratch.first)+scratch.bytes;
  check(scratch.bytes>=0x800u&&end<=std::numeric_limits<std::uint32_t>::max()&&end<=state.gp[4],"Native TU1.5 world requires owned retained scratch below the active frame");
  // Residuals grow their private frames downward from call_stack. Every
  // retained caller packet must stay ABOVE both that frame and its arguments.
  const auto call_stack=std::uint32_t(end)-0x200u,packet=std::uint32_t(end)-0x10u,out=std::uint32_t(end)-0x20u;
  put(m,packet,step.seconds_bits);put(m,packet+4,step.inverse_seconds_bits);
  auto sync_fp=[&]{if(state.floating_point){fp.control_word=state.floating_point->x87_control_word;fp.mxcsr=state.floating_point->mxcsr;fp.status=state.x87_status;}};
  NativeTu15PairDeletionNativeScope pair_deletion_scope;
  bool native_merge_scope=false,native_regroup_scope=false;
  auto call_at=[&](std::uint32_t service_stack,std::uint32_t target,std::span<const std::uint32_t>args,std::optional<std::uint32_t>ecx,unsigned pop){
    if(pair_deletion_scope.active())
      native_tu15_observe_pair_deletion_child_input(m,{target,ecx,
        std::vector<std::uint32_t>(args.begin(),args.end()),pop,fp,state.x87_depth});
    pair_deletion_scope.reject_unconverted(target); // BEFORE transport mutations or original execution.
    if(native_regroup_scope){if(stats)++stats->island_regroup.regroup.unbound_services;throw std::runtime_error("Native TU1.5 regroup refuses an unconverted child owner");}
    if(native_merge_scope){if(stats)++stats->island_merge.unbound_services;throw std::runtime_error("Native TU1.5 merged lifecycle refuses an unconverted child owner");}
    check(bool(invoke),"Native TU1.5 world residual owner unavailable");valid(state);const auto depth=state.x87_depth;
    check(service_stack>=scratch.first+0x200u&&std::uint64_t(service_stack)+4u+args.size()*4u<=end,
      "Native TU1.5 world service requires caller-owned argument/frame storage");
    state.gp[4]=service_stack;if(ecx)state.gp[1]=*ecx;state.x87_status=fp.status;state.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};
    put(m,service_stack,continuation);for(unsigned n=0;n<args.size();++n)put(m,service_stack+4u+n*4u,args[n]);if(residual_calls)++*residual_calls;
    if(stats)++stats->residual_owner_calls[target];
    try{invoke(target,state,observe);}catch(...){sync_fp();throw;}
    check(bool(state.floating_point),"Native TU1.5 world residual lost actual FP policy");sync_fp();valid(state);
    check(state.x87_depth==depth,"Native TU1.5 world void residual changed live x87 depth");
    check(state.gp[4]==service_stack+4u+pop,"Native TU1.5 world residual return stack differs from original ABI");
  };
  auto call=[&](std::uint32_t target,std::span<const std::uint32_t>args,std::optional<std::uint32_t>ecx,unsigned pop){
    call_at(call_stack,target,args,ecx,pop);
  };
  NativeTu15ArrayAllocatorMemory array_allocator(m);
  NativeTu15ArrayAllocatorServices heap_services;
  heap_services.allocate_owner=[&](const auto&request,auto&){
    if(request.owner==0x233250u){
      if(stats)++stats->native_array_pool_allocations;
      NativeTu15PoolMemory pool(m,request.receiver,observe);
      return native_tu15_pool_allocate(pool,request.bytes);
    }
    const std::array args{request.bytes,request.memory_class};
    call(request.owner,args,request.receiver,8);return state.gp[0];
  };
  heap_services.release_owner=[&](const auto&request,auto&){
    if(request.owner==0x233420u){
      NativeTu15PoolMemory pool(m,request.receiver,observe);
      if(native_tu15_pool_release(pool,request.data,request.bytes)){
        if(stats)++stats->native_array_pool_releases;
        return;
      }
    }
    const std::array args{request.data,request.bytes,request.memory_class};
    call(request.owner,args,request.receiver,12);
  };
  heap_services.default_heap=[&](auto&){call(0x149930u,{}, {},0);return state.gp[0];};
  NativeTu15ArrayGrowthServices array_services;
  array_services.allocate=[&](const auto&request,auto&context){
    if(request.owner==0x149da0u)
      return native_tu15_route_array_allocation(array_allocator,request.bytes,request.memory_class,context,heap_services);
    const std::array args{request.bytes,request.memory_class};
    call(request.owner,args,request.receiver,8);return state.gp[0];
  };
  array_services.release=[&](const auto&request,auto&context){
    if(request.owner==0x149e00u){
      native_tu15_route_array_release(array_allocator,request.data,request.bytes,request.memory_class,context,heap_services);
      return;
    }
    const std::array args{request.data,request.bytes,request.memory_class};
    call(request.owner,args,request.receiver,12);
  };
  NativeTu15WorldDeferredServices deferred;
  deferred.remove_body=[&](auto w,auto result,auto body,auto&){
    const std::array args{result.identity,body.identity};call(0x3198a0u,args,w.identity,8);
  };
  deferred.remove_update=[&](auto w,auto result,auto update,auto&){
    const std::array args{result.identity,update.identity};call(0x316aa0u,args,w.identity,8);
  };
  deferred.destroy_reference=[&](auto body,auto owner,auto disposing,auto&){
    const std::array args{disposing};call(owner.identity,args,body.identity,4);
  };
  auto drain_operations=[&](auto w,auto&context){
    native_tu15_drain_world_operations(m,w,context,deferred,observe,stats?&stats->deferred:nullptr);
  };
  // Each result is one caller-owned DATA byte above every residual frame.
  // It is not read as a supplied result or treated as a virtual CPU frame.
  const NativeTu15DeferredResult deferred_result{out-16u};
  auto drain_deletes=[&](auto w,auto&context){
    native_tu15_drain_world_deletes(m,w,deferred_result,context,deferred,observe,stats?&stats->deferred:nullptr);
  };
  auto drain_updates=[&](auto w,auto&context){
    native_tu15_drain_world_updates(m,w,deferred_result,context,deferred,observe,stats?&stats->deferred:nullptr);
  };
  NativeTu15WorldCollisionServices collision;
  collision.timestamp_counter=timestamp_counter?timestamp_counter:timestamp;
  collision.drain_operations=drain_operations;
  NativeTu15IslandCollisionServices island_collision;
  NativeTu15BroadPhaseServices broad_phase;
  broad_phase.timestamp_counter=collision.timestamp_counter;
  auto bounds_cursor=std::uint32_t(end)-0x140u;
  NativeTu15ShapeBoundsServices bounds_services;
  std::function<void(NativeTu15BoundsShape,std::uint32_t,NativeTu15BoundsTransform,
    std::uint32_t,NativeTu15BoundsOutput,NativeTu15PhysicsFrameFp&)> shape_bounds;
  shape_bounds=[&](auto shape,std::uint32_t owner,auto transform,std::uint32_t margin,auto output,auto&context){
    auto*activity=stats?&stats->shape_bounds:nullptr;
    if(owner==0x306760u){
      native_tu15_convex_box_bounds(m,shape,transform,margin,output,context,state.x87_depth,observe,activity);
    }else if(owner==0x307e30u){
      native_tu15_centered_shape_bounds(m,shape,transform,margin,output,context,state.x87_depth,
        observe,stats?&stats->centered_shape_bounds:nullptr);
    }else if(owner==0x2e8d90u){
      native_tu15_sphere_shape_bounds(m,shape,transform,margin,output,context,state.x87_depth,
        observe,stats?&stats->sphere_bounds:nullptr);
    }else if(owner==0x2e8120u){
      native_tu15_endpoint_shape_bounds(m,shape,transform,margin,output,context,state.x87_depth,
        observe,stats?&stats->compound_shape_bounds:nullptr);
    }else if(owner==0x306170u){
      check(bounds_cursor>=scratch.first+0x1000u,"Native TU1.5 compound bounds exhausted caller-owned scratch");
      const auto previous=bounds_cursor;bounds_cursor-=64u;
      try{native_tu15_compound_shape_bounds(m,shape,transform,margin,output,{bounds_cursor},
        context,bounds_services,observe,stats?&stats->compound_shape_bounds:nullptr);}
      catch(...){bounds_cursor=previous;throw;}bounds_cursor=previous;
    }else if(owner==0x2e8870u){
      // Source-compatible temporary DATA, independently retained for every
      // nested transform. Genuine residual children are called below all
      // active matrices, so their private frames cannot overwrite a parent.
      check(bounds_cursor>=scratch.first+0x1000u,"Native TU1.5 transformed bounds exhausted caller-owned scratch");
      const auto previous=bounds_cursor;bounds_cursor-=64u;
      try{native_tu15_transformed_shape_bounds(m,shape,transform,[margin]{return margin;},
        [output]{return output;},{bounds_cursor},context,bounds_services,observe,activity);}
      catch(...){bounds_cursor=previous;throw;}
      bounds_cursor=previous;
    }else if(owner==0x2e9600u){
      native_tu15_forward_shape_bounds(m,shape,transform,margin,output,context,bounds_services,activity);
    }else if(owner==0x2e5e80u){
      native_tu15_motion_shape_bounds(m,shape,transform,margin,output,context,bounds_services,observe,activity);
    }else{
      if(stats){++stats->shape_bounds_residual_calls;++stats->shape_bounds_residual_owners[owner];}
      const std::array args{transform.identity,margin,output.identity};
      const auto child_stack=std::min(call_stack,bounds_cursor-0x100u);
      call_at(child_stack,owner,args,shape.identity,12);
    }
  };
  bounds_services.child_bounds=shape_bounds;
  broad_phase.shape_bounds=[&](auto shape,auto owner,auto transform,auto margin,auto output,auto&context){
    shape_bounds({shape.identity},owner.identity,{transform},margin,{output},context);
  };
  broad_phase.grow_pairs=[&](auto pairs,auto capacity,auto stride,auto&context){
    if(stats)++stats->native_array_reserve_calls;
    native_tu15_reserve_array(m,{pairs.identity},capacity,stride,context,array_services,observe);
  };
  NativeTu15PairConsumerServices pairs;
  NativeTu15PairConsumerInput pair_input;
  const auto observe_pair_service=[&](NativeTu15PairConsumerInputPhase phase,
      auto allocator,auto owner,std::uint32_t data,std::uint32_t bytes,
      std::uint32_t tag,const NativeTu15PhysicsFrameFp&context){
    auto input=pair_input;input.phase=phase;input.allocator=allocator.identity;
    input.owner=owner.identity;input.data=data;input.bytes=bytes;input.tag=tag;
    input.floating_point=context;
    (void)native_tu15_observe_pair_consumer_input(m,input);
  };
  const auto unqualified_pair_service=[&](const std::string&why){
    if(stats)++stats->native_pair_consumer_unqualified_services;
    throw std::runtime_error(why);
  };
  NativeTu15ArrayAllocatorServices pair_heap;
  pair_heap.allocate_owner=[&](const auto&request,auto&)->std::uint32_t{
    if(request.owner!=0x233250u)
      unqualified_pair_service("Native TU1.5 pair storage requires qualified pool allocation owner="+std::to_string(request.owner));
    NativeTu15PoolMemory pool(m,request.receiver,observe);
    const auto data=native_tu15_pool_allocate(pool,request.bytes);
    if(stats){++stats->native_pair_consumer_pool_allocations;++stats->native_array_pool_allocations;}
    return data;
  };
  pair_heap.release_owner=[&](const auto&request,auto&context){
    if(request.owner==0x232d20u)
      native_tu15_observe_pair_pool_release_input(m,{request.receiver,request.owner,
        request.data,request.bytes,request.memory_class,context,state.x87_depth});
    if(request.owner==0x232d20u){
      // Authenticated main-heap ledger lifetime is distinct from233420 pool
      // release. The sole helper refuses unknown virtual+C before any CPU.
      native_tu15_release_bound_main_heap_allocation(m,request,context,state.x87_depth,observe,
        stats?&stats->main_heap_release:nullptr);
      return;
    }
    if(request.owner!=0x233420u)
      unqualified_pair_service("Native TU1.5 pair storage requires qualified pool release owner="+std::to_string(request.owner));
    NativeTu15PoolMemory pool(m,request.receiver,observe);
    if(!native_tu15_pool_release(pool,request.data,request.bytes))
      unqualified_pair_service("Native TU1.5 pair storage requires qualified large pool release bytes="+std::to_string(request.bytes));
    if(stats){++stats->native_pair_consumer_pool_releases;++stats->native_array_pool_releases;}
  };
  pair_heap.default_heap=[&](auto&)->std::uint32_t{
    unqualified_pair_service("Native TU1.5 pair storage requires qualified lazy default heap149930");return 0;
  };
  NativeTu15ArrayGrowthServices pair_storage;
  pair_storage.allocate=[&](const auto&request,auto&context)->std::uint32_t{
    if(request.owner!=0x149da0u)
      unqualified_pair_service("Native TU1.5 pair storage requires qualified allocation owner="+std::to_string(request.owner));
    return native_tu15_route_array_allocation(array_allocator,request.bytes,request.memory_class,context,pair_heap);
  };
  pair_storage.release=[&](const auto&request,auto&context){
    if(request.owner!=0x149e00u)
      unqualified_pair_service("Native TU1.5 pair storage requires qualified release owner="+std::to_string(request.owner));
    native_tu15_route_array_release(array_allocator,request.data,request.bytes,request.memory_class,context,pair_heap);
  };
  broad_phase.sweep=[&](auto dispatcher,auto owner,auto entries,auto bounds,auto count,auto first,auto second,auto&context){
    if(owner.identity==0x2ea380u) {
      native_tu15_update_bound_broad_phase_edges(m,{dispatcher.identity,entries,bounds,count,
        {first.identity},{second.identity}},context,pair_storage,observe,
        stats?&stats->broad_phase_edges:nullptr);
    } else {
      const std::array args{entries,bounds,count,first.identity,second.identity};
      call(owner.identity,args,dispatcher.identity,20);
    }
  };
  // Broad-phase and contact-buffer slow paths use the same qualified arena
  // kernel as pair consumption, with their own activity and strict backing.
  // Keep pair-input observation and pair-specific counters scoped to pairs.
  const auto unqualified_arena_service=[&](const std::string&why){
    if(stats)++stats->native_stack_arena_refusals;
    throw std::runtime_error(why);
  };
  NativeTu15ArrayAllocatorServices arena_heap;
  arena_heap.allocate_owner=[&](const auto&request,auto&context)->std::uint32_t{
    if(request.owner!=0x233250u)
      unqualified_arena_service("Native TU1.5 arena requires qualified pool allocation owner="+std::to_string(request.owner));
    return heap_services.allocate_owner(request,context);
  };
  arena_heap.release_owner=[&](const auto&request,auto&context){
    if(request.owner!=0x233420u||std::bit_cast<std::int32_t>(request.bytes)>0x2000)
      unqualified_arena_service("Native TU1.5 arena requires qualified pool release owner/size="+
        std::to_string(request.owner)+"/"+std::to_string(request.bytes));
    heap_services.release_owner(request,context);
  };
  arena_heap.default_heap=[&](auto&)->std::uint32_t{
    unqualified_arena_service("Native TU1.5 arena requires qualified lazy default heap149930");return 0;
  };
  NativeTu15ArrayGrowthServices arena_storage;
  arena_storage.allocate=[&](const auto&request,auto&context)->std::uint32_t{
    if(request.owner!=0x149da0u)
      unqualified_arena_service("Native TU1.5 arena requires qualified allocation owner="+std::to_string(request.owner));
    return native_tu15_route_array_allocation(array_allocator,request.bytes,request.memory_class,context,arena_heap);
  };
  arena_storage.release=[&](const auto&request,auto&context){
    if(request.owner!=0x149e00u)
      unqualified_arena_service("Native TU1.5 arena requires qualified release owner="+std::to_string(request.owner));
    native_tu15_route_array_release(array_allocator,request.data,request.bytes,request.memory_class,context,arena_heap);
  };
  const auto allocate_arena=[&](auto allocator,auto owner,auto bytes,auto&context)->std::uint32_t{
    if(owner.identity!=0x2e10a0u)
      unqualified_arena_service("Native TU1.5 arena requires qualified slow allocation owner="+std::to_string(owner.identity));
    const auto data=native_tu15_allocate_stack_arena(m,allocator.identity,bytes,context,arena_storage,observe);
    if(stats)++stats->native_stack_arena_allocations;return data;
  };
  const auto release_arena=[&](auto allocator,auto owner,auto data,auto bytes,auto&context){
    if(owner.identity!=0x2e1110u)
      unqualified_arena_service("Native TU1.5 arena requires qualified slow release owner="+std::to_string(owner.identity));
    native_tu15_release_stack_arena(m,allocator.identity,data,bytes,context,arena_storage,observe);
    if(stats)++stats->native_stack_arena_releases;
  };
  NativeTu15AgentCleanupServices agent_cleanup;
  NativeTu15AgentCleanupChildrenServices agent_children;
  agent_children.allocation=arena_storage;
  NativeTu15ContactManagerCleanupServices manager_cleanup;
  manager_cleanup.collector.allocation=arena_storage;
  manager_cleanup.collector.destroy_body=[&](auto body,auto owner,auto disposing,auto&){
    if(stats)++stats->native_contact_manager_service_residuals;
    const std::array args{disposing};call(owner.identity,args,body.identity,4);
  };
  manager_cleanup.collector.release_base_lock=[&](auto object,auto flags,auto&){
    if(flags)throw std::runtime_error("Native TU1.5 contact cleanup requires the original base-lock release flag");
    native_tu15_release_unregistered_base_lock(m,object,observe);
    if(stats)++stats->native_contact_manager_base_lock_releases;
  };
  manager_cleanup.update_world=[&](auto current_world,auto result,auto collector,auto&){
    if(stats)++stats->native_contact_manager_service_residuals;
    const std::array args{result,collector};call(0x316aa0u,args,current_world,8);
  };
  agent_children.remove_point=[&](auto manager,auto owner,auto point,auto&context){
    if(owner.identity==0x2e4d60u)return; // Authentic RET4, used by declared models.
    if(owner.identity==0x322f70u){
      native_tu15_observe_contact_point_removal_input(m,{manager,owner.identity,point,context,state.x87_depth});
      NativeTu15ContactPointBindingActivity activity;
      const auto collect_point=[&]{if(stats){
          stats->native_contact_point_attempts+=activity.attempts;
          stats->native_contact_point_calls+=activity.completed;
          stats->native_contact_point_refusals+=activity.refusals;
          stats->native_contact_point_profile_refusals+=activity.unsupported_profiles;
          stats->native_contact_point_unqualified_services+=activity.unqualified_services;
          stats->native_contact_point_noop_listeners+=activity.noop_listeners;
          stats->native_contact_point_gameplay_listeners+=activity.gameplay_listeners;
          stats->native_contact_point_gameplay_rows+=activity.gameplay.rows;
          stats->native_contact_point_metadata_calls+=activity.collector_updates+activity.detach.native_metadata_removals;
          stats->native_contact_point_world_updates+=activity.world.completed;
          stats->native_contact_point_world_queued_updates+=activity.world.queued;
          stats->native_contact_point_island_detaches+=activity.detach.calls;
          stats->native_contact_point_info_queries+=activity.detach.native_info_queries;
          stats->native_contact_point_reference_releases+=activity.detach.reference_releases;
          stats->native_contact_point_effect_requests+=activity.gameplay.constraint_releases;
          stats->native_contact_point_constraint_reference_calls+=activity.reference.calls;
          stats->native_contact_point_constraint_reference_decrements+=activity.reference.decrements;
          stats->native_contact_point_constraint_reference_runtime_removals+=activity.reference.runtime_removals;
          stats->native_contact_point_constraint_reference_runtime_refusals+=activity.reference.unbound_runtime_removals;
          stats->native_contact_point_part_requests+=activity.gameplay.part_releases;
          stats->native_contact_point_part_calls+=activity.part.attempts;
          stats->native_contact_point_part_completions+=activity.part.completed;
          stats->native_contact_point_part_refusals+=activity.part.refusals;
          stats->native_contact_point_part_inactive+=activity.part.response.inactive;
          stats->native_contact_point_part_material_queries+=activity.part.response.conveyor.material_queries;
          stats->native_contact_point_part_response_vectors+=activity.part.response.response_vectors;
          stats->native_contact_point_part_zero_properties+=activity.part.response.zero_properties;
          stats->native_contact_point_part_vector_properties+=activity.part.response.vector_properties;
          stats->native_contact_point_part_conveyor_bindings+=activity.part.response.conveyor.bindings;
          stats->native_contact_point_part_function_calls+=activity.part.function.calls;
          stats->native_contact_point_part_function_refusals+=activity.part.function.unsupported_kinds+activity.part.function.unsupported_power_profiles+activity.part.function.curves.unsupported_types+activity.part.function.curves.unsupported_inputs;
          stats->native_contact_point_part_function_fp_refusals+=activity.part.function.unsupported_fp_profiles+activity.part.function.curves.unsupported_fp_profiles;
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
    }
    if(stats)++stats->native_contact_agent_point_residuals;
    const std::array args{std::uint32_t(point)};call(owner.identity,args,manager,4);
  };
  std::function<void(std::uint32_t,std::uint32_t,NativeTu15PhysicsFrameFp&)>invalidate_agent;
  agent_cleanup.cleanup_child=[&](auto agent,auto owner,auto&context){
    invalidate_agent(agent.identity,owner.identity,context);
  };
  agent_cleanup.dispose=[&](auto object,auto owner,auto disposing,auto&context){
    if(owner.identity==0x2e4d60u)return; // Authentic RET4, no supplied result.
    if(owner.identity==0x3232b0u){
      native_tu15_observe_contact_manager_cleanup_input(m,{object,disposing,context,state.x87_depth});
      const auto collector=object+0x10u;
      // Admission is limited to source-qualified borrowed/native-pool owned array profiles.
      // Unsupported branches remain visible refusals before native stores.
      const auto native_owned_header=[&](std::uint32_t offset,std::uint32_t stride){
        const auto capacity=word(m,collector+offset);
        return (capacity&0x80000000u)!=0||
          std::uint64_t(capacity&0x7fffffffu)*stride<=0x2000u;
      };
      // The current strict149E00 router verifies each actual release owner at
      // its source phase. No pointer, capacity or allocator outcome is cached.
      const auto supported=word(m,object+0xa4u)==0&&
        native_owned_header(0x98u,28u)&&native_owned_header(0x3cu,32u)&&
        native_owned_header(0x28u,1u)&&
        (!(word(m,collector+4u,2)&0x8000u)||!native_tu15_base_lock_registered(m,collector));
      if(!supported){if(stats)++stats->native_contact_manager_cleanup_refusals;
        throw std::runtime_error("Native TU1.5 contact manager cleanup requires a qualified storage/current-world profile");}
      const std::array bodies{word(m,collector+0xcu),word(m,collector+0x10u)};
      std::array<std::uint32_t,2> worlds{};
      for(unsigned index=0;index<bodies.size();++index){
        const auto body=bodies[index];
        const auto refs=body?word(m,body+6u,2):0u;
        if(body)worlds[index]=word(m,body+8u);
        bool member=false;
        if(body&&worlds[index]&&refs>0&&refs<0xffffu){
          const auto count=word(m,body+0x8cu),entries=word(m,body+0x88u);
          std::uint32_t matches{};
          for(std::uint32_t n=0;std::int32_t(n)<std::int32_t(count);++n)
            if(word(m,entries+n*4u)==collector)++matches;
          member=matches>=(bodies[0]==bodies[1]?2u:1u);
        }
        if(!member){if(stats)++stats->native_contact_manager_cleanup_refusals;
          throw std::runtime_error("Native TU1.5 contact manager cleanup requires qualified retained bodies/listener membership");}
      }
      for(const auto current_world:worlds){
        auto count=word(m,current_world+0x118u),capacity=word(m,current_world+0x11cu)&0x7fffffffu;
        const auto needed=worlds[0]==worlds[1]?2u:1u;
        // Original316DC0 grows a full deferred-reference array through
        // 2E3270 before appending. The native collector already owns that
        // growth and its real allocator. Qualify its bounded pool requests,
        // rather than rejecting every initially full (including empty) list.
        bool qualified=count<=capacity;
        for(unsigned n=0;qualified&&n<needed;++n) {
          if(count==capacity) {
            const auto next=count?std::uint64_t(count)*2u:1u;
            qualified=next*4u<=0x2000u;
            if(qualified)capacity=std::uint32_t(next);
          }
          ++count;
        }
        if(!qualified){if(stats)++stats->native_contact_manager_cleanup_refusals;
          throw std::runtime_error("Native TU1.5 contact manager cleanup requires qualified deferred-list growth");}
      }
      NativeTu15ActivationContactManagerStats activity;
      try{(void)native_tu15_delete_contact_manager(m,object,disposing,
        {std::uint32_t(end)-0x1a0u},context,manager_cleanup,observe,&activity);}
      catch(...){if(stats){++stats->native_contact_manager_cleanup_refusals;stats->native_contact_manager_deferred_growths+=activity.array_growths;}throw;}
      if(stats){++stats->native_contact_manager_cleanups;
        stats->native_contact_manager_deferred_growths+=activity.array_growths;
        stats->native_contact_collector_cleanups+=activity.collector_destructions;
        stats->native_contact_listener_unregistrations+=activity.unregistrations;
        stats->native_contact_listener_deferred_references+=activity.deferred_references;}
      return;
    }
    if(owner.identity==0x2f81b0u){
      // The same native-only guard applies even outside pair deletion. Any
      // deeper unconverted allocator owner refuses before legacy transport.
      (void)native_tu15_delete_bound_reversed_list_contact_agent(m,{object},disposing,
        context,agent_children,pair_deletion_scope,observe,
        stats?&stats->reversed_list_agent_destructor:nullptr);
      if(stats)++stats->native_contact_agent_native_destructors;return;
    }
    if(owner.identity==0x2f8090u||owner.identity==0x3051a0u||owner.identity==0x2fc540u||owner.identity==0x2f35f0u){
      const auto kind=owner.identity==0x2f8090u?NativeTu15AgentDestructor::owner_2f8090:
        owner.identity==0x3051a0u?NativeTu15AgentDestructor::owner_3051a0:
        owner.identity==0x2f35f0u?NativeTu15AgentDestructor::owner_2f35f0:NativeTu15AgentDestructor::owner_2fc540;
      (void)native_tu15_destroy_contact_agent(m,kind,{object},disposing,context,agent_children,observe);
      if(stats)++stats->native_contact_agent_native_destructors;return;
    }
    if(stats)++stats->native_contact_agent_destructor_residuals;
    const std::array args{disposing};call(owner.identity,args,object,4);
  };
  agent_children.agents=agent_cleanup;
  NativeTu15PhantomCallbackCleanupServices phantom_cleanup;
  phantom_cleanup.agents=agent_cleanup;
  phantom_cleanup.leave=[&](auto shape,auto owner,auto self,auto other,auto&context){
    if(owner.identity!=0x1f3550u)
      unqualified_pair_service("Native phantom cleanup requires qualified shape leave owner="+
        std::to_string(owner.identity));
    native_tu15_leave_gameplay_phantom(m,shape.identity,self.identity,other.identity,
      context,pair_storage,observe);
  };
  invalidate_agent=[&](auto receiver,auto owner,auto&context){
    if(owner==0x31d400u)return; // Authentic RET0; no result/postimage.
    if(owner==0x2f0da0u||owner==0x2fbb10u||owner==0x2f9960u||owner==0x2f7790u){
      NativeTu15PairDeletionNativeScopeGuard guard(pair_deletion_scope,
        stats?&stats->pair_deletion_scope:nullptr);
      try{
        if(owner==0x2f0da0u){
          native_tu15_cleanup_bv_contact_agent(m,{receiver},context,agent_children,observe);
          if(stats)++stats->native_contact_agent_wrapped_cleanups;
        }else if(owner==0x2f9960u){
          native_tu15_cleanup_sphere_contact_agent(m,{receiver},context,agent_children,observe);
          if(stats)++stats->native_contact_agent_cached_cleanups;
        }else if(owner==0x2fbb10u){
          native_tu15_cleanup_single_point_contact_agent(m,{receiver},context,agent_children);
          if(stats)++stats->native_contact_agent_cached_cleanups;
        }else{
          native_tu15_cleanup_phantom_callback_agent(m,{receiver},context,phantom_cleanup);
        }
      }catch(...){if(stats)++stats->native_contact_agent_cleanup_refusals;throw;}
      return;
    }
    if(owner==0x2f3100u||owner==0x2f7a20u||owner==0x323220u||owner==0x2f82d0u||owner==0x2f6c80u){
      try{
        if(owner==0x323220u){
          native_tu15_dispose_contact_reference(m,receiver,context,agent_cleanup);
          if(stats)++stats->native_contact_reference_disposals;
        }else if(owner==0x2f82d0u){
          native_tu15_cleanup_cached_contact_agent(m,{receiver},context,agent_children,observe);
          if(stats)++stats->native_contact_agent_cached_cleanups;
        }else if(owner==0x2f6c80u){
          native_tu15_cleanup_wrapped_contact_agent(m,{receiver},context,agent_children);
          if(stats)++stats->native_contact_agent_wrapped_cleanups;
        }else{
          const auto kind=owner==0x2f3100u?NativeTu15AgentCleanupKind::records_2f3100:
            NativeTu15AgentCleanupKind::pointers_2f7a20;
          native_tu15_cleanup_contact_agent(m,kind,{receiver},context,agent_cleanup);
          if(stats){++stats->native_contact_agent_cleanup_calls;
            if(owner==0x2f3100u)++stats->native_contact_agent_record_cleanups;
            else ++stats->native_contact_agent_pointer_cleanups;}
        }
      }catch(...){if(stats)++stats->native_contact_agent_cleanup_refusals;throw;}
      return;
    }
    if(stats)++stats->native_contact_agent_child_residuals;
    call(owner,{},receiver,0);
  };
  NativeTu15IslandLifetimeServices merge_lifetime;
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
       target!=0x2f82d0u&&target!=0x2f6c80u&&target!=0x2f0da0u&&target!=0x2fbb10u&&target!=0x2f9960u&&target!=0x2f7790u)
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
  broad_phase.allocate_block=allocate_arena;
  broad_phase.release_block=release_arena;
  broad_phase.release_owned_pairs=[&](auto allocator,auto owner,auto data,auto bytes,auto memory_class,auto&context){
    array_services.release({allocator.identity,owner.identity,data,bytes,memory_class},context);
  };
  pairs.arena.allocate_block=[&](auto allocator,auto owner,auto bytes,auto&context)->std::uint32_t{
    observe_pair_service(NativeTu15PairConsumerInputPhase::SlowArenaAllocation,allocator,owner,0,bytes,0,context);
    if(owner.identity!=0x2e10a0u)
      unqualified_pair_service("Native TU1.5 pair consumer requires qualified slow arena allocation owner="+std::to_string(owner.identity));
    const auto data=native_tu15_allocate_stack_arena(m,allocator.identity,bytes,context,pair_storage,observe);
    if(stats)++stats->native_pair_consumer_arena_allocations;return data;
  };
  pairs.arena.release_block=[&](auto allocator,auto owner,auto data,auto bytes,auto&context){
    observe_pair_service(NativeTu15PairConsumerInputPhase::SlowArenaRelease,allocator,owner,data,bytes,0,context);
    if(owner.identity!=0x2e1110u)
      unqualified_pair_service("Native TU1.5 pair consumer requires qualified slow arena release owner="+std::to_string(owner.identity));
    native_tu15_release_stack_arena(m,allocator.identity,data,bytes,context,pair_storage,observe);
    if(stats)++stats->native_pair_consumer_arena_releases;
  };
  pairs.arena.release_owned_pairs=[&](auto allocator,auto owner,auto data,auto bytes,auto tag,auto&context){
    observe_pair_service(NativeTu15PairConsumerInputPhase::OwnedRelease,allocator,owner,data,bytes,tag,context);
    pair_storage.release({allocator.identity,owner.identity,data,bytes,tag},context);
    if(stats)++stats->native_pair_consumer_owned_releases;
  };
  pairs.allocate_owned_records=[&](auto allocator,auto owner,auto bytes,auto tag,auto&context)->std::uint32_t{
    observe_pair_service(NativeTu15PairConsumerInputPhase::OwnedAllocation,allocator,owner,0,bytes,tag,context);
    const auto data=pair_storage.allocate({allocator.identity,owner.identity,bytes,tag},context);
    if(stats)++stats->native_pair_consumer_owned_allocations;return data;
  };
  pairs.filter_pair=[&](auto filter,auto owner,auto decision,auto first,auto second,auto&){
    if(owner.identity!=0x2e9a00u)
      unqualified_pair_service("Native TU1.5 pair consumer requires qualified filter owner");
    if(stats)++stats->native_pair_consumer_filter_calls;
    return native_tu15_activation_group_filter(m,filter,decision,{first},{second},observe);
  };
  pairs.create_pair=[&](auto handler,auto owner,auto pair,auto&context){
    if(owner.identity==0x31ffc0u){
      if(stats){++stats->native_pair_consumer_create_calls;++stats->native_pair_creation_attempts;}
      NativeTu15PhantomPairCreateServices phantom;
      phantom.add_overlap=[&](auto receiver,auto child,auto other,auto flag_word,auto&current){
        if(stats)++stats->native_phantom_pair_create_notifications;
        NativeTu15PhantomOverlapAddServices overlap;overlap.allocation=pair_storage;
        overlap.added=[&](auto listener,auto target,auto event,auto&current_fp){
          if(target.identity==0x2e4d60u){native_tu15_ignore_removed_phantom_overlap();return;}
          if(target.identity==0x282700u){
            NativeTu15PhantomAddedBindingActivity activity;
            NativeTu15ObjectDeletionSpatialBindingActivity deletion_activity;
            NativeTu15PhantomAddedListenerServices deletion;
            deletion.delete_object=[&](auto datum,auto&deletion_fp){
              native_tu15_delete_spatially_bound_object(m,datum,deletion_fp,observe,&deletion_activity);
            };
            const auto collect_added=[&]{if(stats){
              stats->native_phantom_added_listener_attempts+=activity.attempts;
              stats->native_phantom_added_listener_calls+=activity.completed;
              stats->native_phantom_added_listener_refusals+=activity.refusals;
              stats->native_phantom_added_components_marked+=activity.listener.marked_components;
              stats->native_phantom_added_protection_queries+=activity.listener.protection.queries;
              stats->native_phantom_added_objects_protected+=activity.listener.protected_objects;
              stats->native_phantom_added_delete_requests+=activity.listener.delete_requests;
              stats->native_phantom_added_unqualified_deletions+=activity.unqualified_deletions;
              stats->native_phantom_object_deletion_attempts+=deletion_activity.attempts;
              stats->native_phantom_object_deletion_calls+=deletion_activity.completed;
              stats->native_phantom_object_deletion_refusals+=deletion_activity.refusals;
              stats->native_phantom_object_deletion_unbound_services+=deletion_activity.deletion.unbound_services+deletion_activity.detachment.unbound_services;
              stats->native_phantom_object_detachment_calls+=deletion_activity.detachment.calls;
              stats->native_phantom_object_detachment_objects+=deletion_activity.detachment.objects;
              stats->native_phantom_object_detachment_refusals+=deletion_activity.detachment.unbound_services;
              stats->native_phantom_object_spatial_queries+=deletion_activity.deletion.spatial_queries;
              stats->native_phantom_object_tracking_clears+=deletion_activity.deletion.tracking_clears;
              stats->native_phantom_object_player_clears+=deletion_activity.deletion.player_clears;
              stats->native_phantom_object_schedule_updates+=deletion_activity.deletion.schedule_updates;
              stats->native_phantom_object_reference_clears+=deletion_activity.deletion.reference_clears;
              stats->native_phantom_object_spatial_flag_queries+=deletion_activity.spatial.flag_queries;
              stats->native_phantom_object_spatial_packets+=deletion_activity.spatial.packets;
              stats->native_phantom_object_spatial_chain_updates+=deletion_activity.spatial.chain_updates;
              stats->native_phantom_object_spatial_chain_rows+=deletion_activity.spatial.chain_rows;
              stats->native_phantom_object_spatial_record_searches+=deletion_activity.spatial.record_searches;
              stats->native_phantom_object_spatial_searched_rows+=deletion_activity.spatial.searched_rows;
              stats->native_phantom_object_spatial_matched_rows+=deletion_activity.spatial.matched_rows;
              stats->native_phantom_object_spatial_missing_rows+=deletion_activity.spatial.missing_rows;
              stats->native_phantom_object_spatial_word_copies+=deletion_activity.spatial.word_copies;
              stats->native_phantom_object_spatial_byte_copies+=deletion_activity.spatial.byte_copies;
              stats->native_phantom_object_spatial_zero_copies+=deletion_activity.spatial.zero_copies;
            }};
            try{native_tu15_notify_bound_gameplay_phantom_added(m,listener,event,
              current_fp,state.x87_depth,deletion,observe,&activity);collect_added();}
            catch(...){collect_added();throw;}
            return;
          }
          if(stats)++stats->native_phantom_pair_create_unqualified_owners;
          unqualified_pair_service("Native TU1.5 phantom addition requires qualified added listener owner="+
            std::to_string(target.identity)+" receiver="+std::to_string(listener)+
            " event="+std::to_string(event.identity));
        };
        const NativeTu15PhantomOverlapAddedEvent event{std::uint32_t(end)-0xa0u};
        if(child.identity==0x315b00u){
          native_tu15_add_phantom_overlap(m,receiver,other,flag_word,event,current,overlap,observe);if(stats)++stats->native_phantom_pair_create_complex_adds;return;
        }
        if(child.identity==0x315300u){
          native_tu15_add_simple_phantom_overlap(m,receiver,other,flag_word,event,current,overlap,observe);if(stats)++stats->native_phantom_pair_create_simple_adds;return;
        }
        if(stats)++stats->native_phantom_pair_create_unqualified_owners;
        unqualified_pair_service("Native TU1.5 phantom pair requires qualified overlap addition owner="+
          std::to_string(child.identity)+" receiver="+std::to_string(receiver)+
          " other="+std::to_string(other.identity)+" pair="+std::to_string(pair));
      };
      try{native_tu15_create_phantom_pair(m,pair,context,phantom);
        if(stats)++stats->native_pair_creation_calls;
      }catch(...){if(stats)++stats->native_pair_creation_refusals;throw;}
      return;
    }
    if(owner.identity!=0x320ef0u)
      unqualified_pair_service("Native TU1.5 pair consumer requires qualified create owner="+std::to_string(owner.identity));
    if(stats)++stats->native_pair_consumer_create_calls;
    if(stats)++stats->native_pair_creation_attempts;
    try{
    check((end&15u)==0,"Native TU1.5 convex factory requires aligned caller-owned DATA");
    NativeTu15PairHandlerServices create;
    // Remaining services below execute genuine current work, never postimages.
    create.activate_island=[&](auto w,auto island,auto&current){
      // This call precedes all pair factory DATA. The low scratch segment is
      // disjoint from the merge segment, high pair packets and lifecycle DATA.
      const auto wake_data=(scratch.first+0x1000u+15u)&~15u;
      check(scratch.bytes>=0x5000u&&std::uint64_t(wake_data)+0x880u<=end-0x2800u,
        "Native TU1.5 pair wake requires distinct caller-owned retained DATA");
      try{
        NativeTu15ObjectActivationNativeServices native_factories;
        native_factories.create_agent=[&](auto owner,auto first,auto second,auto settings,
            auto manager,auto&context,unsigned depth){
          check(depth==state.x87_depth,"Native TU1.5 wake factory lost actual x87 depth");
          return create.agents.create_agent(owner,first,second,settings,manager,context);
        };
        native_tu15_activate_pair_island_with_native_factories(m,w,island,current,state.x87_depth,
          {wake_data+0x20u,wake_data+0x24u,wake_data+0x40u,wake_data+0x80u,0x800u},native_factories,
          observe,stats?&stats->pair_island_wake:nullptr,stats?&stats->world_activation:nullptr);
        if(stats)++stats->native_world_activation_calls;
      }catch(...){if(stats)++stats->native_world_activation_refusals;throw;}
    };
    create.defer_pair=[&](auto island,auto input,auto&current){native_tu15_defer_bound_collision_pair(m,island.identity,input,current,pair_storage,observe,stats?&stats->pair_creation_queues:nullptr);};
    create.merge_bodies=[&](auto w,auto first,auto second,auto&current){native_tu15_queue_bound_body_pair_merge(m,w.identity,first.identity,second.identity,current,pair_storage,observe,stats?&stats->pair_creation_queues:nullptr);};
    create.agents.create_manager=[&](auto factory,auto target,auto first,auto second,auto input,auto&current){
      (void)input; // Original3232E0 ignores its third formal.
      return native_tu15_create_bound_activation_contact_manager(m,factory,target,first,second,
        current,pair_storage,observe,stats?&stats->pair_manager_factory:nullptr);
    };
    create.agents.classify_shape=[&](auto,auto target,auto&){
      if(target.identity==0x308540u)return native_tu15_phantom_shape_class_22();
      if(target.identity==0xa9520u)return native_tu15_contact_child_class_23();
      if(target.identity==0xab4b0u)return native_tu15_contact_child_class_24();
      if(const auto kind=native_tu15_activation_type(target.identity))return *kind;
      unqualified_pair_service("Native TU1.5 pair create requires qualified shape class owner="+
        std::to_string(target.identity));
      return std::uint32_t{}; // The required-owner refusal above never returns.
    };
    unsigned factory_depth{};
    create.agents.create_agent=[&](auto target,auto first,auto second,auto settings,auto manager,auto&current){
      native_tu15_observe_contact_agent_factory_input(m,{target.identity,first.identity,second.identity,
        settings.identity,manager.identity,current,state.x87_depth});
      if(target.identity==0x2f78c0u){
        NativeTu15PhantomCallbackAgentServices callback;callback.allocation=pair_storage;
        callback.classify_shape=create.agents.classify_shape;
        callback.enter=[&](auto shape,auto owner,auto self,auto other,auto context,auto&context_fp){
          if(owner.identity!=0x1f34d0u)
            unqualified_pair_service("Native phantom factory requires qualified shape enter owner="+
              std::to_string(owner.identity));
          native_tu15_enter_gameplay_phantom(m,shape.identity,self.identity,other.identity,
            context.identity,context_fp,pair_storage,observe);
        };
        const NativeTu15PhantomCallbackAgentArguments arguments{
          [&]{return first.identity;},[&]{return second.identity;},
          [&]{return settings.identity;},[&]{return manager.identity;}};
        return native_tu15_create_phantom_callback_agent(m,arguments,current,callback,observe);
      }
      if(target.identity==0x2f3670u||target.identity==0x2f5530u){
        return native_tu15_create_contact_agent(m,target.identity==0x2f3670u?
          NativeTu15ContactAgentFactory::owner_2f3670:NativeTu15ContactAgentFactory::owner_2f5530,
          NativeTu15ContactAgentFactoryArguments{[&]{return manager.identity;}},current,pair_storage,observe);
      }
      if(target.identity==0x2f9990u){
        return native_tu15_create_contact_agent(m,NativeTu15ContactAgentFactory::owner_2f9990,
          NativeTu15ContactAgentFactoryArguments{[&]{return manager.identity;}},current,pair_storage,observe);
      }
      if(target.identity==0x303c10u||target.identity==0x3040d0u){
        return native_tu15_create_contact_agent(m,target.identity==0x303c10u?
          NativeTu15ContactAgentFactory::owner_303c10:NativeTu15ContactAgentFactory::owner_3040d0,
          NativeTu15ContactAgentFactoryArguments{[&]{return manager.identity;}},current,pair_storage,observe);
      }
      if(target.identity==0x2f8030u||target.identity==0x2f81f0u){
        if(stats)++stats->native_list_agent_factory_attempts;
        try{
        // Each nested factory retains separate child DATA. Genuine native
        // pool/class/convex owners execute while it is live; a missing owner
        // refuses before any residual can reuse this retained storage.
        const auto required=0x900ull+512ull*factory_depth;
        check(required+256u+0x1060u<=scratch.bytes,"Native TU1.5 list factory exhausted caller-owned child DATA");
        const auto retained=std::uint32_t(end-required);
        put(m,retained+16u,settings.identity); // Ordinary context argument DATA, whose low byte is the filter output.
        NativeTu15ListContactServices list;list.allocation=pair_storage;
        list.dispatch.classify_shape=create.agents.classify_shape;
        list.dispatch.create_agent=[&](auto child,auto a,auto b,auto context,auto data,auto&context_fp){
          if(stats)++stats->native_list_agent_child_factories;
          const auto result=create.agents.create_agent(child,a,b,context,NativeTu15ActivationContactManager{data},context_fp);
          if(stats)++stats->native_list_agent_child_returns;return result;
        };
        list.child_count=[&](auto collection,auto owner,auto&){
          if(owner.identity!=0x3061f0u)unqualified_pair_service("Native TU1.5 list factory requires qualified child-count owner");
          if(stats)++stats->native_list_agent_count_queries;
          return native_tu15_collection_child_count(m,collection);
        };
        list.filter_child=[&](auto receiver,auto owner,auto output,auto,auto b,auto a,auto collection,auto index,auto&context_fp){
          if(owner.identity!=0x1cbe70u)unqualified_pair_service("Native TU1.5 list factory requires qualified child-filter owner");
          // Complete1CBE70/185020/2E98D0 and both list leaves are independently
          // compared with unchanged i386 across every filter instruction site.
          // All remaining services here keep their actual native owners.
          if(stats)++stats->native_list_agent_filter_queries;
          NativeTu15CollectionFilterServices filter;
          filter.classify_shape=create.agents.classify_shape;
          filter.resolve_child=[&](auto shape,auto resolver,auto key,auto,auto&){
            if(resolver.identity!=0x306220u)unqualified_pair_service(
              "Native TU1.5 collection filter requires qualified child resolver owner="+std::to_string(resolver.identity));
            return native_tu15_collection_child(m,shape,key);
          };
          filter.motion_type=[&](auto,auto motion_owner,auto&){
            if(const auto kind=native_tu15_activation_type(motion_owner.identity))return *kind;
            unqualified_pair_service("Native TU1.5 collection filter requires qualified motion-type owner="+
              std::to_string(motion_owner.identity));return 0u;
          };
          return native_tu15_filter_collection_child(m,receiver,output,b,a,collection,index,
            {retained-256u},context_fp,filter,observe);
        };
        ++factory_depth;
        try{
          if(target.identity==0x2f81f0u){
            NativeTu15CollectionContactServices collection;
            collection.allocation=list.allocation;collection.dispatch=list.dispatch;
            collection.count_children=list.child_count;collection.filter_child=list.filter_child;
            const NativeTu15CollectionContactArguments arguments{
              [&]{return first.identity;},[&]{return second.identity;},
              [&]{return settings.identity;},[&]{return manager.identity;}};
            const auto result=native_tu15_create_collection_contact_agent(m,arguments,
              {retained,retained+16u},current,collection,observe);
            --factory_depth;if(stats)++stats->native_list_agent_factory_calls;return result;
          }
          const NativeTu15ListContactArguments arguments{
            [&]{return first.identity;},[&]{return second.identity;},
            [&]{return settings.identity;},[&]{return manager.identity;}};
          const auto result=native_tu15_create_list_contact_agent(m,arguments,{retained,retained+16u},current,list,observe);
          --factory_depth;if(stats)++stats->native_list_agent_factory_calls;return result;
        }catch(...){--factory_depth;throw;}
        }catch(...){if(stats)++stats->native_list_agent_factory_refusals;throw;}
      }
      if(target.identity==0x2f0eb0u||target.identity==0x2f1cc0u){
        NativeTu15WrappedFactoryBindingActivity activity;
        const auto collect_wrapped=[&]{if(stats){
          stats->native_wrapped_factory_attempts+=activity.attempts;
          stats->native_wrapped_factory_calls+=activity.completed;
          stats->native_wrapped_factory_refusals+=activity.refusals;
          stats->native_wrapped_factory_wrapped_entries+=activity.wrapped_calls;
          stats->native_wrapped_factory_transformed_entries+=activity.transformed_calls;
          stats->native_wrapped_factory_convex_entries+=activity.convex_calls;
          stats->native_wrapped_factory_class_queries+=activity.class_queries;
          stats->native_wrapped_factory_unqualified_owners+=activity.unqualified_owners;
        }};
        try{
          // The closed native constructor graph cannot re-enter the World
          // factory or execute a residual while this DATA is retained.
          const auto wrapped=native_tu15_world_factory_scratch(scratch,factory_depth);
          const NativeTu15WrappedContactArguments arguments{
            [&]{return first.identity;},[&]{return second.identity;},
            [&]{return settings.identity;},[&]{return manager.identity;}};
          const auto result=native_tu15_create_bound_wrapped_contact_factory(m,target,
            arguments,wrapped,current,state.x87_depth,
            pair_storage,observe,&activity);
          collect_wrapped();return result;
        }catch(...){collect_wrapped();
          try{throw;}catch(const std::exception&error){
            throw std::runtime_error(std::string(error.what())+" outer_factory="+std::to_string(target.identity)+
              " first="+std::to_string(first.identity)+" second="+std::to_string(second.identity)+
              " context="+std::to_string(settings.identity)+" manager="+std::to_string(manager.identity)+
              " factory_depth="+std::to_string(factory_depth)+" x87_depth="+std::to_string(state.x87_depth));
          }catch(...){throw;}
        }
      }
      if(target.identity==0x2f1cc0u||target.identity==0x2f7190u||target.identity==0x2f75f0u){
        if(target.identity==0x2f7190u&&stats)++stats->native_transformed_agent_factory_attempts;
        try{
        // Only the native pool/class/factory owners below can execute while
        // this DATA is live. Unknown child factories refuse before residual
        // execution; each recursive wrapper retains16 child-payload bytes
        // plus96 aligned copied/transformed geometry bytes in its own slot.
        const auto required=0x900ull+512ull*factory_depth;
        check(required+0x1060u<=scratch.bytes,"Native TU1.5 wrapped factory exhausted caller-owned child DATA");
        const NativeTu15WrappedContactScratch retained{std::uint32_t(end-required)};
        NativeTu15WrappedContactServices wrapper;wrapper.allocation=pair_storage;
        wrapper.dispatch.classify_shape=create.agents.classify_shape;
        wrapper.dispatch.create_agent=[&](auto child,auto a,auto b,auto context,auto data,auto&context_fp){
          return create.agents.create_agent(child,a,b,context,NativeTu15ActivationContactManager{data},context_fp);
        };
        ++factory_depth;
        try{
          const NativeTu15WrappedContactArguments arguments{
            [&]{return first.identity;},[&]{return second.identity;},
            [&]{return settings.identity;},[&]{return manager.identity;}};
          const auto result=target.identity==0x2f75f0u?
            native_tu15_create_reversed_transformed_contact_agent(m,arguments,
              {retained.child_payload,retained.child_payload-96u},current,wrapper,observe):
            target.identity==0x2f7190u?
            native_tu15_create_transformed_contact_agent(m,arguments,
              {retained.child_payload,retained.child_payload-96u},current,wrapper,observe):
            native_tu15_create_wrapped_contact_agent(m,arguments,retained,current,wrapper,observe);
          --factory_depth;if(target.identity==0x2f7190u&&stats)++stats->native_transformed_agent_factory_calls;return result;
        }catch(...){--factory_depth;throw;}
        }catch(...){if(target.identity==0x2f7190u&&stats)++stats->native_transformed_agent_factory_refusals;throw;}
      }
      if(target.identity!=0x2e4eb0u && target.identity!=0x2f83b0u)unqualified_pair_service("Native TU1.5 pair create requires qualified convex agent factory owner="+
        std::to_string(target.identity));
      const bool gsk=target.identity==0x2f83b0u;
      NativeTu15ConvexAgentBindingActivity activity;
      const auto accumulate=[&]{if(stats){
        if(gsk){stats->native_gsk_agent_attempts+=activity.attempts;
          stats->native_gsk_agent_calls+=activity.completed;stats->native_gsk_agent_refusals+=activity.refusals;}
        stats->native_convex_agent_attempts+=activity.attempts;
        stats->native_convex_agent_calls+=activity.completed;
        stats->native_convex_agent_refusals+=activity.refusals;
        stats->native_convex_agent_pool_allocations+=activity.pool_allocations;
        stats->native_convex_agent_pool_releases+=activity.pool_releases;
        stats->native_convex_agent_class_queries+=activity.class_queries;
        stats->native_convex_agent_centers+=activity.centers;
        stats->native_convex_agent_supports+=activity.supports;
        stats->native_convex_agent_encodings+=activity.encodings;
      }};
      try{
      const auto factory=gsk?native_tu15_create_bound_gsk_contact_agent:native_tu15_create_bound_convex_contact_agent;
      const auto result=factory(m,
        {[&]{return first.identity;},[&]{return second.identity;},[&]{return manager.identity;}},
        {std::uint32_t(end)-0x600u,std::uint32_t(end)-0x700u,
         std::uint32_t(end)-0x720u,std::uint32_t(end)-0x760u,
         std::uint32_t(end)-0x740u,std::uint32_t(end)-0x780u},current,state.x87_depth,observe,&activity);
      accumulate();return result;
      }catch(...){accumulate();throw;}
    };
    create.agents.motion_kind=[&](auto,auto target,auto&){
      if(const auto kind=native_tu15_activation_type(target.identity))return *kind;
      unqualified_pair_service("Native TU1.5 pair create requires qualified motion getter");return 0u;
    };
    create.agents.test_discrete_pair=[&](auto dispatcher,auto target,auto result,auto first,auto second,auto&current){
      if(target.identity!=0x281530u)unqualified_pair_service("Native TU1.5 pair create requires qualified discrete owner");
      return native_tu15_test_bound_discrete_pair(m,dispatcher,result,first,second,current,observe,
        stats?&stats->pair_discrete_query:nullptr);
    };
    create.agents.release_manager=[&](auto manager,auto target,auto&current){
      if(target.identity!=0x323220u)unqualified_pair_service("Native TU1.5 pair create requires qualified manager release");
      native_tu15_dispose_contact_reference(m,manager.identity,current,agent_cleanup);
      if(stats)++stats->native_contact_reference_disposals;
    };
    const auto formal=std::uint32_t(end)-0x1b0u; // Four source-owned DATA bytes.
    put(m,formal,pair);
    native_tu15_create_pair(m,handler,pair,{formal},context,create,observe);
    if(stats)++stats->native_pair_creation_calls;
    }catch(...){if(stats)++stats->native_pair_creation_refusals;throw;}
  };
  pairs.delete_pair=[&](auto handler,auto owner,auto pair,auto&context){
    if(owner.identity==0x320040u){
      if(stats){++stats->native_pair_consumer_delete_calls;++stats->native_pair_consumer_phantom_delete_calls;}
      NativeTu15PhantomPairDeleteServices phantom;
      phantom.remove_overlap=[&](auto receiver,auto child,auto other,auto&context){
        if(stats)++stats->native_pair_consumer_phantom_notifications;
        if(child.identity==0x2e4d60u){
          native_tu15_ignore_removed_phantom_overlap();
          if(stats)++stats->native_pair_consumer_phantom_noops;return;
        }
        if(child.identity==0x315a70u||child.identity==0x315100u){
          NativeTu15PhantomOverlapRemoveServices overlap;
          overlap.removed=[&](auto listener,auto owner,auto event,auto&){
            if(owner.identity==0x2e4d60u){
              native_tu15_ignore_removed_phantom_overlap();
              if(stats)++stats->native_pair_consumer_phantom_overlap_listener_noops;return;
            }
            if(stats)++stats->native_pair_consumer_phantom_unqualified_owners;
            unqualified_pair_service("Native TU1.5 phantom overlap requires qualified removed listener owner="+
              std::to_string(owner.identity)+" receiver="+std::to_string(listener)+
              " event="+std::to_string(event.identity));
          };
          if(child.identity==0x315a70u){
            native_tu15_remove_phantom_overlap(m,receiver,other,{std::uint32_t(end)-0xa0u},context,overlap,observe);
            if(stats)++stats->native_pair_consumer_phantom_overlap_remove_calls;
          }else{
            native_tu15_remove_simple_phantom_overlap(m,receiver,other,{std::uint32_t(end)-0xa0u},context,overlap,observe);
            if(stats)++stats->native_pair_consumer_simple_phantom_overlap_remove_calls;
          }
          return;
        }
        if(stats)++stats->native_pair_consumer_phantom_unqualified_owners;
        unqualified_pair_service("Native TU1.5 phantom pair requires qualified overlap removal owner="+
          std::to_string(child.identity)+" receiver="+std::to_string(receiver)+
          " other="+std::to_string(other.identity)+" pair="+std::to_string(pair));
      };
      native_tu15_delete_phantom_pair(m,pair,context,phantom);return;
    }
    if(owner.identity!=0x320260u)
      unqualified_pair_service("Native TU1.5 pair consumer requires qualified delete owner="+
        std::to_string(owner.identity)+" handler="+std::to_string(handler)+" pair="+std::to_string(pair));
    if(stats)++stats->native_pair_consumer_delete_calls;
    // Four bytes of ordinary formal DATA, outside all live pair/point packets.
    const auto formal=std::uint32_t(end)-0xa4u;put(m,formal,pair);
    NativeTu15PairDeletionBindingServices deletion;
    deletion.cleanup_agent=[&](auto agent,auto target,auto&current){invalidate_agent(agent.identity,target.identity,current);};
    deletion.release_manager=[&](auto manager,auto target,auto&current){invalidate_agent(manager.identity,target.identity,current);};
    native_tu15_delete_bound_pair_in_native_scope(m,handler,pair,{formal},context,deletion,
      pair_deletion_scope,observe,stats?&stats->pair_deletion:nullptr,stats?&stats->pair_deletion_scope:nullptr);
  };
  broad_phase.consume_pairs=[&](auto island,auto first,auto second,auto&context){
    if(stats){++stats->native_pair_consumer_attempts;
      if(first.count||second.count)++stats->native_pair_consumer_nonempty_calls;}
    // The header and merge aliases contain source DATA only. They live above
    // all genuine callback frames and apart from both broad-phase headers.
    const NativeTu15PairConsumerScratch retained{{std::uint32_t(end)-0x60u},
      {std::uint32_t(end)-0x80u,std::uint32_t(end)-0x7cu,
       std::uint32_t(end)-0x78u,std::uint32_t(end)-0x74u}};
    const auto previous_input=pair_input;pair_input={};
    pair_input.island=island;pair_input.first=first;pair_input.second=second;pair_input.scratch=retained;
    pair_input.floating_point=context;pair_input.preserved_x87_depth=state.x87_depth;
    try{
      (void)native_tu15_observe_pair_consumer_input(m,pair_input);
      native_tu15_consume_pairs(m,island,first,second,retained,context,pairs,observe);
    }catch(...){pair_input=previous_input;if(stats)++stats->native_pair_consumer_refusals;throw;}
    pair_input=previous_input;
    if(stats)++stats->native_pair_consumer_calls;
  };
  island_collision.first_phase=[&](auto island,auto dispatcher,auto configuration,auto&context){
    if(stats)++stats->native_broad_phase_calls;
    native_tu15_update_broad_phase(m,island,dispatcher,configuration,
      NativeTu15BroadPhaseScratch{{std::uint32_t(end)-0xc0u},{std::uint32_t(end)-0xe0u}},context,state.x87_depth,broad_phase,observe);
  };
  NativeTu15IslandAgentPhaseServices island_agents;
  island_agents.timestamp_counter=collision.timestamp_counter;
  island_agents.allocate_stack_block=allocate_arena;
  island_agents.release_stack_block=release_arena;
  island_agents.release_owned_contacts=[&](auto allocator,auto owner,auto data,auto bytes,auto memory_class,auto&context){
    array_services.release({allocator.identity,owner.identity,data,bytes,memory_class},context);
  };
  NativeTu15CompoundProcessServices remaining_contact_process;
  remaining_contact_process.process_child=[&](const auto&child,auto&){
    const std::array args{child.first.identity,child.second.identity,
      child.configuration.identity,child.output.identity};
    // All retained native descriptors and geometry stay above this genuine
    // child's arguments and downward-growing private control frame.
    call_at(std::uint32_t(end)-0x1200u,child.owner.identity,args,child.receiver.identity,16);
  };
  NativeTu15ContactProcessBinding native_contact_process(m,
    {std::uint32_t(end)-0x1000u,0x600u},remaining_contact_process,observe,
    stats?&stats->contact_process:nullptr);
  const auto agent_call=[&](auto agent,auto owner,auto first,auto second,auto configuration,auto output,auto&context){
    if(NativeTu15ContactProcessBinding::owns(owner)){
      check(scratch.bytes>=0x1600u,"Native TU1.5 contact process requires retained DATA above its genuine child frames");
      native_contact_process.process({agent,owner,{first.identity},second,configuration,output},context);
      return;
    }
    const std::array args{first.identity,second.identity,configuration.identity,output.identity};
    call(owner.identity,args,agent.identity,16);
  };
  island_agents.process_agent=agent_call;
  NativeTu15ContactCompletionServices completion;
  NativeTu15ContactEntityNotificationServices contact_entities;
  contact_entities.process_contacts=[&](auto event,auto component,auto own,auto other,auto own_slot,auto other_slot,auto&context){
    NativeTu15ContactEntityProcessorServices processor;
    processor.update_contact=[&](auto manager,auto row,bool reversed,auto datum,auto slot,auto&current){
      native_tu15_observe_contact_body_update_input(m,{manager,row,0u,reversed,datum,slot,current,state.x87_depth});
      // Distinct retained DATA: body wake must not overwrite the point-velocity
      // or impulse packets that remain live across its native listener graph.
      const auto body_data=(scratch.first+0x3000u+15u)&~15u;
      check(scratch.bytes>=0x6200u&&std::uint64_t(body_data)+0x920u<=end-0x2800u,
        "Native TU1.5 body contact requires distinct caller-owned retained DATA");
      NativeTu15ContactBodyUpdateNativeServices native;
      native.activate_body=[&](auto body,auto&current_fp,auto depth){
        if(stats)++stats->contact_body_wake_attempts;
        const NativeTu15ObjectActivationNativeServices factories;
        try{
          native_tu15_activate_body_with_native_services(m,body,current_fp,depth,
            {body_data+0x20u,body_data+0x24u,body_data+0x40u,body_data+0x80u,0x800u},factories,
            [&](auto owner,auto,auto,unsigned,auto&)->std::uint32_t{
              if(stats){++stats->contact_body_wake_unqualified_owners;++stats->contact_body_wake_refused_owners[owner];}
              throw std::runtime_error("Native TU1.5 body wake requires native owner="+std::to_string(owner));
            },observe,stats?&stats->world_activation:nullptr);
          if(stats){++stats->contact_body_wake_calls;++stats->native_world_activation_calls;}
        }catch(...){if(stats){++stats->contact_body_wake_refusals;++stats->native_world_activation_refusals;}throw;}
      };
      native_tu15_update_bound_contact_body(m,manager,row,false,reversed,datum,slot,
        {body_data+0x900u,body_data+0x910u},current,state.x87_depth,native,
        pair_deletion_scope,observe,stats?&stats->contact_body:nullptr);
    };
    native_tu15_process_entity_contacts(m,event,component,own,other,own_slot,other_slot,
      context,processor,stats?&stats->contact_processor:nullptr);
  };
  completion.notify=[&](auto listener,auto owner,auto event,auto&context){
    if(owner.identity==0x281e10u){
      native_tu15_notify_contact_entities(m,event,context,contact_entities,stats?&stats->contact_entities:nullptr);
      return;
    }
    const std::array args{event.identity};call(owner.identity,args,listener.identity,4);
  };
  island_agents.finish_agent=[&](auto agent,auto owner,auto first,auto second,auto configuration,auto output,auto&context){
    if(owner.identity==0x3230c0u){
      native_tu15_complete_contact_manager(m,agent,first,second,output,
        {std::uint32_t(end)-0x180u},context,completion,observe,stats?&stats->contact_completion:nullptr);
      return;
    }
    agent_call(agent,owner,first,second,configuration,output,context);
  };
  NativeTu15IslandContactAdvanceServices contact_advance;
  contact_advance.advance_motion=[&](auto motion,auto owner,auto time,auto configuration,auto&context){
    if(owner.identity==0x325810u){
      native_tu15_advance_cached_motion(m,motion,time,configuration.value,context,
        state.x87_depth,observe,{},stats?&stats->cached_motion:nullptr);
      return;
    }
    if(owner.identity==0x8fdb0u){
      // The same authenticated RET8 no-op is used for fixed motion here.
      // It has no guest register postimage or physics result to synthesize.
      native_tu15_apply_prescribed_motion_forces(context);
      if(stats)++stats->native_contact_fixed_motion_calls;
      return;
    }
    if(stats)++stats->contact_motion_residual_calls;
    const std::array args{time,configuration.value};call(owner.identity,args,motion.identity,8);
  };
  contact_advance.process_agent=agent_call;
  island_agents.advance_contact_time=[&](auto island,auto configuration,auto entry,auto output,auto time,auto&context){
    native_tu15_observe_contact_time_input(m,{island,configuration,entry,output,time,context,state.x87_depth});
    native_tu15_advance_island_contact_time(m,island,configuration,entry,output,time,
      context,state.x87_depth,contact_advance,observe,stats?&stats->contact_advance:nullptr);
  };
  // Both packets outlive genuine residual callbacks. They stay above every
  // callback's arguments and downward-growing private frame, as do the step
  // and broad-phase packets. The two island phases use them sequentially.
  const NativeTu15IslandAgentPhaseScratch agent_scratch{
    {std::uint32_t(end)-0x140u},{std::uint32_t(end)-0x150u}};
  island_collision.flag_set_phase=[&](auto island,auto configuration,auto&context){
    native_tu15_collide_island_flag_set(m,island,configuration,agent_scratch,context,
      state.x87_depth,island_agents,observe,stats?&stats->island_agents:nullptr);
  };
  island_collision.flag_clear_phase=[&](auto island,auto configuration,auto&context){
    native_tu15_collide_island_flag_clear(m,island,configuration,agent_scratch,context,
      island_agents,observe,stats?&stats->island_agents:nullptr);
  };
  collision.collide_island=[&](auto island,auto dispatcher,auto configuration,auto&context){
    if(stats)++stats->native_island_collision_calls;
    native_tu15_collide_physics_island(m,island,dispatcher,configuration,context,island_collision,observe);
  };
  NativeTu15WorldCollisionListenerServices collision_listeners;
  collision_listeners.notify=[&](auto listener,auto owner,auto w,auto input,auto&){
    const std::array args{w.identity,input.identity};call(owner.identity,args,listener.identity,8);
  };
  collision_listeners.notify_island=[&](auto listener,auto owner,auto island,auto input,auto&){
    const std::array args{island.identity,input.identity};call(owner.identity,args,listener.identity,8);
  };
  collision.notify_island_listeners=[&](auto w,auto island,auto input,auto&context){
    if(stats)++stats->native_island_collision_listener_calls;
    native_tu15_notify_island_collision_listeners(m,w,island,input,context,collision_listeners,observe);
  };
  collision.drain_deletes=drain_deletes;
  collision.drain_updates=drain_updates;
  collision.notify_world_listeners=[&](auto w,auto input,auto&context){
    if(stats)++stats->native_world_collision_listener_calls;
    native_tu15_notify_world_collision_listeners(m,w,input,context,collision_listeners,observe);
  };
  if(stats)++stats->native_world_collision_calls;
  native_tu15_collide_physics_world(m,world,{packet},fp,state.x87_depth,collision,observe);
  NativeTu15WorldIntegrationMemory storage(m,observe);NativeTu15WorldIntegrationServices s;
  // Profiling consumes the actual host timestamp counter as an external input.
  // It never supplies a collision, force, admission or solver result.
  s.timestamp_counter=timestamp_counter?timestamp_counter:timestamp;
  s.island.timestamp_counter=s.timestamp_counter;
  auto world_call=[&](std::uint32_t target,NativeTu15PhysicsWorld w){call(target,{},w.identity,0);};
  s.drain_operations=drain_operations;
  s.add_actions=[&](auto w,auto&context){
    native_tu15_drain_world_added_actions(m,w,context,deferred,observe,stats?&stats->deferred:nullptr);
  };
  s.check_deactivation=[&](auto island,auto input,auto&context){
    if(stats)++stats->native_deactivation_calls;
    NativeTu15IslandDeactivationMemory test(m,island,input,out,observe);
    return native_tu15_check_island_deactivation(test,context,state.x87_depth,
      [&](auto policy,auto owner,auto body,auto,auto&policy_context){
        if(owner.identity==0x1d9b20u||owner.identity==0x1d9b70u){
          return native_tu15_gameplay_deactivation_policy(m,policy,body,
            owner.identity==0x1d9b20u?NativeTu15DeactivationPeriod::short_period:NativeTu15DeactivationPeriod::long_period,
            out-8u,out-16u,policy_context,state.x87_depth,observe,
            stats?&stats->gameplay_deactivation:nullptr);
        }
        if(owner.identity==0x31e5c0u||owner.identity==0x31e690u){
          if(stats)++stats->native_body_policy_calls;
          NativeTu15BodyDeactivationPolicyMemory test(m,policy,body,out-8u,observe);
          return native_tu15_body_deactivation_policy(test,
            owner.identity==0x31e5c0u?NativeTu15DeactivationPeriod::short_period:NativeTu15DeactivationPeriod::long_period,
            policy_context,state.x87_depth);
        }
        const std::array args{out-8u,body.identity};call(owner.identity,args,policy,8);
        return word(m,state.gp[0],1)!=0;
      });
  };
  NativeTu15IslandTransitionMemory transition_storage(m,observe);
  NativeTu15IslandTransitionServices transition_services;
  transition_services.grow_array=[&](auto header,auto stride,auto&context){
    if(stats)++stats->native_array_growth_calls;
    NativeTu15ArrayGrowthMemory array(m,{header.identity},observe);
    native_tu15_grow_array(array,stride,context,array_services);
  };
  NativeTu15InactiveIslandCleanupServices inactive_cleanup;
  const auto invalidate_inactive=[&](auto receiver,auto owner,auto&context){
    invalidate_agent(receiver,owner.identity,context);
  };
  inactive_cleanup.invalidate_agent=invalidate_inactive;
  inactive_cleanup.invalidate_manager=invalidate_inactive;
  inactive_cleanup.policy_kind=[&](auto policy,auto owner,auto&)->std::uint32_t{
    if(const auto kind=native_tu15_activation_type(owner.identity))return *kind;
    if(stats)++stats->inactive_island_policy_kind_residuals;
    call(owner.identity,{},policy,0);return state.gp[0];
  };
  NativeTu15BodyPolicyServices inactive_policy;
  inactive_policy.allocation=arena_storage;inactive_policy.bounds=bounds_services;
  inactive_policy.policy_kind=inactive_cleanup.policy_kind;
  inactive_policy.destroy_policy=[&](auto policy,auto owner,auto disposing,auto&){
    const std::array args{disposing};call(owner.identity,args,policy,4);
  };
  inactive_policy.release_base_lock=[&](auto object,auto flags,auto&){
    const std::array args{flags};call(0x2e12a0u,args,object,4);
  };
  inactive_cleanup.select_policy=[&](auto body,auto kind,auto&context){
    native_tu15_select_body_policy(m,body,kind,{{std::uint32_t(end)-0x1c0u}},context,
      state.x87_depth,inactive_policy,observe,stats?&stats->inactive_island_policy:nullptr);
  };
  transition_services.island_deactivated=[&](auto listener,auto owner,auto island,auto&context){
    if(owner.identity==0x282ab0u){
      try{native_tu15_cleanup_inactive_island(m,island,context,inactive_cleanup,observe,
        stats?&stats->inactive_island_cleanup:nullptr);}
      catch(...){if(stats)++stats->native_inactive_island_cleanup_refusals;throw;}
      return;
    }
    const std::array args{island.identity};call(owner.identity,args,listener.identity,4);
  };
  transition_services.body_deactivated=[&](auto listener,auto owner,auto body,auto&current){
    if(owner.identity==0x2815f0u){
      native_tu15_notify_bound_body_deactivation_with_frames(m,listener,body,current,arena_storage,observe,
        stats?&stats->body_deactivation:nullptr);return;
    }
    const std::array args{body.identity};call(owner.identity,args,listener.identity,4);
  };
  s.deactivate_island=[&](auto w,auto island,auto&context){
    if(stats)++stats->native_island_transition_calls;
    native_tu15_deactivate_physics_island(transition_storage,w,island,context,transition_services);
  };
  deferred.deactivate_island=s.deactivate_island;
  s.notify_island_listeners=[&](auto w,auto island,auto input,auto&context){
    if(stats)++stats->native_island_step_listener_calls;
    native_tu15_notify_island_step_listeners(m,w,island,input,context,collision_listeners,observe);
  };
  s.regroup_islands=[&](auto w,auto&context){
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
  };
  s.drain_deletes=drain_deletes;s.drain_updates=drain_updates;
  NativeTu15BodyReferenceServices body_references;
  body_references.reserve_array=[&](auto header,auto capacity,auto stride,auto&context){
    if(stats)++stats->native_array_reserve_calls;
    native_tu15_reserve_array(m,{header.identity},capacity,stride,context,array_services,observe);
  };
  s.detach_body=[&](auto body,auto&context){
    // Actual3174A0 compacts three reference cohorts before retirement.
    native_tu15_compact_body_references(m,body,context,body_references,observe,
      stats?&stats->body_references:nullptr);
  };
  s.destroy_body=[&](auto body,auto&){const auto target=word(m,word(m,body.identity));const std::array args{1u};call(target,args,body.identity,4);};
  s.notify_world_listeners=[&](auto w,auto input,auto&context){
    if(stats)++stats->native_world_step_listener_calls;
    native_tu15_notify_world_step_listeners(m,w,input,context,collision_listeners,observe);
  };
  s.island.update_action=[&](auto action,auto owner,auto input,auto&){const std::array args{input.identity};call(owner.identity,args,action.identity,4);};
  s.island.apply_other_motion_forces=[&](auto motion,auto owner,auto input,auto gravity,auto&context){
    if(owner.identity==0x8fdb0u){
      if(stats)++stats->native_prescribed_force_calls;
      native_tu15_apply_prescribed_motion_forces(context);
    }else{const std::array args{input.identity,gravity.identity};call(owner.identity,args,motion.identity,8);}
  };
  s.island.integrate_pose=[&](auto motion,auto owner,auto input,auto&context){
    if(owner.identity==0x325170u){
      if(stats)++stats->native_pose_calls;
      NativeTu15MotionPoseMemory pose(m,motion.identity,input.identity,observe);
      native_tu15_integrate_motion_pose(pose,context,state.x87_depth);
    }else if(owner.identity==0x323730u){
      if(stats)++stats->native_kinematic_pose_calls;
      NativeTu15KinematicPoseMemory pose(m,motion.identity,input.identity,observe);
      native_tu15_integrate_kinematic_motion_pose(pose,context,state.x87_depth);
    }else if(owner.identity==0x2e4d60u){
      if(stats)++stats->native_fixed_pose_calls;
      native_tu15_keep_fixed_motion_pose(context);
    }else{const std::array args{input.identity};call(owner.identity,args,motion.identity,4);}
  };
  s.island.solve_constraints=[&](const auto&i,auto&context){
    if(stats)++stats->native_constraint_calls;
    NativeTu15ConstraintBindingActivity activity;
    const auto record=[&]{if(stats){
      stats->native_dynamic_accumulator_calls+=activity.native_dynamic_setups;
      stats->native_constraint_producer_axis_calls+=activity.producers.axis_factor_calls;
      stats->native_constraint_producer_scaled_calls+=activity.producers.scaled_calls;
      stats->native_constraint_direct_base_calls+=activity.native_direct_base_builders;
      stats->native_constraint_direct_base_rows+=activity.completed_direct_base_rows;
      stats->native_constraint_producer_base_calls+=activity.base.calls;
      stats->native_constraint_producer_base_rows+=activity.base.rows;
      stats->native_constraint_producer_property_lookups+=activity.producers.property_lookups;
      for(unsigned n=0;n<3;++n){
        stats->native_constraint_producer_emissions[n]+=activity.producers.emissions[n];
        stats->native_constraint_producer_completed_emissions[n]+=activity.completed_producer_emissions[n];
      }
      stats->native_constraint_setup_calls+=activity.step.native_accumulator_setups;
      stats->native_motion_commit_calls+=activity.native_motion_commits;
      stats->native_solver_calls+=activity.solver.calls;
      stats->native_solver_initializations+=activity.initializer.calls;
      stats->native_solver_iterations+=activity.solver.iterations;
      stats->native_solver_primary_rows+=activity.solver.primary_dispatches;
      stats->native_solver_secondary_rows+=activity.solver.secondary_rows;
      stats->native_solver_exported_rows+=activity.solver.exported_rows;
      stats->native_solver_custom_refusals+=activity.custom_schema_refusals;
      stats->native_solver_custom_calls+=activity.custom_schemas.calls;
      stats->native_solver_clock_calls+=activity.native_clock_calls;
      stats->native_joint_builder_attempts+=activity.joint_builder.attempts;
      stats->native_joint_builder_calls+=activity.joint_builder.completed;
      stats->native_joint_builder_refusals+=activity.joint_builder.refusals;
      stats->native_joint_builder_unsupported_fp+=activity.joint_builder.unsupported_fp;
      stats->native_joint_builder_kernel_entries+=activity.joint_builder.parent.calls;
      stats->native_joint_builder_kernel_completed+=activity.joint_builder.parent.completed;
      stats->native_joint_builder_headers+=activity.joint_builder.parent.headers;
      stats->native_joint_builder_angular_rows+=activity.joint_builder.parent.angular_rows;
      stats->native_joint_builder_motors+=activity.joint_builder.parent.motors;
      stats->native_joint_builder_linear_requests+=activity.joint_builder.parent.linear_requests;
      stats->native_joint_builder_linear_calls+=activity.joint_builder.linear.calls;
      stats->native_joint_builder_linear_completed+=activity.joint_builder.linear.completed;
      stats->native_joint_builder_linear_rows+=activity.joint_builder.linear.rows;
      stats->native_joint_builder_linear_refusals+=activity.joint_builder.linear.refusals;
      stats->native_joint_builder_limits+=activity.joint_builder.parent.limits;
      stats->native_joint_builder_positive_unwraps+=activity.joint_builder.parent.positive_unwraps;
      stats->native_joint_builder_negative_unwraps+=activity.joint_builder.parent.negative_unwraps;
      stats->native_joint_builder_unbound_linear+=activity.joint_builder.parent.unbound_linear;
      stats->native_joint_builder_linear_unaligned+=activity.joint_builder.linear.unaligned_vectors;
      stats->native_joint_builder_linear_unsupported_fp+=activity.joint_builder.linear.unsupported_fp_profiles;
      stats->native_constraint_arena_allocations+=activity.native_arena_allocations;
      stats->native_constraint_arena_releases+=activity.native_arena_releases;
      stats->native_constraint_arena_refusals+=activity.native_arena_refusals;
      stats->native_stack_arena_allocations+=activity.native_arena_allocations;
      stats->native_stack_arena_releases+=activity.native_arena_releases;
      stats->native_stack_arena_refusals+=activity.native_arena_refusals;
      for(unsigned n=0;n<3;++n)stats->native_solver_custom_kinds[n]+=activity.custom_schemas.owners[n];
      for(unsigned kind=0;kind<3;++kind)stats->native_motion_commit_kinds[kind]+=activity.native_motion_commit_kinds[kind];
    }};
    try{
      native_tu15_step_island_constraints_with_residuals(m,i,context,state.x87_depth,
        std::uint32_t(end)-0x80u,[&](auto owner,auto args,auto receiver,unsigned pop,auto&){
          const bool solver_dependency=owner==0x37c5c0u||owner==0x2e3850u;
          const auto incoming=state.x87;const auto incoming_depth=state.x87_depth;
          call(owner,args,receiver,pop);
          if(solver_dependency)for(unsigned n=0;n<incoming_depth;++n)
            check(std::memcmp(&incoming[n],&state.x87[n],10)==0,
              "Native TU1.5 solver dependency changed a preserved live x87 value");
          return state.gp[0];
        },observe,&activity,actual_filetime,&arena_storage);
    }catch(...){record();throw;}
    record();
  };
  native_tu15_integrate_physics_world(storage,world,{packet},fp,state.x87_depth,s,stats);
  state.x87_status=fp.status;state.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};
}
} // namespace halo2
