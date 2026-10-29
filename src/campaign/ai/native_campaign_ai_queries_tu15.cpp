#include "halo2/native_campaign_ai_queries_tu15.hpp"

namespace halo2 {
std::uint32_t native_campaign_ai_threat_tu15(
    NativeCampaignAiThreatAccessTu15 &records, std::uint32_t actor) {
    const auto property = records.actor_property(actor);
    if(property == 0xffffffffu) return 0;
    const auto query = records.property_query(property);
    if(query == 0xffffffffu || !records.bind_query(query)) return 0;
    if(records.property_state() < 3) return 0;
    return records.query_blocked() ? 0u : 3u;
}
void native_campaign_ai_reset_tu15(NativeCampaignAiResetAccessTu15 &record,
    bool preserve_unmarked) {
    if(!preserve_unmarked || record.marked()) record.reset();
}
}
