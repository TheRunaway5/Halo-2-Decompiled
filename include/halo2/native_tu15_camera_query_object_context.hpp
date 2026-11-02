#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <optional>
namespace halo2 {
struct NativeTu15CameraCollisionContext {
    std::uint32_t object{},model{},collision{},region_state{},node_matrices{};
};
// Actual1F7E00. Successful output is entirely initialized20B semantic DATA.
// Public numeric form preserves source alias/store/error order; native parent
// form returns the same fields as owned values, without a private GP frame.
bool native_tu15_resolve_camera_collision_context(SourcePhysicsContactMemory&,
    std::uint32_t object,std::uint32_t output,
    const SourcePhysicsContactRecordObserver& = {});
std::optional<NativeTu15CameraCollisionContext> native_tu15_camera_collision_context(
    SourcePhysicsContactMemory&,std::uint32_t object);
// Actual1F8040 encoded section/region/permutation pointer lookup.
std::uint32_t native_tu15_camera_collision_region(SourcePhysicsContactMemory&,
    std::uint32_t context,std::uint32_t encoded);
std::uint32_t native_tu15_camera_collision_region_value(SourcePhysicsContactMemory&,
    const NativeTu15CameraCollisionContext&,std::uint32_t encoded);
// Actual16BB40: source query packet kind1/3/4 and unknown/default zero.
std::uint32_t native_tu15_camera_query_collision_geometry(SourcePhysicsContactMemory&,
    std::uint32_t packet);
} // namespace halo2
