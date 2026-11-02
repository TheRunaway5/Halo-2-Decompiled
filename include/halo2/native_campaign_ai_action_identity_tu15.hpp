#pragma once
#include "halo2/physics_contact_record.hpp"
namespace halo2 {
// Complete26C510. Four retained actor-history entries keep the actual raw
// flag byte. Missing entries advance the signed remainder ring; NONE is the
// real retained47D6C0 value, not a supplied action outcome.
void native_campaign_ai_action_history_tu15(SourcePhysicsContactMemory &,
    std::uint32_t actor,std::uint32_t goal,std::uint8_t flag,
    const SourcePhysicsContactRecordObserver & = {});
// Complete277D40 dynamic goal and26C850 scenario/dynamic goal definition.
// Return the real definition identity or the original zero refusal. There is
// no invented owner-index bound, fixed32-goal cap or pathability result.
std::uint32_t native_campaign_ai_dynamic_goal_definition_tu15(const SourcePhysicsContactMemory &,
    std::uint32_t goal);
std::uint32_t native_campaign_ai_goal_definition_tu15(const SourcePhysicsContactMemory &,
    std::uint32_t goal);
}
