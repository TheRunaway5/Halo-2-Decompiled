#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Complete unchanged1296E0 metadata predicate. No driver, cache, device or
// successful resource outcome is created by this query.
bool source_sound_cache_query_tu15_owns_entry(std::uint32_t);
void source_run_sound_cache_query_tu15(SourcePhysicsContactMemory &, std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &, const SourcePhysicsContactRecordObserver & = {});
}
