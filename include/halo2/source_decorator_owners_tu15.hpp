#pragma once
#include "halo2/physics_world_runtime.hpp"

namespace halo2 {
// Complete authenticated TU1.5 decorator cell, regular collector, descriptor,
// draw and authored initializer bodies. Unknown cache, shader, portal and GPU
// calls retain their required full caller ABI. This module supplies no ready
// result, camera, admitted-cell list or framebuffer acceptance.
bool source_decorator_owners_tu15_owns_entry(std::uint32_t);
void source_run_decorator_owners_tu15(SourcePhysicsContactMemory&,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices&,
    const SourcePhysicsContactRecordObserver& = {});
}
