#pragma once
#include "halo2/sound_driver_parameters.hpp"
#include <array>

namespace halo2 {
// Literal TU1.5 operands read from the same authority as the parameter
// constants. Directions are the four rows at44EDA0, in source mix-bin order.
struct SourceLoopCombinedMixConstantsTu15 {
    std::array<std::array<std::uint32_t,3>,4> directions;
    std::uint32_t half,one,normalization_epsilon,minimum_distance;
};
struct SourceLoopCombinedMixCalculationTu15 {
    std::uint32_t minimum,gain;
    std::array<std::uint32_t,3> normalized;
    std::array<std::uint32_t,4> speakers,accumulated;
};
struct SourceLoopCombinedListenerInputTu15 {
    std::array<std::uint32_t,3> local_point;
    std::uint32_t distance;
};
// Fixed TU1.5 executable operands, independently retained/checked by the
// original contribution fixture. These are not device or listener defaults.
inline constexpr SourceLoopCombinedMixConstantsTu15 source_loop_combined_constants_tu15{
    {{{0x3f3504f3u,0x3f3504f3u,0u},{0x3f3504f3u,0xbf3504f3u,0u},
      {0xbf3504f3u,0x3f3504f3u,0u},{0xbf3504f3u,0xbf3504f3u,0u}}},
    0x3f000000u,0x3f800000u,0x38d1b717u,0x3a83126fu};
//220DA3..220DCB. The caller supplies both real byte frame counters. This
//does not advance the parent frame or select/admit a listener.
std::array<std::uint32_t,4> source_loop_combined_reset_tu15(
    const std::array<std::uint32_t,4>& accumulated,
    std::uint8_t previous_frame,std::uint8_t frame);
//220E20..220E4F and220E83..220F21, including complete198D80,30BD0 and
//198CF0(flag1)->198BA0. Point must already be in the selected listener's
//local coordinates. The result retains four independent source mix bins;
//it is not a stereo downmix, voice admission, or a replacement listener.
SourceLoopCombinedMixCalculationTu15 source_loop_combined_mix_tu15(
    const std::array<std::uint32_t,4>& accumulated,
    const std::array<std::uint32_t,3>& listener_local_point,
    std::uint32_t minimum,std::uint32_t maximum,std::uint32_t distance,
    const SourceLoopCombinedMixConstantsTu15&,
    const SoundDriverParameterConstants&,const SoundDriverFloatingPointContext&);
}
