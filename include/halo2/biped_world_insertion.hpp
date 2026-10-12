#pragma once
#include "halo2/biped_body_velocity.hpp"
#include "halo2/physics_body_disposal.hpp"
#include "halo2/physics_constraint_membership.hpp"
#include "halo2/physics_pair_manager.hpp"
#include "halo2/physics_world_membership.hpp"
namespace halo2 {
struct SourceBipedWorldInsertionServices {
  SourcePhysicsContactBatchServices lower;
  SourceBipedBodyTransformServices velocity;
  SourcePhysicsBodyDisposalServices disposal;
  SourcePhysicsBroadphaseServices scratch;
  SourcePhysicsPairManagerServices pairs;
  // Actual3144D0 owner and its original ABI entry. Missing ownership throws.
  std::function<void(SourcePhysicsContactMemory &, std::uint32_t world,
                     std::uint32_t body, std::uint32_t entry_esp,
                     const SourcePhysicsContactRecordObserver &)>
      broadphase;
};
// Complete1D5210 caller specialization of31AC90, including actual body2002
// property lookup. bounds is temporary caller AABB storage.
void source_construct_biped_body_auxiliary_1_1(
    SourcePhysicsContactMemory &, std::uint32_t auxiliary, std::uint32_t body,
    std::uint32_t object, std::uint32_t bounds,
    SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRecordObserver & = {});
// Complete1C7320 registration orchestration. Budget eviction and inactive
// single-body deactivation need their original owners when those branches run.
void source_register_biped_world_body_1_1(
    SourcePhysicsContactMemory &, std::uint32_t body, std::uint8_t inactive,
    std::uint32_t entry_esp, SourcePhysicsFactoryFloatingPoint,
    const SourceBipedWorldInsertionServices &,
    const SourcePhysicsContactRecordObserver & = {});
// Native3144D0 composition with caller-derived ABI scratch and actual source
// pair production/delivery. Unknown pair classes remain required.
void source_insert_biped_world_broadphase_1_1(
    SourcePhysicsContactMemory &, std::uint32_t world, std::uint32_t body,
    std::uint32_t entry_esp, SourcePhysicsFactoryFloatingPoint,
    const SourceBipedWorldInsertionServices &,
    const SourcePhysicsContactRecordObserver & = {});
// Complete1D4F30 insertion orchestration, including listeners, actual current
// velocities, source auxiliary allocation and reference lifetime, authored
// solver scaling and constraint insertion. Broadphase/pair production is a
// required dependency and never a recorded or default successful result.
void source_insert_biped_component_world_1_1(
    SourcePhysicsContactMemory &, std::uint32_t component,
    std::uint32_t entry_esp, SourcePhysicsFactoryFloatingPoint,
    const SourceBipedWorldInsertionServices &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
