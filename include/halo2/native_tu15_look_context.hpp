#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <functional>
namespace halo2 {
struct NativeTu15LookContextServices {
    // ActualCD4A0: ECX object datum, EAX destination, RET0. This camera
    // computation must execute fully, including all of its FP and writes.
    std::function<void(std::uint32_t object,std::uint32_t destination,
        NativeTu15PhysicsFrameFp&)> camera_position;
};
struct NativeTu15LookContextStats {
    std::uint64_t calls{},completed{},refusals{},camera_calls{},base_position_calls{};
};
struct NativeTu15CameraOriginStats {
    std::uint64_t calls{},parent_steps{},actor_points{},world_point_fallbacks{};
};
// ActualBBF20, including every source parent/actor read and BBE00 fallback.
// Source formal object is EAX; destination is ECX. Returns destination EAX.
std::uint32_t native_tu15_object_camera_origin(SourcePhysicsContactMemory&,
    std::uint32_t object,std::uint32_t destination,NativeTu15PhysicsFrameFp&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15CameraOriginStats* = nullptr);
// Whole65-site189FD0 integer parent. Index is the ordinary source EAX
// control-record index; context is24 caller-owned DATA bytes (ESI in source).
// The CD4A0 camera dependency remains explicit. No camera writes are omitted.
void native_tu15_build_look_context(SourcePhysicsContactMemory&,
    std::uint32_t index,std::uint32_t context,NativeTu15PhysicsFrameFp&,
    const NativeTu15LookContextServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15LookContextStats* = nullptr);
} // namespace halo2
