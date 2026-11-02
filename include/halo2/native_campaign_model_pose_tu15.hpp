#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <stdexcept>

namespace halo2 {
class NativeCampaignModelPoseCapacityRequiredTu15 final:public std::runtime_error {
    std::uint32_t instruction_;
public:
    explicit NativeCampaignModelPoseCapacityRequiredTu15(std::uint32_t instruction);
    std::uint32_t instruction()const noexcept{return instruction_;}
    static constexpr unsigned capacity()noexcept{return 255;}
};
// Actual170820 model-hierarchy composition. Root basis and converted local
// transforms are private typed13-DWORD values; authored records, parent
// transforms and published output retain their real source addresses.
// Original private pending storage has255 cells; overflow retains an exact
// required-owner boundary before the original out-of-frame publication.
void native_campaign_model_pose_tu15(SourcePhysicsContactMemory &,
    std::uint32_t model,std::uint32_t destination,std::uint32_t poses,
    std::uint32_t position,std::uint32_t forward,std::uint32_t up,
    NativeTu15PhysicsFrameFp &,const SourcePhysicsContactRecordObserver & = {});
}
