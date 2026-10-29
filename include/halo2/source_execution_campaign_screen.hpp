#pragma once
#include "halo2/source_execution_inputs.hpp"

namespace halo2 {
struct SourceExecutionCampaignScreenTarget {
    std::function<void()> stop, acknowledge_capture;
};
// Validate the complete void-call schema before any world mutation. Empty
// callbacks validate only; replay binds these to the actual native world.
// False means a different call kind, which the main decoder must handle.
bool source_execution_campaign_screen_call(const SourceExecutionCall &,
    const SourceExecutionCampaignScreenTarget & = {});
}
