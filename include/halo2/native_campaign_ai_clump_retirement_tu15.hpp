#pragma once
#include "halo2/physics_contact_record.hpp"
#include <cstdint>
#include <functional>

namespace halo2 {
struct NativeCampaignAiReferenceInvalidationTu15 {
    std::uint32_t callback, actor, behavior_record, reference;
};
using NativeCampaignAiReferenceInvalidationServiceTu15 =
    std::function<void(const NativeCampaignAiReferenceInvalidationTu15 &)>;

// The original clump teardown owns intrusive actor/perception membership and
// pool reclamation. A real behavior callback remains an explicit dependency;
// an absent service throws at that boundary after the preceding real changes.
void native_campaign_ai_clump_retirement_tu15(SourcePhysicsContactMemory &,
    std::uint32_t clump,
    const NativeCampaignAiReferenceInvalidationServiceTu15 & = {});
// Complete274800 child ownership, with the actual raw clump record and group
// datum. The caller supplies its real native invalidation owner; an unknown
// callback still stops at the existing descriptor boundary.
void native_campaign_ai_perception_group_retirement_tu15(SourcePhysicsContactMemory &,
    std::uint32_t clump_record,std::uint32_t group,
    const NativeCampaignAiReferenceInvalidationServiceTu15 &);
}
