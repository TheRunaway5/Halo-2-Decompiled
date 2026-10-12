#pragma once
#include "halo2/biped_body_velocity.hpp"
#include "halo2/biped_controller.hpp"
#include "halo2/physics_body_grouping.hpp"
#include "halo2/physics_component_rebuild.hpp"

namespace halo2 {
struct SourceBipedComponentRebuildServices {
  // Optional explicit caller packets. Zero selects the actual native source
  // frame derivation; production callers need only provide their entry ESP.
  SourcePhysicsBodyGroupingScratch grouping{};
  SourcePhysicsReciprocalSqrtEstimate reciprocal_sqrt;
  SourceBipedVelocityServices velocity;
  SourceBipedControllerScratch controller_scratch{};
  // Optional caller boundary for controller construction. Without it the
  // native1DA6F0 owner uses actual derived caller storage. Optional explicit
  // controller_scratch can retain a separately owned diagnostic call frame.
  std::function<void(const SourceBipedControllerRequest &)> controller;
  std::function<void(std::uint32_t component)> dispose;
  std::function<std::uint32_t(const PhysicsComponentRebuildCall &)> other_owner;
};
// Native composition of1D2B10→1DA570→1D9330 and1C8430. The actual type-remap
// and branch table remain memory inputs. entry_esp identifies caller scratch;
// no recorded allocation, body, constraint or successful return is supplied.
void source_rebuild_biped_component_1_1(
    SourcePhysicsContactMemory &, std::uint32_t component_datum,
    std::uint32_t entry_esp, SourcePhysicsFactoryFloatingPoint,
    const SourceBipedComponentRebuildServices &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
