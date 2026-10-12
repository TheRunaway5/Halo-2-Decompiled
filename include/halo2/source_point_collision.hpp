#pragma once
#include "halo2/physics_body_factory.hpp"
namespace halo2 {
// Complete14B9D0 packed BSP classifier using actual node/plane pointers.
std::uint32_t source_point_collision_leaf_1_1(
    const SourcePhysicsContactMemory &, std::uint32_t bsp, std::uint32_t point,
    std::uint32_t root, SourcePhysicsFactoryFloatingPoint);
// Complete143E70 inverse of the retained source34-byte transform.
void source_point_collision_inverse_1_1(
    SourcePhysicsContactMemory &, std::uint32_t matrix, std::uint32_t point,
    std::uint32_t output, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRecordObserver & = {});
// Complete16A430 checks authored instance collision/property and runtime bank.
bool source_point_collision_instance_admitted_1_1(
    const SourcePhysicsContactMemory &, std::uint32_t definition,
    std::uint32_t instance_index, std::uint32_t flags);
bool source_point_collision_instance_1_1(
    SourcePhysicsContactMemory &, std::uint32_t instance_index,
    std::uint32_t point, std::uint32_t flags, std::uint32_t local_point_scratch,
    SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRecordObserver & = {});
struct SourcePointCollisionScratch {
  std::uint32_t record_pointer, iterator, instance_local_point;
};
// Actual16A9C0 locals and its16A7E0 inverse-transform output packet.
SourcePointCollisionScratch
source_point_collision_scratch_1_1(std::uint32_t entry_esp);
struct SourcePointCollisionObjectRequest {
  std::uint32_t object_datum, flags, point, excluded_a, excluded_b;
};
struct SourcePointCollisionServices {
  // Actual16A8A0 sibling/child and1F3500/1F3780 model containment owner.
  // Required only for an admitted object candidate; no miss is inferred.
  std::function<bool(const SourcePointCollisionObjectRequest &)> object;
};
// Complete16A9C0 terrain/fog/instance/spatial arbitration with actual source
// banks. Mask/type/exclusion semantics are retained. No triangle ray or host
// sphere is substituted for the original geometry containment query.
bool source_point_collision_1_1(
    SourcePhysicsContactMemory &, std::uint32_t flags, std::uint32_t point,
    std::uint32_t excluded_a, std::uint32_t excluded_b,
    const SourcePointCollisionScratch &, SourcePhysicsFactoryFloatingPoint,
    const SourcePointCollisionServices & = {},
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
