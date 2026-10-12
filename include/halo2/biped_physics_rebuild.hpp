#pragma once
#include "halo2/physics_contact_record.hpp"

namespace halo2 {
struct SourceBipedControllerRequest {
  std::uint32_t component{}, definition{}, physical_state{};
  std::uint8_t state_class{}, controller_class{};
  bool constrained{}, dead{};
};
struct SourceBipedPhysicsRebuildServices {
  // The actual1D9330(component,0,0,1) owner constructs the complete authored
  // rigid-body/constraint bank. A positive return alone cannot supply bodies.
  std::function<std::uint8_t(std::uint32_t)> group;
  std::function<void(std::uint32_t)> dispose;
  std::function<void(const SourceBipedControllerRequest &)> controller;
};
// CompleteE23F0, using current physical/contact state and actual unit flags.
bool source_biped_physical_constraint_1_1(const SourcePhysicsContactMemory &,
    std::uint32_t object_datum);
// Complete1DA570. Ragdoll admission and the actual post-construction count
// govern fallback to controller10; mode2 selects controller11 independently.
void source_rebuild_biped_physics_1_1(SourcePhysicsContactMemory &,
    std::uint32_t component, const SourceBipedPhysicsRebuildServices &,
    const SourcePhysicsContactRecordObserver & = {});
struct SourceObjectPhysicsRebuildServices {
  std::function<void(std::uint32_t)> remove, dispose, rebuild, insert;
};
// Complete1C70D0: retain prior world-membership bit5 across disposal/rebuild.
void source_rebuild_object_physics_1_1(SourcePhysicsContactMemory &,
    std::uint32_t object_datum, std::uint32_t flags,
    const SourceObjectPhysicsRebuildServices &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
