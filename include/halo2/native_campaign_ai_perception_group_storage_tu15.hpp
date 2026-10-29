#pragma once
#include "halo2/physics_contact_record.hpp"
namespace halo2 {
// Complete274E30: the group's target object or a shared attached-unit owner.
std::uint32_t native_campaign_ai_perception_group_find_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t clump,std::uint32_t object);
// Complete274AE0: a positive group state or any positive member state.
bool native_campaign_ai_perception_group_active_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t group);
// Complete2660F0; copies the real default point through474CE8 and preserves
// unowned fields. Source/destination aliases retain their publication order.
void native_campaign_ai_perception_point_reset_tu15(
    SourcePhysicsContactMemory &,std::uint32_t point);
// Complete2745E0, including actual16E6A0 allocation/refusal and record fields.
// Returns a real perception datum; it does not attach it to an actor/group.
std::uint32_t native_campaign_ai_perception_property_create_tu15(
    SourcePhysicsContactMemory &,std::uint32_t group);
}
