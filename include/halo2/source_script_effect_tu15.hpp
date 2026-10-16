#pragma once
#include "halo2/physics_world_runtime.hpp"

namespace halo2 {
// Complete authenticated2AA360/179700/1791C0 bodies. Marker lookup and effect
// allocation remain required original owners with their full retained ABI.
bool source_script_effect_tu15_owns_entry(std::uint32_t);
void source_run_script_effect_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
class PhysicsLiveWorldTu15;
// Native script caller on the same source world, including its actual marker
// producer/allocator, stack, TLS and FP bank. Rendering remains a separate
// consumer; no effect, location, PVS, event clock or RNG result is supplied.
void source_script_marker_effect_tu15(PhysicsLiveWorldTu15 &,
    std::uint32_t definition,std::uint32_t object,std::uint32_t marker);
}
