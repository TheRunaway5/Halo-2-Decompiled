#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Complete original12A810 listener selection and its four closed CPU callees.
// Reads the caller's actual listener, request and BSP banks; supplies no device result.
bool source_sound_listener_selection_tu15_owns_entry(std::uint32_t);
void source_run_sound_listener_selection_tu15(SourcePhysicsContactMemory &, std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &, const SourcePhysicsContactRecordObserver & = {});
}
