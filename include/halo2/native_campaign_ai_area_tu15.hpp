#pragma once
#include "halo2/physics_contact_record.hpp"

namespace halo2 {
// Complete1C5870 area-availability query, including the actual flags5 iterator
// initialization/first-result path of20D180/20D2D0. Source records stay lazy;
// no geometric/pathfinding replacement or executable callback is involved.
bool native_campaign_ai_has_area_tu15(const SourcePhysicsContactMemory &,
    std::uint32_t actor);
//1B6F20 behavior candidate: read the actual default; an enabled actor with an
// eligible authored area selects behavior6B. Return the owed SHORT identifier.
std::uint16_t native_campaign_ai_area_behavior_tu15(const SourcePhysicsContactMemory &,
    std::uint32_t actor);
}
