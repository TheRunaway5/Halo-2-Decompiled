#pragma once
#include "halo2/native_tu15_physics_frame.hpp"

namespace halo2 {
// Actual145140/145200: scale+basis direction and inverse affine point.
// Retained matrix/source/output may alias. Native hardware owns MXCSR only.
void native_campaign_ai_transform_direction_tu15(SourcePhysicsContactMemory &,
    std::uint32_t matrix,std::uint32_t source,std::uint32_t destination,
    NativeTu15PhysicsFrameFp &,const SourcePhysicsContactRecordObserver & = {});
void native_campaign_ai_inverse_point_tu15(SourcePhysicsContactMemory &,
    std::uint32_t matrix,std::uint32_t source,std::uint32_t destination,
    NativeTu15PhysicsFrameFp &,const SourcePhysicsContactRecordObserver & = {});
// Actual218FE0 uses the retained inverse permutation table, not an assumed
// inverse of the forward table. Signed table selectors retain source wrap.
void native_campaign_ai_inverse_permute_point_tu15(SourcePhysicsContactMemory &,
    std::uint32_t source,std::uint32_t destination,std::int16_t axis,
    std::uint8_t variant,const SourcePhysicsContactRecordObserver & = {});
bool native_campaign_ai_resolve_direction_tu15(SourcePhysicsContactMemory &,
    std::uint16_t frame,std::uint32_t source,std::uint32_t destination,
    NativeTu15PhysicsFrameFp &,const SourcePhysicsContactRecordObserver & = {});
bool native_campaign_ai_inverse_frame_point_tu15(SourcePhysicsContactMemory &,
    std::uint16_t frame,std::uint32_t source,std::uint32_t destination,
    NativeTu15PhysicsFrameFp &,const SourcePhysicsContactRecordObserver & = {});
struct NativeCampaignAiFramePointValueTu15 {
    std::array<std::uint32_t,3> point;
    bool resolved;
};
// Composed algorithms keep private transformed coordinates as native values;
// no original private stack/CPU state is reconstructed.
NativeCampaignAiFramePointValueTu15 native_campaign_ai_inverse_frame_point_value_tu15(
    const SourcePhysicsContactMemory &,std::uint16_t frame,std::uint32_t source,
    NativeTu15PhysicsFrameFp &);
// Actual218EB0: second-first, with equal-frame direction resolution or two
// independently projected world points. This is geometry, not pathability.
void native_campaign_ai_framed_difference_tu15(SourcePhysicsContactMemory &,
    std::uint32_t first,std::uint32_t second,std::uint32_t destination,
    NativeTu15PhysicsFrameFp &,const SourcePhysicsContactRecordObserver & = {});
}
