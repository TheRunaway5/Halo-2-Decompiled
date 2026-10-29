#pragma once
#include "halo2/physics_contact_record.hpp"
namespace halo2 {
// Actual267750: tracked actor/state or untracked perception availability.
bool native_campaign_ai_perception_available_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t perception);
// Actual2661F0: reset tracking extension and the current group state bytes.
void native_campaign_ai_perception_reset_tu15(
    SourcePhysicsContactMemory &,std::uint32_t datum);
}
