#pragma once
#include "halo2/physics_world_runtime.hpp"
#include "halo2/source_runtime_memory.hpp"
namespace halo2 {
bool source_network_session_startup_tu15_owns_entry(std::uint32_t);
// Complete authenticated constructor bodies. Resource calls and other source
// modules retain their actual ABI and must have real native owners.
void source_run_network_session_startup_tu15(SourcePhysicsContactMemory&,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver& = {});
void mount_native_tu15_network_session_data(SourceRuntimeMemory&);
}
