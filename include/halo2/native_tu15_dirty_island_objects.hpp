#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <functional>
#include <map>
namespace halo2 {
struct NativeTu15DirtyIslandObjectServices {
    std::function<void(std::uint32_t datum,std::uint32_t flags,NativeTu15PhysicsFrameFp&,unsigned)> set_component_flags; //1CC310
    std::function<void(NativeTu15PhysicsFrameFp&,unsigned)> collect_memory; //1496F0
    std::function<void(std::uint32_t component,NativeTu15PhysicsFrameFp&,unsigned)> destroy_component; //1D7CA0
    std::function<void(std::uint32_t pool,std::uint32_t datum,NativeTu15PhysicsFrameFp&,unsigned)> free_component; //16E8A0
    std::function<void(std::uint32_t datum,NativeTu15PhysicsFrameFp&,unsigned)> delete_object; //BA570
    std::function<void(std::uint32_t component,NativeTu15PhysicsFrameFp&,unsigned)> remove_component; //1D9510
    std::function<void(std::uint32_t datum,NativeTu15PhysicsFrameFp&,unsigned)> defer_object; //BA8C0
};
struct NativeTu15DirtyIslandObjectStats {
    std::uint64_t calls{},pending_skips{},reentrant_skips{},island_rows{},dirty_islands{},
        body_rows{},component_queries{},candidate_objects{},deferred_objects{},
        component_destructions{},component_frees{},object_deletions{},recursive_updates{},
        unbound_services{},unsupported_local_set_sizes{};
    std::map<std::uint32_t,std::uint64_t> unbound_owners;
};
// Actual282C10. The two128-entry sets are native local DATA, never a numeric
// representation of source ESP/GP state. Current source arrays/flags are read
// again at their original phases. Selected unknown owners remain explicit.
void native_tu15_process_dirty_island_objects(SourcePhysicsContactMemory&,
    NativeTu15PhysicsFrameFp&,unsigned actual_preserved_x87_depth,
    const NativeTu15DirtyIslandObjectServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15DirtyIslandObjectStats* = nullptr);
} // namespace halo2
