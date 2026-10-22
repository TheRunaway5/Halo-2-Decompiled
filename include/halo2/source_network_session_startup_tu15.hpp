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
// Shared loader-zero configuration storage and the immutable inputs of680C0.
// This does not execute a constructor or publish a configured network session.
void mount_native_tu15_network_configuration_storage(SourceRuntimeMemory&);
// Complete324-byte loader-zero destination of53240,including its initialized
// byte at4D61D8. Preserve the canonical live owner;no startup result is supplied.
void mount_native_tu15_network_saved_session_storage(SourceRuntimeMemory&);
// Shared one-byte cold allocation. Preserve a live canonical owner; this does
// not initialize or publish a network session.
void mount_native_tu15_network_session_presence_storage(SourceRuntimeMemory&);
}
