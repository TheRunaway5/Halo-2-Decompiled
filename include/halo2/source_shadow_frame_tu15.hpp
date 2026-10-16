#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Whole authenticated original TU1.5 shadow entry gates, candidate/history,
// root/frame/view and binding bodies. Actual cache, observer, secondary
// geometry and graphics services remain required native owners. No live
// shadow plan or matched-frame acceptance is implied by these CPU bodies.
bool source_shadow_frame_tu15_owns_entry(std::uint32_t);
void source_run_shadow_frame_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
