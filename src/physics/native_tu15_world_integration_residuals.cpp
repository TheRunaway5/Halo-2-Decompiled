#include "halo2/native_tu15_world_integration_residuals.hpp"
#include "halo2/native_tu15_motion_pose.hpp"
#include "halo2/native_tu15_island_deactivation.hpp"
#include "halo2/native_tu15_body_deactivation_policy.hpp"
#include "halo2/native_tu15_island_transition.hpp"
#include "halo2/native_tu15_kinematic_motion.hpp"
#include "halo2/native_tu15_array_allocator.hpp"
#include "halo2/native_tu15_array_reserve.hpp"
#include "halo2/native_tu15_world_collision.hpp"
#include "halo2/native_tu15_world_collision_listeners.hpp"
#include "halo2/native_tu15_constraint_step_residuals.hpp"
#include "halo2/native_tu15_island_collision.hpp"
#include "halo2/native_tu15_broad_phase.hpp"
#include <algorithm>
#include <limits>
#include <cstring>
#include <stdexcept>
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
  auto call_at=[&](std::uint32_t service_stack,std::uint32_t target,std::span<const std::uint32_t>args,std::optional<std::uint32_t>ecx,unsigned pop){
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
  NativeTu15WorldCollisionServices collision;
  collision.timestamp_counter=timestamp_counter?timestamp_counter:timestamp;
  collision.drain_operations=[&](auto w,auto&){call(0x319750u,{},w.identity,0);};
  NativeTu15IslandCollisionServices island_collision;
  NativeTu15BroadPhaseServices broad_phase;
  broad_phase.timestamp_counter=collision.timestamp_counter;
  broad_phase.allocate_block=[&](auto allocator,auto owner,auto bytes,auto&){
    const std::array args{bytes};call(owner.identity,args,allocator.identity,4);return state.gp[0];
  };
  broad_phase.release_block=[&](auto allocator,auto owner,auto block,auto bytes,auto&){
    const std::array args{block,bytes};call(owner.identity,args,allocator.identity,8);
  };
  broad_phase.release_owned_pairs=[&](auto allocator,auto owner,auto data,auto bytes,auto memory_class,auto&){
    const std::array args{data,bytes,memory_class};call(owner.identity,args,allocator.identity,12);
  };
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
  broad_phase.sweep=[&](auto dispatcher,auto owner,auto entries,auto bounds,auto count,auto first,auto second,auto&){
    const std::array args{entries,bounds,count,first.identity,second.identity};call(owner.identity,args,dispatcher.identity,20);
  };
  broad_phase.grow_pairs=[&](auto pairs,auto capacity,auto stride,auto&context){
    if(stats)++stats->native_array_reserve_calls;
    native_tu15_reserve_array(m,{pairs.identity},capacity,stride,context,array_services,observe);
  };
  broad_phase.consume_pairs=[&](auto island,auto first,auto second,auto&){
    const std::array args{first.data,first.count,second.data,second.count};call(0x321510u,args,island.identity,16);
  };
  island_collision.first_phase=[&](auto island,auto dispatcher,auto configuration,auto&context){
    if(stats)++stats->native_broad_phase_calls;
    native_tu15_update_broad_phase(m,island,dispatcher,configuration,
      NativeTu15BroadPhaseScratch{{std::uint32_t(end)-0xc0u},{std::uint32_t(end)-0xe0u}},context,state.x87_depth,broad_phase,observe);
  };
  NativeTu15IslandAgentPhaseServices island_agents;
  island_agents.timestamp_counter=collision.timestamp_counter;
  island_agents.allocate_stack_block=[&](auto allocator,auto owner,auto bytes,auto&){
    const std::array args{bytes};call(owner.identity,args,allocator.identity,4);return state.gp[0];
  };
  island_agents.release_stack_block=[&](auto allocator,auto owner,auto block,auto bytes,auto&){
    const std::array args{block,bytes};call(owner.identity,args,allocator.identity,8);
  };
  island_agents.release_owned_contacts=[&](auto allocator,auto owner,auto data,auto bytes,auto memory_class,auto&context){
    array_services.release({allocator.identity,owner.identity,data,bytes,memory_class},context);
  };
  const auto agent_call=[&](auto agent,auto owner,auto first,auto second,auto configuration,auto output,auto&){
    const std::array args{first.identity,second.identity,configuration.identity,output.identity};
    call(owner.identity,args,agent.identity,16);
  };
  island_agents.process_agent=agent_call;
  island_agents.finish_agent=agent_call;
  NativeTu15IslandContactAdvanceServices contact_advance;
  contact_advance.advance_motion=[&](auto motion,auto owner,auto time,auto configuration,auto&){
    const std::array args{time,configuration.value};call(owner.identity,args,motion.identity,8);
  };
  contact_advance.process_agent=agent_call;
  island_agents.advance_contact_time=[&](auto island,auto configuration,auto entry,auto output,auto time,auto&context){
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
  collision.drain_deletes=[&](auto w,auto&){call(0x319cd0u,{},w.identity,0);};
  collision.drain_updates=[&](auto w,auto&){call(0x316e10u,{},w.identity,0);};
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
  s.drain_operations=[&](auto w,auto&){world_call(0x319750u,w);};
  s.add_actions=[&](auto w,auto&){world_call(0x316e90u,w);};
  s.check_deactivation=[&](auto island,auto input,auto&context){
    if(stats)++stats->native_deactivation_calls;
    NativeTu15IslandDeactivationMemory test(m,island,input,out,observe);
    return native_tu15_check_island_deactivation(test,context,state.x87_depth,
      [&](auto policy,auto owner,auto body,auto,auto&policy_context){
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
  transition_services.island_deactivated=[&](auto listener,auto owner,auto island,auto&){const std::array args{island.identity};call(owner.identity,args,listener.identity,4);};
  transition_services.body_deactivated=[&](auto listener,auto owner,auto body,auto&){const std::array args{body.identity};call(owner.identity,args,listener.identity,4);};
  s.deactivate_island=[&](auto w,auto island,auto&context){
    if(stats)++stats->native_island_transition_calls;
    native_tu15_deactivate_physics_island(transition_storage,w,island,context,transition_services);
  };
  s.notify_island_listeners=[&](auto w,auto island,auto input,auto&){const std::array args{w.identity,island.identity,input.identity};call(0x322950u,args,{},0);};
  s.regroup_islands=[&](auto w,auto&){world_call(0x3187c0u,w);};
  s.drain_deletes=[&](auto w,auto&){world_call(0x319cd0u,w);};s.drain_updates=[&](auto w,auto&){world_call(0x316e10u,w);};
  s.detach_body=[&](auto body,auto&){call(0x3174a0u,{},body.identity,0);};
  s.destroy_body=[&](auto body,auto&){const auto target=word(m,word(m,body.identity));const std::array args{1u};call(target,args,body.identity,4);};
  s.notify_world_listeners=[&](auto w,auto input,auto&){const std::array args{w.identity,input.identity};call(0x322820u,args,{},0);};
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
        },observe,&activity,actual_filetime);
    }catch(...){record();throw;}
    record();
  };
  native_tu15_integrate_physics_world(storage,world,{packet},fp,state.x87_depth,s,stats);
  state.x87_status=fp.status;state.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};
}
} // namespace halo2
