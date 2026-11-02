#pragma once
#include "halo2/native_campaign_transition_request_tu15.hpp"

namespace halo2 {
// Actual1ACDE0 format, passed by191750 to the real save/archive owner.
// This constructs metadata only; it does not acknowledge a save or session.
struct NativeCampaignTransitionSaveMetadataTu15 {std::array<std::byte,0x33b> bytes;};
void native_campaign_transition_save_metadata_tu15(SourcePhysicsContactMemory &,
    std::uint32_t retained_options,std::uint32_t destination);
// Authenticated191799 private/disjoint input and output need no guest stack.
void native_campaign_transition_save_metadata_value_tu15(SourcePhysicsContactMemory &,
    const NativeCampaignTransitionOptionsTu15 &,NativeCampaignTransitionSaveMetadataTu15 &);
// Complete1AD350 and its two call-free15/16-byte subrecord constructors.
// Source0 selects the original zero/default format, not fabricated profile data.
// Retained forms preserve ordinary forward-copy aliases and lazy accesses.
void native_campaign_transition_profile_metadata_tu15(SourcePhysicsContactMemory &,
    std::uint32_t source_profile,std::uint32_t destination);
void native_campaign_transition_profile_subrecord15_tu15(SourcePhysicsContactMemory &,
    std::uint32_t source,std::uint32_t destination);
void native_campaign_transition_profile_subrecord16_tu15(SourcePhysicsContactMemory &,
    std::uint32_t source,std::uint32_t destination);
}
