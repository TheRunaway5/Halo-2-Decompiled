#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
namespace halo2 {
struct NativeTu15LookReorientationStats {
    std::uint64_t calls{},completed{},yaw_wraps{},refusals{};
};
// Actual18A2A0. Forward is ordinary12B completed direction DATA, not a GP
// file. The source record is reloaded lazily for yaw comparison/wrapping.
// Native x87/SSE math admits masked exceptions and at most4 older x87 values.
void native_tu15_reorient_look_controller(SourcePhysicsContactMemory&,
    std::uint32_t control_index,std::uint32_t forward_data,
    NativeTu15PhysicsFrameFp&,unsigned actual_preserved_x87_depth,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15LookReorientationStats* = nullptr);
} // namespace halo2
