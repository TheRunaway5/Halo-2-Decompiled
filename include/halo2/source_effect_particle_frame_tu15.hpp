#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Complete original179DF0/177830/1778D0 frame owners over the retained
// effect/particle bank. Frame delta is an explicit caller input; downstream
// owners preserve their original ABI and fail when no real owner is bound.
// Original22C440/22C480 also construct/reset the per-frame audio state;
// its arena carve and checksum follow the original source instructions.
// Whole3F3E0 publishes the completed portal collector's visible-cluster
// union. The frame reset and native173130 input publication are callers.
bool source_effect_particle_frame_tu15_owns_entry(std::uint32_t);
void source_run_effect_particle_frame_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
