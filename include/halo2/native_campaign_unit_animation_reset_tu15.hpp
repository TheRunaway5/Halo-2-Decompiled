#pragma once
#include "halo2/physics_contact_record.hpp"
namespace halo2 {
// Original116920: invalidate/reset the actual extension+9C/+BC channels,
// preserving all other extension bytes and the source publication order.
void native_campaign_unit_animation_reset_tu15(SourcePhysicsContactMemory &,
    std::uint32_t unit);
}
