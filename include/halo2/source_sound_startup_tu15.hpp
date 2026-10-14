#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Complete original TU1.5 driver/environment/mapping/feedback CPU owners.
// Full real native section, device, SDK and DSP services remain mandatory
// boundaries. Calling an owner cannot supply an initialized flag or audio.
bool source_sound_startup_tu15_owns_entry(std::uint32_t);
void source_run_sound_startup_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
