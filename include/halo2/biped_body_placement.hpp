#pragma once
#include "halo2/biped_ragdoll_admission.hpp"

namespace halo2 {
// Complete1D3970 current-joint pivot delivery. The source copies its first
// pivot to both outputs; callers must not replace that with the second pivot.
void source_biped_constraint_pivots_1_1(
    SourcePhysicsContactMemory &, std::uint32_t constraint_record,
    std::uint32_t first, std::uint32_t second, std::uint32_t packet_scratch,
    const SourcePhysicsContactRecordObserver & = {});

// Complete31FFC0 motion transform,2E0860 quaternion and2E0BA0 COM updates.
// matrix40 must be the actual aligned source transform, not a render matrix.
void source_biped_motion_transform_1_1(
    SourcePhysicsContactMemory &, std::uint32_t motion, std::uint32_t matrix40,
    SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRecordObserver & = {});
struct SourceBipedBodyTransformServices {
  // Actual2E0980/2E0B60 inverse component correction when bit6 is set.
  std::function<void(std::uint32_t transform40, std::uint32_t component30,
                     std::uint32_t inverse40)>
      inverse_correction;
  std::function<void(std::uint32_t component)> remove, insert;
  std::function<void(std::uint32_t body, std::uint32_t transform40)>
      static_transform;
  std::function<void(std::uint32_t body)> activate;
};
// Complete1D46E0 and1D4610. Current motion vtable dispatch selects the actual
// 31FFC0 owner; unsupported motion classes require their own real owner.
void source_biped_body_transform40_1_1(
    SourcePhysicsContactMemory &, std::uint32_t component, std::uint32_t slot,
    std::uint32_t transform40, std::uint32_t entry_esp,
    SourcePhysicsFactoryFloatingPoint, const SourceBipedBodyTransformServices &,
    const SourcePhysicsContactRecordObserver & = {});
void source_biped_body_transform34_1_1(
    SourcePhysicsContactMemory &, std::uint32_t component, std::uint32_t slot,
    std::uint32_t transform34, std::uint32_t entry_esp,
    SourcePhysicsFactoryFloatingPoint, const SourceBipedBodyTransformServices &,
    const SourcePhysicsContactRecordObserver & = {});
// Complete1D47C0/1D4A00. The latter adds a velocity delta; it does not divide
// a physical impulse by mass. These dispatch the actual31FE90 motion owner.
void source_biped_body_velocity_1_1(
    SourcePhysicsContactMemory &, std::uint32_t component, std::uint32_t slot,
    std::uint32_t velocity3, std::uint32_t entry_esp,
    const SourceBipedBodyTransformServices &,
    const SourcePhysicsContactRecordObserver & = {});
void source_add_biped_body_velocity_1_1(
    SourcePhysicsContactMemory &, std::uint32_t component, std::uint32_t slot,
    std::uint32_t delta3, std::uint32_t entry_esp,
    SourcePhysicsFactoryFloatingPoint, const SourceBipedBodyTransformServices &,
    const SourcePhysicsContactRecordObserver & = {});
// Complete142F20 shortest-arc rotation and144620 alias-safe basis product.
void source_biped_placement_rotation_1_1(
    SourcePhysicsContactMemory &, std::uint32_t from, std::uint32_t to,
    std::uint32_t output34, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRecordObserver & = {});
void source_biped_placement_basis_product_1_1(
    SourcePhysicsContactMemory &, std::uint32_t left9, std::uint32_t right9,
    std::uint32_t output9, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRecordObserver & = {});

struct SourceBipedBodyPlacementServices {
  SourcePhysicsBodyMatrixCorrection correction;
  // Actual16A9C0 and16B240 collision owners on the current source world.
  std::function<bool(std::uint32_t point)> point;
  std::function<bool(std::uint32_t anchor, std::uint32_t direction,
                     std::uint32_t packet)>
      segment;
  // Actual1D4610 over the retained physical component and motion bank.
  std::function<void(std::uint32_t component, std::uint32_t slot,
                     std::uint32_t transform34)>
      set_transform;
};
// CompleteE3270: admission point check, root/body placement, joint pivots,
// source segment retreat and conditional orientation correction. All scratch
// addresses are derived from caller-owned original entry_esp. This is not a
// host capsule placement or a substitute for world insertion/solver owners.
std::uint8_t source_place_biped_body_1_1(
    SourcePhysicsContactMemory &, const SourceBipedPlacementRequest &,
    std::uint32_t entry_esp, SourcePhysicsFactoryFloatingPoint,
    const SourceBipedBodyPlacementServices &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
