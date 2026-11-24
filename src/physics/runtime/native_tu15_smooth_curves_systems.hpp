#pragma once
// Native TU1.5 smooth curves, the shapes the transition fraction (1E1620)
// selects, each clamped to 0..1:
//
//   smoothstep::of     110FC0(XMM2 = x) -> XMM0   3x^2 - 2x^3
//   smootherstep::of   1E1920(XMM3 = x) -> XMM0   6x^5 - 15x^4 + 10x^3
//
// The single-precision arithmetic is the original's scalar SSE, in its
// order (operands as the original's destination and source), on the live
// MXCSR. This is the game logic only. The game's memory is the retained
// TU1.5 bank, reached through a Memory object with typed loads and stores;
// every access names its role (Access), and the boundary adapters
// (native_tu15_smoothstep.hpp, native_tu15_smootherstep.hpp) map the role
// to the original instruction.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include <cstdint>

namespace halo2::native_smooth_curves {
namespace sse=native_aim_clusters::sse;
namespace globals {
inline constexpr std::uint32_t one=0x46a0d0u,two=0x46a11cu,three=0x46a0e4u;   // floats
inline constexpr std::uint32_t six=0x46a10cu,ten=0x46a110u,fifteen=0x46a108u;
}
inline constexpr std::uint32_t zero_bits=0u;
// Where the clamp left the value.
enum class Clamp {low,high,within};
struct Result {std::uint32_t value;Clamp clamp;};
// The clamp both curves end with. Frame:
//   cleared()        XORPS (the zero register) before the first comparison
//   compared(a,b)    COMISS a,b (the flags)
//   one_loaded()     the one register (MOVSS) after the first comparison
template<class Memory,class Frame,class Access> Result clamp(Memory &memory,Frame &frame,std::uint32_t value,Access one_access) {
    frame.cleared();
    frame.compared(zero_bits,value);
    if(!sse::not_above(zero_bits,value))return {zero_bits,Clamp::low};
    const auto one_value=memory.load32(one_access,globals::one);
    frame.compared(value,one_value);
    if(!sse::not_above(value,one_value))return {one_value,Clamp::high};
    return {value,Clamp::within};
}

namespace smoothstep {
enum class Access {three,two,one};
// Frame: shaped(value,two_cube) (XMM1, XMM0), then the clamp's.
template<class Memory,class Frame> Result of(Memory &memory,Frame &frame,std::uint32_t x) {
    using enum Access;
    const auto square=sse::multiply(x,x);
    const auto three_square=sse::multiply(square,memory.load32(three,globals::three));
    const auto cube=sse::multiply(square,x);
    const auto two_cube=sse::multiply(cube,memory.load32(two,globals::two));
    const auto value=sse::subtract(three_square,two_cube);
    frame.shaped(value,two_cube);
    return clamp(memory,frame,value,one);
}
} // namespace smoothstep

namespace smootherstep {
enum class Access {ten,fifteen,six,one};
// Frame: shaped(value,fifteen_fourth) (XMM0, XMM2), then the clamp's.
template<class Memory,class Frame> Result of(Memory &memory,Frame &frame,std::uint32_t x) {
    using enum Access;
    const auto cube=sse::multiply(sse::multiply(x,x),x);
    const auto ten_cube=sse::multiply(cube,memory.load32(ten,globals::ten));
    const auto fourth=sse::multiply(cube,x);
    const auto fifteen_fourth=sse::multiply(fourth,memory.load32(fifteen,globals::fifteen));
    const auto fifth=sse::multiply(fourth,x);
    const auto six_fifth=sse::multiply(fifth,memory.load32(six,globals::six));
    const auto value=sse::add(sse::subtract(six_fifth,fifteen_fourth),ten_cube);
    frame.shaped(value,fifteen_fourth);
    return clamp(memory,frame,value,one);
}
} // namespace smootherstep
} // namespace halo2::native_smooth_curves
