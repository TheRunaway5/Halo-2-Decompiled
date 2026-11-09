#include "halo2/native_tu15_island_regroup_binding.hpp"
#include "halo2/native_tu15_constraint_info_input_observer.hpp"
#include "halo2/native_tu15_constant_constraint_info.hpp"
#include "halo2/native_tu15_extended_constraint_info.hpp"
#include <stdexcept>
#include <string>
namespace halo2 {
void native_tu15_regroup_physics_islands_with_native_services(SourcePhysicsContactMemory&m,
    NativeTu15PhysicsWorld world,NativeTu15IslandRegroupScratch scratch,NativeTu15PhysicsFrameFp&fp,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15IslandRegroupBindingActivity*activity){
  if(activity)++activity->attempts;
  try{
    NativeTu15IslandPartitionAllocatorBinding allocator(m,observe,activity?&activity->allocator:nullptr);
    NativeTu15IslandRegroupServices services;
    services.lifetime=allocator.lifetime_services();services.partition=allocator.graph_services();
    services.edges.allocation=allocator.array_services();services.edges.reserve_array=services.partition.reserve_array;
    const auto unknown=[&](const char*kind,NativeTu15PhysicsVirtualOwner owner){
      if(activity)++activity->regroup.unbound_services;
      throw std::runtime_error(std::string("Native TU1.5 regroup requires native ")+kind+" owner="+std::to_string(owner.identity));
    };
    services.motion_weight=[&](auto,auto owner,auto&)->std::uint32_t{
      if(activity)++activity->unknown_motion_weights;unknown("motion weight",owner);return 0;
    };
    services.constraint_info=[&](auto constraint,auto owner,auto info,auto&context){
      native_tu15_observe_constraint_info_input(m,{constraint,owner.identity,info.identity,context});
      if(owner.identity==0x31d630u){
        (void)native_tu15_get_constant_constraint_info(m,info,observe);
        if(activity){++activity->constant_info_queries;++activity->regroup.native_info_queries;}
        return;
      }
      if(owner.identity==0x31f840u){
        (void)native_tu15_get_extended_constraint_info(m,constraint,info,observe);
        if(activity){++activity->extended_info_queries;++activity->regroup.native_info_queries;}
        return;
      }
      if(owner.identity==0x31f740u){
        (void)native_tu15_get_conveyor_constraint_info(m,constraint,info,observe);
        if(activity){++activity->extended_info_queries;++activity->regroup.native_info_queries;}
        return;
      }
      if(activity)++activity->unknown_constraint_info;unknown("constraint info",owner);
    };
    services.add_info=[&](auto,auto owner,auto,auto,auto&){
      if(activity)++activity->unknown_info_updates;unknown("island info update",owner);
    };
    services.remove_info=services.add_info;
    services.destroy_island=[&](auto,auto owner,auto,auto&){
      if(activity)++activity->unknown_island_destructors;unknown("island destructor",owner);
    };
    services.edges.enumerate_action_bodies=[&](auto,auto owner,auto,auto&){
      if(activity)++activity->unknown_action_enumerators;unknown("action enumeration",owner);
    };
    native_tu15_regroup_physics_islands(m,world,scratch,fp,services,observe,activity?&activity->regroup:nullptr);
    if(activity)++activity->completed;
  }catch(...){if(activity)++activity->refusals;throw;}
}
} // namespace halo2
