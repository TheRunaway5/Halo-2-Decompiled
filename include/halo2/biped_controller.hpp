#pragma once
#include "halo2/biped_physics_rebuild.hpp"
#include "halo2/physics_shape_wrapper.hpp"

namespace halo2 {
struct SourceBipedControllerScratch {
  std::uint32_t entry_esp;
  SourcePhysicsBodyFactoryScratch factory;
  SourcePhysicsShapeWrapperScratch wrapper;
};
SourceBipedControllerScratch source_biped_controller_scratch_1_1(
    std::uint32_t entry_esp);
// Complete1DA6F0 using the current authored physical shape and real object,
// allocator, motion and component storage. entry_esp identifies caller-owned
// temporary calculation storage. No physical state, body or pose is supplied.
void source_construct_biped_controller_1_1(
    SourcePhysicsContactMemory &, const SourceBipedControllerRequest &,
    const SourceBipedControllerScratch &, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsReciprocalSqrtEstimate &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
