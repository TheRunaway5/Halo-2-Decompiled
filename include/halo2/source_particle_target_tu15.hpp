#pragma once
#include "halo2/source_runtime_memory.hpp"
#include "halo2/physics_world_runtime.hpp"

namespace halo2 {
// Original14BA0 target/viewport selection,1CD10 target cache,1DCD0/1DD00
// retained target-record lookup and25940 active auxiliary-target selection.
// SDK attachment/header ownership and actual native target binding remain
// required inputs. No source physical pointer or GPU return is fabricated.
void mount_source_particle_target_data_tu15(SourceRuntimeMemory &);
bool source_particle_target_tu15_owns_entry(std::uint32_t);
void source_run_particle_target_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
