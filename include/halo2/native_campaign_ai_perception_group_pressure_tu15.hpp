#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
namespace halo2 {
// Whole274190. The clump is a datum; each group below minimum_priority is
// retired through its genuine native invalidation/deletion owners.
std::uint16_t native_campaign_ai_perception_group_prune_tu15(
    SourcePhysicsContactMemory &,std::uint32_t clump,std::int16_t minimum_priority);
// Whole274200. The requested clump is a retained record pointer. Allocation
// pressure raises the original candidate's threshold and prunes real groups;
// refusal remains FFFFFFFF. No fabricated free slot or behavior callback.
std::uint32_t native_campaign_ai_perception_group_allocate_tu15(
    SourcePhysicsContactMemory &,std::uint32_t clump_record,
    std::int16_t priority,NativeTu15PhysicsFrameFp &);
struct NativeCampaignAiPerceptionGroupRequestTu15 {
    std::int16_t kind;
    std::uint32_t object;
    std::uint16_t priority;
};
// Whole274490 through real274020. The typed profile is its actual disjoint
// private caller request; the address form retains real request/header aliases.
std::uint32_t native_campaign_ai_perception_group_construct_tu15(
    SourcePhysicsContactMemory &,std::uint32_t clump_record,
    const NativeCampaignAiPerceptionGroupRequestTu15 &,NativeTu15PhysicsFrameFp &);
std::uint32_t native_campaign_ai_perception_group_construct_tu15(
    SourcePhysicsContactMemory &,std::uint32_t clump_record,
    std::uint32_t request,NativeTu15PhysicsFrameFp &);
}
