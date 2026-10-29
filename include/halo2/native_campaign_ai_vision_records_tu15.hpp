#pragma once
#include "halo2/physics_contact_record.hpp"
#include <cstdint>
namespace halo2 {
struct NativeCampaignAiVisionAttachmentTu15 { bool present; std::uint16_t index; };
//26EEF0: first authored attachment whose packed flags contain an index.
// The optional retained output follows all reads, including the match reread.
NativeCampaignAiVisionAttachmentTu15 native_campaign_ai_vision_attachment_index_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t object);
bool native_campaign_ai_vision_attachment_index_store_tu15(
    SourcePhysicsContactMemory &,std::uint32_t object,std::uint32_t output);
//1ECCC0: first nonempty inherited character block, or null.
std::uint32_t native_campaign_ai_vision_character_block_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t character);
//1D0400: actual two-row player association. NONE is returned as signed WORD.
std::int16_t native_campaign_ai_vision_player_slot_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t player);
//272970: current source clock tick publication into the selected clump.
void native_campaign_ai_vision_clump_timestamp_tu15(
    SourcePhysicsContactMemory &,std::uint32_t clump);
}
