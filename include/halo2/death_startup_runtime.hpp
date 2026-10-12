#pragma once
#include "halo2/death_object_phases.hpp"

namespace halo2 {
// Complete TU1.1 physical stage, game arena, object header/body-heap and type
// constructors, plus their original async queue/worker/file callbacks. The
// selected image supplies their literal globals and method tables. Actual
// resource and platform owners retain the full ABI; missing owners stop with
// preceding writes intact. No captured banks, pool validity, clock or success
// outputs are supplied. Call order remains with the original startup caller.
void source_run_death_startup_1_1(
    SourcePhysicsContactMemory &, std::uint32_t canonical_entry,
    SourceUnitAnimationRuntimeAbi &, SourcePhysicsFactoryFloatingPoint,
    const SourceDeathWorldSchedulerServices &,
    const SourcePhysicsContactRecordObserver & = {});
bool source_death_startup_owns_entry_1_1(std::uint32_t canonical_entry);
} // namespace halo2
