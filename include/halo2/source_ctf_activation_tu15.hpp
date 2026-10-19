#pragma once
#include "halo2/physics_world_runtime.hpp"

namespace halo2 {
// Complete original TU1.5 CTF/Assault initializer, activation, registry and
// first-activation scalar owners on the retained native world bank. No engine
// index/identity/bank routing is replaced; unowned callees keep the full actual
// ABI and must resolve through the world's existing native dispatcher.
bool source_ctf_activation_tu15_owns_entry(std::uint32_t actual_entry);
void source_run_ctf_activation_tu15(SourcePhysicsContactMemory&,
    std::uint32_t actual_entry,SourcePhysicsWorldRuntimeAbi&,
    SourcePhysicsFactoryFloatingPoint,const SourcePhysicsWorldRuntimeServices&,
    const SourcePhysicsContactRecordObserver& = {});
} // namespace halo2
