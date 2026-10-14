#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Whole original SDK factory, constructors, accounting/capability bodies and
// their CRT initialization rows. Platform settings, storage, synchronization,
// hardware and virtual methods remain actual required boundaries.
bool source_sound_sdk_tu15_owns_entry(std::uint32_t);
void source_run_sound_sdk_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
