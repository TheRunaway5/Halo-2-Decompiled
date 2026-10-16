#pragma once
#include "halo2/physics_world_runtime.hpp"

namespace halo2 {
// Original query geometry, completion and visibility arithmetic. GPU query
// submission/results are actual native device services supplied by the caller.
bool source_lens_query_tu15_owns_entry(std::uint32_t);
void source_run_lens_query_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
