#pragma once
#include "halo2/physics_pool_host_storage.hpp"
namespace halo2 {
// Complete missing raw main heap producer. Source construction, size-class
// allocation and freeing stay in the existing full-ABI world runtime graph.
void source_run_physics_main_heap_runtime(SourcePhysicsContactMemory &,
    std::uint32_t canonical_entry, SourcePhysicsWorldRuntimeAbi &,
    SourcePhysicsContactImage, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
// TU1.5 source entry composition over this owner's fresh native pages. Supports
// 1482A0 plus the authenticated heap entries in the shared world graph. Does not
// fabricate renderer overflow storage, a mode-2 transition, TLS or CRT owners.
void source_run_physics_main_heap_with_native_storage(SourceRuntimeMemory &,
    std::uint32_t canonical_entry, SourcePhysicsWorldRuntimeAbi &,
    SourcePhysicsFactoryFloatingPoint, const SourcePhysicsWorldRuntimeServices &,
    SourcePhysicsPoolHostStorage &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
