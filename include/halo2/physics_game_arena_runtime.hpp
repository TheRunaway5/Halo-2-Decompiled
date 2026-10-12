#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Complete selected arena/async/CRC/allocator graph. Native resources must own
// their actual reached call ABI. No completion or allocator state is supplied.
void source_run_physics_game_arena_runtime(SourcePhysicsContactMemory &,
    std::uint32_t canonical_entry, SourcePhysicsWorldRuntimeAbi &,
    SourcePhysicsContactImage, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
