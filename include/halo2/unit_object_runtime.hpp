#pragma once
#include "halo2/unit_animation_selection_runtime.hpp"

namespace halo2 {
// Complete TU1.1 request, generic/type/Unit construction, header/body heap,
// manager binding and pressure callers, linked to the existing complete
// animation/pose and weapon routing graphs through the same retained
// GP/stack/MMX/FP state.
// Memory must own the actual initialized banks. This does not create a second
// object pool, initialize a world, fabricate resource readiness or publish a
// native typed Unit. Missing lower owners stop with preceding writes retained.
void source_run_unit_object_runtime_1_1(SourcePhysicsContactMemory &,
    std::uint32_t entry, SourceUnitAnimationRuntimeAbi &,
    SourcePhysicsFactoryFloatingPoint,
    const SourceUnitAnimationRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
// Internal complete graph entry, exposed for isolated owner comparisons.
void source_run_unit_object_graph_1_1(SourcePhysicsContactMemory &,
    std::uint32_t entry, SourceUnitAnimationRuntimeAbi &,
    SourcePhysicsFactoryFloatingPoint,
    const SourceUnitAnimationRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
