#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Whole20B980 and its inherited-character lookup1ECC80, reached by the
// actual1E8BBF placement caller. Original options, tag records and output
// storage are required; no difficulty value or lookup result is supplied.
bool source_campaign_ai_placement_settings_tu15_owns_entry(std::uint32_t);
void source_run_campaign_ai_placement_settings_tu15(SourcePhysicsContactMemory &,
    std::uint32_t,SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
