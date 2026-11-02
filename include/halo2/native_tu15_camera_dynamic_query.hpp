#pragma once
#include "halo2/native_tu15_camera_geometric_query.hpp"
#include "halo2/native_tu15_camera_collision_ray.hpp"
namespace halo2 {
struct NativeTu15CameraDynamicFrame {
    std::uint32_t collision_context{},ray_result{},material{};
    NativeTu15CameraCollisionRayScratch ray;
};
struct NativeTu15CameraDynamicServices {
    // Each active recursion owns separate semantic DATA. This allocator is
    // not a CPU stack/register service and never supplies a physics outcome.
    std::function<NativeTu15CameraDynamicFrame(unsigned)> acquire_frame;
};
struct NativeTu15CameraDynamicStats {
    std::uint64_t calls{},objects{},admission_calls{},sphere_calls{},context_calls{},
        ray_calls{},hits{},recursive_calls{},completed{},unbound_data_owners{};
};
// Actual16C370 with native admission/context/ray/material/publication children.
bool native_tu15_query_dynamic_camera_object(SourcePhysicsContactMemory&,
    const NativeTu15CameraDynamicQuery&,NativeTu15PhysicsFrameFp&,unsigned preserved_depth,
    const NativeTu15CameraDynamicServices&,const SourcePhysicsContactRecordObserver& = {},
    NativeTu15CameraDynamicStats* = nullptr);
} // namespace halo2
