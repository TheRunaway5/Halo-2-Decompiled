#pragma once
#include "halo2/source_runtime_memory.hpp"
namespace halo2 {
// Three exact44-byte blend tables and one separate20-byte geometry dispatch
// table consumed by existing contrail readers. Cold/quiescent admission only;
// no executable, device or live state is imported, and mounting these words
// does not activate any effect.
void mount_native_tu15_contrail_draw_data(SourceRuntimeMemory&);
}
