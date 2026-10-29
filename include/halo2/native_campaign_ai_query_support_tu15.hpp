#pragma once
#include "halo2/physics_contact_record.hpp"
namespace halo2 {
// Actual278650. Refuses actors without their authored perception group;
// otherwise allocates and initializes the real salted query-group record.
std::uint32_t native_campaign_ai_query_create_tu15(SourcePhysicsContactMemory &,
    std::uint32_t actor);
// Actual2789D0. Consumes an already reserved actor slot and publishes the
// behavior's actual group/member, including the group's reference increment.
bool native_campaign_ai_query_attach_tu15(SourcePhysicsContactMemory &,
    std::uint32_t actor,std::int16_t slot,std::uint32_t behavior);
// Actual278DD0. Advances iterator{actor,index,kind}, returning a still-live
// matching actor-slot address or zero. Source signed index/expiry rules apply.
std::uint32_t native_campaign_ai_query_next_tu15(SourcePhysicsContactMemory &,
    std::uint32_t iterator);
}
