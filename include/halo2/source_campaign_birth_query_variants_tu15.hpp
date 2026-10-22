#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Whole284300 selector and all absent original birth/update query variants.
// Borrow the source-produced query context; indirect callbacks stay required.
bool source_campaign_birth_query_variants_tu15_owns_entry(std::uint32_t);
void source_run_campaign_birth_query_variants_tu15(SourcePhysicsContactMemory&,
    std::uint32_t,SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver& = {});
}
