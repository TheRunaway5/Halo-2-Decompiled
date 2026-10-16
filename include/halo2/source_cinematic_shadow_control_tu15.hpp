#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
bool source_cinematic_shadow_control_tu15_owns_entry(std::uint32_t);
void source_run_cinematic_shadow_control_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,const SourcePhysicsContactRecordObserver & = {});
class PhysicsLiveWorldTu15;
// Original2ACCA0 ->30E10 argument composition. Original30E10 owns all
// sixteen-slot insertion/replacement/removal decisions. Rendering is separate.
void source_cinematic_shadow_control_tu15(PhysicsLiveWorldTu15 &,bool enabled,
    std::uint32_t object,std::uint32_t marker,float radius);
}
