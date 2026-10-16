#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
bool source_unit_cinematic_tu15_owns_entry(std::uint32_t);
void source_run_unit_cinematic_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,const SourcePhysicsContactRecordObserver & = {});
class PhysicsLiveWorldTu15;
// Original2AD950's signed-short duration conversion and111080 caller ABI.
// The retained source unit/graph owns facial channels and their per-tick update.
void source_unit_cinematic_emotion_tu15(PhysicsLiveWorldTu15 &,std::uint32_t object,
    std::uint32_t name,float weight,std::int16_t frames);
float source_unit_cinematic_emotion_seconds_tu15(std::int16_t frames,float source_factor);
}
