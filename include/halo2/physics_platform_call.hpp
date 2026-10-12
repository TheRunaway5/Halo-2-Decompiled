#pragma once
#include "halo2/physics_contact_runtime.hpp"
#include "halo2/physics_platform_locks.hpp"
#include "halo2/physics_world_runtime.hpp"

namespace halo2 {
// Native platform ABI for authenticated void, one-argument stdcall imports.
// Reads the actual pushed return/argument, validates the selected source caller,
// and retains incoming register/FP state while consuming the two stack words.
// This is an explicit native ABI, not recovered Xbox kernel register clobbers.
// Unsupported callers return false without changing source state. Missing
// thread/wait/wake ownership stops inside the semantic operation without return.
bool source_try_native_physics_lock_call(SourcePhysicsContactMemory &,
    const SourcePhysicsContactRuntimeCall &, SourcePhysicsContactRuntimeAbi &,
    SourcePhysicsContactImage, PhysicsPlatformLocks &,
    const PhysicsPlatformLockServices &, const PhysicsPlatformLockObserver & = {});
// Join native platform ownership to the complete source world graph. The
// caller still owns all memory, actual ABI, CPU services and remaining game
// callees. An unsupported owner delegates with its unchanged full call state.
void source_run_physics_world_with_native_platform(SourcePhysicsContactMemory &,
    std::uint32_t canonical_entry, SourcePhysicsWorldRuntimeAbi &,
    SourcePhysicsContactImage, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &, PhysicsPlatformLocks &,
    const PhysicsPlatformLockServices &,
    const SourcePhysicsContactRecordObserver & = {}, const PhysicsPlatformLockObserver & = {});
} // namespace halo2
