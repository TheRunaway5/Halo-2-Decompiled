#pragma once
#include "halo2/physics_world_runtime.hpp"
#include "halo2/source_object_audio_views_tu15.hpp"
namespace halo2 {
bool source_object_loop_audio_tu15_owns_entry(std::uint32_t);
// Complete original object-loop and pre-object audio-frame CPU owners. Missing
// native device/listener/resource ownership preserves the original stop; no
// readiness, source voice or platform result is supplied by this module.
void source_run_object_loop_audio_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {},
    const SourceObjectAudioViewsTu15* native_views=nullptr);
}
