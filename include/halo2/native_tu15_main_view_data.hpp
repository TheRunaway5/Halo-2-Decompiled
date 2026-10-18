#pragma once
#include "halo2/source_runtime_memory.hpp"

namespace halo2 {
// Precise TU1.5 main camera/projection/caller fields and per-player geometry
// policy destinations. Cold data only: submitting callers and genuine2FD70/
// 2C470 still publish camera, transforms, flags and scales. Alignment gaps,
// existing environment/frame owners and4C6709 are excluded.
void mount_native_tu15_main_view_data(SourceRuntimeMemory&);
}
