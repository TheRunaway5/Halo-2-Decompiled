#pragma once
#include "halo2/source_runtime_image.hpp"

namespace halo2 {
// Cold, individually declared data for the existing TU1.5 world constructor.
// No file is opened and no constructor is run. The ordinary world executor
// still owns its stack, TLS, native heap/arena services and selective startup
// calls. Logical 32-bit source identities remain distinct from host addresses.
// An undeclared operand remains an error; this is not a sparse XBE image.
SourceRuntimeMemory native_tu15_world_memory(const SourceRuntimeKernelTickCount&);
// Admit the complete native object-type runtime owner before public restore.
// Offline image banks retain their existing whole-section ownership.
void native_tu15_validate_type_runtime_restore(const SourceRuntimeMemory&,
    const SourceRuntimeMemory::Checkpoint&);
}
