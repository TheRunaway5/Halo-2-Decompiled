#pragma once
#include "halo2/physics_contact_record.hpp"
namespace halo2 {
// Real1EB730. Resolve an object's actor datum through its native object-kind
// and optional type12 extension. No substitute current-actor lookup is used.
std::uint32_t native_campaign_ai_object_actor_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t object);
}
