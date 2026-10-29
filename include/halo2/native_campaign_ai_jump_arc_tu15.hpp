#pragma once
#include "halo2/native_campaign_ai_jump_fallback_tu15.hpp"
#include <variant>

namespace halo2 {
struct NativeCampaignAiJumpExtendedRealTu15 {std::array<std::byte,10> bytes;};
struct NativeCampaignAiJumpHorizontalSquaredTu15 {NativeCampaignAiJumpExtendedRealTu15 value;};
struct NativeCampaignAiJumpHorizontalDistanceTu15 {NativeCampaignAiJumpExtendedRealTu15 value;};
struct NativeCampaignAiJumpDiscriminantRootTu15 {NativeCampaignAiJumpExtendedRealTu15 value;};
struct NativeCampaignAiJumpFlightTimeSquaredTu15 {NativeCampaignAiJumpExtendedRealTu15 value;};
struct NativeCampaignAiJumpHorizontalVelocitySquaresTu15 {
    NativeCampaignAiJumpExtendedRealTu15 x_squared,y_squared;
};
struct NativeCampaignAiJumpLaunchMagnitudeTu15 {NativeCampaignAiJumpExtendedRealTu15 value;};
using NativeCampaignAiJumpArcPendingTu15=std::variant<
    NativeCampaignAiJumpHorizontalSquaredTu15,NativeCampaignAiJumpHorizontalDistanceTu15,
    NativeCampaignAiJumpDiscriminantRootTu15,NativeCampaignAiJumpFlightTimeSquaredTu15,
    NativeCampaignAiJumpHorizontalVelocitySquaresTu15,NativeCampaignAiJumpLaunchMagnitudeTu15>;
class NativeCampaignAiJumpArcPendingErrorTu15 final:public std::runtime_error {
    NativeCampaignAiJumpArcPendingTu15 pending_;
    std::exception_ptr cause_;
public:
    NativeCampaignAiJumpArcPendingErrorTu15(NativeCampaignAiJumpArcPendingTu15,std::exception_ptr);
    const NativeCampaignAiJumpArcPendingTu15 &pending()const noexcept{return pending_;}
    const std::exception_ptr &cause()const noexcept{return cause_;}
};
struct NativeCampaignAiJumpArcValueTu15 {
    bool feasible;
    std::array<std::uint32_t,3> direction;
};
// ActualFF1A0 profile called by278E60: gravityScale=1, highArc=true and all
// optional output pointers absent. It still performs every owed mathematical
// operation and returns its private direction as a native value. No path or
// device result is supplied. Other FF1A0 caller contracts are separate owners.
NativeCampaignAiJumpArcValueTu15 native_campaign_ai_jump_arc_profile_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t first,std::uint32_t second,
    std::uint32_t maximum_speed_bits,NativeTu15PhysicsFrameFp &);
NativeCampaignAiJumpArcValueTu15 native_campaign_ai_jump_arc_profile_tu15(
    const SourcePhysicsContactMemory &,const NativeCampaignAiJumpPointTu15 &first,
    const NativeCampaignAiJumpPointTu15 &second,std::uint32_t maximum_speed_bits,
    NativeTu15PhysicsFrameFp &);
// Actual278E60 jump link predicate. Secondary endpoints are optional as a pair.
bool native_campaign_ai_jump_link_tu15(const SourcePhysicsContactMemory &,
    std::int16_t kind,const NativeCampaignAiJumpPointTu15 &from,
    const NativeCampaignAiJumpPointTu15 &to,const NativeCampaignAiJumpPointTu15 *secondary_from,
    const NativeCampaignAiJumpPointTu15 *secondary_to,NativeTu15PhysicsFrameFp &);
}
