#pragma once
#include "halo2/physics_contact_record.hpp"

namespace halo2 {
// Complete606A0 retained peer/link cleanup. This neither closes a network
// connection nor supplies a parent session/UI result. The original gated
// peer scan, byte flag update and redundant NONE/reset stores remain lazy.
void native_campaign_session_clear_peer_tu15(SourcePhysicsContactMemory &,
    std::uint32_t session,std::uint32_t peer_index);
// Complete7E7F0 descriptor predicate. Its signed kind chooses exactly the
// original firstDWORD/eightWORD query, with the authentic null fast path.
bool native_campaign_session_address_present_tu15(const SourcePhysicsContactMemory &,
    std::uint32_t descriptor);
// Complete599B0 immutable source-table policy. Phases7/8 return the raw
// retained byte, including noncanonical nonzero values; no bool normalization.
std::uint8_t native_campaign_session_phase_policy_tu15(const SourcePhysicsContactMemory &,
    std::uint32_t session);
}
