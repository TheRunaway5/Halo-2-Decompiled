#pragma once
#include "halo2/physics_contact_runtime.hpp"

namespace halo2 {
// Complete authenticated process-heap startup, constructor, segment, descriptor
// and SEH/copy bodies. Startup includes its earlier shutdown/DPC/timer gates;
// missing platform/data owners stop before the installed-heap store.
// Source calls retain their real32-bit ABI. Kernel resources,
// synchronization and any other missing callee require explicit owners.
void source_run_process_heap_runtime(SourcePhysicsContactMemory &,
    std::uint32_t canonical_entry, SourcePhysicsContactRuntimeAbi &,
    SourcePhysicsContactImage, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
