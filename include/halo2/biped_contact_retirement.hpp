#pragma once
#include "halo2/physics_body_disposal.hpp"
#include "halo2/physics_pair_manager.hpp"
namespace halo2 {
struct SourceBipedContactRetirementServices {
  SourcePhysicsPairManagerServices lower;
  SourcePhysicsBodyDisposalServices disposal;
  // Actual31F640 caller temporary output is required if its constraint is
  // still attached to a world. Zero does not stand for a successful removal.
  std::uint32_t attached_constraint_output{};
  // Original entry ESP of the public owner being invoked. Descendant ABI
  // storage is derived from its literal prologues, never a supplied contact.
  std::uint32_t entry_esp{};
};
void source_retire_physics_pair_agent_1_1(
    SourcePhysicsContactMemory &, std::uint32_t agent,
    const SourceBipedContactRetirementServices &,
    const SourcePhysicsContactRecordObserver & = {});
void source_destroy_physics_pair_contact_manager_1_1(
    SourcePhysicsContactMemory &, std::uint32_t manager, std::uint32_t flags,
    const SourceBipedContactRetirementServices &,
    const SourcePhysicsContactRecordObserver & = {});
// Complete31C6D0 retained/queued/other-island pair removal. Only the actual
// supported agent and manager virtual classes are dispatched natively.
void source_retire_physics_island_pair_1_1(
    SourcePhysicsContactMemory &, std::uint32_t island_listener,
    std::uint32_t pair, const SourceBipedContactRetirementServices &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
