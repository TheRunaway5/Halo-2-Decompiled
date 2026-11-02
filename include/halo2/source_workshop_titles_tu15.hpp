#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Complete delayed setter13ECC0, HS evaluator2B3620 and lifecycle13F160.
// The caller supplies the actual shared cinematic/clock/scenario banks and
// the real HS parse/completion owners. This interface does not admit scripts
// or provide title rendering/scheduling by itself.
bool source_workshop_titles_tu15_owns_entry(std::uint32_t);
void source_run_workshop_titles_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,const SourcePhysicsContactRecordObserver & = {});
}
