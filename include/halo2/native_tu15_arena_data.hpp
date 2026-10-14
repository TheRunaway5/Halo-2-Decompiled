#pragma once
#include "halo2/source_runtime_memory.hpp"

namespace halo2 {
// Declare only the cold storage and immutable operands consumed by the
// complete physical/async/arena startup graph. The source constructors fill
// job links, handles, allocation cursors, CRC tables and completion state.
// No executable section or previously constructed world is an input.
void mount_native_tu15_arena_data(SourceRuntimeMemory&);
}
