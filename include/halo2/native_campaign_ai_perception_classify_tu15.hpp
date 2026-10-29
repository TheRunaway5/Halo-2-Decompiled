#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <optional>
namespace halo2 {
struct NativeCampaignAiPerceptionClassificationTu15 {
    std::uint16_t kind{},priority{};
    std::uint32_t object{};
};
// Actual273B70 profile used by274450: real nonnull private output, no previous
// group, force=true. Still performs salted object admission, generation stamps,
// world point/separation arithmetic, authored type/team policy and attachment
// traversal. Unknown types leave the original private output unwritten;
// nullopt reports that boundary instead of inventing an uninitialized value.
std::optional<NativeCampaignAiPerceptionClassificationTu15>
native_campaign_ai_perception_classify_new_group_tu15(SourcePhysicsContactMemory &,
    std::uint32_t object,std::uint32_t clump_record,NativeTu15PhysicsFrameFp &);
}
