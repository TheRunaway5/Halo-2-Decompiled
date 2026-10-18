#pragma once
#include "halo2/physics_world_runtime.hpp"

namespace halo2 {
// Complete unchanged TU1.5 environment producer, sky resolver and mixer
// lifecycle bodies. Calling these requires real source tag/world backing and
// the process/map/frame lifecycle owners; this API does not admit a view.
bool source_campaign_environment_tu15_owns_entry(std::uint32_t);
void source_run_campaign_environment_tu15(SourcePhysicsContactMemory &,
    std::uint32_t, SourcePhysicsWorldRuntimeAbi &,
    SourcePhysicsFactoryFloatingPoint, const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
