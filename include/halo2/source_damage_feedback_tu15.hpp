#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Complete original1F5620 and E3E20, reached from E0DDD and E0DE6.
// 1F5620: ECX is object+3EC; the unit is a stack argument.
// E3E20: EAX is the unit; the original caller pushes the raw DWORD flag1.
// Every lower physics/model service remains a required actual owner.
bool source_damage_feedback_tu15_owns_entry(std::uint32_t);
void source_run_damage_feedback_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
