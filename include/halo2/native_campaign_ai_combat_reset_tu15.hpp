#pragma once
#include "halo2/physics_contact_record.hpp"
namespace halo2 {
// Original1EC7B0 actor combat/perception mode transition. Source data and
// engine policy constants remain lazy inputs; query retirement is real.
void native_campaign_ai_combat_reset_tu15(SourcePhysicsContactMemory &,
    std::uint32_t actor,std::int16_t mode);
// Original266490 shared by combat reset and tracking-pressure allocation.
// Both arguments are actual datums; query deletion and cache effects are real.
void native_campaign_ai_tracking_retire_tu15(SourcePhysicsContactMemory &,
    std::uint32_t actor,std::uint32_t perception);
}
