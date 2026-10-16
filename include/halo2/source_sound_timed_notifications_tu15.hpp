#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Whole authenticated timed notification CPU bodies. Native clock/timer and
// actual voice virtual owners remain mandatory; playing PIO paths are unowned.
bool source_sound_timed_notifications_tu15_owns_entry(std::uint32_t);
void source_run_sound_timed_notifications_tu15(SourcePhysicsContactMemory&,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver& = {});
}
