#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <exception>
#include <stdexcept>

namespace halo2 {
struct NativeCampaignAiPathResultPendingTu15 {
    enum class Stage {Distance,ClosestX,ClosestYAndX,ClosestZAndYX};
    Stage stage;
    unsigned count;
    std::array<std::array<std::byte,10>,3> quantities;
};
class NativeCampaignAiPathResultPendingErrorTu15 final:public std::runtime_error {
    NativeCampaignAiPathResultPendingTu15 pending_;
    std::exception_ptr cause_;
public:
    NativeCampaignAiPathResultPendingErrorTu15(NativeCampaignAiPathResultPendingTu15,std::exception_ptr);
    const auto &pending()const noexcept{return pending_;}
    const auto &cause()const noexcept{return cause_;}
};
// Actual27A460 consumes retained hash/parent links; it performs no search or
// pathability substitution. Optional clearance/direction destinations retain
// their actual publication order and may alias retained input records.
bool native_campaign_ai_path_result_tu15(SourcePhysicsContactMemory &,
    std::uint32_t workspace,std::uint32_t key,std::uint32_t goal,
    std::uint32_t cost_output,std::uint32_t clearance_output,std::uint32_t direction_output,
    NativeTu15PhysicsFrameFp &,unsigned occupied_x87,const SourcePhysicsContactRecordObserver & = {});
}
