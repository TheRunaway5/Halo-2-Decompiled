#pragma once
#include "halo2/source_runtime_memory.hpp"

namespace halo2 {
// Exact SDK loader data and immutable operands; no SDK object, refcount,
// statistics binding, device result or capability is preconstructed.
// Existing immutable operands are verified and reused; cold destinations are
// exclusive. Discard an unsuccessful cold bank assembly before retrying.
void mount_native_tu15_sound_sdk_prefix_data(SourceRuntimeMemory&);
}
