#pragma once
#include "halo2/physics_contact_record.hpp"
namespace halo2 {
struct NativeCampaignAiOrdersFp {std::uint32_t mxcsr=0x1f80u;};
// Whole actor camo-control1EB590, including the original unit deactivation
// D3340 and dirty-entity B7E50 gate. Borrow retained actor/unit/session owners;
// return false only for actor.unit==NONE. No action or network result supplied.
bool native_campaign_ai_orders_camouflage_tu15(SourcePhysicsContactMemory &,
    std::uint32_t actor,bool enabled,NativeCampaignAiOrdersFp &);
}
