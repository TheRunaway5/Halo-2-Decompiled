#pragma once
#include "halo2/physics_world_runtime.hpp"
#include "halo2/source_runtime_memory.hpp"
namespace halo2 {
bool source_cache_startup_tu15_owns_entry(std::uint32_t);
void source_run_cache_startup_tu15(SourcePhysicsContactMemory&,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver& = {});
void mount_native_tu15_cache_startup_data(SourceRuntimeMemory&);
// Original unset cached language and the bounded11F440 conversion table.
// The ordinary getter/query path owns resolving and publishing the locale.
void mount_native_tu15_game_language_storage(SourceRuntimeMemory&);
}
