#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include "halo2/native_campaign_ai_jump_point_tu15.hpp"
#include <exception>
#include <stdexcept>

namespace halo2 {
enum class NativeCampaignAiJumpPendingKindTu15 {
    FirstX, FirstY, FirstZ, AbsoluteDistance
};
struct NativeCampaignAiJumpPendingTu15 {
    NativeCampaignAiJumpPendingKindTu15 kind;
    std::array<std::byte,10> quantity;
};
class NativeCampaignAiJumpPendingErrorTu15 final:public std::runtime_error {
    NativeCampaignAiJumpPendingTu15 pending_;
    std::exception_ptr cause_;
public:
    NativeCampaignAiJumpPendingErrorTu15(NativeCampaignAiJumpPendingTu15,std::exception_ptr);
    const NativeCampaignAiJumpPendingTu15 &pending()const noexcept{return pending_;}
    const std::exception_ptr &cause()const noexcept{return cause_;}
};
struct NativeCampaignAiJumpFallbackOutputsTu15 {
    std::uint32_t direction;
    std::uint32_t distance{},speed{},duration{}; // Zero disables each optional output.
};
struct NativeCampaignAiJumpFallbackValueTu15 {
    std::array<std::uint32_t,3> direction;
    std::uint32_t distance_bits,speed_bits,duration_bits;
};
// Private intermediate used by native arc composition. No retained temporary
// address is created, and all actual global/source point reads remain lazy.
NativeCampaignAiJumpFallbackValueTu15 native_campaign_ai_jump_fallback_value_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t first,std::uint32_t second,
    std::uint32_t speed_bits,NativeTu15PhysicsFrameFp &);
NativeCampaignAiJumpFallbackValueTu15 native_campaign_ai_jump_fallback_value_tu15(
    const SourcePhysicsContactMemory &,const NativeCampaignAiJumpPointTu15 &first,
    const NativeCampaignAiJumpPointTu15 &second,std::uint32_t speed_bits,
    NativeTu15PhysicsFrameFp &);
// Complete actualFF580 trajectory fallback. This computes and publishes the
// mathematical direction/distance/speed/duration; no pathability result is
// supplied. Its original boolean is unconditionally true after this work.
void native_campaign_ai_jump_fallback_tu15(SourcePhysicsContactMemory &,
    std::uint32_t first,std::uint32_t second,std::uint32_t speed_bits,
    NativeCampaignAiJumpFallbackOutputsTu15,NativeTu15PhysicsFrameFp &,
    const SourcePhysicsContactRecordObserver & = {});
}
