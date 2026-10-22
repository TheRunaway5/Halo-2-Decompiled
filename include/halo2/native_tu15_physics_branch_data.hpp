#pragma once
#include "halo2/source_runtime_memory.hpp"

namespace halo2 {
// Verified immutable selector/branch, code-reference and fixed operand data
// for compiled TU1.5 physics. Callable prefixes do not infer full class layouts.
// Call on a cold, quiescent bank. Existing owners must be readonly and exact;
// no executable, tag payload, mutable state or computed result is supplied.
// Includes exact fixed readonly operands consumed by the compiled AI frame.
void mount_native_tu15_physics_branch_data(SourceRuntimeMemory&);
}
