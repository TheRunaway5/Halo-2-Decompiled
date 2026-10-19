#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Complete original130890 frame age/limits, prediction, pressure and rollover
// owners. Actual physical cache, I/O, query and release services remain required.
bool source_geometry_cache_frame_tu15_owns_entry(std::uint32_t);
void source_run_geometry_cache_frame_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,const SourcePhysicsContactRecordObserver & = {});
}
