#pragma once
#include "halo2/physics_contact_record.hpp"
namespace halo2 {
// Whole1D0430 lookup in the two actual actor timer records at5020A4.
std::uint32_t native_campaign_ai_actor_timer_tu15(const SourcePhysicsContactMemory &,
    std::uint32_t actor);
// Whole275AB0 retained iterator publication, plus the disjoint native-local
// result form used by semantic callers. Neither chooses a group or actor.
std::uint32_t native_campaign_ai_group_first_member_tu15(const SourcePhysicsContactMemory &,
    std::uint32_t group);
void native_campaign_ai_group_members_begin_tu15(SourcePhysicsContactMemory &,
    std::uint32_t group,std::uint32_t iterator,
    const SourcePhysicsContactRecordObserver & = {});
}
