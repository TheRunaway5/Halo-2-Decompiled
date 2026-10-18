#pragma once
#include "halo2/physics_world_runtime.hpp"

namespace halo2 {
// Complete original21F590, including its codec-domain guard and x87 C1 effects.
// This leaf keeps the existing source-memory/caller ABI and does not supply
// MCC reference operands, PCM duration, resource readiness or a voice owner.
void source_run_sound_duration_tu15(SourcePhysicsContactMemory &,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
