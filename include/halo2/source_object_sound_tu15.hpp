#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
bool source_object_sound_tu15_owns_entry(std::uint32_t);
// Complete TU1.5 object sound cleanup over the retained source bank. Native
// resource/audio boundaries remain required owners, rather than fake results.
void source_run_object_sound_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
