#pragma once
#include "halo2/unit_animation_selection_runtime.hpp"

namespace halo2 {
struct SourceDeathWorldSchedulerServices {
  SourceUnitAnimationRuntimeServices runtime;
  // Original reached RDTSC profiling instructions require a real clock owner.
  std::function<std::uint64_t()> timestamp_counter;
};
// Complete compiled function identities, including the linked solver/contact
// owners. This checks ownership only; it does not test world readiness.
bool source_death_world_owns_entry_1_1(std::uint32_t canonical_entry);
// Complete1C7660 world step and its linked deferred-body/solver/contact owners.
// Current retained source world, component, heap, thread and profiler banks
// are required. Reached unknown virtual/platform owners keep their actual ABI.
void source_run_death_world_scheduler_1_1(
    SourcePhysicsContactMemory &, std::uint32_t canonical_entry,
    SourceUnitAnimationRuntimeAbi &, SourcePhysicsFactoryFloatingPoint,
    const SourceDeathWorldSchedulerServices &,
    const SourcePhysicsContactRecordObserver & = {});
// The original entry has no arguments. Caller scratch and FS base remain
// explicit; scheduling never constructs or marks a missing world as ready.
void source_step_death_physics_world_1_1(
    SourcePhysicsContactMemory &, std::uint32_t entry_esp,
    std::uint32_t fs_base, SourcePhysicsFactoryFloatingPoint,
    const SourceDeathWorldSchedulerServices &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
