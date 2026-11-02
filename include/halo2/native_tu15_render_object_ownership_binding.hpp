#pragma once
#include "halo2/physics_world_runtime.hpp"

namespace halo2 {
bool native_tu15_render_object_ownership_owns_entry(std::uint32_t);
// Complete successful source ABI transport for the four semantic kernels.
// The root caller retains journaling, executor admission and memory lifetime.
// Native algorithms receive data inputs, never this compatibility frame.
void native_tu15_render_object_ownership_run(SourcePhysicsContactMemory&, std::uint32_t,
    SourcePhysicsWorldRuntimeAbi&, const SourcePhysicsContactRecordObserver& = {});
std::uint64_t native_tu15_render_object_ownership_calls(std::uint32_t);
}
