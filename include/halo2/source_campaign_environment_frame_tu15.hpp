#pragma once
#include "halo2/math.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <span>

namespace halo2 {
class PhysicsLiveWorldTu15;
// Completed ordinary main-caller output, owned independently of the source
// scratch region. The original producer owns every environment byte; this
// boundary performs only its actual caller RGB override and far-plane clamp.
struct SourceCampaignEnvironmentFrameTu15 {
    std::array<std::byte,0x120> bytes;
    std::int32_t visible_sky{};
    std::uint8_t sky_model_present{},clear_override{};
    float far_distance{};
};
namespace detail {
// Literal caller-only stage, independently testable against unmodified
// original2B868..2B8A6 and2BA96. This is not an environment producer.
inline float source_campaign_environment_caller_tu15(
    std::array<std::byte,0x120> &environment,std::uint8_t override_rgb,
    std::span<const std::byte,12> rgb,float near_distance,float far_distance) {
    if(override_rgb)std::copy(rgb.begin(),rgb.end(),environment.begin()+4u);
    if(environment[0x18]!=std::byte{}) {
        float limit;std::memcpy(&limit,environment.data()+0x14,sizeof(limit));
        if(far_distance>limit && limit>near_distance)return limit;
    }
    return far_distance;
}
}
// Actual2B845/2B863 source owners followed by literal2B868..2B8A6,
// then2BA96's complete main publication. The containing cluster comes from
// the same completed camera-location owner used for portal visibility.
// Call before constructing clipping/projection banks: their far plane must
// consume the returned far_distance. No visibility or environment result is
// supplied, no frame/view storage is reset, and no GPU work runs here.
SourceCampaignEnvironmentFrameTu15 source_campaign_environment_frame_tu15(
    PhysicsLiveWorldTu15 &,Vec3 eye,Vec3 forward,std::int32_t cluster,
    float near_distance,float far_distance);
}
