#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
namespace halo2 {
struct NativeTu15CameraCollisionRay {
    std::uint32_t context{},bsp_flags{},origin{},displacement{},result{};
};
struct NativeTu15CameraCollisionRayScratch {
    std::uint32_t inverse{},local_origin{},local_displacement{},bsp_context{},iterator{};
};
struct NativeTu15CameraCollisionRayStats {
    std::uint64_t calls{},regions{},inverse_calls{},bsp_calls{},hits{},completed{};
};
// Actual1F8200 composed with sole native region/inverse/BSP kernels. The
// caller owns separate initialized arithmetic DATA, not a source CPU frame.
// Result owns1060 bytes (8-byte match header followed by1052-byte BSP DATA).
bool native_tu15_query_camera_collision_ray(SourcePhysicsContactMemory&,
    const NativeTu15CameraCollisionRay&,NativeTu15CameraCollisionRayScratch,
    NativeTu15PhysicsFrameFp&,unsigned preserved_depth,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15CameraCollisionRayStats* = nullptr);
} // namespace halo2
