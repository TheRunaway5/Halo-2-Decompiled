#pragma once
#include "halo2/source_runtime_memory.hpp"

namespace halo2 {
// Exact loader-zeroed TU1.5 PRT owner records, RGB SH channels and temporal
// fields. Actual22830 and23670 still reset/populate every runtime value.
// Cold composition only: any pre-existing overlapping owner is rejected.
void mount_native_tu15_prt_cache_data(SourceRuntimeMemory&);
}
