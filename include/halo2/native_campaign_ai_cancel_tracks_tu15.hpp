#pragma once
#include "halo2/physics_contact_record.hpp"
namespace halo2 {
// Complete2755B0: actual property tracks with status1/2, excluding one actor.
bool native_campaign_ai_prop_has_other_active_track_tu15(const SourcePhysicsContactMemory &,
    std::uint32_t property,std::uint32_t excluded_actor);
// Complete274F40: find the actual property for one object in a real group.
std::uint32_t native_campaign_ai_group_find_object_prop_tu15(const SourcePhysicsContactMemory &,
    std::uint32_t group,std::uint32_t object);
// Actual217310 policy class used by the campaign path. Mode2 requires its
// real native game-engine callback and remains unavailable; no result is
// supplied for it. The original early sentinels and other modes stay exact.
std::uint16_t native_campaign_ai_group_policy_class_tu15(const SourcePhysicsContactMemory &,
    std::int16_t team);
// Actual275610 with real non-mode2 policy. A disabled->enabled concurrent
// transition requires the original's unavailable uninitialized iterator
// owner, and fails explicitly instead of creating a private source stack.
bool native_campaign_ai_prop_track_conflict_campaign_tu15(const SourcePhysicsContactMemory &,
    std::uint32_t property,std::uint32_t excluded_actor,std::uint32_t group);
}
