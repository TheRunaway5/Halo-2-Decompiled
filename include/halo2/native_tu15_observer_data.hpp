#pragma once
#include "halo2/source_runtime_memory.hpp"

namespace halo2 {
// Exact loader data used by the complete TU1.5 observer owners. This mounts
// storage and authored operands only. Original constructors, map callbacks
// and per-frame owners still produce every observer/reset/readiness value.
void mount_native_tu15_observer_data(SourceRuntimeMemory&);
}
