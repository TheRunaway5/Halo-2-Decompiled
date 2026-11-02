#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
namespace halo2 {
// Source16B810 and186E30 ordinary current instance/definition DATA.
bool native_tu15_admit_camera_query_instance(const SourcePhysicsContactMemory&,
    std::uint32_t instance,std::uint32_t definition,std::uint32_t mask);
std::uint32_t native_tu15_camera_instance_collision_bitmap(const SourcePhysicsContactMemory&,
    std::uint32_t instance);
struct NativeTu15CameraInstanceQuery {
    std::uint32_t instance{},mask{},bsp_filter{},origin{},displacement{},result{};
};
struct NativeTu15CameraInstanceQueryScratch {
    std::uint32_t inverse{},local_origin{},local_displacement{},collision{},bsp_context{};
};
struct NativeTu15CameraInstanceQueryStats {
    std::uint64_t calls{},admissions{},sphere_calls{},inverse_calls{},bsp_calls{},hits{},completed{};
};
// Source16C000. Caller-owned numerical DATA is independent of CPU/control
// stack lifetimes. All selected children are genuine native kernels.
bool native_tu15_query_camera_instance(SourcePhysicsContactMemory&,
    const NativeTu15CameraInstanceQuery&,NativeTu15CameraInstanceQueryScratch,
    NativeTu15PhysicsFrameFp&,unsigned preserved_x87_depth,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15CameraInstanceQueryStats* = nullptr);
} // namespace halo2
