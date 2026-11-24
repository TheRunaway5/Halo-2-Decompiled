#pragma once
// Native TU1.5 pixel scales, entered at 1BE90 (shadow frame unit): the
// current view's scales per pixel of its width.
//
//   pixel_scales::update   1BE90()
//
// The view's width (int16 4C65F4 less int16 4C65F0) divides one, zero and
// the constants at 46A11C and 46A120 into the seven scales at 492348: one,
// zero, zero, the first constant, the second, zero, one, each per pixel.
// With the state at [51DF50] whose byte +5 is set, the last is scaled again
// by the first constant.
//
// The single-precision arithmetic is the original's scalar SSE (the width
// converted by CVTSI2SS) on the live MXCSR. This is the game logic only.
// The game's memory is the retained TU1.5 bank, reached through a Memory
// object with typed loads and stores; every access names its role (Access),
// and the boundary adapter (native_tu15_pixel_scales.hpp) maps the role to
// the original instruction.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include <cstdint>

namespace halo2::native_pixel_scales {
using native_object_update::at;
using native_object_update::widen16;
namespace sse=native_aim_clusters::sse;
namespace globals {
inline constexpr std::uint32_t left=0x4c65f0u,right=0x4c65f4u;   // int16: the view's horizontal bounds
inline constexpr std::uint32_t first_constant=0x46a11cu,second_constant=0x46a120u,one=0x46a0d0u;
inline constexpr std::uint32_t state=0x51df50u;                  // a state whose byte +5 scales the last again
inline constexpr std::uint32_t scales=0x492348u;                 // float[7]
}
inline constexpr std::uint32_t rescaled=5u,zero_bits=0u;
// The CVTSI2SS of a 32-bit integer on the live MXCSR.
inline std::uint32_t converted(std::int32_t value){return sse::bits(_mm_cvtsi32_ss(_mm_setzero_ps(),value));}

namespace pixel_scales {
enum class Access {left,right,first_constant,one,state,first_store,second_constant,scale_store,rescaled,rescaled_store};
// Frame: divided(width, first, zero, one, second) the quotients' registers.
template<class Memory,class Frame> bool update(Memory &memory,Frame &frame) {
    using enum Access;
    const auto left_value=widen16(memory.load16(Access::left,globals::left));
    const auto right_value=widen16(memory.load16(Access::right,globals::right));
    const auto first=memory.load32(first_constant,globals::first_constant);
    const auto one_value=memory.load32(one,globals::one);
    const auto state_value=memory.load32(state,globals::state);
    const auto width=converted(std::int32_t(right_value-left_value));
    const auto first_per_pixel=sse::divide(first,width);
    memory.store32(first_store,globals::scales+12u,first_per_pixel);
    const auto second=memory.load32(second_constant,globals::second_constant);
    const auto zero_per_pixel=sse::divide(zero_bits,width);
    const auto one_per_pixel=sse::divide(one_value,width);
    const auto second_per_pixel=sse::divide(second,width);
    frame.divided(width,first_per_pixel,zero_per_pixel,one_per_pixel,second_per_pixel);
    const std::uint32_t order[]{0u,1u,2u,4u,5u,6u};
    const std::uint32_t values[]{one_per_pixel,zero_per_pixel,zero_per_pixel,second_per_pixel,zero_per_pixel,one_per_pixel};
    for(std::uint32_t k=0;k<6;++k)memory.store32(scale_store,globals::scales+4u*order[k],values[k]);
    if(!state_value || !(memory.load8(Access::rescaled,at(state_value,native_pixel_scales::rescaled))&0xffu))return false;
    memory.store32(rescaled_store,globals::scales+24u,sse::multiply(one_per_pixel,first));
    return true;
}
} // namespace pixel_scales
} // namespace halo2::native_pixel_scales
