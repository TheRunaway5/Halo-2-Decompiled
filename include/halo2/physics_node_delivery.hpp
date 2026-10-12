#pragma once
#include "halo2/physics_body_matrix.hpp"
#include "halo2/physics_contact_runtime.hpp"

namespace halo2 {
struct PhysicsNodeDeliveryScratch {
  std::uint32_t ancestor_mask32{},body_transform40{};
};
struct PhysicsNodeDeliveryServices {
  SourcePhysicsBodyMatrixCorrection correction;
  // Full retained ABI for nonstandard motion/shape-container callbacks.
  // Required by the exact raw current-node path when such a virtual is reached.
  std::function<void(SourcePhysicsContactMemory &,
      const SourcePhysicsContactRuntimeCall &,SourcePhysicsContactRuntimeAbi &,
      const SourcePhysicsContactRecordObserver &)> runtime_owner;
  // Required for a compound body's non-root rigid node. This is the source
  // shape-container+34 owner, returning its actual transform; no bind-pose proxy.
  std::function<std::array<float,13>(std::uint32_t shape,std::uint32_t child)> child_transform;
  // Nonstandard motion subclasses retain their actual virtual+18 owner.
  std::function<std::uint32_t(std::uint32_t motion,std::uint32_t method)> motion_type;
};
//1D7310 plus recursive1D7710 physical-first WORLD/local delivery. The real
// object/model/phmo/current-body banks and caller node mask remain retained.
// Animation selection, world integration and solver stepping are independent
// upstream owners; this consumer never creates a corpse or supplies a pose.
void source_physics_deliver_nodes_1_1(SourcePhysicsContactMemory&,
    std::uint32_t component,std::uint32_t node_mask,float height_correction,
    const PhysicsNodeDeliveryScratch&,const PhysicsNodeDeliveryServices& = {},
    const SourcePhysicsContactRecordObserver& = {});
} // namespace halo2
