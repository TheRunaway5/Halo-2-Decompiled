#pragma once
#include "halo2/physics_world_runtime.hpp"

namespace halo2 {
// Complete original swarm2 controller and its four helpers. The caller owns
// the retained particle/location bank, property values, frame delta and real
// collision services. This module supplies no collision result or RNG seed.
bool source_particle_swarm_tu15_owns_entry(std::uint32_t);
void source_run_particle_swarm_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
