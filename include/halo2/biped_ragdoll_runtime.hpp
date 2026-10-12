#pragma once
#include "halo2/biped_body_placement.hpp"
#include "halo2/biped_component_rebuild.hpp"
#include "halo2/biped_world_insertion.hpp"
#include "halo2/biped_world_removal.hpp"
#include "halo2/source_point_collision.hpp"
#include "halo2/source_segment_collision.hpp"
#include "halo2/source_point_collision_runtime.hpp"
#include <optional>

namespace halo2 {
struct SourceBipedRagdollRuntimeServices {
  SourceBipedWorldRemovalServices removal;
  SourceBipedWorldInsertionServices insertion;
  SourceBipedComponentRebuildServices component;
  SourcePhysicsBodyDisposalServices disposal;
  SourceBipedBodyTransformServices transform;
  SourcePointCollisionServices point;
  // Complete point arbitration including current posed-object containment.
  // When absent, the typed point owner above retains its required object service.
  std::optional<SourceSegmentCollisionServices> point_runtime;
  // When supplied, complete16B240 executes on this same retained world bank.
  // Its reached virtual/platform owners remain required by the raw runtime.
  std::optional<SourceSegmentCollisionServices> segment_runtime;
  // Actual shape correction or explicit segment providers receive the current
  // request and E3270 frame. Point queries, optional native segment dispatch
  // and body transforms share the bank. There is no host terrain-ray substitute.
  std::function<SourceBipedBodyPlacementServices(
      const SourceBipedPlacementRequest &, std::uint32_t entry_esp)> placement;
};
// Complete E2CE0 composition with native object/component removal, disposal,
// reconstruction, placement and insertion. Only real world-query/platform
// services remain external. Every temporary frame is derived from entry_esp.
std::uint8_t source_rebuild_biped_ragdoll_runtime_1_1(
    SourcePhysicsContactMemory &, std::uint32_t object,
    std::uint32_t entry_esp, SourcePhysicsFactoryFloatingPoint,
    const SourceBipedRagdollRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
void source_maintain_biped_ragdoll_runtime_1_1(
    SourcePhysicsContactMemory &, std::uint32_t object,
    std::uint32_t entry_esp, SourcePhysicsFactoryFloatingPoint,
    const SourceBipedRagdollRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
