#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Whole1770C0/1772C0 particle admission/submission and1E0F0/1E4C0
// transparent-command storage owners. Native view, original renderer storage
// and downstream shader/geometry owners remain explicit required inputs.
bool source_effect_particle_draw_tu15_owns_entry(std::uint32_t);
void source_run_effect_particle_draw_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
