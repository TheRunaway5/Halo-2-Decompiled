#pragma once
#include "halo2/source_runtime_memory.hpp"

namespace halo2 {
// Retain already executed native particle state in an unpublished, complete
// authority preparation at the same gameplay tick. Both executors must be
// quiescent and have the same admitted resources, BSP and native pool backing
// layout. Does not create effects, advance kernels or import foreign handles.
// This covers the six particle/effect pools and RNG pair, not other effect
// subowners (lights, beams, contrails, decals or sound).
void source_stage_native_particle_history_tu15(const SourceRuntimeMemory &current,
    SourceRuntimeMemory &replacement);
}
