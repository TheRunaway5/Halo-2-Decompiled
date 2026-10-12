#pragma once
#include "halo2/physics_agent_scheduler.hpp"

namespace halo2 {
// Complete source2E68D0 current endpoint update. Inputs are the real retained
// handle bank, source-produced32-byte bounds and real added/removed pair
// descriptors. Neither a pair nor a collision/contact result is supplied.
void source_update_physics_broadphase_bodies_1_1(SourcePhysicsContactMemory &,
    std::uint32_t backend,const std::array<std::uint32_t,5> &arguments,
    SourcePhysicsFactoryFloatingPoint,const SourcePhysicsContactRecordObserver & = {});
// Complete actual capsule2E4670 and wrapper2E2380 bounds paths. Resident
// shape/transform/radius/velocity/world data must already have real owners.
void source_physics_capsule_bounds_1_1(SourcePhysicsContactMemory &,
    std::uint32_t shape,std::uint32_t transform,std::uint32_t margin,
    std::uint32_t output,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRecordObserver & = {});
void source_physics_current_wrapper_bounds_1_1(SourcePhysicsContactMemory &,
    std::uint32_t wrapper,std::uint32_t transform,std::uint32_t margin,
    std::uint32_t output,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRecordObserver & = {});
struct SourcePhysicsCurrentBroadphaseServices {
    SourcePhysicsBroadphaseServices scratch;
    SourcePhysicsPairManagerServices pairs;
    // Unknown actual shape/backend virtual targets require their real owner.
    // No default success, recorded pair/contact, or geometry approximation.
    std::function<std::uint32_t(SourcePhysicsContactMemory &,
        const SourcePhysicsAgentOwnerCall &,const SourcePhysicsContactRecordObserver &)> required_owner;
    std::function<std::uint64_t()> timestamp_counter;
};
// Complete31DB70 current-body bounds -> backend delta -> queued pairs ->
//31D980 -> release owner. entry_esp preserves original temporary aliases.
void source_update_physics_island_broadphase_1_1(SourcePhysicsContactMemory &,
    std::uint32_t island,std::uint32_t backend,std::uint32_t step,
    std::uint32_t entry_esp,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsCurrentBroadphaseServices & = {},
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
