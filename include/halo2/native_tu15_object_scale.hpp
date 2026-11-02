#pragma once
#include "halo2/native_tu15_physics_frame.hpp"

namespace halo2 {
using NativeTu15ObjectScalePublisher=
    std::function<void(std::uint32_t,std::span<const std::byte>)>;
// Genuine retained-world services. Interpolation is BC380 (including its
// ancestor wake); refresh is the whole BF050 model/bounds/child publication.
// Neither callback may be omitted on an active path. These dependencies are
// explicit migration boundaries, not claims that the callees are native.
// BC380 and its complete 1D38A0/1E14F0/1E16E0/B9390 closure preserve XMM2/3.
struct NativeTu15ObjectScaleServices {
    std::function<void(std::uint32_t object,std::uint32_t seconds_bits,
        NativeTu15PhysicsFrameFp&)> interpolate;
    std::function<void(std::uint32_t object,NativeTu15PhysicsFrameFp&)> refresh;
};
// B970D..B9756 numeric publication after the real interpolation owner.
void native_tu15_object_scale_pose(SourcePhysicsContactMemory&,std::uint32_t body,
    std::uint32_t old_scale_bits,std::uint32_t new_scale_bits,NativeTu15PhysicsFrameFp&,
    const NativeTu15ObjectScalePublisher& = {});
std::uint32_t native_tu15_object_scale_seconds(const SourcePhysicsContactMemory&,
    std::uint16_t ticks,NativeTu15PhysicsFrameFp&);
// Whole B96B0 state effect, with explicit service ownership and source FP.
// NONE, unordered, zero and negative scales perform no stores/services.
void native_tu15_object_scale(SourcePhysicsContactMemory&,std::uint32_t object,
    std::uint32_t scale_bits,std::uint32_t seconds_bits,NativeTu15PhysicsFrameFp&,
    const NativeTu15ObjectScaleServices&,const NativeTu15ObjectScalePublisher& = {});
// Whole 110030 hierarchy policy. Duration consumes its signed low16; source
// 46A17C supplies seconds per tick. Child is pinned before the scale operation,
// sibling is read from the pinned child body after its recursive operation.
void native_tu15_object_scale_hierarchy(SourcePhysicsContactMemory&,std::uint32_t object,
    std::uint32_t scale_bits,std::uint16_t ticks,NativeTu15PhysicsFrameFp&,
    const NativeTu15ObjectScaleServices&,const NativeTu15ObjectScalePublisher& = {});
class PhysicsLiveWorldTu15;
void source_campaign_object_scale_tu15(PhysicsLiveWorldTu15&,std::uint32_t object,
    std::uint32_t scale_bits,std::uint16_t ticks);
}
