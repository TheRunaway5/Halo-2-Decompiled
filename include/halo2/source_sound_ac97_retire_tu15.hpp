#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Actual teardown/stop/shared-pool CPU graph. Physical frees and raw-pool
// destruction remain required native owners; no completed free is supplied.
bool source_sound_ac97_retire_tu15_owns_entry(std::uint32_t);
void source_run_sound_ac97_retire_tu15(SourcePhysicsContactMemory&,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver& = {});
}
