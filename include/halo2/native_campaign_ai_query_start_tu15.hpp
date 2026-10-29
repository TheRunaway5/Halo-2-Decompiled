#pragma once
#include "halo2/physics_contact_record.hpp"
namespace halo2 {
struct NativeCampaignAiQueryStartResult {
    std::uint16_t reservations;
    bool did_reserve;
};
// Actual 1BD9D0 actor/query arguments. The two remaining original stack
// arguments are unused. The captured group chain owns its next link before
// reserving a member; each subsequent actor row uses the current rows pointer
// from the original captured pool owner. No behavior cancellation is implied.
NativeCampaignAiQueryStartResult native_campaign_ai_query_start_tu15(
    SourcePhysicsContactMemory &,std::uint32_t actor,std::uint32_t query,
    const SourcePhysicsContactRecordObserver & = {});
}
