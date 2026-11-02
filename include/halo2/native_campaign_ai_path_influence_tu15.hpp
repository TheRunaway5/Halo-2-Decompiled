#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <exception>
#include <stdexcept>

namespace halo2 {
struct NativeCampaignAiPathInfluenceValueTu15 {
    std::uint32_t distance;
    std::uint32_t influence;
};
struct NativeCampaignAiPathInfluenceQuantityTu15 {
    enum class Kind { DistanceRoot,DistanceRatio,NormalizedInfluence };
    Kind kind;
    std::array<std::byte,10> extended;
};
class NativeCampaignAiPathInfluencePendingErrorTu15 final:public std::runtime_error {
    NativeCampaignAiPathInfluenceQuantityTu15 quantity_;
    std::exception_ptr cause_;
public:
    NativeCampaignAiPathInfluencePendingErrorTu15(NativeCampaignAiPathInfluenceQuantityTu15,std::exception_ptr);
    const auto &quantity()const noexcept{return quantity_;}
    const auto &cause()const noexcept{return cause_;}
};
// Actual27C520 projection/falloff profile. Both inverse transforms use the
// real retained frame/object/node owners. The original endpoint fallback,
// strict radius test and extended sqrt/divide association are preserved.
// Actual caller FP policy and occupied x87 count are required inputs.
std::uint32_t native_campaign_ai_path_influence_tu15(SourcePhysicsContactMemory &,
    std::uint32_t origin,std::uint32_t endpoint,std::uint32_t influence,
    std::uint32_t distance_output,NativeTu15PhysicsFrameFp &,unsigned occupied_x87,
    const SourcePhysicsContactRecordObserver & = {});
// Same algorithm with a native private output; no synthetic source address.
NativeCampaignAiPathInfluenceValueTu15 native_campaign_ai_path_influence_value_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t origin,std::uint32_t endpoint,
    std::uint32_t influence,NativeTu15PhysicsFrameFp &,unsigned occupied_x87);
}
