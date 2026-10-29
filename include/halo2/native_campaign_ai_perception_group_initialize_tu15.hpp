#pragma once
#include "halo2/physics_contact_record.hpp"
namespace halo2 {
// Complete1E7810 for the campaign alliance table, ordinary modes, and the
// absent multiplayer-engine object. A real unknown multiplayer virtual+6C
// identity stops at its callback boundary; no alliance result is supplied.
bool native_campaign_ai_perception_team_relation_tu15(
    const SourcePhysicsContactMemory &,std::int16_t first,std::int16_t second);
// Complete1E78F0: campaign-only, non-NONE/distinct teams, A4 policy bit set
// and C4 bit clear. Preserve the conditional second bitset access.
bool native_campaign_ai_perception_team_secondary_relation_tu15(
    const SourcePhysicsContactMemory &,std::int16_t first,std::int16_t second);
//274020: actual target header/type/attachment fields, campaign alliance policy
// and2660F0 point initialization. No allocator or actor link is synthesized.
void native_campaign_ai_perception_group_initialize_tu15(
    SourcePhysicsContactMemory &,std::uint32_t record,std::uint32_t object,
    std::uint32_t clump_record,std::int16_t kind,std::uint16_t priority);
}
