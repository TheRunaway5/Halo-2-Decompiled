#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include "halo2/native_campaign_ai_point_geometry_tu15.hpp"
#include <exception>
#include <stdexcept>

namespace halo2 {
// At the late end-index read the original has loaded previous Z above its
// already computed Y/X differences. Keep these exact extended quantities
// when that retained read is rejected; no private CPU frame is synthesized.
struct NativeCampaignAiPathDistancePendingTu15 {
    std::array<std::array<std::byte,10>,3> previous_z_dy_dx;
};
class NativeCampaignAiPathDistancePendingErrorTu15 final:public std::runtime_error {
    NativeCampaignAiPathDistancePendingTu15 pending_;
    std::exception_ptr cause_;
public:
    NativeCampaignAiPathDistancePendingErrorTu15(NativeCampaignAiPathDistancePendingTu15,std::exception_ptr);
    const auto &pending()const noexcept{return pending_;}
    const auto &cause()const noexcept{return cause_;}
};
// Complete200C00 remaining path distance from retained actor/path/frame data.
// Each segment spills to source REAL precision; failed frame resolution uses
// the original second XYZ copy. This does not perform path search/selection.
std::uint32_t native_campaign_ai_path_distance_tu15(const SourcePhysicsContactMemory &,
    std::uint32_t actor,NativeTu15PhysicsFrameFp &,unsigned occupied_x87);
// Same algorithm after the actual actor-pool owner has resolved its retained
// row. The numeric boundary publishes its zero scalar between that lookup and
// the first guard read; this seam preserves that source failure ordering.
std::uint32_t native_campaign_ai_actor_path_distance_tu15(const SourcePhysicsContactMemory &,
    std::uint32_t actor_record,NativeTu15PhysicsFrameFp &,unsigned occupied_x87,
    const NativeCampaignAiProjectionCoordinateObserverTu15 & = {});
}
