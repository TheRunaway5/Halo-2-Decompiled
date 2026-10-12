#pragma once
#include "halo2/death_world_scheduler.hpp"

namespace halo2 {
// Complete linked function identity coverage; this does not test readiness.
bool source_death_object_phases_owns_entry_1_1(std::uint32_t canonical_entry);

// The complete original active-mask, prephysics, world, postphysics and new
// object flag phases share the caller's source object/animation/physics banks.
// Reached resource, platform and unknown type owners retain their original ABI.
void source_run_death_object_phases_1_1(
    SourcePhysicsContactMemory &, std::uint32_t canonical_entry,
    SourceUnitAnimationRuntimeAbi &, SourcePhysicsFactoryFloatingPoint,
    const SourceDeathWorldSchedulerServices &,
    const SourcePhysicsContactRecordObserver & = {});

// Executes the original five-phase order over one persistent bank. The game
// clock must already have been advanced by its actual tick owner. This neither
// initializes a world nor resets the bank between phases or ticks.
void source_step_death_objects_1_1(
    SourcePhysicsContactMemory &, std::uint32_t entry_esp,
    std::uint32_t fs_base, SourcePhysicsFactoryFloatingPoint,
    const SourceDeathWorldSchedulerServices &,
    const SourcePhysicsContactRecordObserver & = {});
// Exact139863..13986B game-clock increment fragment used by the original
// caller before these phases. Earlier/later game-tick owners are outside this
// API; the current clock bank must already exist.
void source_advance_death_object_clock_1_1(
    SourcePhysicsContactMemory &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
