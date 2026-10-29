#pragma once
#include "halo2/physics_contact_record.hpp"
namespace halo2 {
// BACD0/FC510: real inherited marker owner and admitted pickup parent.
bool native_campaign_object_pickup_parent_tu15(const SourcePhysicsContactMemory &,std::uint32_t object);
std::uint32_t native_campaign_object_marker_owner_tu15(const SourcePhysicsContactMemory &,std::uint32_t object);
// BC200 and1D8C80/1D8D80 retain actual nullable destinations, signed motion
// admission and lazy read/publication order. These are data getters, not
// physics simulation or wake services.
void native_campaign_object_motion_component_tu15(SourcePhysicsContactMemory &,
    std::uint32_t component,std::uint32_t index,std::uint32_t destination,bool angular);
void native_campaign_object_motion_refs_tu15(SourcePhysicsContactMemory &,
    std::uint32_t object,std::uint32_t linear_destination,std::uint32_t angular_destination);
}
