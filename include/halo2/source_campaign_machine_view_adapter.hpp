#pragma once
#include "halo2/source_campaign_machine_view.hpp"
#include "halo2/source_live_model_geometry_tu15.hpp"
#include <algorithm>
#include <set>

namespace halo2 {
// A specialized packet consumes completed ordinals from this exact view.
// Keep this admission shared by ordinary and texture-camera consumers.
inline std::set<unsigned> source_campaign_machine_view_ordinals(
    const SourceCampaignMachineViewPacket &packet,
    std::span<const SourceLiveModelBaseDrawTu15> source) {
    packet.require_current();
    std::set<unsigned> consumed;
    for(const auto ordinal:packet.ordinals)
        if(!consumed.insert(ordinal).second || std::ranges::none_of(source,
            [&](const auto &draw){return draw.ordinal==ordinal;}))
            throw FormatError("Campaign machine view consumed an unknown/duplicate source ordinal");
    return consumed;
}
}
