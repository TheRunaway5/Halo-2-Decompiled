#pragma once
#include "halo2/physics_agent_scheduler.hpp"
#include "halo2/physics_contact_batches.hpp"
#include "halo2/physics_motion_step.hpp"
#include "halo2/physics_current_broadphase.hpp"
#include "halo2/physics_contact_dispatch.hpp"
#include "halo2/physics_constraint_step.hpp"

namespace halo2 {
struct SourcePhysicsWorldStepServices {
    SourcePhysicsAgentSchedulerServices agents;
    // When present,31E2C0 binds31DB70 directly to the native current bounds,
    // backend endpoint update and pair-manager owners. The retained memory and
    // real lower services belong to the caller, never a recorded result.
    std::optional<SourcePhysicsCurrentBroadphaseServices> current_broadphase;
    // Selected real agent300870 binds native BSP child-cache, narrowphase,
    // GJK and contact lifetime kernels. Its retained packet-frame owner is
    // still mandatory; an absent owner cannot append a contact.
    std::optional<SourcePhysicsContactDispatchServices> contact_dispatch;
    // Explicitly connects322690 orchestration and dynamic solver motions.
    // Actual Jacobian producers remain mandatory lower owners. The island
    // services may connect the complete compiled iterative solver.
    std::optional<SourcePhysicsConstraintStepServices> constrained_islands;
    SourcePhysicsReciprocalSqrtEstimate reciprocal_sqrt;
    // Unbound solver322690, island activation/merging, deferred world operations,
    // deactivators and unknown motion/listener virtuals require real owners.
    // Missing ownership stops at the original call, after preceding stores.
    std::function<std::uint32_t(SourcePhysicsContactMemory &,
        const SourcePhysicsContactBatchOwnerCall &,
        const SourcePhysicsContactRecordObserver &)> required_owner;
    std::function<std::uint64_t()> timestamp_counter;
};
// Complete31CFE0 routes unconstrained dynamic bodies through the native force
// and integration owners. Constrained islands require the original solver.
void source_integrate_physics_island_1_1(SourcePhysicsContactMemory &,
    std::uint32_t island, std::uint32_t world_step, std::uint32_t entry_esp,
    SourcePhysicsFactoryFloatingPoint, const SourcePhysicsWorldStepServices &,
    const SourcePhysicsContactRecordObserver & = {});
std::uint32_t source_check_physics_island_deactivation_1_1(SourcePhysicsContactMemory &,
    std::uint32_t island, std::uint32_t output, std::uint32_t step,
    std::uint32_t entry_esp, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldStepServices &, const SourcePhysicsContactRecordObserver & = {});
// Actual two-phase317650->316200->316440 world step, over current source
// world/islands/bodies. No terrain mover or chosen contact/solver fallback.
void source_collide_physics_world_1_1(SourcePhysicsContactMemory &,
    std::uint32_t world, std::uint32_t step, std::uint32_t entry_esp,
    SourcePhysicsFactoryFloatingPoint, const SourcePhysicsWorldStepServices &,
    const SourcePhysicsContactRecordObserver & = {});
void source_integrate_physics_world_1_1(SourcePhysicsContactMemory &,
    std::uint32_t world, std::uint32_t step, std::uint32_t entry_esp,
    SourcePhysicsFactoryFloatingPoint, const SourcePhysicsWorldStepServices &,
    const SourcePhysicsContactRecordObserver & = {});
void source_step_physics_world_1_1(SourcePhysicsContactMemory &,
    std::uint32_t world, std::uint32_t step, std::uint32_t entry_esp,
    SourcePhysicsFactoryFloatingPoint, const SourcePhysicsWorldStepServices &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
