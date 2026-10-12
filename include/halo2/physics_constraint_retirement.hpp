#pragma once
#include "halo2/physics_body_disposal.hpp"
#include "halo2/physics_constraint_membership.hpp"
namespace halo2 {
struct SourcePhysicsConstraintRetirementServices {
  SourcePhysicsContactBatchServices lower;
  SourcePhysicsBodyDisposalServices disposal;
};
// Complete31C570 subtraction of the source requirements packet.
void source_subtract_physics_constraint_requirements_1_1(
    SourcePhysicsContactMemory &, std::uint32_t island,
    std::uint32_t requirements, const SourcePhysicsContactRecordObserver & = {});
// Complete31C620 stable constraint removal, requirement accounting and lifetime.
void source_retire_physics_island_constraint_1_1(
    SourcePhysicsContactMemory &, std::uint32_t island, std::uint32_t constraint,
    std::uint32_t entry_esp, const SourcePhysicsConstraintRetirementServices &,
    const SourcePhysicsContactRecordObserver & = {});
// Complete312F20 direct/deferred routing. output is actual caller ABI storage.
void source_remove_physics_world_constraint_1_1(
    SourcePhysicsContactMemory &, std::uint32_t world, std::uint32_t output,
    std::uint32_t constraint, std::uint32_t entry_esp,
    const SourcePhysicsConstraintRetirementServices &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
