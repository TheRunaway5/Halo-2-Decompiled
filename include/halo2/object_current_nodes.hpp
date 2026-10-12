#pragma once
#include "halo2/biped_prephysics.hpp"
#include "halo2/unit_animation_selection_runtime.hpp"
#include "halo2/physics_node_delivery.hpp"

namespace halo2 {
struct SourceObjectNodeOwnerCall {
  SourceBipedPhaseCall abi;
  // Only these low bytes, rather than incidental full registers, are inputs.
  std::optional<std::uint8_t> bl, dl;
};
struct SourceObjectCurrentNodeServices {
  PhysicsNodeDeliveryServices physical;
  // Complete retained local-pose producer and packed codecs. Real CPU and
  // cache services operate on the same current bank as physical delivery.
  std::optional<SourceUnitAnimationRuntimeServices> animation;
  // Explicit original RCPSS instruction provider. When connected,143960
  // conversion composes natively, including its lazy initializer/zero basis.
  std::function<std::uint32_t(std::uint32_t)> reciprocal;
  // Actual animation sampling, local-to-matrix conversion, parent transforms,
  // network pose and type notifications. These owners must operate on the
  // current retained banks. Recorded matrices are never production providers.
  std::function<std::uint32_t(SourcePhysicsContactMemory &,
                              const SourceObjectNodeOwnerCall &,
                              const SourcePhysicsContactRecordObserver &)>
      required_owner;
};
// Actual host instruction, not a captured/software reciprocal lookup. Xbox
// estimate equality remains a separate platform validation boundary.
std::uint32_t source_host_x86_rcpss_estimate(std::uint32_t input_word);
// Actual1D7310/1D42D0 caller frames, derived from1D7310 entry ESP.
PhysicsNodeDeliveryScratch
source_physics_node_delivery_scratch_1_1(std::uint32_t entry_esp);
// CompleteE2CA0->1F0030 height correction from the retained admission clock.
// This does not replace the body's solved transform or choose a ground height.
std::uint32_t
source_biped_physical_height_1_1(const SourcePhysicsContactMemory &,
                                 std::uint32_t object,
                                 SourcePhysicsFactoryFloatingPoint);
// CompleteBDF80 physical-first current-node orchestration. A physical node
// mask prevents animation/root composition from overwriting admitted nodes.
// Source animation and model hierarchy fill only the remaining nodes. Both
// camera lookup and skinning must consume this same current WORLD bank.
void source_update_object_current_nodes_1_1(
    SourcePhysicsContactMemory &, std::uint32_t object, std::uint32_t entry_esp,
    SourcePhysicsFactoryFloatingPoint, const SourceObjectCurrentNodeServices &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
