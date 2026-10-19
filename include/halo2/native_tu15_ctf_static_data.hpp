#pragma once
#include "halo2/source_runtime_memory.hpp"
namespace halo2 {
// Original file-backed class dispatch cell/full interface plus loader-zeroed
// bank-pointer destination. Call once during quiescent cold-world allocation;
// no constructor, bank initialization, callback result or mode is supplied.
void mount_native_tu15_ctf_static_data(SourceRuntimeMemory&);
}
