#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Complete original TU1.5 per-view light fade and datum iteration. No source
// visibility query, selected light queue or cached command publication.
bool source_effect_light_view_tu15_owns_entry(std::uint32_t);
void source_run_effect_light_view_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
