#pragma once
#include "halo2/physics_world_runtime.hpp"

namespace halo2 {
struct NativeCampaignAiReadTu15 {
    std::uint32_t instruction, address;
    unsigned bytes;
};
using NativeCampaignAiReadObserverTu15 = std::function<void(const NativeCampaignAiReadTu15 &)>;
bool native_campaign_ai_queries_tu15_owns_entry(std::uint32_t);
// The compatibility boundary decodes real arguments, invokes semantic record
// services, and encodes the public result/callee argument consumption. It does
// not reconstruct dead volatile GPRs/flags or retired private stack frames.
void native_campaign_ai_queries_tu15_run(SourcePhysicsContactMemory &,
    std::uint32_t, SourcePhysicsWorldRuntimeAbi &,
    const SourcePhysicsWorldRuntimeServices & = {},
    const SourcePhysicsContactRecordObserver & = {},
    const NativeCampaignAiReadObserverTu15 & = {});
struct NativeCampaignAiQueryActivityTu15 { std::uint64_t threat{}, reset{}; };
NativeCampaignAiQueryActivityTu15 native_campaign_ai_query_activity_tu15();
}
